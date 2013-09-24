/************************************************************************/ 
/*	 Maximilian Schroeder  												*/
/*																		*/
/*   PROGRAM NAME: client.c  (works with server.c)						*/ 
/*                                                                      */ 
/*   Client creates a socket to connect to Server.                      */ 
/*   When the communication established, Client writes data to server   */ 
/*   and echoes the response from Server.                               */ 
/*                                                                      */ 
/*   To run this program, first compile the server.c and run it			*/ 
/*   on a server machine. Then run the client program on another        */ 
/*   machine.                                                           */ 
/*                                                                      */ 
/*   COMPILE:    gcc -pthread -o client client.c                        */
/*   TO RUN:     client <server name> <port no>							*/ 
/*                                                                      */ 
/************************************************************************/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>  /* define socket */ 
#include <netinet/in.h>  /* define internet socket */ 
#include <netdb.h>       /* define internet socket */
#include <pthread.h>     /* define pthread */

#define MAX_BUFFER_SIZE 512

// struct of client data passed to reader thread
struct clientData
{
	int client_fd;						/* client FD */
	struct sockaddr_in server_addr;		/* server address */
	char * username;                    /* client username */
};

// signal handler to catch SIGINT
void sigHandler(int signal)
{
	fprintf(stderr, "[CLIENT] Ctrl+C detected. Please use '/exit', '/part', or '/quit' to exit the program.\n");
}

// function used by reader thread to read incoming messages from server
void * reader(void * arg)
{
	// get clientData struct from arg
	struct clientData * client_info = (struct clientData *)arg;
	
	// initialize buffer
	char buffer[MAX_BUFFER_SIZE];
	
	// read data from socket
	while (read(client_info->client_fd, &buffer, sizeof(buffer)) > 0)
	{
		// check message received from server, perform special action if it is a command
		if (strncmp(buffer, "-SERVER KILL-", sizeof(buffer)) == 0)
		{
            // print warning message
			fprintf(stderr, "[SERVER] Server will shut down in 10 seconds...\n");
			
			// wait for moment before server shuts down
			sleep(9);
			
			// exit while loop
			break;
		}
		else if (strncmp(buffer, "-CLIENT KILL-", sizeof(buffer)) == 0)
		{
			// exit while loop
			break;
		}
		else
			fprintf(stderr, "%s", buffer);
	}
	
	// close socket, exit program
	close(client_info->client_fd);
	exit(0);
}

// prints an error message to the console, then closes the program
void error(char * message)
{
	fprintf(stderr, "%s", message);
	exit(1);
}

// client main thread
int main(int argc, char ** argv)
{		  
	// set signal handler
	signal(SIGINT, sigHandler);
	
	int sock_fd, port_no;				/* socket/port variables */
	struct sockaddr_in server_addr;		/* server address */
	struct hostent * h;                 /* host entity */
	char username[100];					/* client username */
	char buffer[MAX_BUFFER_SIZE];		/* socket read/write buffer */
	pthread_t tid;						/* pthread for reading */
	struct clientData client;			/* clientData struct */
	
	// check validity of arguments
	if (argc != 3)
		error("[CLIENT] ERROR: Incorrect number of arguments. Usage is 'client <server name> <port no>'\n");
	else if ((h = gethostbyname(argv[1])) == NULL)
	{
		printf("[CLIENT] ERROR: %s - Unknown host.\n", argv[1]);
		exit(1);
	}
	else if ((port_no = atoi(argv[2])) <= 0)
		error("[CLIENT] ERROR: Invalid port number specified. Please specify a nonzero port number.\n");
	
	// copy host data into server_addr.sin_addr
	memcpy(&server_addr.sin_addr, h->h_addr_list[0], h->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
	
	// create new socket
	if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
		error("[CLIENT] ERROR: Failed to open socket.\n");
	
	// connect socket
	if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
		error("[CLIENT] ERROR: Connect failed.\n");
    
    // check to see if server accepted connection
    bzero(buffer, MAX_BUFFER_SIZE);
	read(sock_fd, buffer, sizeof(buffer));
    if (strncmp(buffer, "-CLIENT REFUSED-", sizeof(buffer)) == 0)
        error("[SERVER] Maximum number of clients connected. Connection refused.\n");
    else if (strncmp(buffer, "-CLIENT ACCEPTED-", sizeof(buffer)) == 0)
    {
        printf("[SERVER] Connection successful!\n");
	
        // get client username
        printf("Enter your username: ");
        scanf("%[^\t\n]", username);
        while (getchar() != (int)'\n');
	
        // copy username into buffer
        bzero(buffer, MAX_BUFFER_SIZE);
        memcpy(buffer, username, sizeof(username));
	
        // introduce client to server by sending username
        write(sock_fd, buffer, sizeof(buffer));
        bzero(buffer, MAX_BUFFER_SIZE);
        read(sock_fd, buffer, sizeof(buffer));
        printf("%s\n", buffer);
        bzero(buffer, MAX_BUFFER_SIZE);
	
        // update clientData for new client
        client.client_fd = sock_fd;
        client.server_addr = server_addr;
        client.username = username;
	
        // create reader thread
        int t = pthread_create(&tid, NULL, &reader, &client);
        if (t != 0)
		{
			close(sock_fd);
            error("[CLIENT] ERROR: Reader thread create failed.");
		}
	
        // read input from keyboard until user disconnects
        while (scanf("%[^\t\n]", buffer) > 0)
        {
            // collect terminating characters from stdin
            while (getchar() != (int)'\n');
        
            // write buffer to socket
            write(sock_fd, &buffer, sizeof(buffer));
        
            // empty buffer
            bzero(buffer, MAX_BUFFER_SIZE);
        }
    }
	
	// close socket, exit program
	close(sock_fd);
	exit(0);
}