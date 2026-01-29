#ifndef SERVERGROUPS_H
#define SERVERGROUPS_H


#include "list.h"
#include <wchar.h>
#include <arpa/inet.h>
#include <sys/socket.h>


// Server group management functions
void server_create_group(List registered_groups, List connected_clients, struct sockaddr_in *groupOwner_addr, int sockfd, wchar_t *group_name);

void server_invite_to_group(List registered_groups, List registered_clients, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *rest);
void server_kick_from_group(List registered_groups, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *rest);
void server_leave_from_group(List registered_groups, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *group_name);

void server_delete_group(List registered_groups, List connected_clients, struct sockaddr_in *client_addr, int sockfd, wchar_t *group_name);

#endif /* SERVERGROUPS_H */