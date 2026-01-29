#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "list.h"
#include "userRegister.h"

const char *filename = "Database/users.csv";


/**
 * Creates a new user with the given username and password.
 * The user is saved to the file and added to the registeredUsers list.
 * 
 * @param username The username of the new user.
 * @param password The password of the new user.
 * @param registeredUsers The list of registered users.
 * If username or password is NULL, it prints an error message.
 * If registeredUsers is NULL, it prints an error message.
 */
void create_user (wchar_t* username, wchar_t* password, List registeredUsers) {
    if (username == NULL || password == NULL) {
        printf("Username or password cannot be NULL.\n");
        return;
    }
    if (registeredUsers == NULL) {
        printf("Registered users list is NULL.\n");
        return;
    }

    // Create a new User structure
    User *new_user = (User *)malloc(sizeof(User));
    if (new_user == NULL) {
        perror("Out of memory!");
        exit(EXIT_FAILURE);
    }

    // Copy the username and password into the new User structure
    wcscpy(new_user->username, username);
    wcscpy(new_user->password, password);

    // Save the user to the file
    save_user_file(new_user);

    // Save the user to the registeredUsers list
    save_user_list(registeredUsers, new_user);
}


/**
 * Creates a new user list.
 * Loads users from the CSV file into a linked list.
 * Each line in the file should contain a username and password separated by a comma.
 * 
 * @return A linked list containing all registered users.
 */
List load_users() {
    
    List registeredUsers = createList(); // List for all registered (saved) users

    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("File %s not found. Creating new file on save.\n", filename);
        return registeredUsers; // If the file does not exist, return NULL
    }

    wchar_t line[MAX_LINE];
    while (fgetws(line, MAX_LINE, file)) {
        wchar_t *token;
        wchar_t *context;
        wchar_t username[MAX_USER_INFO];
        wchar_t password[MAX_USER_INFO];

        // Parse username
        token = wcstok(line, L",", &context);
        if (token) {
            wcsncpy(username, token, sizeof(username)/sizeof(wchar_t) - 1);
            username[sizeof(username)/sizeof(wchar_t) - 1] = L'\0';

            // Parse password
            token = wcstok(NULL, L",\n", &context);
            if (token) {
                wcsncpy(password, token, sizeof(password)/sizeof(wchar_t) - 1);
                password[sizeof(password)/sizeof(wchar_t) - 1] = L'\0';

                User* user = (User *)malloc(sizeof(User));
                if (user == NULL) {
                    perror("Out of memory!");
                    fclose(file);
                    exit(EXIT_FAILURE);
                }

                wcscpy(user->username, username);
                wcscpy(user->password, password);
                insert((void *)user, registeredUsers, registeredUsers);
            }
        }
    }
    fclose(file);
    return registeredUsers;
}


/**
 * Saves a user to the CSV file.
 * The user is saved in the format: username,password
 * 
 * @param user Pointer to the User structure to be saved.
 * If the user is NULL, it prints an error message.
 * If the file cannot be opened, it prints an error message.
 */
void save_user_file(User *user) {
    if (user == NULL) {
        printf("No user to save.\n");
        return;
    }
    FILE *file = fopen(filename, "a");
    if (!file) {
        printf("Error opening file.\n");
        return;
    }
    
    // Write the user information to the file
    fwprintf(file, L"%ls,%ls\n" , user->username, user->password);
    fclose(file);
}


/**
 * Saves a user to the registeredUsers list.
 * If the registeredUsers list is NULL, it prints an error message.
 * If the user is NULL, it prints an error message.
 * 
 * @param registeredUsers The list of registered users.
 * @param user Pointer to the User structure to be saved.
 */
void save_user_list(List registeredUsers, User *user) {
    if (registeredUsers == NULL) {
        printf("No users to save.\n");
        return;
    }
    if (user == NULL) {
        printf("No user to save.\n");
        return;
    }
    // Insert the user into the registeredUsers list
    insert((void *)user, registeredUsers, headerList(registeredUsers));
}


/**
 * Authenticates a user by checking the username and password against the registered users list.
 * 
 * @param registeredUsers The list of registered users.
 * @param username The username of the user to authenticate.
 * @param password The password of the user to authenticate.
 * 
 * @return 1 if authentication is successful, 
 *         0 if authentication fails, 
 *         -1 if parameters are invalid.
 */
int authenticateUser(List registeredUsers, wchar_t *username, wchar_t *password) {
    if (registeredUsers == NULL || username == NULL || password == NULL) {
        printf("Invalid parameters for authentication.\n");
        return -1;  // Failure
    }

    // Check if the the provided username exists in the registeredUsers list and if the password matches
    Position pos = firstList(registeredUsers);
    while (pos != NULL) {
        User *user = (User *)retrieveElementList(pos);
        if (wcscmp(user->username, username) == 0 && wcscmp(user->password, password) == 0) {
            return 1;  // Success
        }
        pos = advanceList(pos);
    }
    return 0;  // Failure
}


/**
 * Checks if a user exists in the registered users list.
 * 
 * @param registeredUsers The list of registered users.
 * @param username The username of the user to check.
 * 
 * @return -1 if parameters are invalid, 
 *          1 if the user exists, 
 *          0 if the user does not exist.
 */
int user_exists(List registeredUsers, wchar_t *username) {
    if (registeredUsers == NULL || username == NULL) {
        printf("Invalid parameters for user existence check.\n");
        return -1;  // Failure
    }

    // Check if the username exists in the registeredUsers list
    Position pos = firstList(registeredUsers);
    while (pos != NULL) {
        User *user = (User *)retrieveElementList(pos);
        if (wcscmp(user->username, username) == 0) {
            return 1;  // Success
        }
        pos = advanceList(pos);
    }
    return 0;  // User does not exist
}
