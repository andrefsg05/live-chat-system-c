#include "clientFile_transfer.h"
#include "client_threads.h"
#include "client_messages.h"
#include "packet_types.h" 

#include <stdio.h>     
#include <stdlib.h>    
#include <string.h>    
#include <unistd.h>     
#include <sys/time.h>   
#include <sys/types.h>  
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <pthread.h>    
#include <fcntl.h>      
#include <sys/stat.h>   
#include <stdatomic.h>    


#define FILE_SENDER_IP L"127.0.0.1"


/******************************************************************************************
 * 
 * 
 *                              SENDER SIDE 
 * 
 * 
 ******************************************************************************************/

 
/**
 * Initiates the file sending process (sender side).
 *
 * Converts the wide string filename to a multibyte string, opens the file, determines its size,
 * creates a TCP server socket, and launches a thread to handle incoming TCP connections for file transfer.
 * Notifies the recipient(s) via UDP about the file transfer request.
 *
 * @param sockfd UDP socket file descriptor.
 * @param servaddr Pointer to the server's sockaddr_in structure.
 * @param filename Wide string containing the file name to send.
 */
void send_file(int sockfd, struct sockaddr_in *servaddr, wchar_t *filename) {

    // Convert wide string to multibyte string for file operations
    char filename_c[MAX_FILENAME];
    wcstombs(filename_c, filename, sizeof(filename_c));
    filename_c[MAX_FILENAME - 1] = '\0'; 

    FILE *fp = fopen(filename_c, "rb");
    if (!fp) {
        wprintf(L"Error opening file: %s\n", filename_c);
        fflush(stdout);
        return;
    }
    // Get file size
    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    fclose(fp);

    // Create TCP socket and bind to an available port
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = 0; // Let OS pick port

    if (bind(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        wprintf(L"Error binding TCP server socket.\n");
        fflush(stdout);
        close(server_sock);
        return;
    }

    socklen_t len = sizeof(serv_addr);
    getsockname(server_sock, (struct sockaddr*)&serv_addr, &len);
    int tcp_port = ntohs(serv_addr.sin_port);

    listen(server_sock, 7);

    // Launch the server thread to handle file transfers
    FileServerArgs *args = malloc(sizeof(FileServerArgs));
    strcpy(args->filename, filename_c); // Copy filename
    args->timeout_secs = FILE_TIMEOUT_SECS;
    args->server_sock = server_sock;
    
    pthread_t tid;
    pthread_create(&tid, NULL, file_tcp_server_thread, args);
    pthread_detach(tid);


    // Prepare and send UDP notification to recipient(s)
    wchar_t packet[MAX_LINE];
    swprintf(packet, MAX_LINE, L"%d|%ls|%zu|%ls|%d", FILE_SEND_REQUEST, filename, filesize, FILE_SENDER_IP, tcp_port);
    send_packet(sockfd, servaddr, packet);

    wprintf(L"File send request sent to recipients.\n");
    fflush(stdout);
}


/**
 * Opens a TCP server socket and sends the file to each client that connects.
 *
 * Waits for incoming TCP connections for a specified timeout period. For each connection,
 * launches a thread to send the file to the connected client.
 *
 * @param arg Pointer to FileServerArgs structure containing server socket and file info.
 * @return NULL
 */
void* file_tcp_server_thread(void* arg) {
    FileServerArgs* args = (FileServerArgs*)arg;
    int server_sock = args->server_sock;

    time_t start = time(NULL); // Start time for timeout

    while (difftime(time(NULL), start) < args->timeout_secs) {

        struct timeval tv;
        tv.tv_sec = 1; // 1 second timeout for select
        tv.tv_usec = 0;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_sock, &fds);
        int ret = select(server_sock + 1, &fds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(server_sock, &fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
            if (client_sock < 0) {
                continue;
            }
            
            FileSendArgs* fileSendArgs = malloc(sizeof(FileSendArgs));
            fileSendArgs->sockfd = client_sock;
            strcpy(fileSendArgs->filename, args->filename);

            pthread_t tid;
            pthread_create(&tid, NULL, send_file_thread, (void*)(fileSendArgs));
            pthread_detach(tid);
        }
        // else: 1s timeout, loop again to check total time
    }

    // Notify user of timeout after file transfer period ends
    while(atomic_load(&ThreadData.signalFilePrint) != 0) {
        say_no_to_busy_waiting();   
    }
    wcscpy(ThreadData.FilePrint, L"TCP file transfer stopped due to timeout.");
    atomic_store(&ThreadData.signalFilePrint, 1); 
    free(args);
    close(server_sock);
    return NULL;
}


/**
 * Sends the file to a connected client over TCP.
 *
 * Opens the file and sends its contents in chunks to the client socket.
 * Handles errors and notifies the user if the file cannot be opened.
 *
 * @param send_file_args Pointer to FileSendArgs structure with socket and filename.
 * @return NULL
 */
void* send_file_thread(void* send_file_args){

    FileSendArgs* args = (FileSendArgs*)send_file_args;
    int client_sock = args->sockfd;
    const char* filename = args->filename;

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        while(atomic_load(&ThreadData.signalFilePrint) != 0) {
            say_no_to_busy_waiting();   
        }            
        swprintf(ThreadData.FilePrint, MAX_LINE, L"Error opening file '%s'.", filename);
        atomic_store(&ThreadData.signalFilePrint, 1);
        free(args);
        close(client_sock);
        return NULL;
    }

    char buffer[1024]; // Buffer for file chunks
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_sock, buffer, bytes, 0);
    }
    fclose(fp);
    close(client_sock);

    free(args);
    return NULL;
}


/******************************************************************************************
 * 
 * 
 *                              RECEIVER SIDE 
 * 
 * 
 ******************************************************************************************/


/**
 * Handles an incoming file transfer request from the server.
 *
 * Parses the file transfer request, prompts the user to accept or decline,
 * and starts a thread to receive the file via TCP if accepted.
 *
 * FILE_WARNING | filename | filesize | SENDER_IP | TCP_PORT | <sender_username>
 */
void handle_file_receive_warning() {
    
    int sockfd = ThreadData.sockfd;
    struct sockaddr_in *servaddr = &ThreadData.servaddr;

    wchar_t* rest = NULL;
    wchar_t *filename_wide = wcstok(ThreadData.FileReceiveWarning, L"|", &rest);
    wchar_t *filesize_str = wcstok(NULL, L"|", &rest);
    wchar_t *sender_ip_str = wcstok(NULL, L"|", &rest);
    wchar_t *tcp_port_str = wcstok(NULL, L"|", &rest);
    wchar_t *sender_username = wcstok(NULL, L"|", &rest);
    if (!filename_wide || !filesize_str || !sender_ip_str || !tcp_port_str || !sender_username) {
        wprintf(L"Error processing file request.\n");
        fflush(stdout);
        return;
    }

    char filename[MAX_FILENAME] = {0};
    wcstombs(filename, filename_wide, sizeof(filename));
    filename[MAX_FILENAME - 1] = '\0'; // Ensure null termination
    
    size_t filesize = wcstol(filesize_str, NULL, 10);
    int tcp_port = wcstol(tcp_port_str, NULL, 10);

    wchar_t prompt[MAX_LINE];
    swprintf(prompt, MAX_LINE, L"The user '%ls' wants to send a file \"%ls\" (%zu bytes). Accept? (y/n): ", sender_username, filename_wide, filesize);
    int accept = yn_with_timeout(prompt, FILE_TIMEOUT_SECS);
    
    wchar_t packet[MAX_LINE];
    if (accept == 1) {
        
        // Send acceptance packet to server
        swprintf(packet, MAX_LINE, L"%d|%s", FILE_SEND_ACCEPT, filename);
        send_packet(sockfd, servaddr, packet);

        // Prepare arguments for the receive_file_tcp thread
        FileReceiveArgs *receiveargs = malloc(sizeof(FileReceiveArgs));
        char ip_str[MAX_IP_SIZE] = {0};
        wcstombs(ip_str, sender_ip_str, sizeof(ip_str));
        strcpy(receiveargs->ip, ip_str); // Sender IP
        receiveargs->port = tcp_port; // TCP port
        strcpy(receiveargs->filename, filename); // Filename
        receiveargs->filesize = filesize; // Filesize

        pthread_t tid;
        pthread_create(&tid, NULL, receive_file_tcp, (void*)receiveargs);
        pthread_detach(tid);

    } else {
        // Send decline packet to server
        swprintf(packet, MAX_LINE, L"%d|%s", FILE_SEND_DECLINE, filename);
        send_packet(sockfd, servaddr, packet);
    }
}


/**
 * Asks the user for a yes/no answer with a timeout.
 * Returns 1 for yes, 0 for no, and -1 on error or timeout.
 *
 * @param prompt The prompt to display to the user.
 * @param timeout_secs The number of seconds to wait for a response.
 *
 * @return -1 on error or timeout,
           1 for yes, 
           0 for no.
*/
int yn_with_timeout(const wchar_t *prompt, int timeout_secs) {

    while (1) {
        wprintf(L"%ls", prompt);
        fflush(stdout);

        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set);
        FD_SET(0, &set);

        timeout.tv_sec = timeout_secs;
        timeout.tv_usec = 0;

        int answered = select(1, &set, NULL, NULL, &timeout);
        if (answered == -1) {
            wprintf(L"\nError in select().\n");
            fflush(stdout);
            return -1;
        } else if (answered == 0) {
            wprintf(L"\nTime expired waiting for response.\n");
            fflush(stdout);
            return -1;
        } else {
            wchar_t resposta[8];
            if (fgetws(resposta, 8, stdin) == NULL) {
                wprintf(L"\nError reading response\n");
                fflush(stdout);
                return -1;
            }
            
            if (resposta[0] == L'y' || resposta[0] == L'Y')
                return 1;
            else
                return 0;
        }
    }
}


/**
 * Receives a file via TCP from the sender.
 *
 * Connects to the sender's TCP server, receives the file in chunks, and writes it to disk.
 * Handles errors in connecting or writing the file, and notifies the user of the result.
 *
 * @param fileReceiveArg Pointer to FileReceiveArgs structure with connection and file info.
 * @return NULL
 */
void* receive_file_tcp(void* fileReceiveArg) {
    
    FileReceiveArgs* args = (FileReceiveArgs*)fileReceiveArg;
    const char* ip = args->ip;
    int port = args->port;
    const char* filename = args->filename;
    size_t filesize = args->filesize;

    // Create TCP socket for receiving the file
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Attempt to connect to the sender's TCP server
    if (connect(tcp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        // If connection fails, notify the user and clean up
        while(atomic_load(&ThreadData.signalFilePrint) != 0) {
            say_no_to_busy_waiting();   
        }
        swprintf(ThreadData.FilePrint, MAX_LINE, L"Error connecting to TCP server at %s:%d.", ip, port);
        atomic_store(&ThreadData.signalFilePrint, 1);

        close(tcp_sock);
        free(args);
        return NULL;
    }

    // Open the local file for writing
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        // If file cannot be created, notify the user and clean up
        while(atomic_load(&ThreadData.signalFilePrint) != 0) {
            say_no_to_busy_waiting();   
        }
        swprintf(ThreadData.FilePrint, MAX_LINE, L"Error creating local file '%s'.", filename);
        atomic_store(&ThreadData.signalFilePrint, 1);
        
        close(tcp_sock);
        free(args);
        return NULL;
    }

    // Receive the file in chunks and write to disk
    char buffer[1024];
    size_t received, total = 0;
    while ((received = recv(tcp_sock, buffer, sizeof(buffer), 0)) > 0 && total < filesize) {
        fwrite(buffer, 1, received, fp);
        total += received;
    }

    fclose(fp);
    close(tcp_sock);

    // Notify the user that the file was received successfully
    while(atomic_load(&ThreadData.signalFilePrint) != 0) {
        say_no_to_busy_waiting();   
    }
    wcscpy(ThreadData.FilePrint, L"File received successfully.\n");
    atomic_store(&ThreadData.signalFilePrint, 1);
    
    free(args);
    return NULL;
}
