#include "snc.h"

int main(int argc, char *argv[]){

  if(argc > 7 || argc < 3){
     print_invalid_params();
     exit(1);
  }

  bufsize = 1024; 
  int c;
  int lflag = 0;
  kflag = 0;
  int uflag = 0;
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
          print_invalid_params();
          exit(1); 
        default:
          print_invalid_params();
          exit(1);
     }
  }

  if(kflag && !lflag){
     print_invalid_params();
     exit(1);
  }
  if(source_ip && lflag){
     print_invalid_params();
     exit(1);
  }


  //check to make sure the last argument is a valid port
  port = strtol(argv[argc-1], NULL, 10);
  if(port <= 0 || port > 65535){
     print_internal_error();
     exit(1);
  }

  hostname = argv[argc-2];

  if(!lflag){
     //hostname in this case is required     

     if(hostname_to_ip(hostname, &ip_address) == -1){
	fprintf(stderr, "Error. Invalid hostname specified\n");
	return -1;
     }
     hostname = inet_ntoa(ip_address);
  }
  else {
     // hostname is optional

     if(hostname_to_ip(hostname, &ip_address) == -1){
	hostname = NULL;
     }
     else{
        hostname = inet_ntoa(ip_address);
     }
  }

  

  printf("lflag: %d kflag: %d uflag: %d source_ip: %s hostname: %s\n", lflag, kflag, uflag, source_ip, hostname);

  create_socket(uflag); 

  if(!uflag){
    if(lflag){

       if(!kflag){
          if(listen_and_accept_connection() < 0){
             print_internal_error();
             exit(1);
          }
       }
       else{
          while(1){
             if(listen_and_accept_connection() < 0){
                print_internal_error();
                exit(1);
             }
             create_socket(uflag);
          }
       }
    }
    else{
       if(connect_to_server() < 0){
          print_internal_error();
          exit(1);
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
     int bytes_recv;
     while(1){
        char buffer[bufsize];
        memset(buffer, 0, bufsize);
        bytes_recv = recv(fdlisten, buffer, bufsize, 0);
        if(bytes_recv == 0){
           //EOF was triggered
           pthread_cancel(write_t);
           break;
        }
        else if(bytes_recv < 0){
           print_internal_error();
           exit(1);
        }
        else{
           printf("%s\n", buffer);
        }
     }
}

void *write_thread_tcp(void *void_ptr){
     int fd = *((int *)void_ptr);
     int message_size = bufsize;
     char message[message_size];

     while(1){
        memset(message, 0, message_size);

        int cur_index = 0;
        int c = getchar();
        //add possbile length check here too
        while(c != '\n' && (c != EOF || kflag)){
           if(c != EOF){
              message[cur_index] = c;
              cur_index++;
           }
           if(cur_index >= bufsize-1){
              break;
           }
           c = getchar();
        }
        if(c == EOF){
           break;
        }

        message[bufsize-1] = 0;

        if(send(fd, message, cur_index, 0) != cur_index){
           print_internal_error();
           exit(1);
        }
     }
}

int listen_and_accept_connection(){
     int connfd;

     struct sockaddr_in address_iface;
     address_iface.sin_family = AF_INET;
     address_iface.sin_addr.s_addr = INADDR_ANY;
     address_iface.sin_port = htons(port);
     if(hostname != NULL){
        address_iface.sin_addr = ip_address;
     }

     if((bind(socket_fd, (struct sockaddr *)&address_iface, sizeof(address_iface))) < 0){
         print_internal_error();
         exit(1);
     }

     if((listen(socket_fd, 1)) < 0){
         print_internal_error();
         exit(1);
     }

     int addrlen = sizeof(struct sockaddr_in);
     if((connfd = accept(socket_fd, (struct sockaddr *)&address_iface, &addrlen)) < 0){
         print_internal_error();
         exit(1);
     }

     printf("New socket is %d\n", connfd);

     if(pthread_create(&read_t, NULL, read_thread_tcp, &connfd)){
        print_internal_error();
        exit(1);
     }

     if(pthread_create(&write_t, NULL, write_thread_tcp, &connfd)){
        print_internal_error();
        exit(1);
     }

     if(pthread_join(write_t, NULL)){
        print_internal_error();
        exit(1);
     }

     if(close(socket_fd) < 0){
        print_internal_error();
        exit(1);
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
     if(source_ip != NULL){
        inet_aton(source_ip, &address_iface.sin_addr);
     }

     if(connect(socket_fd, (struct sockaddr *)&address_iface, sizeof(address_iface)) < 0){
        print_internal_error();
        exit(1);
     }
    
     if(pthread_create(&read_t, NULL, read_thread_tcp, &socket_fd)){
        print_internal_error();
        exit(1);
     }

     if(pthread_create(&write_t, NULL, write_thread_tcp, &socket_fd)){
        print_internal_error();
        exit(1);
     }

     if(pthread_join(write_t, NULL)){
        print_internal_error();
        exit(1);
     }

     if(close(socket_fd) < 0){
        print_internal_error();
        exit(1);
     }
     printf("Connection successfully closed.\n");
     return 0;
}


int create_udp_server(){

     struct sockaddr_in server_addr;
     memset((char *)&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);
     server_addr.sin_addr.s_addr = INADDR_ANY;
     if(hostname != NULL){
        server_addr.sin_addr = ip_address;
     }


    if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0){
        print_internal_error();
        exit(1);
     }

     if(pthread_create(&read_t, NULL, read_thread_udp, NULL)){
        print_internal_error();
        exit(1);
     }

     if(pthread_create(&write_t, NULL, write_thread_udp, NULL)){
        print_internal_error();
        exit(1);
     }
     
     if(pthread_join(read_t, NULL)){
        print_internal_error();
        exit(1);
     }

     close(socket_fd);
     
     return 0;
}


int create_udp_client(){

     struct sockaddr_in server_addr;
     //struct hostent *host;
     //host = gethostbyname("127.0.0.1");
     memset((char *)&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);
     //server_addr.sin_addr = *((struct in_addr *)host->h_addr);
     if(source_ip != NULL){
        inet_aton(source_ip, &server_addr.sin_addr);
     }
     memcpy(&addr, &server_addr, sizeof(struct sockaddr_in));


     if(pthread_create(&write_t, NULL, write_thread_udp, NULL)){
        print_internal_error();
        exit(1);
     }

     if(pthread_create(&read_t, NULL, read_thread_udp, NULL)){
        print_internal_error();
        exit(1);
     }

     if(pthread_join(read_t, NULL)){
        print_internal_error();
        exit(1);
     }

     close(socket_fd);

     return 0;
}


void *read_thread_udp(void *void_ptr){
   
    int recvlen;
    socklen_t fromlen = sizeof(addr);


    while(1){
        char buf[bufsize+1];
        memset(buf, 0, bufsize);
        recvlen = recvfrom(socket_fd, buf, bufsize, 0, (struct sockaddr *)&addr, &fromlen); 
        if(recvlen < 0){
           print_internal_error();
           exit(1);
        }
        printf("%s\n", buf);
     }
}


void *write_thread_udp(void *void_ptr){
     int bytes_recv;
     socklen_t addrlen = sizeof(addr);
     
     while(1){
        char send_data[bufsize];
        memset(send_data, 0, bufsize);

        int cur_index = 0;
        int c = getchar();
        //add possbile length check here too
        while(c != '\n'){
           send_data[cur_index] = c;
           cur_index++;
           if(cur_index >= bufsize-1){
              break;
           }
           c = getchar();
        }

        send_data[bufsize-1] = 0;


        if(sendto(socket_fd, send_data, bufsize, 0, (struct sockaddr *)&addr, addrlen) < 0){
           print_internal_error();
           exit(1);
        }
     }
}


int create_socket(int uflag){
  if(!uflag){
     if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        print_internal_error();
        exit(1);
     }
  }
  else{
     if((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        print_internal_error();
        exit(1);
     }
  }
  

  int reuse_port;
  if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_port, sizeof(reuse_port))){
     print_internal_error();
     exit(1);
  }
}

void print_invalid_params(){
  fprintf(stderr, "invalid or missing options\nusage: snc [-k] [-l] [-u] [-s source_ip_address] [hostname] port\n");
}

void print_internal_error(){
  fprintf(stderr, "internal error\n");
}


int hostname_to_ip(char * hostname, struct in_addr *address){
    struct hostent *he;
    struct in_addr **addr_list;

    if(strlen(hostname) < 3){
       return -1;
    }

    if( (he = gethostbyname(hostname)) == NULL ){
	return -1;
    }

    memcpy(address, he->h_addr_list[0], he->h_length);
    //printf("address: %s\n", inet_ntoa(*address));
    return 0;
}


