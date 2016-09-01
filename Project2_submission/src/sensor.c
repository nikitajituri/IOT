/*
 * sensor.c
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
#define MAX 50
#define DefaultInterval 5

struct device{
	char type[30];
	char ipaddress[30];
	int port;
	bool onstatus;
	int timestamp;
	int interval;
};

struct sensorinfo{
	int starttime;
	int endtime;
	char value[10];
};

struct sensorinfo inputdata[MAX];
struct device sdevice;
int inputlines=0;
char outputfilename[50];
long int timediff = 0;
//to split with delimiter
//int split( char * str, char delim, char ***array, int *length );
void * SensorTemperatureHandler(void * serverSocketDescriptor);


int readfromfile(char *filename){
	int linecount = 0,countdelimiter=0,i=0;
	char **res;
	char buff[255];
	FILE *fp = fopen(filename,"r");

	while((fgets(buff, 255, (FILE*)fp))!=NULL){

		linecount++;
		split( buff, ';', &res, &countdelimiter );

		if(strstr(sdevice.type,"DOOR")!=NULL){
			//printf("I am here and I am DOOR\n");
		inputdata[linecount-1].starttime = atoi(res[0]);
		//inputdata[linecount-1].endtime = NULL;//atoi(res[1]);
		//printf("I am converting to upper\n");
		uppercase(res[1]);
		//printf("I am did upper\n");
		strcpy(inputdata[linecount-1].value , res[1]);
		}
		else
		{
			//printf("I am here and I am not door\n");
					inputdata[linecount-1].starttime = atoi(res[0]);
					inputdata[linecount-1].endtime = atoi(res[1]);
					uppercase(res[2]);
					strcpy(inputdata[linecount-1].value, res[2]);
					//printf("Start Time : %d\t End : %d\t Value:%s\n",inputdata[linecount-1].starttime,inputdata[linecount-1].endtime,inputdata[linecount-1].value);
		}
	}
	//for(i=0;i<linecount;i++)
		//printf("%s\t",inputdata[i].value);
	printf("\n");
	//returns the number of lines read
	//printf("I have completed reading your file\n");
	return linecount;

}

int sendregister(int sock,struct device s_device){
	char message[50], port[2];
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

int sendCurrentValue(int sock,char *value){
	char message[50],cvalue[2],timestamp[50];
	int n;
	long int t = (long int) time(NULL) + timediff;
	strcpy(message,"Type:currValue;Action:");
	//sprintf(cvalue,"%d",value);
	strcat(message,value);
	strcat(message,";Timestamp:");
	sprintf(timestamp, "%u",(unsigned) t);
	strcat(message,timestamp);
	n = write(sock , message, strlen(message) );
	//printf("Sent Value to server : %s\n",message);
	return n;
}


int GetTimeStamp(char *server_reply)
{
	int linecount = 0,countdelimiter=0;
		char **res,buff[255];
		//printf("Server reply: %s\n",server_reply);
		split( server_reply, ':', &res, &countdelimiter );

		//printf("Timestamp : %s\n",res[3]);
	return atoi(res[3]);
}

void updatefromserver(char *server_reply,struct device *s_device){
	int linecount = 0,countdelimiter=0;
	char **res,buff[255];
	int servertime;
	char *reply;
	strcpy(reply,server_reply);
	//printf("Getting server %s\n",server_reply);
		servertime = GetTimeStamp(reply);
		//printf("This is ServerTimestamp: %d\n",servertime);

	//printf("I received update from server : %s\n",server_reply);
	if(strstr(server_reply,"setInterval")!=NULL){
		split( server_reply, ':', &res, &countdelimiter );
		s_device->interval = atoi(res[2]);
	}

	else if ((strstr(server_reply,"Switch"))!=NULL)
	{
		//printf("Inside switch\n");
		split( server_reply, ':', &res, &countdelimiter );

		if((strstr(res[2],"On") !=NULL  ) || (strstr(res[2],"Open") !=NULL)) {
			s_device->onstatus = true;

		}
		else if ((strstr(res[2],"Off"))!=NULL || (strstr(res[2],"Close"))!=NULL) s_device->onstatus = false;
		else printf("Corrupted message from server\n");


	}
	else 	printf("Corrupted message from server Message Type Unknown\n");

	if(servertime > ((long int) time(NULL)+timediff))
				timediff = timediff + (servertime - ((long int) time(NULL)));

}

void WriteToOutputFile(char *filename)
{
	char stringBuilder[70],timestamp[20];
	FILE *fp;
	int buffer;
	strcpy(stringBuilder,sdevice.type);
	strcat(stringBuilder, " Sensor: ");

	//printf("Trying to write to file...\n");
	if(strstr(sdevice.type,"DOOR")!=NULL){
		if(sdevice.onstatus== true)
			strcat(stringBuilder,"OPEN : ");
		else
			strcat(stringBuilder,"CLOSE : ");
	}
	else if(strstr(sdevice.type,"KEYCHAIN")!=NULL){
			if(sdevice.onstatus==true)
				strcat(stringBuilder,"Inside Room : ");
			else
				strcat(stringBuilder,"Outside Room : ");
		}
	else{
				if(sdevice.onstatus== true)
					strcat(stringBuilder,"ON : ");
				else
					strcat(stringBuilder,"OFF : ");
		}
	sprintf(timestamp, "%u", ((unsigned) time(NULL)+(unsigned)timediff));
	//sprintf(timestamp, "%d", buffer);
	strcat(stringBuilder,timestamp);
	//strncat(stringBuilder,"\r\n");

	fp = fopen(filename, "a");

	    fprintf(fp, "%s\n", stringBuilder);
	    fclose(fp);
	    //printf("Written to file %s\n",stringBuilder);
}


int main(int argc , char *argv[])
{
	int sock,mysock = 0,c;
	struct sockaddr_in server,client;
	char message[255] , server_reply[255];
	char buff[255],buff2[255],serverport[4],ch;
	char serveripaddress[20];
	pthread_t taskthread;

	int n,countdelimiter=0,currenttimesec=0;
	char **res;
	int len,i=0,count=0;

	char filename[40],inputfilename[40] ;
	FILE *fp;


	if(argc!=4){
		printf("Pass file Names!!!\n");
		exit(0);
	}
	else{
		strcpy(filename, argv[1]);
		strcpy(inputfilename,argv[2]);
		strcpy(outputfilename,argv[3]);
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
	//printf("buff: %s\n",buff2);
	strcpy(sdevice.type,res[0]);
	strcpy(sdevice.ipaddress,res[1]);
	sdevice.port = atoi(res[2]);
	//sdevice.areaid = atoi(res[3]);
	sdevice.onstatus = false;
	sdevice.interval = DefaultInterval; // setting default time interval
	//sdevice.currentvalue = 0;
	//printf("DeviceType:%sand\n", sdevice.type);
    /*  printf("IPAddress:%s\n", sdevice.ipaddress);
       printf("Port:%d\n",sdevice.port);
       printf("AreaID :%d\n",sdevice.areaid);*/

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

	puts("Connected\n");

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


	//Accept and incoming connection
	puts("Waiting for incoming connections from Gateway...");
	c = sizeof(struct sockaddr_in);



	//sends a register request to the Gateway
	n = sendregister(sock,sdevice);
	printf("Number of register  written : %d\n",n);

	//reading the input file sensor input
	inputlines= readfromfile(inputfilename);
	//printf("Read from file %d\n",inputlines);


	// We are creating a thread to go take care of the "Sending the temperature after a certain interval" part
	pthread_create(&taskthread, NULL, SensorTemperatureHandler, (void*) &sock);


	while(1){

		if((n = read(sock ,server_reply, 255)) > 0)
		{
			server_reply[n] = '\0';
			//printf("Received reply from server bytes : %d\t %s\n",n,server_reply);
			updatefromserver(server_reply,&sdevice);
			WriteToOutputFile(outputfilename);
		}

	}

	close(sock);
	close (mysock);
	return 0;
}



/*int split( char * str, char delim, char ***array, int *length ) {
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

void * SensorTemperatureHandler(void * serverSocketDescriptor)
{
	int n=0;
	int ServerSocketDescriptor = * (int *)serverSocketDescriptor;
	int currenttimesec=0,i=0,prevtriggertime=0;
	char currentvalue[10];




	while(1){
		if((strcmp(sdevice.type,"DOOR") !=0)){ //sdevice.onstatus == true &&

			sleep(sdevice.interval);
			currenttimesec = currenttimesec + sdevice.interval;
			if(currenttimesec >= 60 ) currenttimesec=0;
			//printf("Current SEC : %d\n",currenttimesec);
			for(i=0;i<inputlines;i++)
			{
				if(currenttimesec >= inputdata[i].starttime && currenttimesec < inputdata[i].endtime){
					strcpy(currentvalue,inputdata[i].value);
					//printf("%d\t %d\t Current Value : %s\n",inputdata[i].starttime,inputdata[i].endtime,inputdata[i].value);
				}
			}
			//printf("Sensor value  : %s\t",currentvalue);
			//if (sdevice.onstatus==true)printf("ON\n"); else printf("OFF\n");
			//if((strstr(currentvalue,"FALSE")!=NULL)) printf("FALSE i am\n");
			if(((strstr(currentvalue,"TRUE")!=NULL)) && (sdevice.onstatus == false)){
				//printf("I entered : \n");
				sdevice.onstatus = 	true;
				n = sendCurrentValue(ServerSocketDescriptor,currentvalue);

				WriteToOutputFile(outputfilename);
				//printf("Wrote to server not door : %d\n",n);


			}
			else if(((strstr(currentvalue,"FALSE")!=NULL)) && (sdevice.onstatus == true)){
				//printf("I entered : 2 \n");
				sdevice.onstatus = 	false;
				n = sendCurrentValue(ServerSocketDescriptor,currentvalue);
				//printf("Wrote to server not door : %d\n",n);
				WriteToOutputFile(outputfilename);
				//printf("Wrote to server not door : %d\n",n);
				//printf("Sensor false\n");
			}
			else ;

		}
		else if((strstr(sdevice.type,"DOOR") != NULL)){ //sdevice.onstatus == true &&
			//printf("Inside While of DOOR\n");
			for(i=0;i<inputlines;i++)
			{
				if(i==0)
				sleep(inputdata[i].starttime);
				else
					sleep(inputdata[i].starttime - inputdata[i-1].starttime);
				strcpy(currentvalue, inputdata[i].value);
				//printf("Door changes : %s\n",currentvalue);
				if((strstr(currentvalue,"OPEN")!=NULL))
					sdevice.onstatus=true;
				else
					sdevice.onstatus=false;
				n = sendCurrentValue(ServerSocketDescriptor,currentvalue);
				WriteToOutputFile(outputfilename);
				if(i==inputlines-1) i=0;
			}
		}

	}
}



