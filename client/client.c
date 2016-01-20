#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_TCP_PORT 3000
#define	BUFLEN 500

int main(int argc, char *argv[])
{
	char	*host = "localhost"; /* host to use if none supplied */
	char	*bp, sbuf[BUFLEN], rbuf[BUFLEN], path[BUFLEN];
	int 	port, bytes_to_read;
	int		sd, n, quit = 1; /* socket descriptor and socket type */
	FILE	*fp;	

	struct stat fstat;
	struct hostent *hp; /* pointer to host information entry */
	struct sockaddr_in server; /* an Internet endpoint address */
	struct PDU {
		char type;
		int length;
		char data[BUFLEN];
	} rpdu, tpdu;

	switch(argc){
		case 2:
			host = argv[1];
			port = SERVER_TCP_PORT;
			break;
		case 3:
			host = argv[1];
			port = atoi(argv[2]);
			break;
		default:
			fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
			exit(1);
	}

	/* Create a stream socket */	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (hp = gethostbyname(host)) 
		bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if (inet_aton(host, (struct in_addr *) &server.sin_addr)){
		fprintf(stderr, "Can't get server's address\n");
		exit(1);
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't connect \n");
		exit(1);
	}
	
	while (quit) {
		printf("Download (D), Upload (U), Change Directory (P), List Directory (L), Quit (Q):\n");	
		scanf(" %c", &tpdu.type);
		tpdu.length = 0; // set default data unit length

		switch(tpdu.type) {
			/* File Download */
			case 'D':
				printf("Enter filename: \n");
				tpdu.length = read(0, tpdu.data, BUFLEN-1); // get user message
				tpdu.data[tpdu.length-1] = '\0';
				write(sd, (char *)&tpdu, sizeof(tpdu));
				read(sd, (char *)&rpdu, sizeof(rpdu));
	
				if (rpdu.type == 'F') {
					fp = fopen(tpdu.data, "w");
					fwrite(rpdu.data, sizeof(char), rpdu.length, fp); // write data to file
					while (rpdu.length == BUFLEN) { // if there is more data to write
						read(sd, (char *)&rpdu, sizeof(rpdu));
						fwrite(rpdu.data, sizeof(char), rpdu.length, fp);
					}
					fclose(fp);
					printf("Transfer sucessful.\n");
				} else {
					fprintf(stderr, "%s", rpdu.data);
				}
				break;

			/* File Upload */
			case 'U':
				printf("Enter filename:\n");
				tpdu.length = read(0, tpdu.data, BUFLEN-1); // get user message
				tpdu.data[tpdu.length-1] = '\0';
				fp = fopen(tpdu.data, "r"); // open file to be uploaded
				if (fp == NULL) {
					fprintf(stderr, "Error: File \"%s\" not found.\n", tpdu.data);
				} else {
					write(sd, (char *)&tpdu, sizeof(tpdu)); // make upload request
					read(sd, (char *)&rpdu, sizeof(rpdu)); // recieve ready signal
					stat(tpdu.data, &fstat); // get file size
					bytes_to_read = fstat.st_size;
					if (rpdu.type = 'R') { // check if server is ready
						tpdu.type = 'F';
						while(bytes_to_read > 0) { // send data packets
							tpdu.length = fread(tpdu.data, sizeof(char), BUFLEN, fp);
							bytes_to_read -= tpdu.length;
							write(sd, (char *)&tpdu, sizeof(tpdu));
							printf("test\n");
						}
						printf("File sent.\n");
					} else {
						fprintf(stderr, "Error: Server not ready.\n");
					}
					fclose(fp);
				}
				break;

			/* Change Directory */
			case 'P':
				printf("Enter path: \n");
				n = read(0, tpdu.data, BUFLEN); // get user message
				tpdu.data[n - 1] = '\0';
				write(sd, (char *)&tpdu, sizeof(tpdu));
				read(sd, (char *)&rpdu, sizeof(rpdu));
				if (rpdu.type = 'R') {
					printf("Directory change sucessful.\n");
				} else {
					fprintf(stderr, "Error: Invalid directory.\n");
				}
				break;

			/* List Files */
			case 'L':
				write(sd, (char *)&tpdu, sizeof(tpdu)); // request file list				
				read(sd, (char *)&rpdu, sizeof(rpdu)); // recieve file list
				write(1, rpdu.data, rpdu.length); // display to stdout
				break;
	
			/* Quit */
			case 'Q':
				quit = 0;
				break;
		}
	}
	close(sd);
	return(0);	
}
