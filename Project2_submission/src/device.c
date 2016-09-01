/*
 * device.c
 *
 *  Created on: Nov 15, 2015
 *      Author: nikita
 */
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
char outputfilename[50];
struct device{
	char type[30];
	char ipaddress[30];
	int port;
	//int areaid;
	bool onstatus;
	int interval;
};




struct device sdevice;
long int timediff=0;

//to split with delimiter
//int split( char * str, char delim, char ***array, int *length );
void * GatewayHandler(void * serverSocketDescriptor);

void WriteToOutputFile(char *filename)
{
	char stringBuilder[70],timestamp[20];
	FILE *fp;
	strcpy(stringBuilder,sdevice.type);
	strcat(stringBuilder, " : ");

		if(sdevice.onstatus)
			strcat(stringBuilder,"ON : ");
		else
			strcat(stringBuilder,"OFF : ");

		sprintf(timestamp, "%u", (unsigned) time(NULL));
	strcat(stringBuilder,timestamp);
	//strcat(stringBuilder,"\r\n");

	fp = fopen(filename, "a");

	    fprintf(fp, "%s\n", stringBuilder);
	    fclose(fp);
	    //printf("Written to file\n");
}

int sendregister(int sock,struct device s_device){
	char message[50], port[2],areaid[2];
	int n;
	sprintf(port,"%d",s_device.port);
	//sprintf(areaid,"%d",s_device.areaid);

	strcpy(message,"Type:register;Action:");
	strcat(message,s_device.type);
	strcat(message,"-");
	strcat(message,s_device.ipaddress);
	strcat(message,"-");
	strcat(message,port);
	//strcat(message,"-");
	//strcat(message,areaid);
	n = write(sock , message, strlen(message) );

	//returns number of bytes written
	return n;
}


void updatefromserver(char server_reply[255],struct device *s_device){
	int linecount = 0,countdelimiter=0;
	char **res,buff[255];
	char reply[100];
	int servertime;
	//printf("enterrd func");
	strcpy(reply,server_reply);
	servertime = GetTimeStamp(reply);
	//printf("Time : %d\n",servertime);
	if (strstr(server_reply,"Switch")!=NULL)
	{

		split( server_reply, ':', &res, &countdelimiter );

		if((strstr(res[2],"On"))!=NULL) { s_device->onstatus = true;
		//printf("Turned on from server\n");
		}
		else if ((strstr(res[2],"Off"))!=NULL) {
			s_device->onstatus = false;
			//printf("Turned off from server\n");
		}
		else printf("Corrupted message from server\n");
	}
	else 	printf("Corrupted message from server Message Type Unknown\n");

	//Logical Clock
	if(servertime > ((long int) time(NULL)+timediff))
					timediff = timediff + (servertime - ((long int) time(NULL)));
//	printf("End of func\n");
}

int sendCurrentState(int sock,bool status)
{
	char message[50],cvalue[2];
	int n;
	strcpy(message,"Type:currState;Action:");
	if(status == true)
		strcat(message,"On");
	else
		strcat(message,"Off");
	n = write(serverlisteningsock , message, strlen(message) );
	return n;
}

int GetTimeStamp(char server_reply[100])
{
	int linecount = 0,countdelimiter=0;
		char **res,buff[255];
		//printf("Server reply: %s\n",server_reply);
		split( server_reply, ':', &res, &countdelimiter );

		//printf("Timestamp : %s\n",res[3]);
	return atoi(res[3]);
}

int main(int argc , char *argv[])
{
	int sock,mysock = 0,c,server_sock , *new_sock;
	//int inputlines=0;
	struct sockaddr_in server,client, myserver;
	//struct device sdevice;
	char message[255] , server_reply[255];
	int n,countdelimiter=0,currenttimesec=0;
	char **res;
	char filename[40] ;
	int len,i=0,count=0;
	FILE *fp;
	char buff[255],buff2[255],serverport[4],ch;
	char serveripaddress[20];
	pthread_t taskthread;


	if(argc!=3){
		printf("Pass file Names!!!\n");
		exit(0);
	}
	else{
		strcpy(filename, argv[1]);
		strcpy(outputfilename, argv[2]);
	}

	fp = fopen(filename,"r");
	if(fp == NULL)
		printf("Error in file opening\n");
	// getting server details
	fgets(buff, 255, (FILE*)fp);

	while(buff[count] != ','){
		serveripaddress[count]=buff[count];
		count++;
	}
	serveripaddress[count]='\0';

	len=count+4;


	while(count<len)
	{
		count++;
		serverport[i] =  buff[count];
		i++;
	}
	serverport[i]='\0';

	// getting device details from file

	fgets(buff2, 255, (FILE*)fp);


	split( buff2, ',', &res, &countdelimiter );
	strcpy(sdevice.type,res[0]);
	strcpy(sdevice.ipaddress,res[1]);
	sdevice.port = atoi(res[2]);
	//sdevice.areaid = atoi(res[3]);
	sdevice.onstatus = false;
	sdevice.interval = -1; // setting default time interval


	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
	{
		printf("Could not create Server socket");
	}
	puts("Server Socket created");

	mysock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
	{
		printf("Could not create socket");
	}
	puts("My Socket created");



	server.sin_addr.s_addr = inet_addr(serveripaddress);
	server.sin_family = AF_INET;
	server.sin_port = htons( atoi(serverport) );


	client.sin_addr.s_addr = inet_addr(sdevice.ipaddress);
	client.sin_family = AF_INET;
	client.sin_port = htons( sdevice.port );



	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}

	puts("Connected to server\n");

	serverlisteningsock = sock;
	//Bind
	if( bind(mysock,(struct sockaddr *)&client , sizeof(client)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("My bind done");

	//Listen
	listen(mysock , 1);

	//sends a register request to the Gateway
	n = sendregister(sock,sdevice);


	//Accept and incoming connection
	puts("Waiting for incoming connections from Gateway...");
	c = sizeof(struct sockaddr_in);


while(1){
	bzero(server_reply,255);
	if( read(sock ,server_reply, 255) > 0)
	{
//        printf("Received update %s\n",server_reply);
		updatefromserver(server_reply,&sdevice);
		WriteToOutputFile(outputfilename);

	}
}

	while( (server_sock = accept(mysock, (struct sockaddr *)&myserver, (socklen_t*)&c)) )
	{
		//puts("Connection accepted");

		new_sock = malloc(1);
		*new_sock = server_sock;

		if( pthread_create( &taskthread , NULL ,  GatewayHandler , (void *)(&server_sock)) < 0)
		{
			perror("could not create thread");
			return 1;
		}

	}

	close(sock);
	close (mysock);
	return 0;
}




/*
int split( char * str, char delim, char ***array, int *length ) {
	char *p;
	char **res;
	int count=0;
	int k=0;

	p = str;
	// Count occurance of delim in string
	while( (p=strchr(p,delim)) != NULL ) {
		*p = 0; // Null terminate the deliminator.
		p++; // Skip past our new null
		count++;
	}

	// allocate dynamic array
	res = calloc( 1, (count+1) * sizeof(char *));
	if( !res ) return -1;

	p = str;
	for( k=0; k<=count; k++ ){
		if( *p ) res[k] = p;  // Copy start of string
		p = strchr(p, 0 );    // Look for next null
		p++; // Start of next string
	}

	*array = res;
	*length = count;

	return 0;
}
*/

void * GatewayHandler(void * serverSocketDescriptor)
{
	int n;
	int ServerSocketDescriptor = * (int *)serverSocketDescriptor;
	char server_reply[255];




	while(1){

		bzero(server_reply,255);
		if(read(ServerSocketDescriptor ,server_reply, 255) > 0)
		{

			updatefromserver(server_reply,&sdevice);
			n = sendCurrentState(ServerSocketDescriptor,sdevice.onstatus);
			WriteToOutputFile(outputfilename);
		}
	}


	return 0;
}



