#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define RCVBUFSIZE 1000 //assumed max length of an article plus HTML tags
#define MAX_REQUEST 1000

void die(char *message) {
	perror(message);
	exit(1);
}

int main(int argc, char **argv) {
	int sock; //socket descriptor
	struct sockaddr_in httpServAddr; //server address
	unsigned short httpServPort = 80; //server port, default value of 80
	char *servIP; //Server IP address (dotted quad)
	char recBuffer[RCVBUFSIZE]; //buffer to receive data
	char *fileName;
	char* filePath;
	char* serverName;
	int i;	

	//check for correct number of arguments
	if (argc != 2) {
		die("Usage: ./web-downloader <URL>");
	}
	
	if (strstr(argv[1], "//")) {argv[1] = strstr(argv[1], "//") + 2;} //skip over everything before server name
	filePath = strstr(argv[1], "/"); //set file path to be everything after first / in URL
	
	//set serverName to be everything in URL before first /
	int serverNameLength = 0;
	while (argv[1][serverNameLength++] != '/') {} //computer length of serverName
	serverName = (char *) malloc(serverNameLength);
	for(i = 0; i < serverNameLength-1; i++) {
		serverName[i] = argv[1][i];
	}
	serverName[i] = '\0'; //null terminate
	
	//convert host name into IP address
	struct hostent *he;
	if ((he = gethostbyname(serverName)) == NULL) {
		die("gethostbyname failed");
	}
	servIP = inet_ntoa(*(struct in_addr *)he->h_addr);
	free(serverName);

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
		printf("%s\n", recBuffer);
		die("fgets failed");
	}
	
	if (!strstr(recBuffer, "200")) {die(recBuffer);} //Check that 200 is in the first line
	
	fileName = strrchr(filePath, '/')+1; //Get name from filepath
	FILE *fp = fopen(fileName, "wb"); //open file for writing
	
	//Read the rest of the output from the http request
	while (fgets(recBuffer,RCVBUFSIZE-1, output) != NULL){
		if (!strstr(recBuffer, "<p>") && !strstr(recBuffer, "<p ")) {continue;} //skip over any line not containing a p tag
		char *copyBuffer = recBuffer;
		while ((copyBuffer = strstr(copyBuffer, "<p"))) { //skip to next occurrence of <p
			//skip over <picture> tags
			if (copyBuffer[2] == 'i') {
				copyBuffer++;
				continue;
			}
			copyBuffer = strstr(copyBuffer, ">") + 1; //advance past <p>
			int i = 0;
			while (!(copyBuffer[i] == '<' && copyBuffer[i+1] == '/' && copyBuffer[i+2] == 'p' && copyBuffer[i+3] == '>')) {
				//skip over any html tags within a paragraph
				if (copyBuffer[i] == '<') {
					while (i < strlen(copyBuffer) && copyBuffer[i] != '>') {i++;}
					i++;
					continue;
				}
				//if line ends before closing paragraph tag, get next line
				if (i >= strlen(copyBuffer)) {
					copyBuffer = recBuffer; //reset copyBuffer to head of recBuffer so full capacity can be utilized
					if(!fgets(copyBuffer, RCVBUFSIZE-1, output)) {break;}
					i = 0;
					continue;
				}
				fwrite(copyBuffer+i, 1, 1, fp); //write text between <p> and </p> to file
				i++;
			}
			fwrite("\n\n", 1, 2, fp); //put blank line between paragraphs
		}
	}
	
	if (fclose(output) != 0) {die("fclose failed");}; //close the socket
	if (fclose(fp) != 0) {die("fclose failed");} //close file when done
		
	return 0;
}
