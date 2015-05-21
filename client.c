#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <libgen.h>

#define BUFLEN 1500

#define MAX_CLIENTS	5

int poz = 0;

typedef struct
{
    int port_number;
    char *ip_address;
    char *client_name;
    char **shared_files;
} client;

static unsigned get_file_size (const char * file_name)
{
    struct stat sb;
    if (stat (file_name, & sb) != 0) {
        fprintf (stderr, "'stat' failed for '%s': %s.\n",
                 file_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
    return sb.st_size;
}

void error (char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char **argv)
{
	FILE *fp, *f_recv_getfile, *f_send_getfile;

	int sockfd, newsockfd, listen_sockfd, n, portno, clilen;
	struct sockaddr_in serv_addr;
	struct sockaddr_in serv_client_addr;
	struct sockaddr_in client_serv_addr;
	struct sockaddr_in cli_addr; 

	fd_set temp_fs;
	fd_set read_fs;

	//valoare maxima file descriptor din multimea read_fds
	int fdmax;            
	char *aux = malloc (BUFLEN * sizeof(char));
	char *token = malloc(50 * sizeof(char));

	//structuri deschidere director client
	DIR *dir;
	struct dirent *dp;

	client *vector_clients = malloc(MAX_CLIENTS * sizeof(client));

	char buffer[BUFLEN];
	if (argc < 6)
	{
		fprintf(stderr,"Client usage: %s server_address server_port.\n", argv[0]);
		exit(0);
	}

	char *log_file = malloc(50*sizeof(char));
	strcpy(log_file,argv[1]);
	strcat(log_file,".log");

	fp = fopen(log_file, "w");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("Client:error opening socket. \n");

	listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sockfd < 0)
		error("Client: error opening socket for listen. \n");

	
	portno = atoi(argv[3]);

	int k;
	for (k = 0; k < poz; k++)
		if (strcmp(vector_clients[k].client_name, argv[1]) == 0)
			break;

	memset((char *) &client_serv_addr, 0, sizeof(client_serv_addr));
	client_serv_addr.sin_family = AF_INET;
 	client_serv_addr.sin_addr.s_addr = INADDR_ANY;
    client_serv_addr.sin_port = htons(portno);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[5]));
	inet_aton(argv[4], &serv_addr.sin_addr);

	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		error("Client: error connecting.\n");
	else
		printf("Client: connected to server.\n");

	//Trimitere mesaj initial
	// ./client <nume_client> <director> <port_client> <ip_server> <port_server>
	memset(buffer, 0, BUFLEN);
	strcpy(buffer, argv[1]);
	strcat(buffer, " ");
	strcat(buffer, argv[3]);
	
	if ((n = send (sockfd, buffer, strlen(buffer), 0)) < 0)
		error("Client: error sending initial data.\n");
	else
		printf("Client: sent initial data.\n");


	if (bind(listen_sockfd, (struct sockaddr *) &client_serv_addr, sizeof(struct sockaddr)) < 0)
		error("Client-server: error on binding.\n");

	// ascult cereri conexiune pe listen_socckfd
	listen(listen_sockfd, MAX_CLIENTS);
	int x;

	FD_ZERO(&read_fs);
	FD_ZERO(&temp_fs);

	FD_SET(sockfd, &read_fs);
	FD_SET(listen_sockfd, &read_fs);
	FD_SET(0, &read_fs);

	if (sockfd < listen_sockfd)
		fdmax = listen_sockfd;              //valoarea maxima a socketului din multimea de citire;
	else
		fdmax = sockfd;


	while (1)
	{
		temp_fs = read_fs;
		if (select(fdmax+1, &temp_fs, NULL, NULL, NULL) == -1)
			error("Client: error in select.\n");

		for (x = 0; x <= fdmax; x++)
		{
			if (FD_ISSET(x, &temp_fs))
			{
				if (x == listen_sockfd)
				{
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(listen_sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1)
						error("Client-server: error in accept.\n");
					else
					{
						printf("Client-server: accepted.\n");
						memset(buffer, 0 , BUFLEN);
						if ((n = recv(newsockfd, buffer, sizeof(buffer), 0)) > 0)
						{
							token = strtok(buffer, " ");
							token = strtok(NULL, " ");
							token = strdup(strtok(NULL, " "));
							
							char *file_name = malloc(100 * sizeof(char));
							strcpy(file_name, argv[2]);
							strcat(strcat(file_name, "/"), token);

							strcpy(buffer, token);
							send(newsockfd, buffer, strlen(buffer), 0);

							memset(buffer, 0 , BUFLEN);
							f_send_getfile = fopen(file_name, "r");

							while (fread(buffer, sizeof(char), 1024, f_send_getfile) > 0)
								send(newsockfd, buffer, strlen(buffer), 0);								
						}	
					}	
				}
				if (FD_ISSET(0, &temp_fs))
				{
					memset(buffer, 0, BUFLEN);
					memset(aux, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					strncpy(aux, buffer, strlen(buffer) - 1);
					token = strtok(aux," ");

					// infoclients command
					if (strcmp(token,"infoclients") == 0)
					{
						n = send(sockfd, buffer, strlen(buffer), 0);
						if (n < 0)
							error("Client:error writing to socket.\n");

						memset(token, 0, 50);
					}
					// unshare <nume_fisier> command
					if (strcmp(token,"unshare") == 0)
					{
						fprintf(fp, "%s> %s ", argv[1], token);
						token = strtok(NULL, " ");
						fprintf(fp, "%s\n", token);

						strncpy(aux, buffer, strlen(buffer) - 1);				
						n = send(sockfd, aux, strlen(aux), 0);

						if (n < 0)
							error("Client:error writing to socket.\n");

						memset(token, 0, 50);
					}
					// share <nume_fisier> command
					if (strcmp(token, "share") == 0)
					{
						token = strtok(NULL, " ");
						dir = opendir(argv[2]);
						if (dir != NULL)
						{
							while ((dp = readdir(dir)) != NULL)
							{
								if (strcmp(dp->d_name, token) == 0)
								{
									printf("Succes\n");
									fprintf(fp, "%s> share %s\nSucces\n", argv[1], token);
									char dim[50];
									
									const char *file_name;
									unsigned size;
									strcpy(aux, argv[2]);
									strcat (strcat(aux, "/"), token);

									file_name = aux;
									size = get_file_size(file_name);

									int i,j;
									for (i = 0; i < poz; i++)
										if (strcmp(vector_clients[i].client_name, argv[1]) == 0)
											break;							
									for (j = 0; j < 100; j++)
										if (vector_clients[i].shared_files[j] == NULL)
											break;
									else
										if (strcmp(vector_clients[i].shared_files[j], token) == 0)
											break;

									if (vector_clients[i].shared_files[j] == NULL)
									{
										vector_clients[i].shared_files[j] = malloc(120 * sizeof(char));	
										strcpy(vector_clients[i].shared_files[j], aux);	
									}	
								/*	
									for (j = 0; j < 100; j++)
										if (vector_clients[i].shared_files[j] != NULL)	
								    		printf("%s\n", vector_clients[i].shared_files[j]);	
								*/
							    	memset(aux, 0, BUFLEN);	
									sprintf(dim, " %d", size);
									strncpy(aux, buffer, strlen(buffer)-1);
									strcat(aux,dim);

									n = send(sockfd, aux, strlen(aux), 0);

									if (n < 0)
										error("Client:error writing to socket.\n");

									break;
								}
							}
							if (dp == NULL)
							{
								printf("-5 : Fisier inexistent\n");
								fprintf(fp, "%s> share %s\n-5 : Fisier inexistent\n", argv[1], token);
							}	
						}
						closedir(dir);
						memset(token, 0, 50);
					}
					// getshare <nume_client> commmand
					if (strcmp(token, "getshare") == 0)
					{
						strncpy(aux, buffer, strlen(buffer)-1);
						fprintf(fp, "%s> %s\n", argv[1], aux);
						n = send (sockfd, aux, strlen(aux), 0);
						if (n < 0)
							error("Client:error writing to socket.\n");

						memset(token, 0, 50);
					}
					// getfile <nume_client> <nume_fisier> command
					if (strcmp(token, "getfile") == 0)
					{		
						strncpy(aux, buffer, strlen(buffer)-1);
						fprintf(fp, "%s> %s\n", argv[1], aux);
						n = send (sockfd, aux, strlen(aux), 0);
						if (n < 0)
							error("Client:error writing to socket.\n");						
					}

				}
				// primire mesaj de la server
				if (FD_ISSET(sockfd, &temp_fs))
				{
					memset(buffer,0,BUFLEN);
					n = recv(sockfd, buffer, BUFLEN, 0);
					if (n > 0)
					{
						printf("Client received message\n");

						if (strstr(buffer, "infoclients") != NULL)
						{
							token = strtok(buffer," ");
							fprintf(fp, "%s> %s\n", argv[1], token);
						
							while ( (token = strtok(NULL," ")) != NULL )
							{
								vector_clients[poz].client_name = malloc(50 * sizeof(char));
								strcpy(vector_clients[poz].client_name, token);
								printf("%s ", token);
								fprintf(fp, "%s ", token);

								token = strtok(NULL, " ");
								vector_clients[poz].ip_address = malloc(50 * sizeof(char));	
								strcpy(vector_clients[poz].ip_address, token);
								printf("%s ", token);
								fprintf(fp, "%s ", token);

								token = strtok(NULL, " ");
								vector_clients[poz].port_number = atoi(token);
								printf("%s\n", token);
								fprintf(fp, "%s\n", token);

								vector_clients[poz].shared_files = malloc(120 * sizeof(char*));
								poz++;
							}
							memset(buffer,0,BUFLEN);					
						}
						if (strstr(buffer, "getshare") != NULL)
						{
							token = strtok(buffer, " ");
							while ( (token = strtok(NULL, " ")) != NULL )
							{
								printf("%s ", token);
								fprintf(fp, "%s ", token);
								token = strtok(NULL," ");
								printf("%s\n", token);
								fprintf(fp, "%s\n", token);	
							}
							memset(buffer,0,BUFLEN);	
						}
						if (strstr(buffer, "unshare") != NULL)
						{
							token = strtok(buffer, " ");
							token = strtok(NULL, " ");
							fprintf(fp, "%s\n", token);
							printf("%s\n", token);
							memset(buffer,0,BUFLEN);
						}
						if (strstr(buffer, "getfile") != NULL)
						{
							memset(aux, 0, BUFLEN);
							strcpy(aux, buffer + strlen("getfile "));
							printf("%s\n", aux);
                        
							token = strtok(buffer, " ");
							if (strcmp(token, "corect!") == 0)
							{
								int sockfd_cli = socket(AF_INET, SOCK_STREAM, 0);	
								token = strtok(NULL, " ");
								token = strtok(NULL, " ");
								char *filename = malloc(100 * sizeof(char));

								printf("token %s\n", token);
								serv_client_addr.sin_family = AF_INET;

								int i;
								for (i = 0; i < poz; i++)
									if (strcmp(vector_clients[i].client_name, token) == 0)
									{
										printf("port %d\n", vector_clients[i].port_number);
										printf("ip %s\n", vector_clients[i].ip_address);
										serv_client_addr.sin_port = htons(vector_clients[i].port_number);
										if (inet_aton(vector_clients[i].ip_address, &(serv_client_addr.sin_addr)) != 0)
											printf("e bine!\n");
										break;
									}

								if (connect(sockfd_cli, (struct sockaddr*) &serv_client_addr, sizeof(serv_client_addr)) <= 0)
									error("Client: error connecting.\n");
								else
									printf("Client: connected to client_server.\n");		

								int m;
								token = strtok(NULL, " ");
								strcpy(filename, argv[2]);
								strcat(strcat(filename,"/"), token);
								f_recv_getfile = fopen(filename, "w"); 

								if ((m = send (sockfd_cli, aux, strlen(aux), 0)) < 0)
									error("Client: error sending getfile command.\n");
								else
									printf("Client: sent getfile command.\n");

								close(sockfd_cli);	
							}
							else
							{
								printf("%s\n", aux);
								fprintf(fp, "%s\n", aux);
							}

						}
						// conditie care verifica daca este primit un bloc de date
					}
					if (n == 0)
						printf("Client: socket %d hung up, closing.\n", sockfd);
					if (n < 0)
						error("Client: error in recv.\n");
					if (n <= 0)
					{
						perror("Client: exiting.\n");
						FD_CLR(sockfd, &read_fs);
						close(sockfd);
						return 1;
					}
				}

				if ((x != listen_sockfd) && (x != sockfd) && (x != 0))		
				{
					memset(buffer, 0, BUFLEN);
					if ((n = recv(x, buffer, sizeof(buffer), 0)) <= 0) 
					{
						if (n == 0) 
						    	  // conexiunea s-a inchis
						    	  printf("Client-Server: socket %d hung up.\n", x);
						    else
							      error("Client-Server: error in recv.\n");

						    close(x);
						    FD_CLR(x, &read_fs);	
					}
					else
					{
						printf ("Client-Server: incoming message from client %d\n", x);	
						fwrite(buffer, 1, sizeof(buffer), f_recv_getfile);
					}	
				}
			}		 			
		
		}	
	}
	fclose(f_recv_getfile);
	fclose(f_send_getfile);
	fclose(fp);
	close(listen_sockfd);
	close(sockfd);
	printf("Client: done.\n");
	return 0;
}

