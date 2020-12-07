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

int retransmit[256] = {};
int expected[256] = {};
int message_size = 1;
int sequence = 1;
int RTT;
int SRTT = 8000;
float alpha = 0.8;

void transmit(int client_desc, int cwnd){
	
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
		if (i<sizeof(retransmit)) {
			// MESSAGE NEEDS TO BE SPECIFIED BECAUSE WE'RE NOT READING THE FILE HERE
			duplicate_sequence = retransmit[i];
			generateSequenceNumber(message, duplicate_sequence);
			printf("duplicate_sequence = %s\n", message);
			memcpy(message+6, buffer, RCVSIZE);
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
			expected[i] = sequence;
			sequence++;
			memset(buffer, 0, RCVSIZE);
			memset(message, 0, RCVSIZE);
		} else {
			sendto(client_desc, "FIN", 3, 0, (struct sockaddr*) &adresse_client, alen);
		}
	}
	expect(client_desc);
	return;
}

void expect(int client_desc){
	int received_size;
	struct timeval timer, start, end;
	timer.tv_sec=0;
	timer.tv_usec=SRTT;
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
		memset(buffer, 0, RCVSIZE);
		received_size = recvfrom(client_desc, buffer, 9, 0, (struct sockaddr*) &adresse_client, &alen);
		printf("Answer received : %s\n", buffer);
		// STOPPED CODING HERE (DETERMIN WHAT WE NEED TO RETRANSMIT)
		// RETRANSMIT = EXPECTED AND THEN DELETE ELEMENTS WHEN RECEIVED ?
		// DONT FORGET TO CLEANUP DATA WITH AN APPROPRIATE FUNCTION
	}
	transmit(client_desc, cwnd);
}



int sendingFile(int client_desc){

	struct sockaddr_in adresse_client;
	socklen_t alen = sizeof(adresse_client);
	int msgSize;
	char buffer[RCVSIZE];
	int ack = 0;
	int lack = -1;
	FILE* file = NULL;
	int numSeq;
	struct timeval start, current, end;

	msgSize = recvfrom(client_desc,buffer,RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
	buffer[msgSize-1]='\0';
	printf("Sending %s file to client, it take %d char\n", buffer, msgSize);
	file = fopen(buffer, "r");
	FILE* filecopy;

	filecopy = fopen("testCopy", "w");
	if(filecopy==NULL){
		filecopy = fopen("testCopy", "a");
	}

	if(file != 0 && file!=NULL)
	{
		numSeq=1;
		int byteToSend = 1;
		char stringSeq[RCVSIZE];
		int size=0;
		int RTT;
		int SRTT = 8000;
		float alpha = 0.8;
		byteToSend= fread(buffer, 1, RCVSIZE, file);
		fd_set sockTab;
		struct timeval timer;
		gettimeofday(&start, NULL);
		while (byteToSend > 0) 
		{
			//Sending paquet
			generateSequenceNumber(stringSeq, numSeq);
			printf("numSeq = %s\n", stringSeq);
			memcpy(stringSeq+6, buffer, byteToSend);
			//printf("Data to send: %s \n", stringSeq);
			sendto(client_desc, stringSeq, byteToSend+6, 0, (struct sockaddr*) &adresse_client, alen);
			gettimeofday(&current, NULL);

			//select
			FD_ZERO(&sockTab);
			FD_SET(client_desc, &sockTab);
			// TO DO utilisation de pointeur ?
			timer.tv_sec=0;
			timer.tv_usec=SRTT;

			if(select(client_desc+1, &sockTab,NULL, NULL, &timer)==-1){
				perror("select is ravaged\n");
				return -1;
			}
			if(FD_ISSET(client_desc, &sockTab)){
					// Receiving paquet
				gettimeofday(&end, NULL);
				
				RTT = (end.tv_sec - current.tv_sec)*1000000 + (end.tv_usec - current.tv_usec);
				
				
				

				memset(buffer, 0, RCVSIZE);
				msgSize = recvfrom(client_desc,buffer,9, 0, (struct sockaddr*) &adresse_client, &alen);
				printf("The message received is : %s\n", buffer);
				
				buffer[9] = '\0';
				ack = atoi(buffer+3);
				
				if(ack == lack) {
					printf("Duplicate ACK detected\n");
					RTT=8000;
				} else {
					numSeq++;
					byteToSend= fread(buffer, 1, RCVSIZE, file);
				}
				
				lack = ack;
				memset(buffer, 0, RCVSIZE);
				memset(stringSeq, 0, RCVSIZE);
			}else{
				gettimeofday(&end, NULL);
				printf("retransmitting at %ld\n", (end.tv_sec - current.tv_sec)*1000000 + (end.tv_usec - current.tv_usec));
				RTT=8000;
			}
			
			if(RTT <= SRTT / 2) {
				RTT=8000;
			}
			SRTT = alpha * (float)SRTT + (1.0 - alpha) * (float)RTT;
			SRTT = (int)SRTT;
			printf("RTT = %d\n", RTT);
			printf("SRTT = %d\n", SRTT);

		}
		gettimeofday(&end, NULL);
		printf("total time = %ld\n", (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec) );
		printf("The number of block read is %d\n", numSeq);
		printf("Total size of message is %d\n", size);

		sendto(client_desc, "FIN", 3, 0, (struct sockaddr*) &adresse_client, alen);

		
		// WHERE TO IMPLEMENT ? MAIN ?
		close(client_desc);
		exit(0);
	}else{
		printf("Could not open file \n");
	}

	

}

int main (int argc, char *argv[]) {

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
			sendingFile(data_desc);
		}

		if(FD_ISSET(ctrl_desc, &sockTab)){
			printf("Control msg received \n");
		}
 
	}
close(ctrl_desc);
return 0;
}