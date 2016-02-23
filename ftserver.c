/*
 * ftserver.c
 * Sam Snyder
 * samuel.l.snyder@gmail.com
 * Reference: 
 *	Beej's guide: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * 	Java Networking tutorial: https://docs.oracle.com/javase/tutorial/networking/sockets/
 * 	Accessing directories: http://www.gnu.org/software/libc/manual/html_node/Accessing-Directories.html#index-accessing-directories
 * */
#include <dirent.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>

// buffer size puts maximum size on how much memory at a time stored in 
// buffer during file transfer
#define BUFFER_SIZE 1024

// for initailizeSocket()
#define INIT_SOCK_CONNECTION_MODE 1
#define INIT_SOCK_BINDING_MODE 0

// socket fd's
int listening_socket_fd;
int data_socket_fd;
int control_socket_fd;

// close sockets print error and exit
void die(){
  close(listening_socket_fd);
  close(data_socket_fd);
  close(control_socket_fd);
  perror("ftserver");
  exit(EXIT_FAILURE);
} 

/*
Initialize socket creates a new socket for specified
port and host. Mode defines whether connect() or bind()
will be called after socket()
mode: 0 is bind, 1 is connect
*/
int initializeSocket(const char *port, const char *host, int mode){
  int success= 0;
  fflush(stdout);
  printf("Initializing socket for host %s on port %s\n",host, port );
  int socket_fd;
  // structs for contianing socket info for socket creatikon
  struct addrinfo hints, *res;

  // make listening socket. 1) get address info, 2) make socket, 3) bind address to socket
  // zero out hints before loading it
  bzero(&hints, sizeof hints);

  // get hints ready
  hints.ai_family = AF_INET;			// IPv4
  hints.ai_socktype = SOCK_STREAM; 	// TCP	
  if (mode == INIT_SOCK_BINDING_MODE){
    hints.ai_flags = AI_PASSIVE; 		// Fill in IP address of server for us 
  }
  printf("getting address info\n");

  if ( 0 != getaddrinfo(host, //const char *node, if this address, null
        port, //const char *service,  port number from input
        &hints, //const struct addrinfo *hints, hints for creating address info struct
        &res)) //struct addrinfo **res
  {
    die();
  }

  // make a socket
  if ((socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
  {
    die();
  }

  // loop over results linked list until we can bind or connect
  for(;res != NULL; res = res->ai_next){
    // try to bind or connect
    if (mode == INIT_SOCK_BINDING_MODE){
      if (bind(socket_fd, res->ai_addr, res->ai_addrlen)==0){
        // success
        int success = 1;
        printf("socket bound\n");
        break;
      }
      else
        printf("Unable to bind.\n");
      perror("ftserve");
      continue;
    }
    else if (mode == INIT_SOCK_CONNECTION_MODE) {
   		// pause to ensure that client is ready for us
    	sleep(2);
         if (connect(socket_fd, res->ai_addr, res->ai_addrlen) == 0) {
        // success
        int success = 1;
        printf("socket connected\n");
        break;
      }
      else {
        close(socket_fd);
        printf("Unable to connect.\n");
        perror("ftserve");
        continue;
      }
    }
  }
  if (success = 1)
  {
  	return socket_fd;
  }
  else
  	return -1;
}

void sighandler(int sig){
  die();
}

int main(int argc, char const *argv[])
{
	char buffer[BUFFER_SIZE];
  	size_t bufferSize=BUFFER_SIZE;

  	// for accept()
  	struct sockaddr_storage client_sockaddr;
  	socklen_t client_sockaddr_len; 

  // validate command line parameters
  if (argc != 2)
  {
    printf("Usage: ftserver <SERVER_PORT>\n");
    exit(EXIT_FAILURE);
  }


  //listening_socket_fd = initializeSocket(argv[1], NULL, INIT_SOCK_BINDING_MODE);
  listening_socket_fd = initializeSocket(argv[1], "localhost", INIT_SOCK_BINDING_MODE);
  if (listening_socket_fd == -1)
  {
  	printf("Error initializing socket\n");
  	die();
  }

  // register signal hanlder
  signal(SIGINT, sighandler);

  // listen 
  if (listen(listening_socket_fd, 0) != 0)
  {
    die();
  }

  printf("Waiting for conection request on socket %s\n", argv[1]);
  // begin main loop
  while(1){

    // accept new connection for control (P)
    if( (control_socket_fd =  accept(listening_socket_fd, (struct sockaddr *) &client_sockaddr, &client_sockaddr_len)) ==-1 ){
      perror("ftserver: accept");
      printf("Error accepting connection.\n");
      continue;
    }

    //  read command from connected client into a buffer
    bzero(buffer, bufferSize);
    read(control_socket_fd, buffer, bufferSize-1);
    printf("Control message recieved: %s\n", buffer);
    ;
    // variables for command parameters
    char const *errorMsg = "Invalid command";
    // char *tokens[5];
    char *command; // either -l (list) or -g (get file)
    char *fileName; // file name 
    char *dataPort; // port on client to send data to

    // get first token, check if empty
    if (!(command = strtok(buffer, " \n")) ){
      // nothing there send error and start loop over
      send(control_socket_fd, errorMsg, strlen(errorMsg)+1, 0);
      continue;
    }
    else if( strcmp(command, "-l") == 0 ||  strcmp(command, "-g") == 0 )
    {
      // client sent LIST or GET command. next token should be data port
      /*
         if ((dataPort = strtol(tokens[1], NULL, 10)) == 0){
         send(control_socket_fd, errorMsg, strlen(errorMsg)+1, 0);
         co]ntinue;
         }
         */
      dataPort = strtok(NULL, " \n");
      // strcpy(dataPort, tokens[1]); 
      printf("Data port =  %s\n", dataPort);

      // Open data connectino on DATA_PORT
      // get client address
      struct sockaddr_storage addr;
      char ipstr[INET_ADDRSTRLEN];
      int port;
      socklen_t len = sizeof addr; 

      getpeername(control_socket_fd, (struct sockaddr*)&addr, &len);
      struct sockaddr_in *s = (struct sockaddr_in *)&addr;
      inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

      printf("Client host = %s\n", ipstr);
      data_socket_fd= initializeSocket(dataPort, ipstr, INIT_SOCK_CONNECTION_MODE);
      if (data_socket_fd == -1)
      {
      	printf("Error connecting to data socket.\n");
      	continue;
      }

      printf("data_socket_fd = %d\n", data_socket_fd );
      // if -l (list)
      if (strcmp(command, "-l") == 0 )
      {
        // send list of directory contents
        // 
        printf("Listing contents of directory\n");
        bzero(buffer, BUFFER_SIZE);
        DIR *d;
        struct dirent *dir;
        d = opendir(".");
        if (d)
        {
          printf("Sending directory listing.\n");
          while ((dir = readdir(d)) != NULL)
          {
            sprintf(buffer, "%s\n", dir->d_name);
            send(data_socket_fd, buffer, strlen(buffer) + 1,0 );
          }
          closedir(d);
        }
        printf("Directory listing sent.\n");
        close(data_socket_fd);
        continue;
      }

      if (strcmp(command, "-g") == 0 )
        // -g <FILENAME>	
      {
        // get file name from command
        fileName = strtok(NULL, " \n");	
        char fileName2[100]; 
        bzero(fileName2, 100);
       	strncpy(fileName2, fileName, strlen(fileName));// not sure why, but works when i do this
        printf("Requested file: >%s<\n", fileName2);
        bzero(buffer, BUFFER_SIZE);
        
        // get file
        FILE *file;
        file = fopen(fileName2, "r");

        if ( ! file  ){
          // send error message if unavailable
          sprintf(buffer, "File %s unavailable.\n", fileName2);
          printf("%s\n", buffer);
          send(control_socket_fd, buffer, strlen(buffer) + 1, 0);
          close(data_socket_fd);
          continue;
        }

        printf("File is available.\n");
        // we will transfer bytes by reading contents of file into buffer
        // and then transfering from buffer into socket. On the other end
        // the bytes are read and placed in a file.
        int count;
        while( (count = fread(buffer, 1, BUFFER_SIZE, file ) ) > 0){
          send(data_socket_fd, buffer, count,0 );
        }
        // we have completed the file transfer
        printf("Done tranferring file.\n");
        close(data_socket_fd);	
      }
      else 
      {
        // command not recognzied, send error
        printf("Command not recognized. Sending error message.\n");
        send(control_socket_fd, errorMsg, strlen(errorMsg)+1, 0);
        continue; 

        // close DATA connection
        close(data_socket_fd);
        printf("Data connection closed.\n");
      }
    }
  }
  return 0;
}
