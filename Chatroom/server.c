/************************************************************************/
/*   Maximilian Schroeder												*/
/*																		*/
/*   PROGRAM NAME: server.c  (works with client.c)                      */
/*                                                                      */
/*   Server creates a socket to listen for the connection from Client   */
/*   When the communication established, Server echoes data from Client */
/*   and writes them back.                                              */
/*                                                                      */
/*   Using socket() to create an endpoint for communication. It         */
/*   returns socket descriptor. Stream socket (SOCK_STREAM) is used here*/
/*   as opposed to a Datagram Socket (SOCK_DGRAM)                       */  
/*   Using bind() to bind/assign a name to an unnamed socket.           */
/*   Using listen() to listen for connections on a socket.              */
/*   Using accept() to accept a connection on a socket. It returns      */
/*   the descriptor for the accepted socket.                            */
/*                                                                      */
/*   To run this program, first compile the server.c and run it			*/
/*   on a server machine. Then run the client program on another        */
/*   machine.                                                           */
/*                                                                      */
/*   COMPILE:         gcc -pthread -o server server.c					*/
/*	 RUN:			  server <port number>								*/
/*                                                                      */
/************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>		/* define socket */
#include <netinet/in.h>		/* define internet socket */
#include <netdb.h>			/* define internet socket */
#include <pthread.h>		/* define pthread */

#define MAX_BUFFER_SIZE 512		/* define max buffer size */
#define MAX_CLIENT 10			/* define max number of clients */

// struct of client data passed to client threads
struct clientData
{			
	int client_index;					/* client index */
	int client_fd;						/* client FD */
	struct sockaddr_in server_addr;		/* server address */
	struct sockaddr_in client_addr;		/* client address */
	char username[100];					/* client username */
};

// global variables
int sock_fd;								/* socket FD */
int fd_array[MAX_CLIENT];					/* array of FDs for connected clients */
pthread_t tid[MAX_CLIENT];					/* array of threads (clients) */
struct clientData client_array[MAX_CLIENT];	/* array of clientData structs */
pthread_mutex_t m_lock;						/* mutex lock */
struct sockaddr_in server_addr;				/* server address */

// signal handler to catch SIGINT
void sigHandler(int signal)
{
	// send message to all connected clients that server is about to shut down
	printf("[SERVER] Server will shut down in 10 seconds...\n");
	char buffer[MAX_BUFFER_SIZE];
	buffer[0] = '\0';
	pthread_mutex_lock(&m_lock);
	strcat(buffer, "-SERVER KILL-");
	unsigned int i;
	for (i = 0; i < MAX_CLIENT; i++)
	{
		if (fd_array[i] != -1)			
			write(fd_array[i], &buffer, sizeof(buffer));
	}
	pthread_mutex_unlock(&m_lock);
	
	// wait 10 seconds
	sleep(10);
	
	// close sockets, exit program
	close(sock_fd);
	for (i = 0; i < MAX_CLIENT; i++)
	{
		if (fd_array[i] != -1)
			close(fd_array[i]);
	}
	unlink(server_addr.sin_addr);
	exit(0);
}

// adds the specified new_sock_fd to fd_array, then returns index of new connection; FAIL = -1
int openConnection(int new_sock_fd)
{
	unsigned int i;
	for (i = 0; i < MAX_CLIENT; i++)
	{
		if (fd_array[i] == -1)
		{
			fd_array[i] = new_sock_fd;
			return i;
		}			
	}
	return -1;	/* fd_array is full, no more new connections allowed */
}

// removes the specified sock_fd from fd_array
void closeConnection(int client_index)
{
	fd_array[client_index] = -1;
}

// prints an error message to the console, then closes the program.
void error(char * message)
{
	fprintf(stderr, "%s", message);
	exit(1);
}

// function used by new thread whenever a client connects to server
void * client(void * arg)
{
	// get clientData struct from arg
	struct clientData * client_info = (struct clientData *)arg;
	
	// print message about new client
	fprintf(stderr, "A new client has connected! (%s)\n", client_info->username);
	
	// initialize buffers
	char read_buffer[MAX_BUFFER_SIZE];
	
	// listen for messages from client until client disconnects
	while ((read(client_info->client_fd, read_buffer, sizeof(read_buffer))) > 0)
	{		
		// read data from buffer
		pthread_mutex_lock(&m_lock);
		
		// initialize write buffer
		char buffer[MAX_BUFFER_SIZE];
		buffer[0] = '\0';
		
		// initialize kirby buffer
		char kirby_buffer[8];
		
		// initialize flags
		int write_to_all = 0;
		int disconnect = 0;
		int kirby = 0;
				
		// check if read buffer contains a command, else write contents to all connected clients
		if (read_buffer[0] == '/')
		{
			// get command text
			char command_text[MAX_BUFFER_SIZE];
			memcpy(command_text, &read_buffer[1], (sizeof(read_buffer) - 1));
			
			// execute specified command, else print error message
			if (strncmp(command_text, "exit", sizeof(command_text)) == 0)
			{
				// empty read_buffer
				bzero(read_buffer, MAX_BUFFER_SIZE);
				read_buffer[0] = '\0';
				
				// print disconnect message
				strcat(read_buffer, "[SERVER] Client (");
				strcat(read_buffer, client_info->username);
				strcat(read_buffer, ") has disconnected.");
				
				// set write_to_all & disconnect flag
				write_to_all = 1;
				disconnect = 1;
			}
			else if (strncmp(command_text, "part", sizeof(command_text)) == 0)
			{
				// empty read_buffer
				bzero(read_buffer, MAX_BUFFER_SIZE);
				read_buffer[0] = '\0';
				
				// print disconnect message
				strcat(read_buffer, "[SERVER] Client (");
				strcat(read_buffer, client_info->username);
				strcat(read_buffer, ") has disconnected.");
				
				// set write_to_all & disconnect flag
				write_to_all = 1;
				disconnect = 1;
			}
			else if (strncmp(command_text, "quit", sizeof(command_text)) == 0)
			{
				// empty read_buffer
				bzero(read_buffer, MAX_BUFFER_SIZE);
				read_buffer[0] = '\0';
				
				// print disconnect message
				strcat(read_buffer, "[SERVER] Client (");
				strcat(read_buffer, client_info->username);
				strcat(read_buffer, ") has disconnected.");
				
				// set write_to_all & disconnect flag
				write_to_all = 1;
				disconnect = 1;
			}
			else if (strncmp(command_text, "kirby", sizeof(command_text)) == 0)
			{
				// empty read_buffer
				bzero(read_buffer, MAX_BUFFER_SIZE);
				read_buffer[0] = '\0';
				
				// randomly determine which ASCII kirby to send :)
				int y = rand() % 5;
				switch (y)
				{
					case 0:
						strcpy(kirby_buffer, "<('.')>");
						break;
					case 1:
						strcpy(kirby_buffer, "<(^.^)>");
						break;
					case 2:
						strcpy(kirby_buffer, "<(^.^<)");
						break;
					case 3:
						strcpy(kirby_buffer, "(>^.^)>");
						break;
					case 4:
						strcpy(kirby_buffer, "<('O')>");
						break;
				}
				strcat(read_buffer, kirby_buffer);
				write_to_all = 1;
				kirby = 1;
			}
			else if (strncmp(command_text, "man", sizeof(command_text)) == 0)
			{
				// empty read_buffer
				bzero(read_buffer, MAX_BUFFER_SIZE);
				read_buffer[0] = '\0';
				
				// print list of valid commands
				strcat(read_buffer, "\n--- CHAT CLIENT COMMANDS ---\n\n");
				strcat(read_buffer, "/exit --> Disconnects from chat server and exits.\n");
				strcat(read_buffer, "/part --> Disconnects from chat server and exits.\n");
				strcat(read_buffer, "/quit --> Disconnects from chat server and exits.\n");
				strcat(read_buffer, "/man --> Well, you made it here, didn't you?\n");
				strcat(read_buffer, "/kirby --> Try it. You know you want to. :)\n\n");
			}
			else
			{
				// empty read_buffer
				bzero(read_buffer, MAX_BUFFER_SIZE);
				read_buffer[0] = '\0';
				
				// print error message
				strcat(read_buffer, "[SERVER] Invalid command. Use '/man' for a list of valid commands.\n");
			}
		}
		else
			write_to_all = 1;
		
		// if write_to_all flag is true, write buffer to all connected clients; else write buffer to this client
		if (write_to_all)
		{		
			// if not a disconnect message, prepend username so clients can identify sender
			if (!disconnect)
			{
				strcat(buffer, "(");
				strcat(buffer, client_info->username);
				strcat(buffer, "): ");						
			}
			
			// append message to buffer and write buffer to client sockets
			strcat(buffer, read_buffer);
			strcat(buffer, "\n");
			unsigned int i;
			for (i = 0; i < MAX_CLIENT; i++)
			{				
				if ((fd_array[i] != -1) && (fd_array[i] != client_info->client_fd))
					write(fd_array[i], &buffer, sizeof(buffer));
			}
			
			// echo message on server console
			fprintf(stderr, "%s", buffer);
		}
		if (!write_to_all || kirby)
		{
			if (kirby)
			{
				// display the ASCII kirby generated by the client's command
				bzero(buffer, MAX_BUFFER_SIZE);
				buffer[0] == '\0';
				strcat(buffer, "You sent a Kirby! --> ");
				strcat(buffer, kirby_buffer);
				strcat(buffer, "\n");
			}			
			else
			{
				// append message to buffer and write buffer to socket
				strcat(buffer, read_buffer);
				strcat(buffer, "\n");
			}
			write(client_info->client_fd, &buffer, sizeof(buffer));
		}		
		pthread_mutex_unlock(&m_lock);
		
		// if disconnect flag is true, disconnect this client from server.
		if (disconnect)
		{
			// empty read_buffer
			bzero(buffer, MAX_BUFFER_SIZE);
			buffer[0] = '\0';
			strcat(buffer, "-CLIENT KILL-");
			pthread_mutex_lock(&m_lock);
			write(client_info->client_fd, &buffer, sizeof(buffer));
			pthread_mutex_unlock(&m_lock);
			
			// exit while loop
			break;
		}
	}
	
	// remove client's FD from fd_array
	pthread_mutex_lock(&m_lock);
	closeConnection(client_info->client_index);
	pthread_mutex_unlock(&m_lock);
	
	// exit thread
	return NULL;
}

// server main thread
int main(int argc, char ** argv)
{
	// set signal handler
	signal(SIGINT, sigHandler);
	
	// seed random
	srand(time(NULL));
	
	int new_sock_fd, port_no, client_addr_len, n;			/* socket/connection variables */
	struct sockaddr_in client_addr;							/* client address */
	char buffer[MAX_BUFFER_SIZE];							/* socket read/write buffer */
	char hostname[100];									/* server hostname */
	struct hostent * h;										/* serve hostent */
	unsigned int i;											/* loop iterator variable */
	
	// check for validity of arguments
	if (argc != 2)
		error("[SERVER] ERROR: Incorrect number of arguments. Usage is 'server <port number>'.\n");
	else if ((port_no = atoi(argv[1])) <= 0)
		error("[SERVER] ERROR: Invalid port number specified. Please specify a nonzero port number.\n");
	
	// initialize fd_array	
	for (i = 0; i < MAX_CLIENT; i++)
		fd_array[i] = -1;
	
	// create new socket
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0)
		error("[SERVER] ERROR: Failed to open socket.\n");
	
	// set values for server_addr struct
	server_addr.sin_family = AF_INET;						/* Internet addresses */
	server_addr.sin_port = htons(port_no);					/* port number */
	server_addr.sin_addr.s_addr = INADDR_ANY;				/* IP address of machine running server */
	
	// bind socket to server address
	if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		error("[SERVER] ERROR: Failed to bind socket.\n");
	
	// server started successfully, print server IP address
	gethostname(hostname, sizeof(hostname));
	h = gethostbyname(hostname);
	printf("Server started successfully. [%s]\n", h->h_name);
	
	// listen for clients
	printf("Server is listening for clients...\n");
	if (listen(sock_fd, 1) == -1)
		error("[SERVER] ERROR: Listen failed.\n");
	
	// accept client connections
	while ((new_sock_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len)) > 0)
	{
		pthread_mutex_lock(&m_lock);
		
		// update fd_array		
		int client_index = openConnection(new_sock_fd);
		
		// check if there is room for new client and add client if true
		if (client_index == -1)
		{
			// send refusal message to client, close socket
			fprintf(stderr, "[SERVER] Maximum number of clients connected. New client refused.\n");
			bzero(buffer, MAX_BUFFER_SIZE);
			buffer[0] = '\0';
			strcat(buffer, "-CLIENT REFUSED-");
			write(new_sock_fd, buffer, sizeof(buffer));
			close(new_sock_fd);
		}
		else
		{			
			// send acceptance message to client
			bzero(buffer, MAX_BUFFER_SIZE);
			buffer[0] = '\0';
			strcat(buffer, "-CLIENT ACCEPTED-");
			write(new_sock_fd, buffer, sizeof(buffer));
			bzero(buffer, MAX_BUFFER_SIZE);
			
			// update clientData for new client
			client_array[client_index].client_index = client_index;
			client_array[client_index].client_fd = new_sock_fd;
			client_array[client_index].client_addr = client_addr;
			client_array[client_index].server_addr = server_addr;
			
			// get client's username
			read(new_sock_fd, buffer, sizeof(buffer));
			bzero(client_array[client_index].username, 100);
			client_array[client_index].username[0] = '\0';
			sprintf(client_array[client_index].username, "#%i: %s", (client_index + 1),buffer);
			
			// send client a reply, acknowledging connect
			bzero(buffer, MAX_BUFFER_SIZE);
			buffer[0] = '\0';
			strcat(buffer, "Welcome to the chat server, (");
			strcat(buffer, client_array[client_index].username);
			strcat(buffer, ")!\nType a message and press ENTER to send.\n");
			write(new_sock_fd, buffer, sizeof(buffer));
			bzero(buffer, MAX_BUFFER_SIZE);
			
			// create new thread for client
			int t = pthread_create(&tid[client_index], NULL, &client, &client_array[client_index]);
			if (t != 0)
				error("[SERVER] ERROR: Client thread create failed.\n");
		}
		pthread_mutex_unlock(&m_lock);
	}
	if (new_sock_fd == -1)
		error("[SERVER] ERROR: Accept failed.\n");
	
	// close sock_fd, client's sock_fd's exit program
	close(sock_fd);
	for (i = 0; i < MAX_CLIENT; i++)
	{
		if (client_array[i].client_fd != -1)
			close(client_array[i].client_fd);
	}
	unlink(server_addr.sin_addr);
	exit(0);
}