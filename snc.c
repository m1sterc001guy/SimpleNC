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
  int port;
  char *hostname = NULL;
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
  else if( (*argv[argc-2] == 'k') || (*argv[argc-2] == 'l') || (*argv[argc-2] == 'u') ) { 
  	 //EDITED BY: JAMES
  	 //hey, I'm not sure if I did this right but hopefully one of you gets the idea I'm going for.
  	 //let me know what you think. 
  	 //if argv[argc-2] == 'u', OR 'l', OR 'k' then it cannot be the hostname 
     // ========================================================================

     //here we need to somehow check if the hostname is there or not because in this case its optional.
     //for now lets just assume it is there
     hostname = NULL;
  }
  else {
  	 // otherwise it is the hostname
  	 hostname = argv[argc-2];
  }

  

  printf("lflag: %d kflag: %d uflag: %d source_ip: %s hostname: %s\n", lflag, kflag, uflag, source_ip, hostname);

  int socket_fd;
  struct sockaddr_in address_iface;
  if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
      fprintf(stderr, "Error. Socket failed to initialize. Quitting...\n");
      return -1;
  }

  if(lflag){
     int connfd;
     address_iface.sin_family = AF_INET;
     //i think we need to replace this with source_addr if its specified
     address_iface.sin_addr.s_addr = INADDR_ANY;
     address_iface.sin_port = htons(port);

     if((bind(socket_fd, (struct sockaddr *)&address_iface, sizeof(address_iface))) < 0){
         fprintf(stderr, "Error. Socket binding failed. Quitting...\n");
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
     
     sleep(10);
     if(close(socket_fd) < 0){
        fprintf(stderr, "Error closing the connection.\n");
        return -1;
     }
  }
  else{
     //connect to a server 
     memcpy(&address_iface, hostname, 4);
     address_iface.sin_family = AF_INET;
     address_iface.sin_port = htons(port);

     if(connect(socket_fd, (struct sockaddr *)&address_iface, sizeof(address_iface)) < 0){
        fprintf(stderr, "Error when connecting. Quitting...\n");
        return -1;
     }
  }

  /*
  int input_char;
  do{
    input_char = getchar();
    putchar(input_char);
  }while(input_char != '.'); 
  */

  

  return 0;
}
