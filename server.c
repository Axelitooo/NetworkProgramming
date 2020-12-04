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

#define RCVSIZE 1024

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

int sendingFile(int client_desc){

  struct sockaddr_in adresse_client;
  socklen_t alen = sizeof(adresse_client);
  int msgSize;
  char buffer[RCVSIZE];
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
      timer.tv_usec=7000; // Mettre un RTO glissant selon la valeur du RTT precedant

      if(select(client_desc+1, &sockTab,NULL, NULL, &timer)==-1){
        perror("select is ravaged\n");
        return -1;
      }
      if(FD_ISSET(client_desc, &sockTab)){
        // Receiving paquet
      gettimeofday(&end, NULL);
      printf("rtt = %ld\n", (end.tv_sec - current.tv_sec)*1000000 + (end.tv_usec - current.tv_usec));
      memset(buffer, 0, RCVSIZE);
      msgSize = recvfrom(client_desc,buffer,9, 0, (struct sockaddr*) &adresse_client, &alen);
      
      printf("The message received is : %s\n",buffer);      
      memset(buffer, 0, RCVSIZE);
      memset(stringSeq, 0, RCVSIZE);
      numSeq++;
      byteToSend= fread(buffer, 1, RCVSIZE, file);
      }else{
        gettimeofday(&end, NULL);
        printf("retransmitting at %ld\n", (end.tv_sec - current.tv_sec)*1000000 + (end.tv_usec - current.tv_usec));
      }

    }
    gettimeofday(&end, NULL);
    printf("total time = %ld\n", (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec) );
    printf("The number of block read is %d\n", numSeq);
    printf("Total size of message is %d\n", size);

    sendto(client_desc, "FIN", 3, 0, (struct sockaddr*) &adresse_client, alen);

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
