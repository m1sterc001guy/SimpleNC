#include "snc.h"

int main(int argc, char *argv[]){

  if(argc > 7 || argc < 3){
     fprintf(stderr, "Error. Incorrect number of arguments\n");
     return -1;
  }

   
  int c;
  int lflag = 0;
  int kflag = 0;
  int uflag = 0;
  char *source_ip = NULL;
  while((c = getopt(argc, argv, "klus:")) != -1){
     switch(c){
        case 'k':
          kflag = 1;
          break;
        case 'l':
          lflag = 1;
          break;
        case 'u':
          uflag = 1;
          break;
        case 's':
          source_ip = optarg;
          break;
        case '?':
          if(optopt == 's')
             fprintf(stderr, "Option -%c requires an argument.\n", optopt);
          else if(isprint(optopt))
             fprintf(stderr, "Unknown option '-%c'.\n", optopt);
          else
             fprintf(stderr, "Unknown option character '\\x%x',\n", optopt);
          return -1;
          break;
        default:
          fprintf(stderr, "An error occurred. Quitting...\n");
          return -1;
     }
  }

  if(kflag && !lflag){
     fprintf(stderr, "Invalid arguments. Quitting...\n");
     return -1;
  }
  if(source_ip && lflag){
     fprintf(stderr, "Invalid arguments. Quitting...\n");
     return -1;
  }


  //check to make sure the last argument is a valid port
  port = strtol(argv[argc-1], NULL, 10);
  if(port <= 0 || port > 65535){
     fprintf(stderr, "Error. Invalid port specified\n");
     return -1;
  }

  if(!lflag){
     //hostname in this case is required
     hostname = argv[argc-2]; 
  }
  else {
     // otherwise it is the hostname
     hostname = argv[argc-2];
  }

  

  printf("lflag: %d kflag: %d uflag: %d source_ip: %s hostname: %s\n", lflag, kflag, uflag, source_ip, hostname);

  if(!uflag){
     if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error. TCP Socket failed to initialize. Quitting...\n");
        return -1;
     }
  }
  else{
     if((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        fprintf(stderr, "Error. UDP Socket failed to initialize. Quitting...\n");
        return -1;
     }
  }
  

  int reuse_port;
  if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_port, sizeof(reuse_port))){
     perror("setsockopt");
     return -1;
  }

  if(!uflag){
    if(lflag){
       if(listen_and_accept_connection() < 0){
          fprintf(stderr, "Error listening and accepting connection\n");
          return -1;
       }   
    }
    else{
       if(connect_to_server() < 0){
          fprintf(stderr, "Error connecting to server\n");
          return -1;
       } 
    }
  }
  else{
    if(lflag){
       create_udp_server();
    }
    else{
       create_udp_client();
    }
  }
  
  return 0;
}


void *read_thread_tcp(void *void_ptr){
     //read data from the server
     int fdlisten = *((int *)void_ptr);
     int bufsize = 1024;
     int bytes_recv;
     while(1){
        char buffer[bufsize];
        memset(buffer, 0, bufsize);
        bytes_recv = recv(fdlisten, buffer, bufsize, 0);
        if(bytes_recv == 0){
           //EOF was triggered
           free(buffer);
           pthread_cancel(write_t);
           break;
        }
        else if(bytes_recv < 0){
           perror("Error");
        }
        else{
           printf("%s", buffer);
        }
     }
}

void *write_thread_tcp(void *void_ptr){
     int fd = *((int *)void_ptr);
     //what happens when the message you type is longer than 1024 bytes?
     int message_size = 1023;
     char message[message_size];

     while(1){
        memset(message, 0, message_size);

        int cur_index = 0;
        int c = getchar();
        //add possbile length check here too
        while(c != '\n' && c != EOF){
           message[cur_index] = c;
           cur_index++;
           c = getchar();
        }
        message[cur_index] = '\n';
        if(c == EOF){
           break;
        }

        if(send(fd, message, strlen(message), 0) != strlen(message)){
           perror("send");
           exit(-1);
        }
     }
}

int listen_and_accept_connection(){
     int connfd;
     struct sockaddr_in address_iface;
     address_iface.sin_family = AF_INET;
     address_iface.sin_addr.s_addr = INADDR_ANY;
     address_iface.sin_port = htons(port);

     if((bind(socket_fd, (struct sockaddr *)&address_iface, sizeof(address_iface))) < 0){
         fprintf(stderr, "Error. Socket binding failed. Quitting...\n");
         perror("Error: ");
         return -1;
     }

     if((listen(socket_fd, 1)) < 0){
         fprintf(stderr, "Error. Socket listening failed. Quitting...\n");
         return -1;
     }

     int addrlen = sizeof(struct sockaddr_in);
     if((connfd = accept(socket_fd, (struct sockaddr *)&address_iface, &addrlen)) < 0){
         fprintf(stderr, "Error. Socket accept failed. Quitting...\n");
         return -1;
     }

     printf("New socket is %d\n", connfd);

     if(pthread_create(&read_t, NULL, read_thread_tcp, &connfd)){
        fprintf(stderr, "Error creating thread\n");
        return -1;
     }

     if(pthread_create(&write_t, NULL, write_thread_tcp, &connfd)){
        fprintf(stderr, "Error creating write thread\n");
        return -1;
     }

     if(pthread_join(write_t, NULL)){
        fprintf(stderr, "Error joing thread\n");
        return -1;
     }

     if(close(socket_fd) < 0){
        fprintf(stderr, "Error closing the connection.\n");
        return -1;
     }
     printf("Connection successfully closed.\n");
     return 0;
}

int connect_to_server(){
     struct sockaddr_in address_iface;
     //connect to a server 
     memcpy(&address_iface, hostname, 4);
     address_iface.sin_family = AF_INET;
     address_iface.sin_port = htons(port);

     if(connect(socket_fd, (struct sockaddr *)&address_iface, sizeof(address_iface)) < 0){
        fprintf(stderr, "Error when connecting. Quitting...\n");
        return -1;
     }
    
     if(pthread_create(&read_t, NULL, read_thread_tcp, &socket_fd)){
        fprintf(stderr, "Error creating thread\n");
        return -1;
     }

     if(pthread_create(&write_t, NULL, write_thread_tcp, &socket_fd)){
        fprintf(stderr, "Error creating write thread\n");
        return -1;
     }

     if(pthread_join(write_t, NULL)){
        fprintf(stderr, "Error joing thread\n");
        return -1;
     }

     if(close(socket_fd) < 0){
        fprintf(stderr, "Error closing the connection.\n");
        return -1;
     }
     printf("Connection successfully closed.\n");
     return 0;
}


int create_udp_server(){
     if(pthread_create(&read_t, NULL, read_thread_udp, &socket_fd)){
        fprintf(stderr, "Error creating write thread\n");
        return -1;
     }

     if(pthread_create(&write_t, NULL, write_thread_udp, &socket_fd)){
        fprintf(stderr, "Error creating write thread\n");
        return -1;
     }

     if(pthread_join(write_t, NULL)){
        fprintf(stderr, "Error joing thread\n");
        return -1;
     }
     return 0;
}


int create_udp_client(){

     if(pthread_create(&read_t, NULL, read_thread_udp, &socket_fd)){
        fprintf(stderr, "Error creating write thread\n");
        return -1;
     }

     if(pthread_create(&write_t, NULL, write_thread_udp, &socket_fd)){
        fprintf(stderr, "Error creating write thread\n");
        return -1;
     }

     if(pthread_join(write_t, NULL)){
        fprintf(stderr, "Error joing thread\n");
        return -1;
     }

     return 0;
}


void *read_thread_udp(void *void_ptr){
     struct sockaddr_in server_addr;
     struct sockaddr_in client_addr;

     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);
     server_addr.sin_addr.s_addr = INADDR_ANY;
     bzero(&(server_addr.sin_zero), 8);

     int bytes_read;
     char recv_data[1024];
     int addr_len;
  
     if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0){
        perror("UDP Bind");
        exit(-1);
     }

     addr_len = sizeof(struct sockaddr);

     while(1){
        memset(recv_data, 0, sizeof(recv_data));
        bytes_read = recvfrom(socket_fd, recv_data, sizeof(recv_data), 0, (struct sockaddr *)&client_addr, &addr_len);
        printf("%s", recv_data);
     }
}


void *write_thread_udp(void *void_ptr){
     int bytes_recv;
     struct sockaddr_in server_addr;
     struct hostent *host;
     char send_data[1024];

     host = (struct hostent *)(gethostbyname((const char *)"127.0.0.1"));

     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);
     server_addr.sin_addr = *((struct in_addr *)host->h_addr);
     bzero(&(server_addr.sin_zero), 8);

     while(1){
        memset(send_data, 0, sizeof(send_data));

        int cur_index = 0;
        int c = getchar();
        //add possbile length check here too
        while(c != '\n'){
           send_data[cur_index] = c;
           cur_index++;
           c = getchar();
        }
        send_data[cur_index] = '\n';
        
        sendto(socket_fd, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
     }
}


