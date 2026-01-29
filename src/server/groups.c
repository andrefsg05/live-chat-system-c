#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "groups.h"

const char *groupsFilename = "Database/groups.csv";
const char *groupsTempFilename = "Database/groupsTemp.csv";


/**
 * Creates a new group and adds the owner as the first member.
 * Allocates memory for the new group, sets the owner, adds the owner to the members list,
 * and inserts the group into the global registeredGroups list. Also persists the group to file.
 *
 * @param group_name Name of the group to be created.
 * @param ownerUsername Username of the group owner.
 * @param registeredGroups List containing all registered groups.
 * @return 1 if the group was created successfully, 0 otherwise.
 */
int create_group(wchar_t *group_name, wchar_t *ownerUsername, List registeredGroups){

    if (group_name == NULL || ownerUsername == NULL || registeredGroups == NULL) {
        printf("Invalid parameters for creating a group.\n");
        return 0; // Failure
    }

    // Allocates memory for the new group
    Group *new_group = (Group *)malloc(sizeof(Group));
    if (new_group == NULL) {
        perror("Out of memory!");
        exit(EXIT_FAILURE);
    }

    wcscpy(new_group->groupName, group_name);
    wcscpy(new_group->ownerName, ownerUsername);
    new_group->members = createList(); // Create an empty list for members
    

    // Insert the owner on the members list
    wchar_t *owner_member = (wchar_t *)malloc((wcslen(ownerUsername) + 1) * sizeof(wchar_t));
    if (owner_member == NULL) {
        perror("Out of memory!");
        exit(EXIT_FAILURE);
    }
    wcscpy(owner_member, ownerUsername);
    insert((void *)owner_member, new_group->members, headerList(new_group->members));

    // Insert the new group on the registeredGroups list
    insert((void *)new_group, registeredGroups, headerList(registeredGroups));

    // Save the group to groups file with the owner as a member (1 if owner, 0 otherwise)
    add_user_to_group_file(group_name, ownerUsername, 1); 

    return 1; // Success
}


/**
 * Loads all groups from the persistent CSV file into a List structure in memory.
 * Parses each line and organizes users into the corresponding groups.
 * If the group doesn't exist in memory yet, it's created.
 *
 * @return A list of all groups loaded from the file.
 */
List load_groups(){
    
    List registeredGroups = createList(); 

    FILE *file = fopen(groupsFilename, "r");
    if (!file) {
        printf("File %s not found. Creating new file on save.\n", groupsFilename); 
        return registeredGroups; // If the file does not exist, return NULL
    }

    // Parse line into groupName, username, userType
    wchar_t line[MAX_LINE];
    while (fgetws(line, MAX_LINE, file)) {
        wchar_t *token;
        wchar_t *context;
        wchar_t groupName[MAX_USER_INFO];
        wchar_t username[MAX_USER_INFO];
        wchar_t userType[MAX_USER_INFO];
        

        token = wcstok(line, L",", &context);
        if (token){
            wcsncpy(groupName, token, sizeof(groupName)/sizeof(wchar_t) - 1);
            groupName[sizeof(groupName)/sizeof(wchar_t) - 1] = L'\0';

            token = wcstok(NULL, L",\n", &context);
            if (token){
                wcsncpy(username, token, sizeof(username)/sizeof(wchar_t) - 1);
                username[sizeof(username)/sizeof(wchar_t) - 1] = L'\0';

                token = wcstok(NULL, L",\n", &context);
                if (token){
                    wcsncpy(userType, token, sizeof(userType)/sizeof(wchar_t) - 1);
                    userType[sizeof(userType)/sizeof(wchar_t) - 1] = L'\0';
                }


                Group *group = find_group_by_name(registeredGroups, groupName);
 
                if (group == NULL) {
                    // If the group does not exist, create it and add to list
                    group = (Group *)malloc(sizeof(Group));
                    if (group == NULL) {
                        perror("Out of memory!");
                        exit(EXIT_FAILURE);
                    }
                    wcscpy(group->groupName, groupName);

                    group->members = createList(); // Create an empty list for members          
                    
                    insert((void *)group, registeredGroups, headerList(registeredGroups));
                }

                // Owner is identified by userType == "1"
                if(wcscmp(userType, L"1") == 0) {
                    wcscpy(group->ownerName, username);
                }
                
                // Add the user to the group list
                wchar_t *new_member = (wchar_t *)malloc((wcslen(username) + 1) * sizeof(wchar_t));
                if (new_member == NULL) {
                    perror("Out of memory!");
                    exit(EXIT_FAILURE);
                }
                wcscpy(new_member, username);  

                insert((void *)new_member, group->members, headerList(group->members));
            }

        }

    }

    fclose(file);
    return registeredGroups;
}


/**
 * Adds a user to the in-memory members list of a group if they are not already present.
 *
 * @param group Pointer to the group.
 * @param group_name Name of the group.
 * @param username Username to be added.
 * @return 1 if the user was added successfully, 0 if already present or failed.
 */
int add_user_to_group_list(Group* group, wchar_t *group_name, wchar_t *username){

    if (group == NULL) {
        printf("Group %ls not found.\n", group_name);
        return 0;
    }

    if (username == NULL || group_name == NULL) {
        printf("Invalid parameters for adding user to group.\n");
        return 0;
    }

    // Check if the user is already a member of the group
    Position pos = firstList(group->members);
    while (pos!=NULL) {
        wchar_t *member = (wchar_t *)retrieveElementList(pos);
        if (wcscmp(member, username) == 0) {
            printf("User %ls is already a member of group %ls.\n", username, group_name);
            return 0; // User already in the group
        }
        pos = advanceList(pos);
    }

    // Add the user to the group's member list
    wchar_t *new_member = (wchar_t *)malloc((wcslen(username) + 1) * sizeof(wchar_t));
    if (new_member == NULL) {
        perror("Out of memory!");
        exit(EXIT_FAILURE);
    }
    wcscpy(new_member, username);

    // Insert the new member into the group's member list
    insert((void*)new_member, group->members, headerList(group->members));
    
    return 1;
}


/**
 * Appends a user entry to the persistent CSV file.
 * Used to persist a user joining a group, including whether they're the owner.
 *
 * @param group_name Name of the group.
 * @param username Username being added.
 * @param is_owner Boolean indicating if the user is the owner.
 * @return 1 if successfully written to file, 0 on failure.
 */
int add_user_to_group_file(wchar_t *group_name, wchar_t *username, bool is_owner){

    if (group_name == NULL || username == NULL) {
        printf("Invalid parameters for adding user to group file.\n");
        return 0;
    }

    FILE *file = fopen(groupsFilename, "a"); // Open file in append mode
    if (!file) {
        printf("Error opening file %s.\n", groupsFilename);
        return 0;
    }

    fwprintf(file, L"%ls,%ls,%d\n", group_name, username, is_owner); // Write group name and username
    fclose(file);
    return 1;
}


/**
 * Adds a user to both the in-memory group structure and the file.
 * Rolls back if writing to file fails to maintain consistency.
 *
 * @param registeredGroups List of all registered groups.
 * @param group_name Name of the group.
 * @param username Username to be added.
 * @param is_owner Boolean indicating if the user is the owner.
 * @return 1 if added successfully, 0 otherwise.
 */
int add_user_to_group (List registeredGroups, wchar_t *group_name, wchar_t *username, bool is_owner){
    
    // Checks if the groups exists.
    Group *group = find_group_by_name(registeredGroups, group_name);
    if (group == NULL) {
        printf("Group %ls not found.\n", group_name);
        return 0; // Failure
    }
    
    // Add user to the in-memory group structure.
    if (add_user_to_group_list(group, group_name, username) == 0) {
        printf("Failed to add user %ls to group %ls in list.\n", username, group_name);
        return 0; // Failure
    }

    // Add user to the file
    if (add_user_to_group_file(group_name, username, is_owner) == 0) {
        // If adding to the file fails, we should remove the user from the group list
        remove_user_from_group_list(group, group_name, group->ownerName, username);
        printf("Failed to add user %ls to group %ls in file.\n", username, group_name);
        return 0; // Failure
    }

    return 1; // Success
}


/**
 * Finds a group in the registered groups list by its name.
 *
 * @param registeredGroups List of all registered groups.
 * @param group_name Name of the group to find.
 * @return Pointer to the group if found, NULL otherwise.
 */
Group *find_group_by_name(List registeredGroups, wchar_t *group_name) {

    if (registeredGroups == NULL || group_name == NULL) {
        printf("Invalid parameters for finding group by name.\n");
        return NULL;
    }

    Position pos = firstList(registeredGroups);
    while (pos != NULL) {
        Group *group = (Group *)retrieveElementList(pos);
        if (wcscmp(group->groupName, group_name) == 0) {
            return group; // Found the group
        }
        pos = advanceList(pos);
    }

    return NULL; // Group not found
}


/**
 * Removes a user from the in-memory group member list.
 * Does not modify the CSV file.
 *
 * @param group Pointer to the group.
 * @param group_name Name of the group.
 * @param username_kicker Username of the client attempting the removal.
 * @param username_to_kick Username of the member to be removed.
 * @return 1 if removed successfully, 0 otherwise.
 */
int remove_user_from_group_list(Group* group, wchar_t *group_name, wchar_t *username_kicker, wchar_t *username_to_kick){

    Position pos = firstList(group->members);
    while (pos!=NULL) {
        wchar_t *member = (wchar_t *)retrieveElementList(pos);
        if (wcscmp(member, username_to_kick) == 0) {
            delete(member, group->members);
            free(member);
            printf("User %ls has been removed from group %ls.\n", username_to_kick, group_name);
            return 1; // Removed successfully
        }
        pos = advanceList(pos);
    }

    return 0; // Fails to remove the user.
}


/**
 * Removes a user entry from the group CSV file by rewriting all entries except the target.
 * Uses a temporary file as an intermediate step.
 *
 * @param group_name Name of the group.
 * @param username_to_kick Username of the user to remove.
 * @return 1 if the user was removed from file, 0 otherwise.
 */
int remove_user_from_group_file(wchar_t *group_name, wchar_t *username_to_kick){

    FILE *file = fopen(groupsFilename, "r");
    if (!file) {
        printf("Error opening file %s for reading.\n", groupsFilename);
        return 0;
    }

    // Creates the temporary file
    FILE *temp_file = fopen(groupsTempFilename, "w");
    if (!temp_file) {
        fclose(file);
        printf("Error creating temporary file for writing.\n");
        return 0;
    }

    int removed = 0; // Used to mark if the user was removed

    // Parse the line: groupName, username, userType
    wchar_t line[MAX_LINE];
    while (fgetws(line, MAX_LINE, file)) {
        wchar_t *token;
        wchar_t *context;
        wchar_t groupName[MAX_USER_INFO];
        wchar_t username[MAX_USER_INFO];
        wchar_t userType[MAX_USER_INFO];
        

        token = wcstok(line, L",", &context);
        if (token){
            wcsncpy(groupName, token, sizeof(groupName)/sizeof(wchar_t) - 1);
            groupName[sizeof(groupName)/sizeof(wchar_t) - 1] = L'\0';

            token = wcstok(NULL, L",\n", &context);
            if (token){
                wcsncpy(username, token, sizeof(username)/sizeof(wchar_t) - 1);
                username[sizeof(username)/sizeof(wchar_t) - 1] = L'\0';

                token = wcstok(NULL, L",\n", &context);
                if (token){
                    wcsncpy(userType, token, sizeof(userType)/sizeof(wchar_t) - 1);
                    userType[sizeof(userType)/sizeof(wchar_t) - 1] = L'\0';
                }

                // Only copy entries that don't match both group and user
                if (wcscmp(groupName, group_name) != 0 || wcscmp(username, username_to_kick) != 0) {
                    fwprintf(temp_file, L"%ls,%ls,%ls\n", groupName, username, userType);
                } else {
                    removed = 1; // Mark that we did remove the user
                }
            }
        }
    }

    fclose(file);
    fclose(temp_file);

    // Replace original file with temp file
    if (remove(groupsFilename) != 0 || rename(groupsTempFilename, groupsFilename) != 0) {
        printf("Error updating groups file.\n");
        return 0;
    }

    return removed; // 1 if success, 0 otherwise
}


/**
 * Removes a user from both the in-memory group and the CSV file.
 * Only the owner can kick users, and the owner cannot be kicked.
 *
 * @param registeredGroups List of all registered groups.
 * @param group_name Name of the group.
 * @param username_kicker Username of the user attempting to kick.
 * @param username_to_kick Username of the user to be kicked.
 * @return 1 if removed successfully, 0 otherwise.
 */
int remove_user_from_group(List registeredGroups, wchar_t *group_name, wchar_t *username_kicker, wchar_t *username_to_kick){

    if (username_kicker == NULL || username_to_kick == NULL || group_name == NULL || registeredGroups == NULL) {
        printf("Invalid parameters for removing user from group.\n");
        return 0;
    }

    // Checks if the group exists.
    Group *group = find_group_by_name(registeredGroups, group_name);
    if (group == NULL) {
        printf("Group %ls not found.\n", group_name);
        return 0; // Failure
    }
    
    // Checks if the kicker is the owner of the group and the user to kick is not the owner.
    if((wcscmp(username_kicker, group->ownerName) == 0 && wcscmp(username_kicker, username_to_kick) == 0) || wcscmp(group->ownerName, username_to_kick) == 0) {
        printf("The group owner can't remove himself. And can't be kicked\n");
        return 0;
    }
    
    // Attempts to remove the user from the in-memory group list.
    if(remove_user_from_group_list(group, group_name, username_kicker, username_to_kick) == 0) {
        printf("Failed to remove user %ls from group %ls in list.\n", username_to_kick, group_name);
        return 0; // Failure
    }

    // Attempts to remove the user from the CSV file.
    if (remove_user_from_group_file(group_name, username_to_kick) == 0) {
        // If removing from the file fails, we should add the user back to the group list.
        add_user_to_group_list(group, group_name, username_to_kick);
        printf("Failed to remove user %ls from group %ls in file.\n", username_to_kick, group_name);
        return 0; // Failure
    }

    return 1; // Success
}

/**
 * Deletes a group completely from both memory and persistent file.
 * Only the group owner can perform the deletion.
 *
 * @param registeredGroups List of all registered groups.
 * @param deleter_name Username of the user attempting deletion.
 * @param group_name Name of the group to delete.
 * @return 1 if the group was successfully deleted, 0 otherwise.
 */
int delete_group(List registeredGroups, wchar_t *deleter_name, wchar_t *group_name) {
    if (registeredGroups == NULL || group_name == NULL) {
        printf("Invalid parameters for deleting group.\n");
        return 0;
    }

    // Checks if the group exists.
    Group *group = find_group_by_name(registeredGroups, group_name);
    if (group == NULL) {
        printf("Group %ls not found.\n", group_name);
        return 0;
    }

    // Checks if the user attempting deletion is the group owner.
    if (wcscmp(deleter_name, group->ownerName) != 0) {
        printf("Only the group owner can delete the group.\n");
        return 0; // Failure
    }

    // Open the groups file and a temporary file for rewriting
    FILE *file = fopen(groupsFilename, "r");
    FILE *temp_file = fopen(groupsTempFilename, "w");// Temporary file to rewrite the group entries
    if (!file || !temp_file) {
        printf("Error opening files.\n");
        if (file) fclose(file);
        if (temp_file) fclose(temp_file);
        return 0;
    }


    // Remove all members from the group in memory
    Position pos = firstList(group->members);
    while (pos != NULL) {
        wchar_t *member = (wchar_t *)retrieveElementList(pos);
        pos = advanceList(pos);
        free(member);
    }
    deleteList(group->members);

    // Remove the group from the registeredGroups list
    delete(group, registeredGroups); 
    free(group);


    // Rewrite the groups file, excluding the deleted group
    wchar_t line[MAX_LINE];
    while (fgetws(line, MAX_LINE, file)) {
        wchar_t current_group[MAX_USER_INFO];

        wchar_t *token, *context;
        token = wcstok(line, L",", &context);
        if (token != NULL) {
            wcsncpy(current_group, token, MAX_USER_INFO);
            current_group[MAX_USER_INFO - 1] = L'\0';

            // Filter out all group entries from CSV
            if (wcscmp(current_group, group_name) == 0) {
                continue;
            }

            token = wcstok(NULL, L"\n", &context);
            fwprintf(temp_file, L"%ls,%ls\n", current_group, token);

        }
    }

    fclose(file);
    fclose(temp_file);

    if (remove(groupsFilename) != 0 || rename(groupsTempFilename, groupsFilename) != 0) {
        printf("Error updating groups file.\n");
        return 0; // Failure
    }

    return 1; // Success
}