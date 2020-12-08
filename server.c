#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#define RCVSIZE 1018
#define ARRSIZE 256
#define ALPHA	0.8

void removeInt(int array[], int index);
void removeCharArray(char* array[], int index);
int serverHandShake(int ctrl_desc, int port);
void generateSequenceNumber(char *buffer, int number);
void transmit(int client_desc, int message_size, int sequence, int RTT, int SRTT, int mode, int cwnd, char* forwarded[RCVSIZE], int expected[], int retransmit[]);
void expect(int client_desc, int message_size, int sequence, int RTT, int SRTT, int mode, int cwnd, char* forwarded[RCVSIZE], int expected[], int retransmit[]);




void removeInt(int array[], int index){
	for (int i=index;i<ARRSIZE-1;i++) {
		array[i] = array[i+1];
	}
	array[ARRSIZE-1]=-1;
}
// OPTMIMISATION : PARSE ONLY RELEVANT ARRAY ENTRIES
void removeCharArray(char* array[], int index){
	for (int i=index;i<ARRSIZE-1;i++) {
		for (int j=0;j<RCVSIZE-1;j++) {
			array[i][j] = array[i+1][j];
		}
	}
	for (int k=0;k<RCVSIZE-1;k++) {
		array[ARRSIZE-1][k]='\0';
	}
}

int serverHandShake(int ctrl_desc, int port){
		char Tx[RCVSIZE]="SYN-ACK";
		char buffer[10];
		sprintf(buffer, "%d", port); // mettre plusieur numero de port pour plusieur client
		strcat(Tx, buffer);
		char Rx[RCVSIZE]="";
		struct sockaddr_in adresse_client, adresse_server;
		socklen_t alen= sizeof(adresse_client);

		adresse_server.sin_family= AF_INET;
		adresse_server.sin_port= htons(port);
		adresse_server.sin_addr.s_addr= htonl(INADDR_ANY);

		int client_desc= socket(AF_INET, SOCK_DGRAM,0);
		if (bind(client_desc, (struct sockaddr*) &adresse_server, sizeof(adresse_server)) == -1) {
				perror("Bind failed\n");
				close(client_desc);
				return -1;
		}


		recvfrom(ctrl_desc, Rx, RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
		printf("Message received\n");
		if(strcmp(Rx,"SYN")==0){
				printf("Sending SYN-ACK\n");
						sendto(ctrl_desc,Tx, strlen(Tx), 0, (struct sockaddr*) &adresse_client, alen);
				memset(Rx,0, RCVSIZE);
				recvfrom(ctrl_desc, Rx, RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
				if(strcmp(Rx,"ACK")!=0){
						printf("NO ACK RECEIVED FROM CLIENT, msg=%s", Rx);
				}else{
					printf("Covid was sucessfully transmited!\n");
						return client_desc;
				}
		}else{
				printf("ERRORR, msg:%s\n", Rx);
		}
}

void generateSequenceNumber(char *buffer, int number){
	char string[6];
	sprintf(string, "%d", number);
	number%=1000000;
	if(number>=100000){
		strcpy(buffer, "");
	}else{
		if(number>=10000){
			strcpy(buffer, "0");
		}else{
			if(number>=1000){
				strcpy(buffer, "00");
			}else{
				if(number>=100){
					strcpy(buffer, "000");
				}else{
					if(number>=10){
						strcpy(buffer, "0000");
					}else{
						if(number>=0){
							strcpy(buffer, "00000");
						}
					}

				}
			}
		}
	}
	strcat(buffer, string);	
}


void transmit(int client_desc, int message_size, int sequence, int RTT, int SRTT, int mode, int cwnd, char* forwarded[RCVSIZE], int expected[], int retransmit[]){
	int received_size;
	int duplicate_sequence;
	char message[RCVSIZE];
	char buffer[RCVSIZE];
	struct sockaddr_in adresse_client;
	socklen_t alen = sizeof(adresse_client);
	FILE* file = NULL;
	FILE* filecopy = NULL;
	
	received_size = recvfrom(client_desc, buffer, RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
	buffer[received_size-1]='\0';
	printf("Sending %s file to client, it take %d char\n", buffer, received_size);
	file = fopen(buffer, "r");
	filecopy = fopen("testCopy", "w");
	if(filecopy==NULL){
		filecopy = fopen("testCopy", "a");
	}
	for (int i=0;i<cwnd;i++) {
		if (i<sizeof(&retransmit)) {
			duplicate_sequence = retransmit[i];
			generateSequenceNumber(message, duplicate_sequence);
			printf("duplicate_sequence = %s\n", message);
			memcpy(message+6, forwarded[i], sizeof(&forwarded[i]));
			sendto(client_desc, message, RCVSIZE+6, 0, (struct sockaddr*) &adresse_client, alen);
			expected[i] = duplicate_sequence;
			memset(buffer, 0, RCVSIZE);
			memset(message, 0, RCVSIZE);
		} else if ((file!=0 && file!=NULL) && (message_size>0)) {
			message_size=fread(buffer, 1, RCVSIZE, file);
			generateSequenceNumber(message, sequence);
			printf("sequence = %s\n", message);
			memcpy(message+6, buffer, message_size);
			sendto(client_desc, message, message_size+6, 0, (struct sockaddr*) &adresse_client, alen);
			memcpy(forwarded[i], message+6, sizeof(message+6));
			expected[i] = sequence;
			sequence++;
			memset(buffer, 0, RCVSIZE);
			memset(message, 0, RCVSIZE);
		} else {
			sendto(client_desc, "FIN", 3, 0, (struct sockaddr*) &adresse_client, alen);
		}
	}
	expect(client_desc, message_size, sequence, RTT, SRTT, mode, cwnd, forwarded, expected, retransmit);
	return;
}

void expect(int client_desc, int message_size, int sequence, int RTT, int SRTT, int mode, int cwnd, char* forwarded[RCVSIZE], int expected[], int retransmit[]){
	struct timeval timer, start, end;
	timer.tv_sec=0;
	timer.tv_usec=SRTT;
	char buffer[RCVSIZE];
	struct sockaddr_in adresse_client;
	socklen_t alen = sizeof(adresse_client);
	fd_set socket_table;
	FD_ZERO(&socket_table);
	FD_SET(client_desc, &socket_table);
	gettimeofday(&start, NULL);
	if(select(client_desc+1, &socket_table, NULL, NULL, &timer)==-1){
		perror("Select is ravaged\n");
		return;
	}
	if(FD_ISSET(client_desc, &socket_table)){
		gettimeofday(&end, NULL);
		RTT=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
//		memset(buffer, 0, RCVSIZE);
		int received_size = recvfrom(client_desc, buffer, 9, 0, (struct sockaddr*) &adresse_client, &alen);
		printf("Answer received : %s\n", buffer);
		if (memcmp(buffer, "ACK", 3) == 0) {
			
			// SLOW STARTCWND INCREMENTATION
			if (mode==0) {
				cwnd++;
			}
			// REMOVE SEQUENCE NUMBER FROM EXPECTED
			for (int i=0;i<sizeof(&expected);i++) {
				char current_ack[6];
				sscanf(current_ack, "%d", &expected[i]);
				if (memcmp(buffer+3, current_ack, 6) == 0) {
					removeInt(expected, i);
					removeCharArray(forwarded, i);
					break;
				}
			}
		}
	}
	// BUILDING RETRANSMIT ARRAY
	for (int i=0;i<sizeof(&expected);i++) {
		retransmit[i]=expected[i];
	}
	// CONGESTION AVOIDANCE CWND INCREMENTATION
	if (mode==1) {
		cwnd++;
	}
	transmit(client_desc, message_size, sequence, RTT, SRTT, mode, cwnd, forwarded, expected, retransmit);
}

int main (int argc, char *argv[]) {
	char forwarded[ARRSIZE][RCVSIZE];
	int retransmit[ARRSIZE];
	int expected[ARRSIZE];
	int message_size = 1;
	int sequence = 1;
	int RTT;
	int SRTT = 8000;
	int mode = 0;
	int cwnd = 1;

	struct sockaddr_in adresse_udp, client_udp;
	int port=0;
	if(argc==2){
		port= atoi(argv[1]);
	}
	
	int valid= 1;
	int test;
	socklen_t alen_udp = sizeof(client_udp);
	char buffer[RCVSIZE]="";

	//create socket
	int ctrl_desc = socket(AF_INET, SOCK_DGRAM, 0);

	//Handle error
	if(ctrl_desc <0){
		perror("Cannot create udp socket\n");
		return -1;
	}

	fd_set sockTab;

	//SERVEUR UDP
	setsockopt(ctrl_desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

	adresse_udp.sin_family= AF_INET;
	adresse_udp.sin_port= htons(port);
	adresse_udp.sin_addr.s_addr= htonl(INADDR_ANY);

	//initialize socket
	if (bind(ctrl_desc, (struct sockaddr*) &adresse_udp, sizeof(adresse_udp)) == -1) {
		perror("Bind failed\n");
		close(ctrl_desc);
		return -1;
	}

	int data_desc = serverHandShake(ctrl_desc, port+1); //generer un num de port aleatoire

	while (1) {
		FD_ZERO(&sockTab);
		FD_SET(ctrl_desc, &sockTab);
		FD_SET(data_desc, &sockTab);
		
		if(select(ctrl_desc+data_desc+1, &sockTab,NULL, NULL, NULL)==-1){
			perror("select is ravaged\n");
			return -1;
		}

		if(FD_ISSET(data_desc, &sockTab)){
			transmit(data_desc, message_size, sequence, RTT, SRTT, mode, cwnd, forwarded, expected, retransmit);
		}

		if(FD_ISSET(ctrl_desc, &sockTab)){
			printf("Control msg received \n");
		}
 
	}
close(ctrl_desc);
return 0;
}