#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <locale.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>


#include "client.h"


// Struct to hold thread data for communication
ThreadStruct ThreadData;


// Struct to hold old and new terminal settings
struct termios old_terminal, new_terminal;


int main(int argc, char *argv[]) {

    // Set locale for wide character support
    setlocale(LC_ALL, "");
    
    bool exit_app;
    int sockfd;
    struct sockaddr_in servaddr;
    
    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        // perror("Socket creation failed");
        wprintf(L"Socket creation failed");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    create_address(&servaddr);

    // Initialize the thread data structure
    initialize_thread_data(&ThreadData, sockfd, &servaddr);


    // Display the landing page and handle user input
    exit_app = landing_page(sockfd, &servaddr);

     if(!exit_app) {
        wprintf(L"Exiting the application...\n");
        fflush(stdout);
        close(sockfd);
        return 0; // Exit if user chooses to exit
    }

    // recvfrom() -> MSG Warning
    pthread_t recv_tid, heartbeat_tid;
    pthread_create(&recv_tid, NULL, receiver_thread, &sockfd);
    pthread_create(&heartbeat_tid, NULL, heartbeat_thread, &sockfd);

    wchar_t recipient[MAX_USER_INFO] = {0};
    wchar_t filename[MAX_FILENAME] = {0};

    while (true) {
        wchar_t command[MAX_LINE] = {0};
        read_command_with_interrupt(command, MAX_LINE);
        
        if (wcslen(command) == 0) {
            continue;
        }

        if (wcscmp(command, L"/exit") == 0){
            // Exit command
            wchar_t exit_packet[MAX_LINE];
            swprintf(exit_packet, MAX_LINE, L"%d", EXIT);
            send_packet(sockfd, &servaddr, exit_packet);
            wprintf(L"Exiting the application...\n");
            fflush(stdout);
            break;
        }
        else if(wcscmp(command, L"/broadcast") == 0){
            if(!request_message(MSG_BROADCAST_REQUEST, NULL)) {
                wprintf(L"\nYou cannot send broadcast messages at this time.\n");
                fflush(stdout);
                continue; // Skip sending broadcast message
            }
            send_messages(MSG_BROADCAST);
        } 
        else if (wcscmp(command, L"/group") == 0) {
            wprintf(
                L"Choose the desired group command from (0-6):\n"
                L"    1. message <group_name>\n"
                L"    2. invite <group_name> <username>\n"
                L"    3. leave <group_name>\n"
                L"    4. kick <group_name> <username>\n"
                L"    5. create <group_name>\n"
                L"    6. disband <group_name>\n"
                L"    0. back to main menu\n"
                L"    Option: "
                
            );            
            fflush(stdout);
            group_page();
        } 
        else if (wcscmp(command, L"/pm") == 0) {
            // Handle private message command
            wprintf(L"\nEnter recipient's name: ");
            fflush(stdout);
            read_command_with_interrupt(recipient, MAX_LINE);

            if(!request_message(MSG_ONE_REQUEST, recipient)) {
                wprintf(L"\nYou cannot send private messages at this time.\n");
                fflush(stdout);
                continue; // Skip sending private message
            }
            send_messages(MSG_ONE);
        }
        else if (wcscmp(command, L"/sendfile") == 0) {
            wprintf(L"\nEnter file's name: ");
            fflush(stdout);
            read_command_with_interrupt(filename, MAX_FILENAME);
            // Handle file sending command
            send_file(sockfd, &servaddr, filename);
        }
        else if (wcscmp(command, L"/help") == 0){
            wprintf(
                L"\nAvailable commands:\n"
                L"  /help            - Show this help message\n"
                L"  /exit            - Disconnect and exit the application\n"
                L"  /broadcast       - Send a message to all users\n"
                L"  /pm              - Send a private message to a specific user\n"
                L"  /group           - Manage group messaging and memberships\n"
                L"                     Options: message, invite, leave, kick, create, disband\n"
                L"  /sendfile        - Send a file to another user or group\n"
            );
            fflush(stdout);

        }else {
            // Handle unknown command
            wprintf(L"Unknown command: %ls\n", command);
            fflush(stdout);

            wprintf(L"Type /help for a list of available commands.\n");
            fflush(stdout);

        }
    }

    
    pthread_cancel(recv_tid);
    pthread_cancel(heartbeat_tid);

    pthread_join(recv_tid, NULL);
    pthread_join(heartbeat_tid, NULL);
    
    // Close the socket before exiting
    close(sockfd);

    return 0;
}


/**
 * Reads a command from user input, handling asynchronous server events.
 *
 * This function waits for user input from the terminal, but also checks for
 * asynchronous events (such as server warnings, announcements, group info, file warnings, etc.)
 * and displays them immediately, interrupting the input prompt if necessary.
 * The function uses select() for non-blocking input and flushes the input buffer
 * after handling any interrupting event, ensuring the user always sees the latest prompt.
 *
 * @param command Buffer to store the user command (wide string).
 * @param max_len Maximum length of the command buffer.
 */
void read_command_with_interrupt(wchar_t *command, size_t max_len) {
    int len = 0;
    int mode = 0;
    command[0] = L'\0';
    bool interrupt_received = false;

    wprintf(L"\n\r> ");
    fflush(stdout);

    while (1) {
        
        // Check for server warning signals (broadcast, private, or group)
        mode = atomic_load(&ThreadData.signalWarning);
        if (mode == MSG_BROADCAST_WARNING || mode == MSG_ONE_WARNING || mode == MSG_GROUP_WARNING) {
            
            // If a warning is present, display and handle it immediately
            receive_message(mode);
            fflush(stdout);

            interrupt_received = true;

        }

        // Check for server announcements
        if(atomic_load(&ThreadData.signalAnnouncement)) {
            wprintf(L"\n[Server]: %ls\n", ThreadData.Announcement);
            atomic_store(&ThreadData.signalAnnouncement, 0); // Reset the announcement signal
            fflush(stdout);

            interrupt_received = true;
        }

        // Check for group info messages
        if(atomic_load(&ThreadData.signalGroupInfo)) {
            wprintf(L"\n[Server]: %ls\n", ThreadData.GroupInfo);
            atomic_store(&ThreadData.signalGroupInfo, 0); // Reset the announcement signal
            fflush(stdout);

            interrupt_received = true;
        }

        // Check for file receive warnings
        if (atomic_load(&ThreadData.signalFileReceiveWarning)) {
            handle_file_receive_warning(); // Handle file receive warning
            atomic_store(&ThreadData.signalFileReceiveWarning, 0); // Reset the file receive warning signal

            interrupt_received = true;
        }

        // Check for file print signals
        if (atomic_load(&ThreadData.signalFilePrint)) {
            wprintf(L"\n%ls\n", ThreadData.FilePrint);
            atomic_store(&ThreadData.signalFilePrint, 0); // Reset the file print signal
            fflush(stdout);

            interrupt_received = true; 
        }


        // If any interrupt was handled, flush input and reprint prompt
        if (interrupt_received) {
            tcflush(STDIN_FILENO, TCIFLUSH); // Clear any pending input
            wprintf(L"\n\r> ");
            fflush(stdout);
            interrupt_received = false;
        }


        // Use select() for non-blocking input with a timeout
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        if (ready > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            // If input is available, read it
            if (fgetws(command, max_len, stdin) != NULL) {
                len = wcslen(command);
                // Remove trailing newline
                if (len > 0 && (command[len - 1] == L'\n' || command[len - 1] == L'\r')) {
                    command[len - 1] = L'\0';
                }
                break; // Exit loop after successful input
            }
        }
        // If not ready, just loop again (allows message check)

    }

}


/**
 * Function to create a server address structure.
 * This function initializes the sockaddr_in structure with the server's IP and port.
 * 
 * @param address Pointer to the sockaddr_in structure to be initialized.
 */
void create_address(struct sockaddr_in *address) {
    // Initialize the sockaddr_in structure to zero
    memset(address, 0, sizeof(*address));

    // Set the address family, port, and IP address
    address->sin_family = AF_INET;  // IPv4
    address->sin_port = htons(SERVER_PORT);  // Port number in network byte order
    address->sin_addr.s_addr = inet_addr(SERVER_IP);  // Accept connections from any IP address
}