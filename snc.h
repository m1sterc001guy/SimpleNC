#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>

void *read_thread_tcp(void *void_ptr);
void *write_thread_tcp(void *void_ptr);
void *read_thread_udp(void *void_ptr);
void *write_thread_udp(void *void_ptr);
int listen_and_accept_connection();
int connect_to_server();
int create_udp_server();
int create_udp_client();

int socket_fd;
int close_threads;
int port;
char *hostname = NULL;
pthread_t read_t;
pthread_t write_t;
