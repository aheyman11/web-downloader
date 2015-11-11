#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define RCVBUFSIZE 1000000 //assumed max length of an article plus HTML tags
#define MAX_REQUEST 1000

void die(char *message) {
	perror(message);
	exit(1);
}

int main(int argc, char **argv) {
	int sock; //socket descriptor
	struct sockaddr_in httpServAddr; //server address
	unsigned short httpServPort; //server port
	char *servIP; //Server IP address (dotted quad)
	char recBuffer[RCVBUFSIZE]; //buffer to receive data
	char *filePath;
	char *fileName;

	//check for correct number of arguments
	if (argc != 4) {
		die("Usage: ./http-client <Host> <Port Number> <File Path>");
	}
	
	httpServPort = atoi(argv[2]); //set server port to that given by user
	filePath = argv[3];

	//convert host name into IP address
	struct hostent *he;
	char *serverName = argv[1];
	if ((he = gethostbyname(serverName)) == NULL) {
		die("gethostbyname failed");
	}
	servIP = inet_ntoa(*(struct in_addr *)he->h_addr);

	//create a reliable, stream socket using TCP
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		die("socket() failed");
	}
	
	//Construct the server address structure
	memset(&httpServAddr, 0, sizeof(httpServAddr)); //zero out structure
	httpServAddr.sin_family = AF_INET; //Internet address family
	httpServAddr.sin_addr.s_addr = inet_addr(servIP); //server IP address
	httpServAddr.sin_port = htons(httpServPort); //server port

	//Establish the connection to the server
	if (connect(sock, (struct sockaddr *) &httpServAddr, sizeof(httpServAddr)) < 0) {
		die("connect() failed");
	}

	//Send http request to the server
	char request[MAX_REQUEST];
       	sprintf(request, "GET %s HTTP/1.0\nHost: %s:<%d>\n\r\n", filePath, serverName, httpServPort);
	if (send(sock, request, strlen(request), 0) != strlen(request)) {
		die("send() sent a different number of bytes than expected");
	}
	
	//read the response
	FILE *output = fdopen(sock, "r"); //wrap socket file in file descriptor
	if (!fgets(recBuffer, RCVBUFSIZE, output)) {
		die("fgets failed");
	}
	//Check that 200 is in the first line
	if (!strstr(recBuffer, "200")) {
		die(recBuffer);
	}
	
	//Skip over rest of header file-- we assume that all lines are under 1000 characters
	while (strcmp(recBuffer, "\r\n") != 0) {
		if (!fgets(recBuffer, RCVBUFSIZE, output)) {
			die("fgets failed");
		}
	}

	fileName = strrchr(filePath, '/')+1; //Get name from filepath
	FILE *fp = fopen(fileName, "wb"); //open file for writing
	size_t n;
	
	//skip over everything not containing actual article
	while (fgets(recBuffer,RCVBUFSIZE, output) > 0){
		if (!strstr(recBuffer, "<p>")) {
			continue;
		}

		//Copy everything between <p> tags
		//char *copyBuffer = malloc(1000000);
		//copyBuffer = strstr(recBuffer, "<p>")+3; //advance past next <p>
		char *copyBuffer = recBuffer;
		while ((copyBuffer = strstr(copyBuffer, "<p"))) {
			if (copyBuffer[2] == 'i') {
				copyBuffer++;
				continue; //skip over <picture> tags
			}
			copyBuffer = strstr(copyBuffer, ">") + 1; //advance past <p>
			int i = 0;
			while (!(copyBuffer[i] == '<' && copyBuffer[i+1] == '/' && copyBuffer[i+2] == 'p' && copyBuffer[i+3] == '>')) {
				//skip over any links
				if (copyBuffer[i] == '<') {
					while (copyBuffer[i] != '>') {i++;}
					i++;
					continue;
				}
				fwrite(copyBuffer+i, 1, 1, fp);
				i++;
			}
			fwrite("\n\n", 1, 2, fp); //put blank line between paragraphs
		}
	}
	
	/*
			//copy data from socket to file using recBuffer
			while ((n=fread(recBuffer, 1, RCVBUFSIZE, output)) > 0) {
		if(fwrite(recBuffer, 1, n, fp) != n) {
			die("fwrite failed");
		}
	}
	
	if (ferror(output)) {
		die("fread failed");
	}
	*/
	fclose(output); //close the socket
	fclose(fp); //close file when done
		
	return 0;
}
