#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //standard symbolic constants and types 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>//Internet address family
#include <arpa/inet.h> //definitions for internet operations 
#include <netdb.h>
#include <mysql.h>//definitions for network database operations 


#define SERVER "localhost"
#define USER "root"
#define PASSWORD "H@nh1321"
#define DATABASE "chatHistory"
#define PORT 5000
#define BUFSIZE 1024

void send_to_all(int j, int i, int sockfd, int nbytes_recvd, char *recv_buf, fd_set *master)//sockf: file descriptor
{
	if (FD_ISSET(j, master)){
		if (j != sockfd && j != i) {
			if (send(j, recv_buf, nbytes_recvd, 0) == -1) {
				perror("Send");
			}
		}
	}
}

// Add mysql to this function	
void send_recv(int i, fd_set *master, int sockfd, int fdmax)
{
	int nbytes_recvd, j;
	char recv_buf[BUFSIZE] =" ";
	char buf[BUFSIZE];
	//Connect to database
	  MYSQL *connect;
	  connect=mysql_init(NULL);
	  if (!connect)
	  {
		printf("MySQL Initialization is failed \n");
	  }
	  connect=mysql_real_connect(connect, "localhost", "root", "H@nh1321" , "chatHistory" ,3306,NULL,0);
	  if (connect){ 
	  	printf("Connection is successful\n"); 
	  }
	  else{ 
	  	printf("Connection is failed\n"); 
	  }
	//End
	
	//Try to get command
	MYSQL_RES *res_set;
	char command[200]= " ";
	char user_name[30] = " ";
	char message[100] = " ";
	if ((nbytes_recvd = recv(i, recv_buf, BUFSIZE, 0)) <= 0) {
		if (nbytes_recvd == 0) {
			printf("Socket %d hung up\n", i);
		}
		else {
			perror("recv");
		}
		close(i);
		FD_CLR(i, master);
	}
	else { 
		printf("%s\n", recv_buf);
		for(j = 0; j <= fdmax; j++){
			send_to_all(j, i, sockfd, nbytes_recvd, recv_buf, master );	
		}
	
	}
	int k; 	  
   	for( k=0; k< strlen(recv_buf); k++)
   	{
		if ( recv_buf[k] == ':' )
		{
		strncpy(user_name,recv_buf,k);
		strncpy(message,&recv_buf[k+1],strlen(recv_buf)-k);
	//	printf("%s\n", user_name);
	//	printf("%s\n", message);
		break;
		}
   	}
	sprintf(command,"INSERT INTO chat (username, message) VALUES(\"%s\",\"%s\");", user_name, message);		
	mysql_query(connect,command);
	res_set = mysql_store_result(connect);	
	mysql_close (connect);
}
//end 

void connection_accept(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr)
{
	socklen_t addrlen;
	int newsockfd;
	
	addrlen = sizeof(struct sockaddr_in);
	if((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1) {
		perror("accept");
		exit(1);
	}else {
		FD_SET(newsockfd, master);
		if(newsockfd > *fdmax){
			*fdmax = newsockfd;
		}
		printf("new connection from %s on port %d \n",inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
	}
}
	
void connect_request(int *sockfd, struct sockaddr_in *my_addr)
{
	int yes = 1;
    //create a master socket
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
		
	my_addr->sin_family = AF_INET;
	my_addr->sin_port = htons(5000);
	my_addr->sin_addr.s_addr = INADDR_ANY;
	memset(my_addr->sin_zero, '\0', sizeof my_addr->sin_zero);
		
    //set master socket to allow multiple connections
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
    
	//bind the socket to localhost port
	if (bind(*sockfd, (struct sockaddr *)my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Unable to bind");
		exit(1);
	}
    //try to specify maximum of 10 pending connections for the master socket
	if (listen(*sockfd, 10) == -1) {
		perror("listen");
		exit(1);
	}
	printf("\nTCPServer Waiting for client on port 5000\n");
	fflush(stdout);
}

int main()
{
    //set of socket descriptors
	fd_set master;
    //set of read socket descriptors
	fd_set read_fds;
	int fdmax, i;
	int sockfd= 0;
	struct sockaddr_in my_addr, client_addr;
	
    //Clear an fd_set master
	FD_ZERO(&master);
    //Clear an fd_set read_fds
	FD_ZERO(&read_fds);
	connect_request(&sockfd, &my_addr);
    //Add a descriptor sockfd to an fd_set (master)
	FD_SET(sockfd, &master);
	
	fdmax = sockfd;
	while(1){
		read_fds = master;
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}
		
		for (i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &read_fds)){
                //If something happened on the master socket , then its an incoming connection
				if (i == sockfd)
					connection_accept(&master, &fdmax, sockfd, &client_addr);
                //else its some IO operation on some other socket :)
				else
					send_recv(i, &master, sockfd, fdmax);
			}
		}
	}
	return 0;
}
		
