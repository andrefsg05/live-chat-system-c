#include "client_terminal.h"
#include "packet_types.h"
#include "client_authentication.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <termios.h>
#include <unistd.h>


/**
 * Function to display the landing page and handle user input for login or registration.
 * This function prompts the user for their choice, username, and password,
 * and sends the appropriate packet to the server.
 *
 * @param sockfd The socket file descriptor for communication with the server.
 * @param servaddr Pointer to the sockaddr_in structure containing server address information.
 *
 * @return false if the user chooses to exit, true otherwise.
 */
bool landing_page(int sockfd, struct sockaddr_in *servaddr) {
    PacketType packet_type;
    int choice;

    wprintf(L"Welcome to the UDP CHAT RC Application!\n");
    wprintf(L"1. LOGIN\n2. REGISTER\n3. EXIT\n");
    wprintf(L"Please enter your choice: ");
    wscanf(L"%d", &choice);
    getwchar(); // Consume newline left in input buffer

    switch (choice) {
        case LOGIN:
        case REGISTER:
            packet_type = (choice == LOGIN) ? LOGIN : REGISTER;
            return authenticate_user(packet_type, sockfd, servaddr);  
            break;

        case EXIT:
            return false;
    }

    return false;
}


/**
 * Reads a limited wide-character string from user input with a prompt.
 *
 * This function enables raw mode, displays a prompt, and reads user input up to max_len-1 characters.
 * It handles backspace/delete, enforces the length limit, and disables raw mode after input.
 *
 * @param buffer Buffer to store the input string.
 * @param max_len Maximum number of characters to read (including null terminator).
 * @param prompt Prompt to display to the user.
 */
void read_limited_wide_input(wchar_t *buffer, int max_len, const wchar_t *prompt) {
    enable_raw_mode();

    wprintf(L"%ls", prompt);
    fflush(stdout);

    int len = 0;
    int ch;
    while (1) {
        ch = getwchar();
        if (ch == L'\n' || ch == L'\r') break;
        if (ch == EOF) break;

        // Handle backspace (ASCII 127 or 8) 
        if ((ch == 127 || ch == 8)) {
            if (len > 0) {
                len--;
                // Move cursor back, overwrite with space, move back again
                wprintf(L"\b \b");
                fflush(stdout);
            }
            continue;
        }

        if (len < max_len - 1) {
            buffer[len++] = (wchar_t)ch;
            putwchar((wchar_t)ch);
            fflush(stdout);
        } else {
            // Already at limit, beep and ignore further input
            putwchar(L'\a');
            fflush(stdout);
        }
    }

    buffer[len] = L'\0';

    // Always consume the rest of the line after input
    while (ch != '\n' && ch != '\r' && ch != EOF) {
        ch = getwchar();
    }
    wprintf(L"\n");

    disable_raw_mode();
}


/**
 * Disables user input by modifying terminal settings.
 *
 * This function saves the current terminal settings and disables canonical mode,
 * echo, and signal processing to prevent user input.
 */
void disable_input() {
    // Save current terminal settings
    tcgetattr(STDIN_FILENO, &old_terminal);
    new_terminal = old_terminal;

    // Modify the new settings to disable input
    new_terminal.c_lflag &= ~(ICANON | ECHO | ISIG); // Turn off canonical mode and echo
    new_terminal.c_cc[VMIN] = 0;              // No minimum input
    new_terminal.c_cc[VTIME] = 0;             // No timeout

    // Apply the new settings
    tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
}


/**
 * Enables user input by restoring original terminal settings.
 *
 * This function flushes any pending input and restores the terminal to its original settings.
 */
void enable_input() {
    // Restore the original terminal settings
    tcflush(STDIN_FILENO, TCIFLUSH); // Flush any pending input
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
}


/**
 * Enables raw mode for terminal input.
 *
 * This function saves the current terminal settings and disables canonical mode and echo,
 * allowing character-by-character input.
 */
void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &old_terminal);      // Save current terminal settings
    new_terminal = old_terminal;
    new_terminal.c_lflag &= ~(ICANON | ECHO);    // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
}


/**
 * Disables raw mode and restores terminal settings.
 *
 * This function restores the terminal to the previously saved settings.
 */
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal); // Restore terminal settings
}