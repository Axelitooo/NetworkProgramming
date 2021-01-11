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

#define RCVSIZE 1444
#define ARRSIZE 128
#define SEQSIZE 6
#define ALPHA	0.8

void removeInt(int array[ARRSIZE]);
//int removeInt(int array[ARRSIZE]);
void removeCharArray(char** array);
int serverHandShake(int ctrl_desc, int port);
void generateSequenceNumber(char buffer[RCVSIZE], int number);
int transmit(int client_desc, int* message_size, int* sequence, int* RTT, int* SRTT, int* mode, int* cwnd, char** data, int acks[ARRSIZE], FILE* file, struct sockaddr_in addr, struct timeval* startRTT, char buffer[RCVSIZE], char message[RCVSIZE+SEQSIZE]);
void expect(int client_desc, int* message_size, int* sequence, int* RTT, int* SRTT, int* mode, int* cwnd, char** data, int acks[ARRSIZE], FILE* file, struct sockaddr_in addr, struct timeval* startRTT, char buffer[RCVSIZE], char message[RCVSIZE+SEQSIZE]);

void removeInt(int array[ARRSIZE]){
	for (int i=0;i<ARRSIZE-1;i++) {
		array[i] = array[i+1];
		if (array[i+1] == -1) {
			array[i]=-1;
			break;
		}
	}
	array[ARRSIZE-1]=-1;
}

// OPTIMISATION : PARSE ONLY RELEVANT ARRAY ENTRIES
void removeCharArray(char** array){
	memset(array[0], 0, RCVSIZE);
	for (int i=0;i<ARRSIZE-1;i++) {
		memcpy(array[i], array[i+1], RCVSIZE);
	}
	memset(array[ARRSIZE-1], 0, RCVSIZE);
}/*
int removeInt(int array[ARRSIZE]){
	//printf("- int removing");
	for (int i=0;i<ARRSIZE-1;i++) {
		array[i] = array[i+1];
		if (array[i+1] == -1) {
			return i+1;
		} else if (i == ARRSIZE-1) {
			array[i+1] = -1;
		}
	}
	return ARRSIZE;
}

// OPTIMISATION : PARSE ONLY RELEVANT ARRAY ENTRIES
void removeCharArray(char array[ARRSIZE][RCVSIZE], int index){
	//printf("- array removing");
	for (int i=0;i<index-1;i++) {
		memset(array[i], 0, RCVSIZE);
		memcpy(array[i], array[i+1], RCVSIZE);
	}
	memset(array[index-1], 0, RCVSIZE);
}*/

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

// OPTIMISATION : ZERO PADDING
void generateSequenceNumber(char buffer[RCVSIZE], int number){
	char* string = (char*)calloc(SEQSIZE, SEQSIZE*sizeof(char));
	memset(string, 0, SEQSIZE);
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
	free(string);
}

int transmit(int client_desc, int* message_size, int* sequence, int* RTT, int* SRTT, int* mode, int* cwnd, char** data, int acks[ARRSIZE], FILE* file, struct sockaddr_in addr, struct timeval* startRTT, char buffer[RCVSIZE], char message[RCVSIZE+SEQSIZE]){
	int* duplicate_sequence = (int*)malloc(sizeof(int));
	//socklen_t alen = sizeof(addr);
	socklen_t* alen = (socklen_t*)malloc(sizeof(socklen_t));
	*alen = sizeof(addr);
	gettimeofday(startRTT, NULL);
	for (int i=0;i<*cwnd;i++) {
		if (acks[i] != -1) {
			*duplicate_sequence = acks[i];
			generateSequenceNumber(message, *duplicate_sequence);
			memcpy(message+SEQSIZE, data[i], RCVSIZE);
			if (i != ARRSIZE) {
				if (acks[i+1] == -1) {
					sendto(client_desc, message, *message_size+SEQSIZE, 0, (struct sockaddr*) &addr, *alen);
				} else {
					sendto(client_desc, message, RCVSIZE+SEQSIZE, 0, (struct sockaddr*) &addr, *alen);
				}
			} else {
				sendto(client_desc, message, *message_size+SEQSIZE, 0, (struct sockaddr*) &addr, *alen);
			}
			memset(message, 0, RCVSIZE+SEQSIZE);
		} else if ((file!=0 && file!=NULL) && (*message_size==RCVSIZE)) {
			*message_size=fread(buffer, 1, RCVSIZE, file);
			generateSequenceNumber(message, *sequence);
			memcpy(message+SEQSIZE, buffer, *message_size);
			sendto(client_desc, message, *message_size+SEQSIZE, 0, (struct sockaddr*) &addr, *alen);
			memcpy(data[i], message+SEQSIZE, RCVSIZE);
			acks[i] = *sequence;
			*sequence+=1;
			memset(buffer, 0, RCVSIZE);
			memset(message, 0, RCVSIZE+SEQSIZE);
			if(*message_size < RCVSIZE){
				break;
			}
		} else if(i==0){
			sendto(client_desc, "FIN", 3, 0, (struct sockaddr*) &addr, *alen);
			return 1;
		}
	}
	free(duplicate_sequence);
	free(alen);
	// flight = cwnd;
	return 0;
	//expect(client_desc, message_size, sequence, RTT, SRTT, mode, cwnd, data, acks, file, addr, startRTT, buffer, message);
}

void expect(int client_desc, int* message_size, int* sequence, int* RTT, int* SRTT, int* mode, int* cwnd, char** data, int acks[ARRSIZE], FILE* file, struct sockaddr_in addr, struct timeval* startRTT, char buffer[RCVSIZE], char message[RCVSIZE+SEQSIZE]){
	//struct timeval timer, start, end;
	struct timeval* timer = (struct timeval*)malloc(sizeof(struct timeval));
	struct timeval* start = (struct timeval*)malloc(sizeof(struct timeval));
	struct timeval* end = (struct timeval*)malloc(sizeof(struct timeval));
	int* received_size = (int*)malloc(sizeof(int));
	timer->tv_sec=0; // Gerer les secs
	timer->tv_usec=*SRTT;
	//socklen_t alen = sizeof(addr);
	socklen_t* alen = (socklen_t*)malloc(sizeof(socklen_t));
	*alen = sizeof(addr);
	//fd_set socket_table;
	fd_set* socket_table = (fd_set*)malloc(sizeof(fd_set));
	FD_ZERO(socket_table);
	FD_SET(client_desc, socket_table);
	gettimeofday(start, NULL);
	int* received = (int*)calloc(1, sizeof(int));
	int* time = (int*)calloc(1, sizeof(int));
	int* maxACK = (int*)calloc(1, sizeof(int));
	while (*received < *cwnd && *time < *SRTT) {
		if(select(client_desc+1, socket_table, NULL, NULL, timer)==-1){
			perror("Select is ravaged\n");
			return;
		}
		if(FD_ISSET(client_desc, socket_table)){
			gettimeofday(end, NULL);
			*RTT=(end->tv_sec-startRTT->tv_sec)*1000000+(end->tv_usec-startRTT->tv_usec);
			*received_size = recvfrom(client_desc, buffer, 9, 0, (struct sockaddr*) &addr, alen);
			if (memcmp(buffer, "ACK", 3) == 0) {
				*received+=1;
				//flight--;
				// REMOVE SEQUENCE NUMBER AND BYTES FROM DATA
				if(atoi(buffer+3)>*maxACK){
					*maxACK=atoi(buffer+3);
				}
				for (int i=0;i<ARRSIZE;i++) {
					if (acks[i] == -1){
						break;
					}
					if (*maxACK >= acks[i]) {
						removeInt(acks);
						removeCharArray(data);
						i--;
					}
				}
			}
		}
		gettimeofday(end, NULL);
		*time = (end->tv_sec-start->tv_sec)*1000000+(end->tv_usec-start->tv_usec);
		timer->tv_usec = *SRTT - *time;
		if(timer->tv_usec<0){
			timer->tv_usec=0;
		}
	}
	if (*mode==0) { // SLOW START CWND INCREMENTATION
		*cwnd+=*received;
	} else if (*mode==1) { // CONGESTION AVOIDANCE CWND INCREMENTATION
		*cwnd+=1;
	}
	if (*cwnd>ARRSIZE-1) { // CORE DUMPED NOT COOL
		*cwnd = ARRSIZE-1;
	}
	//printf("received_size : %ls\nreceived : %ls\ntime : %ls\n", received_size, received, time);
	*SRTT = ALPHA * (*SRTT) + (1 - ALPHA) * (*RTT);
	//printf("SRTT = %d\n", *SRTT);
	free(received_size);
	free(received);
	free(time);
	free(maxACK);
	free(socket_table);
	free(alen);
	return;
	//transmit(client_desc, message_size, sequence, RTT, SRTT, mode, cwnd, data, acks, file, addr, buffer, message);
}

int main (int argc, char *argv[]) {
	char** data = (char**)calloc(ARRSIZE, sizeof(char*));
	int* acks = (int*)malloc(ARRSIZE*sizeof(int));
	for (int i=0;i<ARRSIZE;i++) {
		data[i] = (char*)calloc(RCVSIZE, sizeof(char));
		acks[i] = -1;
	}
	int* message_size = (int*)malloc(sizeof(int));
	*message_size = RCVSIZE;
	int* sequence = (int*)malloc(sizeof(int));
	*sequence = 1;
	int* RTT = (int*)malloc(sizeof(int));
	*RTT = 12000;
	int* SRTT = (int*)malloc(sizeof(int));
	*SRTT = 12000;
	int* mode = (int*)calloc(1, sizeof(int)); // mode = 0 <=> SLOW START ; mode = 1 <=> CONGESTION AVOIDANCE
	int* cwnd = (int*)malloc(sizeof(int));
	int* flight = (int*)calloc(1, sizeof(int));
	int* thresh = (int*)calloc(1, sizeof(int));
	*cwnd = 1;
	// rwnd
	// int flight = 0;
	// int ssthresh = 32;
	struct timeval* startRTT = (struct timeval*)malloc(sizeof(struct timeval));
	struct sockaddr_in adresse_udp, client_udp;
	int port=0;
	if(argc==2){
		port= atoi(argv[1]);
	}
	int valid= 1;
	int test;
	socklen_t alen_udp = sizeof(client_udp);
	char* buffer = (char*)calloc(RCVSIZE, RCVSIZE*sizeof(char));
	char* message = (char*)calloc(RCVSIZE, RCVSIZE*sizeof(char));
	struct timeval startUpload, endUpload;
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
		if (select(ctrl_desc+data_desc+1, &sockTab,NULL, NULL, NULL) == -1) {
			perror("select is ravaged\n");
			return -1;
		}
		if (FD_ISSET(data_desc, &sockTab)) {
			int received_size;
			int eof = 0;
			struct sockaddr_in addr;
			FILE* file = NULL;
			socklen_t alen = sizeof(addr);
			received_size = recvfrom(data_desc, buffer, RCVSIZE, 0, (struct sockaddr*) &addr, &alen);
			buffer[received_size-1]='\0';
			file = fopen(buffer, "r");
			gettimeofday(&startUpload, NULL);
			while (eof == 0) {
				eof = transmit(data_desc, message_size, sequence, RTT, SRTT, mode, cwnd, data, acks, file, addr, startRTT, buffer, message);
				expect(data_desc, message_size, sequence, RTT, SRTT, mode, cwnd, data, acks, file, addr, startRTT, buffer, message);
			}
			free(message_size);
			free(sequence);
			free(RTT);
			free(SRTT);
			free(mode);
			free(cwnd);
			free(buffer);
			free(message);
			free(acks);
			for (int i=0;i<ARRSIZE;i++) {
				free(data[i]);
			}
			free(data);
			free(startRTT);
			gettimeofday(&endUpload, NULL);
			printf("Time : %ld\n", (endUpload.tv_sec-startUpload.tv_sec)*1000000+(endUpload.tv_usec-startUpload.tv_usec));
			break;
		}
		if(FD_ISSET(ctrl_desc, &sockTab)){
			printf("Control msg received \n");
		}
 
	}
close(ctrl_desc);
return 0;
}