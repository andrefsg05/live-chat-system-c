#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>

#include "client.h"
#include "client_groups.h"
#include "client_threads.h"
#include "client_messages.h"


/**
 * Handles the group management page, allowing the user to select and perform group actions.
 *
 * This function presents the user with group-related options such as messaging a group,
 * inviting users, leaving, kicking, creating, or deleting a group. It reads user input,
 * processes the selected option, and calls the appropriate action handler.
 */
void group_page() {
    wchar_t group_name[MAX_USER_INFO] = {0};
    wchar_t username[MAX_USER_INFO] = {0};
    wchar_t command[MAX_LINE] = {0};

    // Read the user's command input (option selection)
    read_command_with_interrupt(command, MAX_LINE);
    int group_option = wcstol(command, NULL, 10); // Convert wide string to integer
    PacketType packet_type = 0;

    // Handle group commands based on user selection
    switch(group_option){
        case 0: // Go back to main menu
            return;
        case 1: // Send message to group
            wprintf(L"\nEnter group's name: "); 
            fflush(stdout);
            read_command_with_interrupt(group_name, MAX_USER_INFO);
            
            if(!request_message(MSG_GROUP_REQUEST, group_name)) {
                return; // Return to main menu
            }
            send_messages(MSG_GROUP);
            return;

        case 2: // Invite user to group
            wprintf(L"\nEnter group's name:"); 
            fflush(stdout);
            read_command_with_interrupt(group_name, MAX_USER_INFO);

            wprintf(L"\nEnter username to invite:"); 
            fflush(stdout);
            read_command_with_interrupt(username, MAX_USER_INFO);
            packet_type = GROUP_INVITE;

        break;
        case 3: // Leave group
            wprintf(L"\nEnter Group name:"); 
            fflush(stdout);
            read_command_with_interrupt(group_name, MAX_USER_INFO);
            packet_type = GROUP_LEAVE;

        break;
        case 4: // Kick user from group
            wprintf(L"\nEnter Group name:");
            fflush(stdout);
            read_command_with_interrupt(group_name,MAX_USER_INFO);

            wprintf(L"\nEnter username to kick:");
            fflush(stdout);
            read_command_with_interrupt(username,MAX_USER_INFO);
            packet_type = GROUP_KICK;

        break;                           
        case 5: // Create group
            wprintf(L"\nEnter Group name:");
            fflush(stdout);
            read_command_with_interrupt(group_name,MAX_USER_INFO);
            packet_type = GROUP_CREATE;

        break;
        case 6: // Delete group
            wprintf(L"\nEnter Group name:");
            fflush(stdout);
            read_command_with_interrupt(group_name,MAX_USER_INFO);
            packet_type = GROUP_DELETE;

        break;
        default:
            wprintf(L"\nInvalid Option. Please enter a number from 0 to 6\n");
            fflush(stdout);
            return;
    } 

    // Perform the requested group action
    request_group_action(packet_type, group_name, username);
}


/**
 * Sends a group action request to the server and waits for a response.
 *
 * This function builds the appropriate packet for the group action (create, invite, leave, kick, delete),
 * sends it to the server, and waits for a response with a timeout. It provides user feedback
 * based on the server's reply.
 *
 * @param group_type The type of group action to perform.
 * @param group_name The name of the group involved in the action.
 * @param username The username involved in the action (if applicable).
 */
void request_group_action(PacketType group_type, const wchar_t *group_name, const wchar_t *username) {
    wchar_t packet[MAX_LINE] = {0};

    // Build the packet according to the action type
    switch (group_type) {
        case GROUP_CREATE:
        case GROUP_LEAVE:
        case GROUP_DELETE:
            swprintf(packet, MAX_LINE, L"%d|%ls", group_type, group_name);
            break;
        case GROUP_INVITE:
        case GROUP_KICK:
            swprintf(packet, MAX_LINE, L"%d|%ls|%ls", group_type, group_name, username);
            break;
        default:
            wprintf(L"Invalid group action.\n");
            fflush(stdout);
            return;
    }

    // Reset the signal for group action response
    atomic_store(&ThreadData.signalGroupAction, 0);
    send_packet(ThreadData.sockfd, &ThreadData.servaddr, packet);

    int timeout_count = 0;
    PacketType answer = 0;

    // Wait for server response or timeout
    while ((answer = atomic_load(&ThreadData.signalGroupAction)) == 0) {
        if (timeout_count >= MAX_TIMEOUT) {
            wprintf(L"\nRequest timed out. Please try again later.\n");
            fflush(stdout);
            return;
        }
        sleep(TIMEOUT_TIME);
        timeout_count++;
    }

    // Handle server response
    if (answer == GROUP_OK) {
        wprintf(L"\nGroup action successful.\n");
        fflush(stdout);
    } else if(answer == GROUP_NOK) {
        wcscpy(packet, ThreadData.GroupAction);
        wprintf(L"\nGroup action failed: %ls\n", packet);
        fflush(stdout);
    }else{
        wprintf(L"\nGroup action failed.\n");
        fflush(stdout);
    }

    // Reset the signal for future actions
    atomic_store(&ThreadData.signalGroupAction, 0);
}