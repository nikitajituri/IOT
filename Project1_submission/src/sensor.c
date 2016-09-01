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
#define MAX 50
#define DefaultInterval 5

struct device{
	char type[30];
	char ipaddress[30];
	int port;
	int areaid;
	bool onstatus;
	int currentvalue;
	int interval;
};

struct sensorinfo{
	int starttime;
	int endtime;
	int value;
};

struct sensorinfo inputdata[MAX];
struct device sdevice;
int inputlines=0;
//to split with delimiter
int split( char * str, char delim, char ***array, int *length );
void * SensorTemperatureHandler(void * serverSocketDescriptor);


int readfromfile(char *filename){
	int linecount = 0,countdelimiter=0;
	char **res;
	char buff[255];
	FILE *fp = fopen(filename,"r");

	while((fgets(buff, 255, (FILE*)fp))!=NULL){
		linecount++;
		split( buff, ';', &res, &countdelimiter );
		inputdata[linecount-1].starttime = atoi(res[0]);
		inputdata[linecount-1].endtime = atoi(res[1]);
		inputdata[linecount-1].value = atoi(res[2]);
	}

	//returns the number of lines read
	return linecount;

}

int sendregister(int sock,struct device s_device){
	char message[50], port[2],areaid[2];
	int n;
	sprintf(port,"%d",s_device.port);
	sprintf(areaid,"%d",s_device.areaid);

	strcpy(message,"Type:register;Action:");
	strcat(message,s_device.type);
	strcat(message,"-");
	strcat(message,s_device.ipaddress);
	strcat(message,"-");
	strcat(message,port);
	strcat(message,"-");
	strcat(message,areaid);
	n = write(sock , message, strlen(message) );

	//returns number of bytes written
	return n;
}

int sendCurrentValue(int sock,int value){
	char message[50],cvalue[2];
	int n;
	strcpy(message,"Type:currValue;Action:");
	sprintf(cvalue,"%d",value);
	strcat(message,cvalue);
	n = write(sock , message, strlen(message) );
	return n;
}

void updatefromserver(char *server_reply,struct device *s_device){
	int linecount = 0,countdelimiter=0;
	char **res,buff[255];

	if(strstr(server_reply,"setInterval")>0){
		split( server_reply, ':', &res, &countdelimiter );
		s_device->interval = atoi(res[2]);
	}

	else if (strstr(server_reply,"Switch")>0)
	{

		split( server_reply, ':', &res, &countdelimiter );

		if((strstr(res[2],"On"))>0) { s_device->onstatus = true;

		}
		else if ((strstr(res[2],"Off"))>0) s_device->onstatus = false;
		else printf("Corrupted message from server\n");
	}
	else 	printf("Corrupted message from server Message Type Unknown\n");

}



int main(int argc , char *argv[])
{
	int sock,mysock = 0,c;
	//int inputlines=0;
	struct sockaddr_in server,client;
	//struct device sdevice;
	char message[255] , server_reply[255];
	int n,countdelimiter=0,currenttimesec=0;
	char **res;
	char filename[40],inputfilename[40] ;
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
		strcpy(inputfilename,argv[2]);

	}

	fp = fopen(filename,"r");
	if(fp == NULL)
		printf("Error in file opening\n");
	// getting server details
	fgets(buff, 255, (FILE*)fp);

	while(buff[count] != ':'){
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


	split( buff2, ':', &res, &countdelimiter );
	strcpy(sdevice.type,res[0]);
	strcpy(sdevice.ipaddress,res[1]);
	sdevice.port = atoi(res[2]);
	sdevice.areaid = atoi(res[3]);
	sdevice.onstatus = false;
	sdevice.interval = DefaultInterval; // setting default time interval
	sdevice.currentvalue = 0;
	/* printf("DeviceType:%s\n", sdevice.type);
      printf("IPAddress:%s\n", sdevice.ipaddress);
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
	//printf("Number of register bytes written : %d\n",n);

	//reading the input file sensor input
	inputlines= readfromfile(inputfilename);



	// We are creating a thread to go take care of the "Sending the temperature after a certain interval" part
	pthread_create(&taskthread, NULL, SensorTemperatureHandler, (void*) &sock);


	while(1){

		if( read(sock ,server_reply, 255) > 0)
		{

			updatefromserver(server_reply,&sdevice);

		}

	}

	close(sock);
	close (mysock);
	return 0;
}



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


void * SensorTemperatureHandler(void * serverSocketDescriptor)
{
	int n=0;
	int ServerSocketDescriptor = * (int *)serverSocketDescriptor;
	int currenttimesec=0,i=0,currentvalue=0;

	while(1){
		if(sdevice.onstatus == true){
			sleep(sdevice.interval);
			currenttimesec = currenttimesec + sdevice.interval;
			if(currenttimesec >= 60 ) currenttimesec=0;
			for(i=0;i<inputlines;i++)
			{
				if(currenttimesec >= inputdata[i].starttime && currenttimesec < inputdata[i].endtime){
					currentvalue = inputdata[i].value;
				}
			}
			if(currentvalue!= sdevice.currentvalue){

				sdevice.currentvalue = 	currentvalue;
				n = sendCurrentValue(ServerSocketDescriptor,currentvalue);

			}

		}
	}



}
