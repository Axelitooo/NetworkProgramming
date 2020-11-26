#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>

#define RCVSIZE 1024

int serverHandShake(int server_desc_udp, int port){
    char Tx[RCVSIZE]="SYN-ACK";
    char buffer[10];
    sprintf(buffer, "%d", port);
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


    recvfrom(server_desc_udp, Rx, RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
    printf("Message received\n");
    if(strcmp(Rx,"SYN")==0){
        printf("Sending SYN-ACK\n");
            sendto(server_desc_udp,Tx, strlen(Tx), 0, (struct sockaddr*) &adresse_client, alen);
        memset(Rx,0, RCVSIZE);
        recvfrom(server_desc_udp, Rx, RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
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


int gestionClientUDP(int client_desc_udp, char buffer[RCVSIZE]){

  struct sockaddr_in adresse_client;
  socklen_t alen = sizeof(adresse_client);
  int msgSize = recvfrom(client_desc_udp,buffer,RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);

  while (msgSize > 0) {
    printf("BUFFER TO SEND: %s",buffer);
    sendto(client_desc_udp,buffer,msgSize, 0, (struct sockaddr*) &adresse_client, alen);
    memset(buffer,0,RCVSIZE);
    msgSize = recvfrom(client_desc_udp,buffer,RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
  }
  close(client_desc_udp);
  //printf("Closing the UDP child\n");
  exit(0);
  
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

int sendingFile(int client_desc){

  struct sockaddr_in adresse_client;
  socklen_t alen = sizeof(adresse_client);
  int msgSize;
  char buffer[RCVSIZE];
  FILE* file = NULL;
  int numSeq;

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
    numSeq=0;
    int byteToSend = 1;
    char stringSeq[RCVSIZE];
    int size=0;
    while (byteToSend > 0) 
    {
      byteToSend= fread(buffer, 1, RCVSIZE, file);
      generateSequenceNumber(stringSeq, numSeq);
      printf("numSeq = %s\n", stringSeq);
      memcpy(stringSeq+6, buffer, byteToSend);
      //printf("Data to send: %s \n", stringSeq);
      sendto(client_desc, stringSeq, byteToSend+6, 0, (struct sockaddr*) &adresse_client, alen);
      fwrite(stringSeq+6, byteToSend, 1, filecopy);
      memset(buffer, 0, RCVSIZE);
      memset(stringSeq, 0, RCVSIZE);
      //msgSize = recvfrom(client_desc,buffer,RCVSIZE, 0, (struct sockaddr*) &adresse_client, &alen);
      numSeq++;
    }
    printf("The number of block read is %d\n", numSeq);
    printf("Total size of message is %d\n", size);
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
  int server_desc_udp = socket(AF_INET, SOCK_DGRAM, 0);

  //Handle error
  if(server_desc_udp <0){
    perror("Cannot create udp socket\n");
    return -1;
  }

  fd_set sockTab;

  //SERVEUR UDP
  setsockopt(server_desc_udp, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  adresse_udp.sin_family= AF_INET;
  adresse_udp.sin_port= htons(port);
  adresse_udp.sin_addr.s_addr= htonl(INADDR_ANY);

  //initialize socket
  if (bind(server_desc_udp, (struct sockaddr*) &adresse_udp, sizeof(adresse_udp)) == -1) {
    perror("Bind failed\n");
    close(server_desc_udp);
    return -1;
  }

  int client_desc_udp = serverHandShake(server_desc_udp, port+1);

  
  while (1) {
    FD_ZERO(&sockTab);
    FD_SET(server_desc_udp, &sockTab);
    FD_SET(client_desc_udp, &sockTab);
    
    if(select(server_desc_udp+client_desc_udp+1, &sockTab,NULL, NULL, NULL)==-1){
      perror("select is ravaged\n");
      return -1;
    }

    if(FD_ISSET(client_desc_udp, &sockTab)){
      sendingFile(client_desc_udp);
    }

    if(FD_ISSET(server_desc_udp, &sockTab)){
      printf("Control msg received \n");
    }
 
  }
close(server_desc_udp);
return 0;
}
