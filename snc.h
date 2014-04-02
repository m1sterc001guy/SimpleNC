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

void *read_thread(void *void_ptr);

void *write_data(void *void_ptr);

int socket_fd;
int close_threads;
pthread_t read_t;
pthread_t write_t;
