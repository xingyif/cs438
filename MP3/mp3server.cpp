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
#include <sys/stat.h>
#include <sys/time.h>

#include "libcommon.h"
#include "mp3.hh"



int packet_size = 100;
int expects;

static struct sockaddr_storage their_addr;
static unsigned int addr_len = sizeof(their_addr);

int sock = -1;
FILE *fp = NULL;
int filelen = 0;



int transmit(void *buff, int len, int sendseq, int cwnd , int thresh, int phase );
void slowStart(int cwnd, int threshold, int sendseqnum);
void sendFile();
int packetsAcked(int cwnd, int sendseq, int phase , int thresh);
void linearGrowth(int cwnd, int threshold, int sendseqnum);



int transmit(void *buff, int len, int sendseq, int cwnd , int thresh, int phase ) 
{
	int ret;
	struct timeval tv1;
	gettimeofday(&tv1, NULL);	
	int time = tv1.tv_sec;
	int time1 = tv1.tv_usec;
	
	
	int length = len - sizeof(arq);
	int endseq = sendseq + length;
	char p[21];
	switch (phase)
	{
		case 0:
			strcpy(p , "     slow start     ");
			break;
			
		case 1:
			strcpy(p, "congestion avoidance");
			break;
			
		case 2: 
			strcpy(p, "  fast  retransmit  ");  
			break;
	}
	
	p[20] = '\0';
	
	printf("Time: %d.%d  || %d : %d (%d) || %s || %d || %d \n" , time , time1 , sendseq, endseq , length,  p , cwnd , thresh);
	if (-1 == (ret = mp3_sendto(sock, (void *) buff, len, 0, 
				  (struct sockaddr *) &their_addr, addr_len)) ) 
	{
		printf("error sending packet %d!\n" , sendseq);
		return -1;
		
	} else if (ret != len) {
	printf("sendto didn't send everything!\n");
	return -1;
	}

}

/* transmit the given data over the given socket */
/* IN UDP
   client is assumed to have connected b4 calling this
   server is assumed to have received b4 calling this
*/
int transmitinit(void *buff, int len) {

  int ret, retry = 10;
  char buffer[100];
  memcpy(buffer , buff , 100);
  //Timeout stuff && select stuff
	struct timeval tv , tv1;
	fd_set readfds;

	tv.tv_sec = 2;
	tv.tv_usec = 500000;

	tv1.tv_sec = 0;
	tv1.tv_usec = 0;
	settimeofday(&tv1, NULL);
	printf("Time  || Send || phase || cwnd || ssthresh \n");
	do 
	{
		printf("sending init packet 0 \n");
		if (-1 == (ret = transmit(buffer, len, 0, 1 ,65536, 0 )))
		{
			printf("initial transmit failed\n" );
			break;
		}

		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		select(sock+1, &readfds, NULL, NULL, &tv);

		if (FD_ISSET(sock, &readfds))
		{
			ret = recvfrom(sock, buff, MAXBUFFER, 0,(struct sockaddr *) &their_addr, &addr_len);

			if (-1 == ret) {
			  printf("error receiving init ACK!\n");
			  break;
			} 
			else 
			{
			  printf("init packet succcessful\n");
			  break; /* success, dont retry */
			}
		}
	} while (--retry);
  
  if (!retry && ret == -2) {
    printf("abandoning init packet, it failed too many times\n");
    ret = -1;
  }
  return ret;
}




void sendFile()
{
	char buff[MAXBUFFER];
	char *buffer = buff;
	int buffsize;
	int	done = 0;
	int sendseqnum;
	
	//send init packet
	initarq init;
	init.seqnum = htonl(0);
	init.flen = htonl(filelen);
	init.cwnd = htonl(1);

	
    

    /* fill the buffer for the init packet */
    memcpy((void *) buffer, (void *) &init, sizeof(initarq));
    int initdata = packet_size -  sizeof(initarq);
    
    /* fill any empty space w/ file data */
    if (initdata > 0) 
    {
		buffsize = fread((void *) &buffer[sizeof(initarq)], 1, initdata, fp);
		if (ferror(fp))
			printf("error reading from file!\n");
	  	else if (feof(fp)) 
	  		done = 1;
      
    }
    
    /* transmits the init packet and waits for an ACK */
    if (-1 == transmitinit((void *) buffer, buffsize+ sizeof(initarq))) 
    	printf("transmit fail\n");
    /* next seqnum should be in buffer now */
    ack sack;
    memcpy((void *) &sack, (void *) buffer, sizeof(ack));
    expects = ntohl(sack.expects);
    //printf("initdata: %d\n" , initdata);
    //printf("ACK for init, expects: %u\n", expects);
    
    
    //Check for ack vs. dupack
    if( expects == initdata )
    {
		sendseqnum = initdata;
		printf("slow start begins! seqnum: %d \n" , sendseqnum);
		slowStart(2 , 65536, sendseqnum);

    	//Recieve ack -- implement as a function
    	//Increment cwnd by 2 untit cwnd threshold 
    	//Linear growth --implement as function
    	//Calculate RTO
    	//On timeout cwnd = 1
    	// If dupacks > 3 fast retransmint => cwnd = cwnd/2 && threshold = cwnd / 2
    	//Retransmit from required file pointer
    	//Continue linear growth
    	//Maintain the last ack-ed byte
  
	}

	


}
//return  values
//-1 error
//0 on success ACK
//1 DUPACK
//2 timeout
//3 done with file
int packetsAcked(int cwnd, int sendseq, int phase , int thresh)
{

	
	char p[21];
	switch (phase)
	{
		case 0:
			strcpy(p , "     slow start     ");
			break;
			
		case 1:
			strcpy(p, "congestion avoidance");
			break;
			
		case 2: 
			strcpy(p, "  fast  retransmit  ");  
			break;
	}
	
	p[20] = '\0';
	
	

	ack sack;
	char BUFF[sizeof(ack)];
	char * buff = BUFF;
	int buffsize;
	int datalen = packet_size -  sizeof(arq);
	

	//Timeout stuff && select stuff
	struct timeval tv;
	fd_set readfds;
	tv.tv_sec = (1);
	tv.tv_usec = 500000;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	//Wait for ack
	select(sock+1, &readfds, NULL, NULL, &tv);
		
	if (FD_ISSET(sock, &readfds))
	{
		buffsize = recvfrom(sock, buff, sizeof(ack), 0, (struct sockaddr *) &their_addr, &addr_len);
		if ( buffsize == -1)
			return -1;

		memcpy((void *) &sack, (void *) buff, sizeof(ack));
		int tempExpects = ntohl(sack.expects);
		//printf("expects: %d\n" , tempExpects);
	//	printf("sendseqnum + datalen : %d\n", (sendseq + datalen));
		
		
		struct timeval tv1;
		gettimeofday(&tv1, NULL);	
		int time = tv1.tv_sec;
		int time1 = tv1.tv_usec;
		printf("\n\nACK\nTime:   || ACK # || phase || cwnd || ssthresh\n");
		printf("Time: %d.%d || ACK %d || %s || %d || %d\n" , time,time1,  tempExpects, p , cwnd , thresh);
			
			
		//client received file
		if(tempExpects == 0) return 3;
		

		
		if(sendseq == tempExpects)
		{
			printf("ACK # %d is CORRECT \n\n\n", sendseq);
			//Recieve the remaining acks if any
			for(int i = 0; i < 2; i++)
			{
				FD_ZERO(&readfds);
				FD_SET(sock, &readfds);
				select(sock+1, &readfds, NULL, NULL, &tv);
				if (FD_ISSET(sock, &readfds))
				{
					buffsize = recvfrom(sock, buff, sizeof(ack), 0,(struct sockaddr *) &their_addr, &addr_len);
					continue;
				}
			}

			return 0;
		}
		else
		{
			printf("ACK # %d is DUPACK\n\n\n", tempExpects);
			expects = tempExpects;
			//Recieve the remaining dupacks if any
			for(int i = 0; i < 2; i++)
			{
				FD_ZERO(&readfds);
				FD_SET(sock, &readfds);
				select(sock+1, &readfds, NULL, NULL, &tv);
				if (FD_ISSET(sock, &readfds))
				{
					buffsize = recvfrom(sock, buff, sizeof(ack), 0,(struct sockaddr *) &their_addr, &addr_len);
					continue;
				}
			}
			
			return 1;
		}
			        
	}
	//Timeout 
	else
	{
		printf("\n\nACK\nTime:   || ACK # || phase || cwnd || ssthresh\n");
		printf("ACK # %d not received: TIMEOUT\n\n\n" , sendseq);
		return 2;
	}

}

//Set cwnd = 1 and 
	//Start sending shit
	//Increment cwnd * 2 if acks are in order
	//If dupack cwnd = cwnd/2 and threshold = cwnd/2 => initiate fast retransmit and go to linear growth
	//If timeout cwnd = 1 and threshold = cwnd/2 => call slow start again
void slowStart(int cwnd, int threshold, int sendseqnum)
{

	int i , done = 0;
	
	char buffer[100]; 
	arq send;
	int thresh = threshold;
	int buffsize;
	
	
	while(cwnd < thresh)
	{
		for(i = 0 ; i < cwnd ; i++) 
		{
			
			//populate header
			send.seqnum = htonl(sendseqnum);
			send.cwnd = htonl(cwnd);
			memcpy((void *) buffer, (void *) &send, sizeof(arq));

			int datalen = packet_size -  sizeof(arq);
	
			//populate packet with data
			fseek ( fp , sendseqnum , SEEK_SET );
			buffsize = fread((void *) &buffer[sizeof(arq)], 1, datalen, fp);
			if (ferror(fp))
				printf("error reading from file!\n");
			
			datalen = buffsize - sizeof(arq);
			//printf("FOR slowstart: transmitting seqnum: %d	\n" , sendseqnum);
			int ret = transmit(&buffer,  (sizeof(arq) + buffsize), sendseqnum , cwnd, thresh , 0);
			if(ret == -1)
				printf("Packet not sent! We're fucked \n");
		  				
			
			//send packet
			sendseqnum += buffsize;
			if(sendseqnum ==filelen) 
			{
			//printf("premature FOR break!\n");			
			break;
			}
		}
		int ret = packetsAcked (cwnd, sendseqnum, 0 , thresh);
		//printf("slow start ret=%d\n" , ret);
		
		switch ( ret )
			{ 
				// Error
				case -1: 
					printf("Packet not acked! Slow start \n");
					break;
				//Normal ack
				case 0:
					cwnd = cwnd * 2;
					//Check for final packet being acked
					if (feof(fp)) 
		  				done = 1;	
					break;
				//Dupack			
				case 1:
					//This causes the while condition to fail, triggering linearGrowth/fastRetransmit
					if (cwnd%2 != 0) cwnd++;
					cwnd = cwnd / 2;
					thresh = cwnd;
					sendseqnum = expects;
					break;
				//Timeout
				case 2:
					if (cwnd%2 != 0) cwnd++;
					thresh = (cwnd / 2);
					cwnd = 1;
					break;
				//Client received file
				case 3:
					done = 1;
					printf("Client received file!\n");
					break;
			}
		
			//Dupack , go to linear growth with new cwnd & threshold
			if ( ret == 1 )
				break;
				
		if (done) return; 
	}
	linearGrowth(cwnd, thresh, sendseqnum);
	
}

void linearGrowth(int cwnd, int threshold, int sendseqnum)
{
	//set cwnd as whatever it is from slowStart phase
	//send the shit
	//on dupack =>initiate fast retransmit
	//if timeout => slowstart
	int i ,done = 0;
	char buffer[100]; 
	arq send;
	int thresh = threshold;
	int windowseqnum;
	int buffsize;
	int FFflag = 0; //fast retransmit flag
	
	//printf(" Inside Linear's ass\n"); 
	//printf(" sendseqnum = %d \n", sendseqnum);
	
	while(1)
	{
		windowseqnum = sendseqnum;
		for(i = 0 ; (i < cwnd) ; i++) 
		{
			//populate header
			send.seqnum = htonl(sendseqnum);
			send.cwnd = htonl(cwnd);
			memcpy((void *) buffer, (void *) &send, sizeof(arq));

			int datalen = packet_size -  sizeof(arq);
			
			//populate packet with data
			fseek ( fp , sendseqnum , SEEK_SET );
			buffsize = fread((void *) &buffer[sizeof(arq)], 1, datalen, fp);
			if (ferror(fp))
				printf("error reading from file!\n");
			
	
			int ret = transmit(&buffer,  (sizeof(arq) + buffsize), sendseqnum, cwnd , thresh , 1+FFflag);
			if(ret == -1)
				printf("Packet not sent! We're fucked \n");
		  				
		
			//send packet
			sendseqnum += buffsize;
			if(sendseqnum ==filelen) break;
			

		}
		
		
		
		int ret = packetsAcked (cwnd, sendseqnum, 1+FFflag , thresh);
		FFflag = 0;
		
		switch ( ret )
		{ 
				// Error
				case -1: 
					printf("Packet not acked! Slow start \n");
					break;
				//Normal ack
				case 0:
					cwnd = cwnd + 1;
					//Check for final packet being acked
					if (feof(fp)) 
		  				done = 1;	
					break;
				//Dupack			
				case 1:
					//This causes the while condition to fail, triggering linearGrowth/fastRetransmit
					if (cwnd%2 != 0) cwnd++;
					cwnd = cwnd / 2;
					thresh = cwnd;
					sendseqnum = expects;
					FFflag = 1;
					printf("\n\n\n\n\n\nFUCK MEEEEEEE\n\n\n\n\n\n");
					break;
				//Timeout
				case 2:
					if (cwnd%2 != 0) cwnd++;
					thresh = (cwnd / 2);
					cwnd = 1;
					break;
				//Client received file
				case 3:
					done = 1;
					printf("Client received file!\n");
					break;					
		}
	
		//Timeout , go to slow start again with new cwnd & threshold
		if ( ret == 2 )
			break;
			
		if (done) return; 
	}
	
	
	//probe client for what it expects so we don't default back to linear growth 
	//when slow start receives a dupack
	
	//populate header
	send.seqnum = htonl(65536);
	send.cwnd = htonl(1);
	memcpy((void *) buffer, (void *) &send, sizeof(arq));
	int datalen = packet_size -  sizeof(arq);

	datalen = buffsize - sizeof(arq);
	//printf("FOR slowstart: transmitting seqnum: %d	\n" , sendseqnum);
	printf("\n\nignore the next transmit & ack!! used to get expected sendseq # so slow start doesn't default to linear growth\n\n");
	int ret = transmit(&buffer,  (sizeof(arq) + 92), sendseqnum , cwnd, thresh , 1);
	ret = packetsAcked (cwnd, sendseqnum, 1 , thresh);
	slowStart(cwnd, thresh, expects );
}
	




int main(int argc, char* argv[]) {


	unsigned int seqnum, newseqnum, ackseqnum = 0, addr_len = sizeof(their_addr);
	unsigned int sendacknum, eofacknum = UINT_MAX;
	unsigned int flen = 0, datalen;
	int buffsize, buffptr = 0;
	ack sack;
	char buffer[100];
	char* buff = buffer;
	
	
	//Timeout stuff && select stuff
	struct timeval tv;
	fd_set readfds;

	tv.tv_sec = 2;
	tv.tv_usec = 500000;

	FD_ZERO(&readfds);
	
	mp3_init();	
	
	if (argc != 3) {
	printf("Usage: mp3server <server_port_number> ");
	printf("<input_file_name>\n");
	return 0;
	}
	printf("port: %s\n", argv[1]);
	printf("input file name: %s\n", argv[2]);
  
	if (NULL == (fp = fopen(argv[2], "r"))) 
    {
		printf("error opening file");
		return 0;
	}	
	
	
	//get file length
	struct stat st;
    if (-1 == stat(argv[2], &st)) {
		printf("error stating file!\n");
		return 0;
      }	
	filelen = st.st_size;
   printf("file size: %u\n", filelen);


	//This is the server listening on its port for incoming connections
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];
	int rv, numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1] , &hints, &servinfo)) != 0) {
		    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		    if ((sock = socket(p->ai_family, p->ai_socktype,
		                    p->ai_protocol)) == -1) {
		            perror("listener: socket");
		            continue;
		    }

		    if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
		            close(sock);
		            perror("listener: bind");
		            continue;
		    }

		    break;
	}

	if (p == NULL) {
		    fprintf(stderr, "listener: failed to bind socket\n");
		    
	}

   freeaddrinfo(servinfo);


	printf("Listening for connections...\n");

	
	while(1)
	{
		//listen for client connection
		struct timeval tv;
		fd_set readfds;

		tv.tv_sec = 2;
		tv.tv_usec = 500000;

		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		select(sock+1, &readfds, NULL, NULL, &tv);

		if (FD_ISSET(sock, &readfds))
		{
			//printf("Connection sorta accepted\n");
			buffsize = recvfrom(sock, buff, 8, 0, (struct sockaddr *) &their_addr, &addr_len);
			//printf("Buff : %d \n", buffsize);
			//printf( "Recvfrom error: %s\n", strerror( errno ) );
			
			if (buffsize != -1)
			{
				//Create new thread and start sending shit
				printf("Connection Accepted\n");
				printf("buff is : %s \n" , buff);
				break;
			}
			sleep(6);
	
		}
		else
			printf("No connection received\n");	    
	}
	
	sendFile();
}

