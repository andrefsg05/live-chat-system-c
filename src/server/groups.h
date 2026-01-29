#ifndef groups_h
#define groups_h


#include <wchar.h>
#include <stdbool.h>

#include "list.h"
#include "userAuthentication.h"


// Structure to hold user information
typedef struct {
    wchar_t groupName[MAX_USER_INFO]; // Group name
    wchar_t ownerName[MAX_USER_INFO]; // Group owner name
    List members; // List of members in the group wchar
} Group;


// Group management functions
int create_group(wchar_t *group_name, wchar_t *ownerUsername, List registeredGroups);
List load_groups();
Group *find_group_by_name(List registeredGroups, wchar_t *group_name);

int add_user_to_group_file(wchar_t *group_name, wchar_t *username, bool is_owner);
int add_user_to_group_list(Group* group, wchar_t *group_name, wchar_t *username);
int add_user_to_group (List registeredGroups, wchar_t *group_name, wchar_t *username, bool is_owner);

int remove_user_from_group_list(Group* group, wchar_t *group_name, wchar_t *username_kicker, wchar_t *username_to_kick);
int remove_user_from_group_file(wchar_t *group_name, wchar_t *username_to_kick);
int remove_user_from_group(List registeredGroups, wchar_t *group_name, wchar_t *username_kicker, wchar_t *username_to_kick);

int delete_group(List registeredGroups, wchar_t *deleter_name, wchar_t *group_name);

#endif // groups_h