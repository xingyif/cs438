#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <sys/wait.h>

#include <signal.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#include "libcommon.h"
#include "mp3.hh"
static char buffer[MAXBUFFER];
static char buff[MAXBUFFER];
static unsigned int windowsize;


struct sockaddr_in saddr; /* the server as the client sees them */
static unsigned int slen = sizeof(saddr);

int sock = -1; /* initialize with a static ptr */
FILE *fp = NULL;

extern int errno;

//TODO check for last packet receive length 


void flushbuffer(char *buf) {
	char *buffwrite = buf;
	unsigned int writelen;
	//printf("emptying buffer to file\n");

	memcpy((void *) &writelen, (void *) buffwrite, sizeof(unsigned int));
//	printf("writelen: %u \n" , writelen);
	
	/* write out all buffer data */
	int i = sizeof(unsigned int);
	int written = 0;
	
	while (written != writelen)
	{
		written += fwrite((void *) &buffwrite[i], 1, writelen-written, fp);
		if (written < writelen) 
		{
			printf("error writing to file!\n");
			i += written;
		}
		if(written == -1)
		{
			printf("ERROR WRITING FILE\n");
			return;
			
		}
	}
	//windowsize += writelen + sizeof(arq);
	/* increment buffwrite */

	/* reset buffer pointer to start of buffer */
	//buff = buffer;	    
}

/*
given the hostname of the server in dotted decimal notation and a port #
attempts to create a socket and connect to the server returning the socket
handler
*/
int sconnect(char *hostname, int port) {
    int socket;
    struct hostent *he;
    struct in_addr a;

    socket = createUDPsocket();

    /* setup server socket address */
    memset((void *) &saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);

    /* get 32-bit IP address from host name - already in network byte order */
    if (NULL != (he = gethostbyname(hostname))) {
      /* printf("name: %s\n", he->h_name); */
      /*
	while (*he->h_aliases)
	printf("alias: %s\n", *he->h_aliases++);
      */
      while (*he->h_addr_list) {
	memcpy((char *) &a, *he->h_addr_list++, sizeof(a));
	//printf("set address: %s\n", inet_ntoa(a));
      }
    } else {
      eprint("unable to resolve hostname!\n");
      sclose(socket);
      exit(-1);
    }
    saddr.sin_addr.s_addr = a.s_addr;

    return socket;
}

void sendack( int ackno)
{
	if(ackno == 0) printf("sending end of file acks!\n");
	
	printf("sendacknum = %d\n", ackno);
	
	ack sack;
	sack.expects = htonl(ackno);
	if( -1 == mp3_sendto(sock, (void *)&sack, sizeof(ack), 0, (struct sockaddr *)&saddr, slen))
		printf("error sending ACK \n");
}

int main(int argc, char* argv[]) {

	mp3_init();
	unsigned int seqnum, newseqnum, ackseqnum = 0, slen = sizeof(saddr);
	unsigned int sendacknum, eofacknum = UINT_MAX;
	unsigned int flen = 0, datalen, cwnd;
	int waiting = 1; /* flag for waiting for a new client or handling a client */
	int buffsize, buffptr = 0;
	ack sack;
	char *hostname;
	char buffer[8] = "Connect";
	char BUFF[100];
	char * buff = BUFF;
	char BUFFER[100];

	if (argc < 4) {
	printf("Usage: mp3client <server_name> ");
	printf("<server_port_number> <output_filename>\n");
	return 0;
	}
//	printf("port: %s\n", argv[2]);
//	printf("hostname: %s\n", argv[1]);
	
	windowsize = 1;
	seqnum = 0;
	
	/* connect to send */
  	sock = sconnect(argv[1], atoi(argv[2]));
  	if(NULL == (fp = fopen(argv[3], "w"))){
  	perror("error creating file");
  	}
  	
  	//setup select to wait for connection ack
  	struct timeval tv;
    fd_set readfds;

    tv.tv_sec = 2;
    tv.tv_usec = 500000;

	
  	//Tell mp3server we want to connect to it
  	//Try reconnect 10 times if ack not recieved
  	int retry = 1;
  	do {
	  	printf("retry is: %d\n" , retry);
	  	if( -1 == mp3_sendto(sock, (void *)&buffer, sizeof(buffer), 0, (struct sockaddr *)&saddr, slen))
			printf("error sending ACK \n");

		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		select(sock+1, &readfds, NULL, NULL, &tv);
		
		if (FD_ISSET(sock, &readfds))
			{
			printf("Ack for server connection!\n\n");
			break;	        
	      }
	     
	   
        	retry--;
		
  		}while(retry != 0);
  	
	//receive initial packet
  	buffsize = recvfrom(sock, buff, 100, 0,
			   (struct sockaddr *) &saddr, &slen);
  	if(buffsize == -1)
	  	printf( "Recvfrom error: %s\n", strerror( errno ) );

  	memcpy((void *) &newseqnum, (void *) buff, sizeof(unsigned int));
  	newseqnum = ntohl(newseqnum);
  	printf("\n\n\n\nwhile newseqnum: %u\n" , newseqnum);
  	
  	
  	//init packet
  	if(newseqnum == 0)
  	{
  		initarq iarq;
  		memcpy((void *) &iarq, (void *) buff, sizeof(initarq));
		flen = ntohl(iarq.flen);
		printf("file length: %u\n\n\n", flen);
		cwnd = ntohl(iarq.cwnd);
		//printf("congestion window size:%u\n",cwnd);
	
		ackseqnum = 0;
		seqnum = 0;

		datalen = buffsize - sizeof(initarq);
		//printf("main datalen: %u \n" , datalen);
		
		//make like regular packet to be flushed (initarq -> arq header) for datalen
		memcpy((void *) &BUFFER, (void *) &datalen, sizeof(unsigned int));
		memcpy((void *) &BUFFER[sizeof(unsigned int)], (void *) &buff[sizeof(initarq)], datalen);
		flushbuffer(BUFFER);
		sendacknum = datalen;
		sendack(sendacknum);
		
		if (datalen ==flen) 
			{
			fclose(fp);
			return 0;
			}
	}


	
	cwnd = 2;
	int dupFlag = 1;
	
	while(1)
  	{
		//Check one cwnd
		for(int i = 0; i < cwnd; i++)
		{
		
			if(cwnd == 7) printf(" I is : %d \n" , i );
			buffsize = recvfrom(sock, buff, 100, 0,(struct sockaddr *) &saddr, &slen);
			if(buffsize == -1)
				printf( "Recvfrom error: %s\n", strerror( errno ) );


			//check new packet
			arq	narq;
			memcpy((void *)&narq, (void *)buff, sizeof(arq));
			newseqnum = ntohl(narq.seqnum);
			int newcwnd = ntohl(narq.cwnd);

								
	
			//Check the cwnd for each packet received for timeouts or next cwnd has arrived
			if(newcwnd != cwnd)
			{
				i = 0;
				cwnd = newcwnd;
				printf("\n\n\n\nNEW CWND: %d\n" , cwnd);
			}
			
			datalen = buffsize - sizeof(arq);
			int total =	newseqnum + datalen;
 		 	printf("newseqnum + datalen: %d\n" , total);
			printf("for newseqnum: %u\n" , newseqnum);
			

			//received file
			if(total == flen)
			{
				
				memcpy((void *) &BUFFER, (void *) &datalen, sizeof(unsigned int));
				memcpy((void *) &BUFFER[sizeof(unsigned int)], (void *) &buff[sizeof(arq)], datalen);
				flushbuffer(BUFFER);
				for (int i = 0; i < 3; i++)
				{
					printf("sending Final ACK! \n");
					sendack(0);
				}
				sleep(5);
				fclose(fp);
				return 0;
			}
		
			//correct next sequence number
			if(newseqnum == sendacknum )
			{
				printf("success!\n");
				memcpy((void *) &BUFFER, (void *) &datalen, sizeof(unsigned int));
				memcpy((void *) &BUFFER[sizeof(unsigned int)], (void *) &buff[sizeof(arq)], datalen);
				flushbuffer(BUFFER);
				sendacknum += datalen;		
			}
			
			//packet not in order
			else
			{
				dupFlag = 0;
				printf("fail\n");
				//send dupack
				for (int i = 0; i < 3; i++)
				{
					printf(" Elese Send DUPACK! \n");
					sendack(sendacknum);
				}
			}
		
	
		}//end of for loop
		
		if(dupFlag)
		{
			//ack for CWND
			for (int i = 0; i < 3; i++)
			{
				printf(" Elese Send SACK! \n");
					sendack(sendacknum);
			}
			printf("end of for loop\n\n\n\n");
		}
		dupFlag =1;
	}

	
  	

 } 	
  	
  	

