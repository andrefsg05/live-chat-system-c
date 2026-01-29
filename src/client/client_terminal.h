#ifndef clientterminal_h
#define clientterminal_h


#include <wchar.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "packet_types.h"


extern struct termios old_terminal, new_terminal;


// Client terminal control functions
bool landing_page(int sockfd, struct sockaddr_in *servaddr);
void read_limited_wide_input(wchar_t *buffer, int max_len, const wchar_t *prompt);
void disable_input();
void enable_input();
void enable_raw_mode();
void disable_raw_mode();

#endif // clientterminal_h