#include "serverGroups.h"
#include "groups.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Creates a new group with the given name and assigns the requesting client as the owner.
 * This function first checks if the group already exists and if the requester is a connected client.
 * If all conditions are met, it registers the group and sends confirmation.
 *
 * CREATE_GROUP|<group_name>
 *
 * @param registered_groups List of all registered groups.
 * @param connected_clients List of all currently connected clients.
 * @param groupOwner_addr The address of the client requesting group creation.
 * @param sockfd The socket file descriptor for sending responses.
 * @param group_name The desired name of the new group.
 */
void server_create_group(List registered_groups, List connected_clients, struct sockaddr_in *groupOwner_addr, int sockfd, wchar_t *group_name) {
    wchar_t msg[MAX_LINE] = {0};

    // Check if the clients list is valid
    if (connected_clients == NULL) {
        swprintf(msg, MAX_LINE, L"%d|%ls", GROUP_NOK, L"Invalid clients list.");
        wprintf(msg);
        send_packet(sockfd, groupOwner_addr, msg);
        return;
    }

    // Check if the group already exists
    if (find_group_by_name(registered_groups, group_name) != NULL) {
        swprintf(msg, MAX_LINE, L"%d|%ls", GROUP_NOK, L"Group already exists.");
        wprintf(msg);
        send_packet(sockfd, groupOwner_addr, msg);
        return;
    }

    // Ensure the requesting client is known and connected
    Client *groupOwner = find_client_by_address(connected_clients, groupOwner_addr);
    if (groupOwner == NULL) {
        swprintf(msg, MAX_LINE, L"%d|%ls", GROUP_NOK, L"Client not found.");
        wprintf(msg);
        send_packet(sockfd, groupOwner_addr, msg);
        return;
    }

    // Attempt to register the group
    if (create_group(group_name, groupOwner->username, registered_groups) != 1) {
        swprintf(msg, MAX_LINE, L"%d|Failed to create %ls group.", GROUP_NOK, group_name);
        wprintf(msg);
        send_packet(sockfd, groupOwner_addr, msg);
        return;
    }

    // Send confirmation to the group owner
    swprintf(msg, MAX_LINE, L"%d", GROUP_OK);
    send_packet(sockfd, groupOwner_addr, msg);
}


/**
 * Adds a specified user to an existing group.
 * Attempts to add the user.
 * Sends confirmations or error messages accordingly to both inviter and invited.
 *
 * GROUP_INVITE|<group_name>|<username_to_invite>
 *
 * @param registered_groups List of all registered groups.
 * @param registered_clients List of all registered clients. 
 * @param connected_clients List of currently connected clients.
 * @param client_addr The address of the client performing the invitation.
 * @param sockfd The socket file descriptor for sending responses.
 * @param rest A string containing the group name and the username to invite, separated by '|'.
 */
void server_invite_to_group(List registered_groups, List registered_clients, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *rest) {
    wchar_t oKmsg[MAX_LINE] = {0};
    wchar_t msg[MAX_LINE] = {0};

    // Parse the input: group name and user to invite
    wchar_t *context;
    wchar_t *group_name = wcstok(rest, L"|", &context);
    wchar_t *username_to_invite = wcstok(NULL, L"|", &context);

    if (registered_groups == NULL) {
        swprintf(msg, MAX_LINE, L"%d|%ls", GROUP_NOK, L"Invalid group list.");
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }


    bool found = false;
    Position pos = firstList(registered_clients);
    while (pos != NULL) {
        Client *client = (Client *)retrieveElementList(pos);
        if (wcscmp(client->username, username_to_invite) == 0) {
            found = true;
            break;
        }
        pos = advanceList(pos);
    }


    // Attempt serch for the user and to add the user to the group
    if(!found || add_user_to_group(registered_groups, group_name, username_to_invite, 0) == 0) {
        swprintf(msg, MAX_LINE, L"%d|Failed to add %ls to group %ls.", GROUP_NOK, username_to_invite, group_name);
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }

    // Send confirmation to the client who invited
    swprintf(oKmsg, MAX_LINE, L"%d", GROUP_OK);
    send_packet(sockfd, client_addr, oKmsg);

    // Find the invited client and notify them
    Client *invited_client = find_client_by_username(connected_clients, username_to_invite);
    swprintf(msg, MAX_LINE, L"%d|You were invited to group \"%ls\".", GROUP_INFO, group_name);
    send_packet(sockfd, &invited_client->clientAddress, msg);
}


/**
 * Removes a specific user from a group.
 * Validates the request, ensures the sender is the owner,
 * then updates the group and notifies both the kicker and the kicked user.
 *
 * GROUP_KICK|<group_name>|<username_to_kick>
 *
 * @param registered_groups List of registered groups.
 * @param connected_clients List of connected clients.
 * @param client_addr Address of the client issuing the kick.
 * @param sockfd Socket descriptor for sending messages.
 * @param rest A wide string containing the group name and the username to kick.
 */
void server_kick_from_group(List registered_groups, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *rest) {
    wchar_t oKmsg[MAX_LINE] = {0};
    wchar_t msg[MAX_LINE] = {0};

    // Parse the group name and target username
    wchar_t *context;
    wchar_t *group_name = wcstok(rest, L"|", &context);
    wchar_t *username_to_kick = wcstok(NULL, L"|", &context);

    if (registered_groups == NULL) {
        swprintf(msg, MAX_LINE, L"%d|Invalid group list.", GROUP_NOK);
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }

    // Verifies if the requester is connected
    Client *client = find_client_by_address(connected_clients, client_addr);
    if (client == NULL) {
        swprintf(msg, MAX_LINE, L"%d|Invalid client address.", GROUP_NOK);
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }

    // Remove user from group (checks if requester is owner)
    if (remove_user_from_group(registered_groups, group_name, client->username, username_to_kick) == 0) {
        swprintf(msg, MAX_LINE, L"%d|Failed to remove %ls from group %ls.\n", GROUP_NOK, username_to_kick, group_name);
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }

    // Send confirmation to the client who kicked
    swprintf(oKmsg, MAX_LINE, L"%d", GROUP_OK);
    send_packet(sockfd, client_addr, oKmsg);

    // Notify the kicked user
    Client *kicked_client = find_client_by_username(connected_clients, username_to_kick);
    swprintf(msg, MAX_LINE, L"%d|You were kicked from group \"%ls\"", GROUP_INFO, group_name);
    send_packet(sockfd, &kicked_client->clientAddress, msg);
}


/**
 * Allows a client to voluntarily leave a group.
 * Verifies that the client is connected and attempts to remove them from the group.
 * Sends a success or failure response accordingly.
 *
 * GROUP_LEAVE|<group_name>
 *
 * @param registered_groups List of all groups.
 * @param connected_clients List of connected clients.
 * @param client_addr Address of the client who wants to leave the group.
 * @param sockfd Socket used to send the server response.
 * @param group_name Name of the group to leave.
 */
void server_leave_from_group(List registered_groups, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *group_name) {
    wchar_t msg[MAX_LINE] = {0};

    // Verifies if the requester is connected
    Client *client = find_client_by_address(connected_clients, client_addr);
    if (client == NULL) {
        swprintf(msg, MAX_LINE, L"%d|%ls", GROUP_NOK, L"Client not found.");
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }

    // Remove the client from the group
    if (remove_user_from_group(registered_groups, group_name, client->username, client->username) == 0) {
        swprintf(msg, MAX_LINE, L"%d|Failed to leave group %ls.", GROUP_NOK, group_name);
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }

    // Notify the client of successful leave
    swprintf(msg, MAX_LINE, L"%d", GROUP_OK);
    send_packet(sockfd, client_addr, msg);
}


/**
 * Deletes a group owned by the requesting client.
 * Validates the client and confirms ownership before deleting the group.
 *
 * GROUP_DELETE|<group_name>
 *
 * @param registered_groups List of all registered groups.
 * @param connected_clients List of currently connected clients.
 * @param client_addr Address of the client who wants to delete the group.
 * @param sockfd Socket used for server-to-client communication.
 * @param group_name The name of the group to be deleted.
 */
void server_delete_group(List registered_groups, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *group_name){
    wchar_t msg[MAX_LINE] = {0};
    
    // Verifies if the requester is connected
    Client *client = find_client_by_address(connected_clients, client_addr);
    if (client == NULL) {
        swprintf(msg, MAX_LINE, L"%d|%ls", GROUP_NOK, L"Invalid client address.");
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }
    
    // Attempt to delete the group; success only if requester is owner
    if (delete_group(registered_groups, client->username, group_name) == 0) {
        swprintf(msg, MAX_LINE, L"%d|Failed to delete group %ls.", GROUP_NOK, group_name);
        wprintf(msg);
        send_packet(sockfd, client_addr, msg);
        return;
    }

    // Send confirmation to the deleter client
    swprintf(msg, MAX_LINE, L"%d", GROUP_OK);
    send_packet(sockfd, client_addr, msg);
}