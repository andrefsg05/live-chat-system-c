#ifndef userRegister_h
#define userRegister_h

#include "list.h"
#include <wchar.h>

#define MAX_USER_INFO 32
#define MAX_LINE 1024

// Structure to hold user information
typedef struct {
    wchar_t username[MAX_USER_INFO];
    wchar_t password[MAX_USER_INFO];
} User;


// Register functions
List load_users();
void save_user_file(User *user);
void save_user_list(List registeredUsers, User *user);
void create_user (wchar_t* username, wchar_t* password, List registeredUsers);
int user_exists(List registeredUsers, wchar_t *username);


// Password checker functions
int authenticateUser(List registeredUsers, wchar_t *username, wchar_t *password);

#endif // userRegister_h