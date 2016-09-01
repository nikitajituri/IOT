/*
 C ECHO client example using sockets
 */

#include<stdlib.h>    //strlen
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include "util.h"
#define MAX 40
#define DefaultInterval 5

int serverlisteningsock = 0;
char storagefilename[50];

struct device {
	char type[30];
	char ipaddress[30];
	int port;
};

struct device sdevice;

//to split with delimiter
//int split(char * str, char delim, char ***array, int *length);
void * GatewayHandler(void * serverSocketDescriptor);

int sendregister(int sock, struct device s_device) {
	char message[50], port[2], areaid[2];
	int n;
	sprintf(port, "%d", s_device.port);
	//sprintf(areaid,"%d",s_device.areaid);

	strcpy(message, "Type:register;Action:");
	strcat(message, s_device.type);
	strcat(message, "-");
	strcat(message, s_device.ipaddress);
	strcat(message, "-");
	strcat(message, port);
	//strcat(message,"-");
	//strcat(message,areaid);
	n = write(sock, message, strlen(message));

	//returns number of bytes written
	return n;
}

void WriteToFile(char server_reply[255]) {
	FILE *fp;
	char **res;
	char buff[256];
	int countdelimiter = 0;
	//printf("Inside WriteToFile\n");
	split(server_reply, ',', &res, &countdelimiter);
	//printf("Finished split\n");
	//printf("%s\n",server_reply);
	//printf("%s\n",res[0]);
	//printf("%s\n",res[1]);
	//strcpy(buff, res[0]);
	//strcat(buff, "----");
	strcpy(buff, res[1]);
	strcat(buff, "----");
	strcat(buff, res[2]);
	strcat(buff, "----");
	strcat(buff, res[3]);
	strcat(buff, "----");
	strcat(buff, res[4]);
	strcat(buff, "----");
	strcat(buff, res[5]);
	//printf("I am trying to write please wait\n");
	fp = fopen(storagefilename, "a+");
	if (fp == NULL)
		printf("Error in file opening\n");
//SensorID,SensorType,SensorState,TimeStamp,IPAddress,PortNumber.
//1----Door----Open----1445850587----10.23.33.44----1034
	//printf("%s\n",buff);
	fprintf(fp, "%s\n", buff);
	fclose(fp);
	//printf("I just wrote to file\n");
//return 1;
}

int main(int argc, char *argv[]) {
	int sock, mysock = 0, c, server_sock, *new_sock;
	//int inputlines=0;
	struct sockaddr_in server, client, myserver;
	//struct device sdevice;
	char message[255], server_reply[255];
	int n, countdelimiter = 0, currenttimesec = 0;
	char **res;
	char filename[40];
	int len, i = 0, count = 0;
	FILE *fp;
	char buff[255], buff2[255], serverport[4], ch;
	char serveripaddress[20];
	pthread_t taskthread;

	if (argc != 3) {
		printf("Pass file Names!!!\n");
		exit(0);
	} else {
		strcpy(filename, argv[1]);
		strcpy(storagefilename, argv[2]);

	}

	fp = fopen(filename, "r");
	if (fp == NULL)
		printf("Error in file opening\n");
	// getting server details
	fgets(buff, 255, (FILE*) fp);

	while (buff[count] != ',') {
		serveripaddress[count] = buff[count];
		count++;
	}
	serveripaddress[count] = '\0';

	len = count + 4;

	while (count < len) {
		count++;
		serverport[i] = buff[count];
		i++;
	}
	serverport[i] = '\0';

	// getting device details from file

	fgets(buff2, 255, (FILE*) fp);

	split(buff2, ',', &res, &countdelimiter);
	strcpy(sdevice.type, res[0]);
	strcpy(sdevice.ipaddress, res[1]);
	sdevice.port = atoi(res[2]);
	//sdevice.areaid = atoi(res[3]);
	//sdevice.onstatus = false;
	//sdevice.interval = -1; // setting default time interval
	fclose(fp);
	fp = NULL;
	fp = fopen(storagefilename, "w+");
	if (fp == NULL) {
		printf("Storage File does not exist\n");
		exit(1);
	}

	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Could not create Server socket");
	}
	puts("Server Socket created");

	mysock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Could not create socket");
	}
	puts("My Socket created");

	server.sin_addr.s_addr = inet_addr(serveripaddress);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(serverport));

	client.sin_addr.s_addr = inet_addr(sdevice.ipaddress);
	client.sin_family = AF_INET;
	client.sin_port = htons(sdevice.port);

	//Connect to remote server
	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}

	puts("Connected to server\n");

	serverlisteningsock = sock;
	//Bind
	if (bind(mysock, (struct sockaddr *) &client, sizeof(client)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("My bind done");

	//Listen
	listen(mysock, 1);

	//sends a register request to the Gateway
	n = sendregister(sock, sdevice);
	printf("send register %d\n",n);
	//Accept and incoming connection
	puts("Backend Waiting for incoming connections from Gateway...");
	c = sizeof(struct sockaddr_in);

	while ((server_sock = accept(mysock, (struct sockaddr *) &myserver,
			(socklen_t*) &c))) {
		puts("Connection accepted");

		new_sock = malloc(1);
		*new_sock = server_sock;

		if (pthread_create(&taskthread, NULL, GatewayHandler,
				(void *) (&server_sock)) < 0) {
			perror("could not create thread");
			return 1;
		}
	}

	close(sock);
	close(mysock);
	return 0;
}

/*
int split(char * str, char delim, char ***array, int *length) {
	char *p;
	char **res;
	int count = 0;
	int k = 0;

	p = str;
	// Count occurance of delim in string
	while ((p = strchr(p, delim)) != NULL) {
		*p = 0; // Null terminate the deliminator.
		p++; // Skip past our new null
		count++;
	}

	// allocate dynamic array
	res = calloc(1, (count + 1) * sizeof(char *));
	if (!res)
		return -1;

	p = str;
	for (k = 0; k <= count; k++) {
		if (*p)
			res[k] = p;  // Copy start of string
		p = strchr(p, 0);    // Look for next null
		p++; // Start of next string
	}

	*array = res;
	*length = count;

	return 0;
}
*/

void * GatewayHandler(void * serverSocketDescriptor) {
	int n;
	int ServerSocketDescriptor = *(int *) serverSocketDescriptor;
	char server_reply[255];
//printf("Inside Handler\n");
	while (1) {

		bzero(server_reply, 255);
		//insert,SensorID,SensorType,SensorState,TimeStamp,IPAddress,PortNumber.

		if (read(ServerSocketDescriptor, server_reply, 255) > 0) {
			//printf("Received reply from server %s\n",server_reply);
			if(strstr(server_reply,"BACKEND")==NULL)
			WriteToFile(server_reply);
		}
	}

	return 0;
}


