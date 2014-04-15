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
#include <arpa/inet.h>

void *read_thread_tcp(void *void_ptr);
void *write_thread_tcp(void *void_ptr);
void *read_thread_udp(void *void_ptr);
void *write_thread_udp(void *void_ptr);
int listen_and_accept_connection();
int connect_to_server();
int create_udp_server();
int create_udp_client();
int create_socket(int uflag);
void print_invalid_params();
void print_internal_error();

int socket_fd;
int close_threads;
int port;
int kflag;
char *hostname = NULL;
struct sockaddr_in addr;
pthread_t read_t;
pthread_t write_t;
int bufsize;
struct in_addr ip_address;
