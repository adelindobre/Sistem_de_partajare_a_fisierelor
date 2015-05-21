
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS	5
#define BUFLEN 1500

int poz = 0;

typedef struct
{
    int port_number;
    char *ip_address;
    char *client_name;
    char **shared_files;
    int socket_id;
} client;


void error(char *msg)
{
    perror(msg);
    exit(1);
}

// infoclients command
void infoclientsResults(int sockfd, client *vector, char *buffer)
{
	strcpy(buffer, "infoclients ");

    char *port = malloc(50 * sizeof(char));

    int i;
    for (i = 0; i < poz; i++)
    {
        strcat(buffer, vector[i].client_name);
        strcat(buffer, " ");

        strcat(buffer, vector[i].ip_address);
        strcat(buffer, " ");

        sprintf(port, "%d", vector[i].port_number);
        strcat(buffer, port);

        if (i != (poz-1))
        	strcat(buffer, " ");
    }

     send(sockfd, buffer, strlen(buffer), 0);
}

// share command
void shareFile(client *vector_clients, char *buffer, int sock_id)
{
	int i,j;

	strcpy(buffer, buffer + strlen("share "));

	for (i = 0; i < poz; i++)
		if (vector_clients[i].socket_id == sock_id)
			break;

	for (j = 0; j < 100; j++)
		if (vector_clients[i].shared_files[j] == NULL)
			break;
		else
			if (strcmp(vector_clients[i].shared_files[j], buffer) == 0)
				break;

	vector_clients[i].shared_files[j] = malloc(120 * sizeof(char));	
	strcpy(vector_clients[i].shared_files[j], buffer);	
		
}

void sendListOfFiles(client *vector_clients, char *buffer, int sock_id)
{
	char *token = malloc(50 * sizeof(char));
	char *token2 = malloc(50 * sizeof(char));
	char *aux = malloc(120 * sizeof(char));
	char *var = malloc(50 * sizeof(char));
	int i,j;

	token = strdup(strtok(buffer, " "));
	token = strdup(strtok(NULL, " "));

	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "getshare ");

	for (i = 0; i < poz; i++)
		if (strcmp(vector_clients[i].client_name, token) == 0)
			break;

	if (i == poz) 	// clientul nu exista
	{
		strcat(buffer, "-3");
		send(sock_id, buffer, strlen(buffer), 0);
	}
	else
	{
		int j;
		for (j = 0; j < 100; j++)
			if (vector_clients[i].shared_files[j] == NULL)
				break;
			else
			{
				strcpy(aux, vector_clients[i].shared_files[j]);
				token2 = strtok(aux, " ");
				strcat(buffer, token2);
				strcat(buffer," ");
				token2 = strtok(NULL, " ");
				int size = atoi(token2);
				if (size < 1024)
					strcat(buffer, "0KiB ");
				else
					if (size > 1024 && size < (1024 * 1024))
					{
						size = size / 1024;
						sprintf(var,"%dKiB ",size);
						strcat(buffer, var);
					}
					else
						if (size > (1024 * 1024) && size < (1024 * 1024 * 1024))
						{
							size = size / (1024 * 1024);
							sprintf(var,"%dMiB ", size);
							strcat(buffer, var);
						}
						else
							if (size > (1024 * 1024 * 1024))	
							{
								size = size / (1024 * 1024 * 1024);
								sprintf(var,"%dGiB ", size);
								strcat(buffer, var);		
							}			
			}
		buffer[strlen(buffer) - 1] = 0;
		send(sock_id, buffer, strlen(buffer), 0);
	}
}
void unshareFile(client *vector_clients, char *buffer, int sock_id)
{
	int i,j;
	char *token = malloc(50 * sizeof(char));

	for (i = 0; i < poz; i++)
		if (vector_clients[i].socket_id == sock_id)
			break;

	token = strtok(buffer, " ");
	token = strtok(NULL, " ");

	for (j = 0; j < 100; j++)
		if (vector_clients[i].shared_files[j] == NULL)
			break;
		else
			if (strstr(vector_clients[i].shared_files[j], token) != NULL)
				break;
	
	memset(buffer, 0, BUFLEN);
	strcpy(buffer, "unshare ");

	if (vector_clients[i].shared_files[j] == NULL)
		strcat(buffer, "-5 : Fisier inexistent");
	else
	{
		free(vector_clients[i].shared_files[j]);
		strcat(buffer, "Succes");
	}

	send(sock_id, buffer, strlen(buffer), 0);
}
void verifyConditions(client *vector_clients, char *buffer, int sock_id)
{
	char *token = malloc(50 * sizeof(char));
	char *token2 = malloc(50 * sizeof(char));
	char *aux = malloc(BUFLEN * sizeof(char));
	char *aux2 = malloc(BUFLEN * sizeof(char));
	strcpy(aux, buffer);

	token = strtok(buffer, " ");
	token = strtok(NULL, " ");
	int i,j,m,k;

	for (i = 0; i < poz; i++)
		if (strcmp(vector_clients[i].client_name, token) == 0)
			break;	
	if (i == poz)
	{
		strcpy(buffer, "getfile -4 : Client necunoscut");
		send(sock_id, buffer, strlen(buffer), 0);
	}
	else
	{
		token = strtok(NULL," ");
		for (j = 0; j < 100; j++)
			if (vector_clients[i].shared_files[j] == NULL)
				break;
			else
			{
				strcpy(aux2, vector_clients[i].shared_files[j]);
				token2 = strtok(aux2, " ");
				if (strcmp(token2, token) == 0)
					break;
			}
		if (vector_clients[i].shared_files[j] == NULL)
		{
			strcpy(buffer, "getfile -5 : Fisier inexistent");
			send(sock_id, buffer, strlen(buffer), 0);
		}
		else
		{
			for (m = 0; m < poz; m++)
				if (vector_clients[m].socket_id == sock_id)
					break;

			for (k = 0; k < 100; k++)
			{
				if (vector_clients[m].shared_files[k] != NULL)
				{
					strcpy(aux2, vector_clients[m].shared_files[k]);
					token2 = strtok(aux2, " ");

					if (strcmp(token2, token) == 0)							
						break;				
				}
			}
			if (vector_clients[m].shared_files[k] != NULL)	
			{
				strcpy(buffer, "getfile -6 : Fisier partajat cu acelasi nume");
				send(sock_id, buffer, strlen(buffer), 0);
			}
			else
				if (k == 100)
				{
					strcpy(buffer,"corect! ");
					strcat(buffer,aux);
					send(sock_id, buffer, strlen(buffer), 0);
				}			
		}		
	}	

}

int main(int argc, char **argv) 
{
    int sockfd, newsockfd, portno, clilen;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, j, k, to_socket;

    client *vector_clients = malloc(MAX_CLIENTS * sizeof(client));

    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	    //multime folosita temporar
    int fdmax;		    //valoare maxima file descriptor din multimea read_fds

     if (argc < 2) 
     {
         fprintf(stderr,"Server usage: %s port.\n", argv[0]);
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("Server: error opening socket.\n");

     portno = atoi(argv[1]);

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
    	error("Server: error on binding.\n");

     listen(sockfd, MAX_CLIENTS);
     printf("Server: waiting for clients.\n");

     //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);

     //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
     FD_SET(sockfd, &read_fds);
     fdmax = sockfd;

    // main loop
	while (1) 
	{
		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error("Server: error in select.\n");

		for(i = 0; i <= fdmax; i++) 
		{
			if (FD_ISSET(i, &tmp_fds)) 
			{
				if (i == sockfd) 
				{
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) 
					{
        				error("Server: error in accept.\n");
				    }
					else 
					{
						// adaug noul socket intors de accept() la multimea descriptorilor de citire
                        memset(buffer, 0, BUFLEN);
                        if ((n = recv(newsockfd, buffer, sizeof(buffer), 0)) > 0)
                        {
                            char *token = malloc(50 * sizeof(char));
                            token = strtok(buffer," ");
                            int j = 0;
                            
                            printf("Client Name : %s\n", token);

                            if (poz != 0)
                            {
                                for (j = 0; j < poz; j++)    
                                    if (strcmp(vector_clients[j].client_name, token) == 0)
                                    {
                                        printf("Warning! Connection already exists!\n");
                                        close(newsockfd);
                                        break;   
                                    }    
                            }
                            
                            if (j == poz)
                            {
                                printf("New connection!\n");
                                vector_clients[poz].client_name = malloc(50*sizeof(char));
                                strcpy(vector_clients[poz].client_name, token);

                                vector_clients[poz].ip_address = malloc(50*sizeof(char));
                                vector_clients[poz].ip_address = inet_ntoa(cli_addr.sin_addr);    

                                token = strtok(NULL, " ");                               
                                vector_clients[poz].port_number = atoi(token);

                                vector_clients[poz].shared_files = (char**)malloc(20*sizeof(char*));
                                vector_clients[poz].socket_id = newsockfd;
                              //  printf("Socket %d\n", newsockfd);

                                vector_clients[poz].shared_files = malloc(100*sizeof(char*));
                                poz++; 

                                printf("Server: new connection from %s, on port %d, using socket %d\n.", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

                                FD_SET(newsockfd, &read_fds);

                                if (newsockfd > fdmax)
                                {               
                                    fdmax = newsockfd;
                                }
                            }
                        }                         
   						
  					}
  					
  				}
  				else
  				{
  						// am primit date pe unul din socketii cu care vorbesc cu clientii
		    			// actiunea serverului: recv()
		    			memset(buffer, 0, BUFLEN);

		    			if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) 
		    			{
		     	        	if (n == 0) 
						    	  // conexiunea s-a inchis
						    	  printf("Server: socket %d hung up.\n", i);
						    else
							      error("Server: error in recv.\n");
						   
						    close(i);
						    FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care
					    }
					    //recv intoarce > 0
					    else
					    {
		    	 			printf ("Server: incoming message from client on socket %d:%s\n", i, buffer);

                           // printf("%s",buffer);
						    if (strcmp(buffer, "infoclients\n") == 0) 
					        {
					           memset(buffer, 0, BUFLEN);
                               infoclientsResults(i, vector_clients, buffer);
                            } 
                            else
	                            if (strstr(buffer, "unshare") != NULL)
	                        	{
	                        		unshareFile(vector_clients, buffer, i);
	                        	}
	                            else
		                            if (strstr(buffer, "getshare") != NULL)
		                            {
		                            	sendListOfFiles(vector_clients, buffer, i);
		                            }	    
                        			else
			                        	if (strstr(buffer, "share") != NULL)
			                            {
			                            	shareFile(vector_clients, buffer, i);
			                            }
			                            else
			                            	if (strstr(buffer, "getfile") != NULL)
			                            	{
			                            		verifyConditions(vector_clients, buffer, i);
			                            	} 
			                        	else
			                        		if (!strcmp(buffer, "quit\n")) 
			                        		{
			                            		printf("shutting %i down\n", i);
			                            		FD_CLR(i, &read_fds);
			                            		close(i);
			                        		}
			                        		else
			                        		{
			                        		    int to = buffer[0] - '0';
			                          	        send(to, buffer+2, strlen(buffer)-2, 0);
			                        		}
					    }
  				}
			}
		}	
	}

	close(sockfd);

	return 0;
}