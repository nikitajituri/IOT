#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <errno.h>
#include <stdbool.h>
#define DefaultInterval 5

struct device{
	int id;
	char type[30];
	char ipaddress[30];
	int port;
	int areaid;
	bool onstatus;
	int interval;
	int currentvalue;
};


struct device deviceList[1000];
int threadID = 0;


//the thread function
void *connection_handler(void *);


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


void getdevicedetails(char str[],struct device *newDevice){
	char **res,**res2;
	int k=0,k2=0;
	int count =0,count2=0;
	int rc;
	char reschar[50];
	//struct Device *newDevice=((struct Device *)malloc(sizeof(struct Device)));

	rc = split( str, ':', &res, &count );
	if( rc ) {
		printf("Error: %s errno: %d \n", strerror(errno), errno);
	}

	strcpy(reschar,res[2]);

	rc = split( reschar, '-', &res2, &count2 );
	if( rc ) {
		printf("Error: %s errno: %d \n", strerror(errno), errno);
	}

	strcpy(newDevice->type,res2[0]);
	strcpy(newDevice->ipaddress,res2[1]);
	newDevice->port = atoi(res2[2]);
	newDevice->areaid = atoi(res2[3]);
	newDevice->interval = DefaultInterval;

	free(res );
	free(res2);
	return;
}

//sends a on message to the client
int SwitchDeviceOn(int sock,struct device *newDevice)
{
	char message[255]= "Type:Switch;Action:On";
	return write(sock , message , strlen(message));

}

//sends an off message to the client
int SwitchDeviceOff(int sock,struct device *newDevice)
{
	char message[255]= "Type:Switch;Action:Off";
	return write(sock , message , strlen(message));
}


int SetInterval(int sock,int interval,struct device sdevice)
{
	char message[255]= "Type:setInterval;Action:";
	char cinterval[2];
	sprintf(cinterval,"%d",interval);
	strcat(message,cinterval);

	return write(sock , message , strlen(message));
}


int GetSmartDeviceItemID(int SensoritemID)
{
	int i=0,areaid;

	areaid = deviceList[SensoritemID].areaid;
	//printf("AreaID : %d, SensorItemID : %d\n",areaid,SensoritemID);
	//printf("Threads : %d\n",threadID);
	for(i=0;i<=threadID;i++)
	{
		//printf("Item : %d\t deviceList[i].areaid:%d\t deviceList[i].type:%s\n ",i,deviceList[i].areaid,deviceList[i].type);
		if(deviceList[i].areaid == areaid && (strstr(deviceList[i].type,"device")>0)) {
			//printf("Match found\n");
			return deviceList[i].id;
		}
	}
	return -1;
}


void TemperatureCheck(int sock,char client_reply[],int ItemID)
{
	int currentvalue = 0,countdelimiter=0;
	char **res,buff[255];
	int DeviceItemID;

	struct sockaddr_in DeviceSocket;
	int SmartDeviceDescriptor,n ;


	if(strstr(client_reply,"currValue") > 0){
		split( client_reply, ':', &res, &countdelimiter );
		currentvalue = atoi(res[2]);
		deviceList[ItemID].currentvalue = currentvalue;
		if(currentvalue < 32 || currentvalue > 34)
		{
			DeviceItemID = GetSmartDeviceItemID(ItemID);
			//printf("Device ID : %d",DeviceItemID);
			if((DeviceItemID)>-1)
			{
				SmartDeviceDescriptor = socket(AF_INET, SOCK_STREAM, 0);
				bzero(&DeviceSocket, sizeof(DeviceSocket));

				DeviceSocket.sin_family = AF_INET;
				DeviceSocket.sin_port = htons(deviceList[DeviceItemID].port);



				// Write IP address of a DeviceSocket to the address structure
				DeviceSocket.sin_addr.s_addr = inet_addr(deviceList[DeviceItemID].ipaddress);


				// Connect to the remote DeviceSocket
				int Result = connect(SmartDeviceDescriptor, (struct sockaddr*) &DeviceSocket, sizeof(DeviceSocket));



				if(currentvalue < 32 && deviceList[DeviceItemID].onstatus == false){

					n = SwitchDeviceOn(SmartDeviceDescriptor,&deviceList[DeviceItemID]);

				}
				else if(currentvalue > 34 && deviceList[DeviceItemID].onstatus == true){
					n = SwitchDeviceOff(SmartDeviceDescriptor,&deviceList[DeviceItemID]);

				}
				else printf(" ");
			}
		}

	}

}

void ChangeStatusDevice(int ItemID,bool status){

	deviceList[ItemID].onstatus = status;

}


int DisplayState()
{
	int i=0;
	char message[1024],itemid[2],port[5],areaid[3],currentvalue[3];
	strcpy(message,"-----------------------------------------------------\n");

	for(i=0;i<threadID;i++)
	{
		if(deviceList[i].type == " ")  continue;// do nothing
		else if(deviceList[i].type == "sensor" && deviceList[i].currentvalue == 0) continue;
		else //if(deviceList[i].type != " " || deviceList[i].currentvalue !=0 )
		{
			sprintf(itemid, "%d",deviceList[i].id);
			sprintf(port, "%d",deviceList[i].port);
			sprintf(areaid, "%d",deviceList[i].areaid);
			sprintf(currentvalue, "%d",deviceList[i].currentvalue);
			strcat(message,itemid);
			strcat(message,"----");
			strcat(message,deviceList[i].ipaddress);
			strcat(message,":");
			strcat(message,port);
			strcat(message,"----");
			strcat(message,deviceList[i].type);
			strcat(message,"----");
			strcat(message,areaid);
			strcat(message,"----");
			if(strstr(deviceList[i].type,"sensor") > 0){
				strcat(message,currentvalue);
				strcat(message,"\n");
			}
			else{
				if(deviceList[i].onstatus == true)
					strcat(message,"On\n");
				else
					strcat(message,"Off\n");
			}


		}
	}
	strcat(message,"---------------------------------------------------------\n");
	printf("%s\n",message);

	return 0;
}


int main(int argc , char *argv[])
{
	int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	int read_size;
	char *message , client_message[2000];
	char filename[25];
	int len,i=0,count=0;
	FILE *fp;
	char buff[255],port[4],ch;
	char ipAddress[20];
	pthread_t taskthread[1000];


	if(argc!=2){
		printf("Improper Arguments!!!\n");
		exit(0);
	}
	else{
		strcpy(filename, argv[1]);
	}
	//TODO: IF FILE DOESNT EXIST, exit!
	fp = fopen(filename,"r");
	fgets(buff, 255, (FILE*)fp);

	// Reading IP
	while(buff[count] != ':'){
		ipAddress[count]=buff[count];
		count++;
	}
	ipAddress[count]='\0';

	len=count+4;

	// Reading port
	while(count<len)
	{
		count++;
		port[i] =  buff[count];
		i++;
	}
	// null terminate the string
	port[i]='\0';



	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(ipAddress);
	server.sin_port = htons(atoi(port));

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 5);


	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");

		//pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;

		if( pthread_create( &taskthread[threadID] , NULL ,  connection_handler , (void *)(&client_sock)) < 0)
		{
			perror("could not create thread");
			return 1;
		}


	}

	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}

	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size=0,ItemID,n;
	char *message , client_message[255];
	struct device newdevice;

	//Thread Number of the thread
	ItemID = threadID++;

	bzero(client_message,256);
	read_size = read(sock ,client_message, 255);



	if(strstr(client_message,"register")!=NULL)
	{
		getdevicedetails(client_message,&newdevice);

		newdevice.onstatus= false;
		newdevice.interval= 5;
		newdevice.id = ItemID;
		newdevice.currentvalue = 0;// default value
		deviceList[ItemID] = newdevice;

		SwitchDeviceOn(sock,&deviceList[ItemID]);
	}	

	while(1){
		bzero(client_message,255);
		read_size = read(sock ,client_message, 255);
		if(read_size > 1){
			if(strstr(client_message,"currValue")!=NULL)
			{
				TemperatureCheck(sock,client_message,ItemID);
				DisplayState();
			}

			if(strstr(client_message,"currState")!=NULL)
			{
				ChangeStatusDevice(ItemID,(strstr(client_message,"On")) > 0);
				DisplayState();
			}
			read_size = 0;
		}
	}



	return 0;
}


