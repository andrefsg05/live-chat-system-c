#include "client_authentication.h"

#include <wchar.h>
#include <stdio.h>
#include <sys/select.h>


/**
 * Function to authenticate the user by sending credentials to the server.
 * It handles both login and registration based on the packet type.
 * Timeouts are managed to ensure the user has multiple attempts to authenticate.
 *
 * @param packet_type The type of packet (LOGIN or REGISTER).
 * @param sockfd The socket file descriptor for communication with the server.
 * @param servaddr Pointer to the sockaddr_in structure containing server address information.
 *
 * @return true if authentication is successful, false otherwise.
 */
bool authenticate_user(PacketType packet_type, int sockfd, struct sockaddr_in *servaddr) {
    wchar_t username[MAX_USER_INFO];
    wchar_t password[MAX_USER_INFO];
    wchar_t packet[MAX_LINE];
    wchar_t response[MAX_LINE];
    int bytes_recieved;
    int n_timeout = 0;
    socklen_t len = sizeof(*servaddr);

    bool is_authenticated = false;

    while (!is_authenticated) {
        // Prompt for credentials
        read_limited_wide_input(username, MAX_USER_INFO, L"Username (max 31 chars): ");
        read_limited_wide_input(password, MAX_USER_INFO, L"Password (max 31 chars): ");
        n_timeout = 0;

        // Prepare authentication packet
        swprintf(packet, MAX_LINE, L"%d|%ls|%ls", packet_type, username, password);

        while (n_timeout < MAX_TIMEOUT) {
            // Send authentication request to the server
            send_packet(sockfd, servaddr, packet);

            // Set up select for timeout
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            struct timeval tv;
            tv.tv_sec = TIMEOUT_TIME;  // Wait for TIMEOUT_TIME seconds
            tv.tv_usec = 0;

            int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

            // Check if there is data available to read
            if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
                // Data is available, receive response
                bytes_recieved = recvfrom(sockfd, response, WCHAR_BUFFER_SIZE, 0, (struct sockaddr *)servaddr, &len);
                if (bytes_recieved < 0) {
                    // perror("Error receiving response from server");
                    wprintf(L"Error receiving response from server");
                    continue; // Try again
                }

                // Check if authentication was successful
                int answer_type = wcstol(response, NULL, 10); // Convert wide string to integer
                
                if (answer_type == LOGIN_OK) {
                    is_authenticated = true;
                    break; // Exit inner loop and function
                    
                } else {
                    wprintf(L"Authentication failed. Please try again.\n");
                    break; // Prompt user for new credentials
                }

            // Check if select timed out
            } else if (activity == 0) {
                // Timeout, resend authentication packet
                n_timeout++;
                wprintf(L"No response from server, retrying...\n");
                continue; // Resend packet, do not prompt user again

            // Handle select error
            } else {
                // perror("select error");
                wprintf(L"select error");
                break;
            }
        }
        if (n_timeout >= MAX_TIMEOUT) {
            wprintf(L"Maximum retries reached. Exiting...\n");
            return false; // Exit the authentication loop
        }
    }
    

    // If we reach here, the user is authenticated
    if (packet_type == LOGIN) {
        wprintf(L"Welcome back, %ls!", username);
    } else if (packet_type == REGISTER) {
        wprintf(L"Registration successful for user %ls!\n", username);
    }
    return true; // User is authenticated
}
