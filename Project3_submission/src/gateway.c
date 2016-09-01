#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <errno.h>
#include <stdbool.h>
#include "util.h"
#define DefaultInterval 5

struct device {
	int id;
	char type[30];
	char ipaddress[30];
	int port;
	int timestamp;
	bool onstatus;
	int interval;
	int currentvalue;
};

struct CoordinatorDetail{
	int id;
	char ipaddress[30];
	int port;
};

struct MessageBuffer{
bool written;
char messageid[50];
char msg[200];
};

int TransactionID = 0;
struct CoordinatorDetail coordinator;
struct CoordinatorDetail replica;
struct device deviceList[1000];
struct MessageBuffer msgList[1000];
int threadID = 0;
long int timediff= 0;
int backendsocket = -1;
char outputfilename[100];
int msgNumber=0;
int  GatewayPort=0;
//the thread function
void *connection_handler(void *);

void getCoordinatorDetails(char rawdetails[100])
{
//struct CoordinatorDetails c;
char **res1,**res;
int count=0;

split(rawdetails, ':', &res1, &count);
split(res1[1], ',', &res, &count);
strcpy(coordinator.ipaddress , res[0]);
coordinator.port = atoi(res[1]);
coordinator.id=-1;

//printf("Primary Cordinator is : %s,%d\n", coordinator.ipaddress,coordinator.port);

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

void getdevicedetails(char str[], struct device *newDevice) {
	char **res, **res2;
	int k = 0, k2 = 0;
	int count = 0, count2 = 0;
	int rc;
	char reschar[50];
	//struct Device *newDevice=((struct Device *)malloc(sizeof(struct Device)));

	rc = split(str, ':', &res, &count);
	if (rc) {
		printf("Error: %s errno: %d \n", strerror(errno), errno);
	}

	strcpy(reschar, res[2]);

	rc = split(reschar, '-', &res2, &count2);
	if (rc) {
		printf("Error: %s errno: %d \n", strerror(errno), errno);
	}

	strcpy(newDevice->type, res2[0]);
	strcpy(newDevice->ipaddress, res2[1]);
	newDevice->port = atoi(res2[2]);
	//newDevice->areaid = atoi(res2[3]);
	newDevice->interval = DefaultInterval;

	free(res);
	free(res2);
	return;
}

//sends a on message to the client
int SwitchDeviceOn(int sock, struct device *newDevice) {
	char message[255] ;
	char timestamp[50],stringBuilder[255];
	long int t = (unsigned long int) time(NULL) + timediff;
	int i;

	strcpy(message,"Type:Switch;Action:On");
	strcat(message,";Timestamp:");
	sprintf(timestamp, "%u",(unsigned) t);
	//printf("I am here %s\n",timestamp);
	strcat(message,timestamp);
	i = (unsigned)strlen(message);
	message[i] = '\0';
	//printf("Message to device : %s and long %u\n",message,(unsigned)strlen(message));
	strcpy(stringBuilder,"Switch : ON : ");
	strcat(stringBuilder,newDevice->type);
			WriteToOutputFile(outputfilename,stringBuilder);
	return write(sock, message, strlen(message));
}

//sends an off message to the client
int SwitchDeviceOff(int sock, struct device *newDevice) {
	char message[255] = "Type:Switch;Action:Off";
	char timestamp[50],stringBuilder[255];
	int i;
		long int t = (unsigned long int) time(NULL) + timediff;
	strcat(message,";Timestamp:");
	sprintf(timestamp, "%u",(unsigned) t);
	strcat(message,timestamp);
		i = (unsigned)strlen(message);
		message[i] = '\0';
		strcpy(stringBuilder,"Switch : OFF : ");
			strcat(stringBuilder,newDevice->type);
			WriteToOutputFile(outputfilename,stringBuilder);
	return write(sock, message, strlen(message));
}

int SetInterval(int sock, int interval, struct device sdevice) {
	char message[255] = "Type:setInterval;Action:";
	char cinterval[2];
	long int t = (long int) time(NULL) + timediff;
	sprintf(cinterval, "%d", interval);
	strcat(message, cinterval);

		strcat(message,";Timestamp:");
		strcat(message,(char *)t);

	return write(sock, message, strlen(message));
}

int EvaluateDeviceDetails(char *client_message,int ItemID)
{
	int linecount = 0,countdelimiter=0,i=0;
		char **res;
		char client_message1[100];
		strcpy(client_message1,client_message);
	if((strstr(client_message1, "TRUE")!=NULL)||(strstr(client_message1, "OPEN")!=NULL))
		deviceList[ItemID].onstatus = true;
	if((strstr(client_message1, "FALSE")!=NULL)||(strstr(client_message1, "CLOSE")!=NULL))
		deviceList[ItemID].onstatus = false;
	split( client_message1, ':', &res, &countdelimiter );
	deviceList[ItemID].timestamp = atoi(res[3]);
	//Logical Clock
	if(deviceList[ItemID].timestamp > ((long int) time(NULL)+timediff))
		timediff = timediff + (deviceList[ItemID].timestamp - ((long int) time(NULL)));
	return 0;
}

/*int GetSmartDeviceItemID(int SensoritemID)
 {
 int i=0,areaid;

 //areaid = deviceList[SensoritemID].areaid;
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
 */
void ChangeStatusDevice(int ItemID, bool status) {
	deviceList[ItemID].onstatus = status;
}

void WriteToBackend(StatechangeDeviceID) {
	//printf("WriteToBackend\n");
	int n;
	char buff[200],buffer[40];
	int i;


	 //insert,SensorID,SensorType,SensorState,TimeStamp,IPAddress,PortNumber.*/
	strcpy(buff,"insert,");
	sprintf(buffer, "%d", deviceList[StatechangeDeviceID].id);
	strcat(buff, buffer);
	strcpy(buffer ,"");
	strcat(buff, ",");
	strcat(buff, deviceList[StatechangeDeviceID].type);
	strcat(buff, ",");
	if (strcmp(deviceList[StatechangeDeviceID].type,"DOOR")==0) {

		if (deviceList[StatechangeDeviceID].onstatus == true) {
			strcat(buff, "Open");
		} else {
			strcat(buff, "Close");
		}
	}

	else if (strcmp(deviceList[StatechangeDeviceID].type,"KEYCHAIN")==0){
		if (deviceList[StatechangeDeviceID].onstatus == true) {
			strcat(buff, "True");
		} else {
			strcat(buff, "False");
		}
}
else if(strcmp(deviceList[StatechangeDeviceID].type,"MOTION")==0){
		if (deviceList[StatechangeDeviceID].onstatus == true) {
			strcat(buff, "True");
		} else {
			strcat(buff, "False");
		}
}
else if (strcmp(deviceList[StatechangeDeviceID].type,"DEVICE")==0){
		if (deviceList[StatechangeDeviceID].onstatus == true) {
			strcat(buff, "On");
		} else {
			strcat(buff, "Off");
		}
}
else{
		;
	}

	strcat(buff,",");
	sprintf(buffer, "%d", deviceList[StatechangeDeviceID].timestamp);
	strcat(buff,buffer);
	strcpy(buffer,"");
	strcat(buff,",");
	strcat(buff,deviceList[StatechangeDeviceID].ipaddress);
	strcat(buff,",");
	sprintf(buffer, "%d", deviceList[StatechangeDeviceID].port);
	strcat(buff,buffer);
	//printf("Before calling Prepare msg to multicast %s\n",buff);
if(strlen(deviceList[StatechangeDeviceID].type)>1){
	//printf("After bypassing calling Prepare msg to multicast %s\n",buff);
	PrepareMsgToReplicas(buff,GatewayPort);
}
}

void CallBackend(char msg[200]){
	printf("CallBackend\n");
	struct sockaddr_in DeviceSocket;
	int SmartDeviceDescriptor,n,i;
	int ItemID=-1;
		//printf("DeviceiD : %d\n",StatechangeDeviceID);
			if (backendsocket == -1){
			for(i=0;i<=threadID;i++)
			if(strcmp(deviceList[i].type, "BACKEND")==0){ ItemID = deviceList[i].id;
			printf("Found Backend %d\n",ItemID);}

			SmartDeviceDescriptor = socket(AF_INET, SOCK_STREAM, 0);
			bzero(&DeviceSocket, sizeof(DeviceSocket));
			DeviceSocket.sin_family = AF_INET;
			DeviceSocket.sin_port = htons(deviceList[ItemID].port);

			// Write IP address of a DeviceSocket to the address structure
			DeviceSocket.sin_addr.s_addr = inet_addr(deviceList[ItemID].ipaddress);
			// Connect to the remote DeviceSocket
			int Result = connect(SmartDeviceDescriptor,
					(struct sockaddr*) &DeviceSocket, sizeof(DeviceSocket));
			backendsocket  = SmartDeviceDescriptor;
			}
			else{
				SmartDeviceDescriptor = backendsocket;
				printf("AssignedBackendID");
			}
		n = write(SmartDeviceDescriptor, msg, strlen(msg));
}

void InitiateTwoPhaseCommit(char messageID[20]){
char strBuilder[100],outputBuilder[100];
int n=0;
strcpy(strBuilder,"Type:ReadyToCommit:Action:");
strcat(strBuilder,messageID);
printf("InitiateTwoPhaseCommit msgid: %s\t %s\n",messageID,strBuilder);
//printf("Tryimg to wrt to replica id : %d\t %d bytes\n",replica.id,n);
n= write(replica.id , strBuilder, strlen(strBuilder) );
strcpy(outputBuilder,"Initiate Ready To Commit : ");
strcat(outputBuilder,messageID);
WriteToOutputFile(outputfilename,outputBuilder);
//printf("Wrote to replica id : %d\t %d bytes",replica.id,n);
}

void AddMsgToBuffer(char message[200]){
	//printf("AddMsgToBuffer : %s\n",message);
	char **res,*res1,*res2;
	int count=0;
	split( message, ':', &res, &count);
	msgList[msgNumber].written=false;
	strcpy(msgList[msgNumber].messageid,res[2]);
	strcpy(msgList[msgNumber++].msg,res[4]);
	if(coordinator.id==0)
	InitiateTwoPhaseCommit(res[2]);
}

bool CheckIfMsgInBuffer(char message[200]){
	//printf("CheckIfMsgInBuffer : %s\n",message);
	int i=0,count=0;
	char **res,msg[200];
	strcpy(msg,message);
	split( msg, ':', &res, &count);
	for(i=0;i<msgNumber;i++){
		if(strstr(msgList[i].messageid,res[3])!=NULL)
			return true;
	}
	return false;
}

void VoteTwoPhaseCommit(char msg[200],bool response){
//	printf("VoteTwoPhaseCommit\n");
char strBuilder[100], message[200],outputBuilder[100];
char **res;
int count=0;
strcpy(message,msg);
//printf("Inside Vote : %s\n",message);
	split( message, ':', &res, &count);
strcpy(strBuilder,"Type:Vote:Action:");
if(response) strcat(strBuilder,"Yes,");
else strcat(strBuilder,"No,");
strcat(strBuilder,res[3]);
//printf("I am inside Vote %d\n",coordinator.id);
write(coordinator.id , strBuilder, strlen(strBuilder) );
//printf("I am inside Vote Wrote to Coordinator %s\n",strBuilder);
//write to output file
strcpy(outputBuilder,"Vote : ");
if(response) strcat(outputBuilder,"Yes for Message : ");
else strcat(outputBuilder,"No for Message : ");
strcat(outputBuilder,res[3]);

WriteToOutputFile(outputfilename,outputBuilder);
}

void AckTwoPhaseCommit(char messageID[200]){
//printf("AckTwoPhaseCommit\n");
char strBuilder[100],outputBuilder[100];
strcpy(strBuilder,"Type:Ack:Action:");
strcat(strBuilder,messageID);
write(coordinator.id , strBuilder, strlen(strBuilder) );
strcpy(outputBuilder,"Ack of Commit : ");
strcat(outputBuilder,messageID);
WriteToOutputFile(outputfilename,outputBuilder);
}

void CommitRollbackTwoPhaseCommitToReplica(char message[200]){
printf("CommitRollbackTwoPhaseCommitToReplica\n");
char strBuilder[100];
char **res,**res1;
int count=0;
char outputBuilder[100];
if(strstr(message,"Yes")!=NULL){
strcpy(strBuilder,"Type:Commit:Action:");
}
else strcpy(strBuilder,"Type:Rollback:Action:");
split( message, ':', &res, &count);
split( res[3], ',', &res1, &count);
strcat(strBuilder,res1[1]);
strcat(strBuilder,",");
strcat(strBuilder,res1[2]);
write(replica.id , strBuilder, strlen(strBuilder) );
if((strstr(strBuilder,"Commit")!=NULL)&& coordinator.id==0){
	printf("WRt to bckend\n");
	CommitWrtBackend(strBuilder);
	printf("DONE\n");
}
if((strstr(strBuilder,"Commit")!=NULL))strcpy(outputBuilder,"Send Commit to Replica : ");
else strcpy(outputBuilder,"Send Rollback to Replica : ");
strcat(outputBuilder,res1[1]);
strcat(outputBuilder,",");
strcat(outputBuilder,res1[2]);
WriteToOutputFile(outputfilename,outputBuilder);
printf("End of Rollback\n");
printf("End   WRt to bckend done\n");
}

int PrepareMsgToReplicas(char message[200],int port){
	printf("PrepareMsgToReplicas : %d\n",port);
	char strBuilder[200],cport[5],tid[10],outputBuilder[200];
	int n=0;
	if(strstr(message,"BACKEND")==NULL){
		//printf("I am Inside if backend\n");
	TransactionID++;
	strcpy(strBuilder,"Type:2PC:");
	sprintf(cport, "%d", port);
	strcat(strBuilder,cport);
	strcat(strBuilder,",");
	sprintf(tid, "%d", TransactionID++);
	strcat(strBuilder,tid);
	strcat(strBuilder,":Action:");
	strcat(strBuilder,message);
	strcpy(msgList[msgNumber].msg,message);
	msgList[msgNumber].written = false;
	if(coordinator.id==0){

			n = write(replica.id , strBuilder, strlen(strBuilder) );
			strcpy(outputBuilder,"Multicast Message : ");
			strcat(outputBuilder,cport);
				strcat(outputBuilder,",");
				strcat(outputBuilder,tid);

			WriteToOutputFile(outputfilename,outputBuilder);
			//printf("Wrote Bytes : %d\n",n);
			strcpy(strBuilder,"");
			strcpy(strBuilder,cport);
			strcat(strBuilder,",");
			strcat(strBuilder,tid);
			//printf("String Builder to init : %s\n",strBuilder);
			sleep(1);
			InitiateTwoPhaseCommit(strBuilder);

	}
	else {
		printf("strBuilder if not Primary : %s\n",strBuilder);
		write(coordinator.id , strBuilder, strlen(strBuilder) );
		strcpy(outputBuilder,"Multicast Message : ");
		strcat(outputBuilder,cport);
		strcat(outputBuilder,",");
		strcat(outputBuilder,tid);
		WriteToOutputFile(outputfilename,outputBuilder);
	}
	strcpy(strBuilder,"");
	strcpy(strBuilder,cport);
	strcat(strBuilder,",");
	strcat(strBuilder,tid);
	strcpy(msgList[msgNumber++].messageid,strBuilder);
	}
	return -1;
}

void CommitWrtBackend(char msg[200]){
	printf("CommitWrtBackend\n");
	char **res,*ret;
	char message[200],val[20];
	int count=0,i=0;
	strcpy(message,msg);
	split( message, ':', &res, &count);
	printf("msg to commit %s\n",res[3]);
	strcpy(val,res[3]);
	ret = (strstr(msgList[1].messageid,res[3]));
	//printf("%d\n",);
	for(i=0;i<msgNumber;i++){
		printf("MsgID :%s\n",msgList[i].messageid);
		if(strcmp(msgList[i].messageid,res[3])==0){
			//printf("Found Msg");
			CallBackend(msgList[i].msg);
			msgList[i].written = true;
			if(coordinator.id==0) printf("Backend Wrt : %s\n",msgList[i].msg);
			if(coordinator.id!=0) AckTwoPhaseCommit(msgList[i].messageid);
		}
		else printf("HERE\n");

	}
}

void Infer(char client_message[255],int ItemID){
	//printf("Infer\n");
int i=0,j=0;
bool presence=false;
//printf("INFERINGGGG %s\n",deviceList[ItemID].type);
	if((strstr(deviceList[ItemID].type,"DOOR")!=NULL) && (strstr(client_message,"OPEN")!=NULL))
	{
		//printf("I am at 1\n");
		for(i=0;i<=threadID;i++){
			//printf("%s\t %s\n",deviceList[i].type,(deviceList[i].onstatus ? "true" : "false"));
			if(strstr(deviceList[i].type,"MOTION")!=NULL && (deviceList[i].onstatus==true)){
			//	printf("Door and motion\n");
				if ((deviceList[i].timestamp + timediff) < (deviceList[ItemID].timestamp+ timediff)){
					WriteToOutputFile(outputfilename,"User just left from house AWAY MODE System");
					for(j=0;j<=threadID;j++){
						if(strstr(deviceList[j].type,"DEVICE")!=NULL && (deviceList[j].onstatus==false)){
							deviceList[j].onstatus = true;
							SwitchDeviceOn(deviceList[j].interval, &deviceList[j]);
						}
					}
				}
			}
		}
	}
	else if((strstr(deviceList[ItemID].type,"MOTION")!=NULL)&&strstr(client_message,"TRUE")!=NULL)
	{
		//printf("I am at 2\n");
		for(i=0;i<=threadID;i++){
			//printf("%s\t %s\n",deviceList[i].type,(deviceList[i].onstatus ? "true" : "false"));
		if(strstr(deviceList[i].type,"DOOR")!=NULL && (deviceList[i].onstatus==true)){

						if ((deviceList[i].timestamp + timediff) < (deviceList[ItemID].timestamp+ timediff)){
							WriteToOutputFile(outputfilename,"User entered house System HOME MODE");
								for(j=0;j<=threadID;j++){
								if(strstr(deviceList[j].type,"KEYCHAIN")!=NULL && (deviceList[j].onstatus==true)){
											presence= true;
									}
								}
								if(presence == true){
							for(j=0;j<=threadID;j++){
								if(strstr(deviceList[j].type,"DEVICE")!=NULL && (deviceList[j].onstatus==true)){
														deviceList[j].onstatus = false;
														SwitchDeviceOff(deviceList[j].interval, &deviceList[j]);

								}
							}
								}
								else WriteToOutputFile(outputfilename,"ALARM : Intruder in house");
						}
						else;
		}
		else if(strstr(deviceList[i].type,"DOOR")!=NULL && (deviceList[i].onstatus==false)){
			//printf("Motion and Door CLOSE\n");
			if ((deviceList[i].timestamp + timediff) < (deviceList[ItemID].timestamp+ timediff))
				WriteToOutputFile(outputfilename,"User is inside the house HOME MODE");

				else;
		}
		else;
		}
	}
		else;
	}

void WriteToOutputFile(char filename[100],char message[255])
{
	//printf("WriteToOutputFile\n");
	FILE *fp;
	char stringBuilder[255];
	strcpy(stringBuilder,message);
	fp = fopen(filename, "a");

		    fprintf(fp, "%s\n", stringBuilder);
		    fclose(fp);

}

int CheckLoadBalance(){
	//printf("CheckLoadBalance\n");
	int i,sensor=0,device=0;
	if(replica.id > 0) // if device is replica
	return 0;


	for(i=0 ; i<=threadID;i++){
	if((strstr(deviceList[i].type,"BACKEND")==NULL) && (strstr(deviceList[i].type,"DEVICE")==NULL)&& (strstr(deviceList[i].type,"REPLICA")==NULL)){
		// then the device is sensor
		sensor++;
	}
	else if((strstr(deviceList[i].type,"DEVICE")!=NULL)){
		device++;
	}
	else;
	}

	if(sensor >=2 || device >=1) return -1;
	return 0;
}

int SendReplicaDetailsToCordinator(char ipaddress[30],int port)
{
	//printf("SendReplicaDetailsToCordinator\n");
	char message[100],cport[5];
	struct sockaddr_in DeviceSocket;
	int SmartDeviceDescriptor, n;

	if(coordinator.id == -1)
	{
		SmartDeviceDescriptor = socket(AF_INET, SOCK_STREAM, 0);
			bzero(&DeviceSocket, sizeof(DeviceSocket));
			DeviceSocket.sin_family = AF_INET;
			DeviceSocket.sin_port = htons(coordinator.port);

			// Write IP address of a DeviceSocket to the address structure
			DeviceSocket.sin_addr.s_addr = inet_addr(coordinator.ipaddress);
			// Connect to the remote DeviceSocket
			int Result = connect(SmartDeviceDescriptor,
					(struct sockaddr*) &DeviceSocket, sizeof(DeviceSocket));
			coordinator.id  = SmartDeviceDescriptor;
	}
	strcpy(message,"Type:register;Action:");
	strcat(message,"REPLICA");
	strcat(message,"-");
	strcat(message,ipaddress);
	strcat(message,"-");
	sprintf(cport, "%d", port);
	strcat(message,cport);
	//strcat(message,"-");
	//strcat(message,areaid);
	n = write(coordinator.id , message, strlen(message) );
	return n;
}

void ConnectToReplica()
{
	char message[100],cport[5];
		struct sockaddr_in server;
		int sock, n;

		if(replica.id == -1)
		{
			sleep(2);
			sock = socket(AF_INET , SOCK_STREAM , 0);
					if (sock == -1)
					{
						printf("Could not create Server socket");
					}

					server.sin_addr.s_addr = inet_addr(replica.ipaddress);
								server.sin_family = AF_INET;
								server.sin_port = htons( replica.port );
			//SmartDeviceDescriptor = socket(AF_INET, SOCK_STREAM, 0);
				//bzero(&DeviceSocket, sizeof(DeviceSocket));
				//DeviceSocket.sin_family = AF_INET;
				//DeviceSocket.sin_port = htons(replica.port);

				// Write IP address of a DeviceSocket to the address structure
				//DeviceSocket.sin_addr.s_addr = inet_addr(replica.ipaddress);
				// Connect to the remote DeviceSocket
				//int Result = connect(SmartDeviceDescriptor,
				//		(struct sockaddr*) &DeviceSocket, sizeof(DeviceSocket));
				//if(Result==0)printf("Connected to replica Result = 0\n");
				//else printf("Kitkond hoitu ip : %s port : %d\n",replica.ipaddress,replica.port);
								if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
												{
													perror("connect failed. Error");
													//return 1;
												}
								else
												puts("Connected to replica server\n");
								replica.id  = sock;
		}
		//printf("Connected to replica with id : %d\n",replica.id);
}

//send a message to device to connect to replica instead
int ConnectToReplicaToDevice(int sock){
	//printf("ConnectToReplicaToDevice\n");
	int n=0;
	char message[50],cport[5];
	sprintf(cport, "%d", replica.port);
	strcpy(message,"Type:Connect;Action:");
	strcat(message,replica.ipaddress);
	strcat(message,",");
	strcat(message,cport);
	return write(sock, message, strlen(message));
}





/*int DisplayState() {
	int i = 0;
	char message[1024], itemid[2], port[5], areaid[3], currentvalue[3];
	strcpy(message, "-----------------------------------------------------\n");

	for (i = 0; i < threadID; i++) {
		if (deviceList[i].type == " ")
			continue; // do nothing
		else if (deviceList[i].type == "sensor"
				&& deviceList[i].currentvalue == 0)
			continue;
		else //if(deviceList[i].type != " " || deviceList[i].currentvalue !=0 )
		{
			sprintf(itemid, "%d", deviceList[i].id);
			sprintf(port, "%d", deviceList[i].port);
			//sprintf(areaid, "%d",deviceList[i].areaid);
			sprintf(currentvalue, "%d", deviceList[i].currentvalue);
			strcat(message, itemid);
			strcat(message, "----");
			strcat(message, deviceList[i].ipaddress);
			strcat(message, ":");
			strcat(message, port);
			strcat(message, "----");
			strcat(message, deviceList[i].type);
			//strcat(message,"----");
			//strcat(message,areaid);
			strcat(message, "----");
			if (strstr(deviceList[i].type, "sensor") > 0) {
				strcat(message, currentvalue);
				strcat(message, "\n");
			} else {
				if (deviceList[i].onstatus == true)
					strcat(message, "On\n");
				else
					strcat(message, "Off\n");
			}

		}
	}
	strcat(message,
			"---------------------------------------------------------\n");
	printf("%s\n", message);

	return 0;
}
*/

int main(int argc, char *argv[]) {
	int socket_desc, client_sock, c, *new_sock;
	struct sockaddr_in server, client;

//	struct ip_mreq mreq;

	int read_size;
	char *message, client_message[200];
	char filename[25];
	int len, i = 0, count = 0;
	FILE *fp;
	char buff[255], port[4], ch;
	char ipAddress[20];
	pthread_t taskthread[1000];
    char **res;


	if (argc != 3) {
		printf("Improper Arguments!!!\n");
		exit(0);
	} else {
		strcpy(filename, argv[1]);
		strcpy(outputfilename, argv[2]);
	}
	//TODO: IF FILE DOESNT EXIST, exit!
	fp = fopen(filename, "r");
	fgets(buff, 255, (FILE*) fp);
	printf("Buffer : %s\n",buff);
	split(buff,',',&res,&count);
	printf("res[0] : %s\n",res[0]);
	printf("res[1] : %s\n",res[1]);
	strcpy(ipAddress,res[0]);
	strcpy(port,res[1]);
	// Reading IP
	/*while (buff[count] != ',') {
		ipAddress[count] = buff[count];
		count++;
	}
	ipAddress[count] = '\0';

	len = count + 4;

	// Reading port
	while (count < len) {
		count++;
		port[i] = buff[count];
		i++;
	}
	// null terminate the string
	port[i] = '\0';
	*/

	strcpy(buff,"");
	fgets(buff, 255, (FILE*) fp);
	GatewayPort = atoi(port);
	getCoordinatorDetails(buff);
	printf("ip address : %s , Port : %s",ipAddress,port);
	if((strcmp(coordinator.ipaddress,ipAddress)==0) && (coordinator.port == atoi(port))){
		printf("I am the Coordinator\n");
		coordinator.id = 0; // if current is coordinator
	}
	else{
		// send your ip to primary server
		printf("I am Replica\n");
		coordinator.id = -1; // if current is not coordinator
		i = SendReplicaDetailsToCordinator(ipAddress,atoi(port));
		//printf("I wrote details to Primary : %d\n",i);
	}
	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(ipAddress);
	server.sin_port = htons(atoi(port));

	//For multicast



	//Bind
	if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");


	//Listen
	listen(socket_desc, 10);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	while ((client_sock = accept(socket_desc, (struct sockaddr *) &client,
			(socklen_t*) &c))) {
		puts("Connection accepted");

		//pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;

		if (pthread_create(&taskthread[threadID], NULL, connection_handler,
				(void *) (&client_sock)) < 0) {
			perror("could not create thread");
			return 1;
		}

	}

	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}

	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc) {
	//Get the socket descriptor
	int sock = *(int*) socket_desc;
	int read_size = 0, ItemID, n, load;
	char *message;
	char client_message[255],stringBuilder[255],outputBuilder[200];
	struct device newdevice;

	//Thread Number of the thread
	ItemID = threadID++;
	//printf("Inside Handler\n");
while(1){
	bzero(client_message, 256);
	read_size = read(sock, client_message, 255);
	//printf("CM : %s\n",client_message);

		if (strstr(client_message, "register") != NULL) {
		if(strstr(client_message, "REPLICA") != NULL){
			//printf("Received Replica register\n");
			getdevicedetails(client_message, &newdevice);
			strcpy(replica.ipaddress , newdevice.ipaddress);
			replica.port = newdevice.port;
			replica.id=-1;
			ConnectToReplica();

			//printf("Replica id : %d\n",replica.id);
			//write(sock,"HI REPLICA You are undr control",50);
		}
		else{
		load = CheckLoadBalance();
		printf("Checking for backend%s\n",deviceList[ItemID].type);
		if((strstr(client_message,"BACKEND")!=NULL)){
			load=0;
			printf("I am here\n");
		}
		if(load == 0){
			printf("Load Balance = 0 \n");
		getdevicedetails(client_message, &newdevice);
		newdevice.onstatus = false;
		newdevice.timestamp = time(NULL);
		newdevice.interval = 5;
		newdevice.id = ItemID;
		newdevice.currentvalue = 0;			// default value
		deviceList[ItemID] = newdevice;
		strcpy(stringBuilder,"Registered New Device :");
		strcat(stringBuilder,deviceList[ItemID].type);
		WriteToOutputFile(outputfilename,stringBuilder);

		printf("Registered New Device :  %d\t which is %s\n",ItemID,deviceList[ItemID].type);
		if(strstr(deviceList[ItemID].type,"DEVICE")!=NULL) deviceList[ItemID].interval= sock;
		n = SwitchDeviceOn(sock, &deviceList[ItemID]);

		WriteToBackend(ItemID);
		//printf("Written turnOn device: %d\n",n);
		}
		else{
			//connect device to other server
			printf("Over Loaded so connect to different server\n");
			n = ConnectToReplicaToDevice(sock);
			strcpy(outputBuilder,"Sent Connect To Replica Request to ");
			strcat(outputBuilder,deviceList[ItemID].type);
			WriteToOutputFile(outputfilename,outputBuilder);
			threadID--;
			//printf("I wrote to device to connect to Replica %d\n",n);
		}
		}
	}

	while (1) {
		bzero(client_message, 255);
		read_size = read(sock, client_message, 255);
		if (read_size > 1) {
			printf("Received something %s\n",client_message);
			if (strstr(client_message, "ReadyToCommit") != NULL) {
				//printf("Inside Ready To commit : %s\n",client_message);
							strcpy(outputBuilder,"Received Ready To Commit");
							WriteToOutputFile(outputfilename,outputBuilder);
							if(CheckIfMsgInBuffer(client_message)){
								//printf("After CheckifMsgInBuffer : %s\n",client_message);
								VoteTwoPhaseCommit(client_message,true);
								//printf("After Vote : %s\n",client_message);
							}
							else VoteTwoPhaseCommit(client_message,false);
			}
			else if (strstr(client_message, "Vote") != NULL) {
							//printf("Received Vote\n");
							CommitRollbackTwoPhaseCommitToReplica(client_message);
							printf("Commit sent\n");
			}
			else if (strstr(client_message, "Commit") != NULL) {
				CommitWrtBackend(client_message);
				strcpy(outputBuilder,"Received Commit");
				WriteToOutputFile(outputfilename,outputBuilder);
			}
			else if (strstr(client_message, "Rollback") != NULL) {
							//do Nothing
				strcpy(outputBuilder,"Received Rollback");
											WriteToOutputFile(outputfilename,outputBuilder);
			}
			else if (strstr(client_message, "Ack") != NULL) {
				// do nothing
				strcpy(outputBuilder,"Received Ack");
											WriteToOutputFile(outputfilename,outputBuilder);
			}
			else if (strstr(client_message, "PC") != NULL) {
							// do nothing
				printf("Receievd PC\n");
				strcpy(outputBuilder,"Received Multicast Message");
				WriteToOutputFile(outputfilename,outputBuilder);
				AddMsgToBuffer(client_message);
						}
			else;
			if (strstr(client_message, "currValue") != NULL) {
				//TemperatureCheck(sock, client_message, ItemID);
				//DisplayState();
				EvaluateDeviceDetails(client_message,ItemID);
				Infer(client_message,ItemID);
				printf("Received current value\n");
				WriteToBackend(ItemID);

			}


			if (strstr(client_message, "currState") != NULL) {
				printf("Received change");
				ChangeStatusDevice(ItemID, (strstr(client_message, "On")) > 0);
				//DisplayState();
				WriteToBackend(ItemID);
			}

			read_size = 0;
			bzero(client_message, 256);
		}
	}
}
	return 0;
}

