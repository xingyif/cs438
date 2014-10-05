#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>


//header file with function & struct declarations?? or just put them at top of file
struct frame {
int length;
int file_posn;
char Data[1016]; //or make char * data , & ���
char CRC_12[2]; 

struct frame * prev;
};


int enqueue (struct frame * new_frame);
struct frame * dequeue (void);


//GLOBAL Vars for queue??
struct frame * front = NULL;   // index of front of queue
struct frame * back  = NULL;   // index of back of queue
int size= 0;                                            // # frames in queue


// Queue implemented as a linked list

//ENQUEUE
//1 == success, 0 == failure
int enqueue (struct frame * new_frame) {

        if (new_frame == NULL) return 0;






		//if ( size == 1 ) fork(sleep) child write thread
		
		
		
		
		
        //empty queue , add 1st frame
        if (size == 0 ) {
                front = new_frame;
                back  = new_frame;
                new_frame->prev = NULL;
                return 1;
                }

        back->prev = new_frame;
        back  = new_frame;
        new_frame->prev = NULL;
        size++;

        return 1;
}

//DEQUEUE
//returns pointer to front of queue , NULL if empty
struct frame * dequeue (void){

        if (front == NULL || size == 0) return NULL;
                struct frame * temp = front;

                front = front->prev;
                size--;

                //dequeued last frame
                if(size == 0) back = NULL;
        return temp;
}


int main ( int argc, char *argv[] )
{
// mp1client <server> <port #> <output file>

        char server[40];
        char port_char[40];
        int port;
        char output_file[40];
        
        //argv[0] == 'mp1client'
        strcpy(server , argv[1]);
        strcpy(port_char , argv[2]);
        strcpy( output_file , argv[3]);
        
        //converts string to int
        port = atoi(port_char);
             
               
// 1
//sockets stuff gets FD

// setting up the struct 
struct addrinfo hints, *res, *p;
int status;
char ipstr[INET6_ADDRSTRLEN];
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;


////WTFFFF
if ( (status = getaddrinfo ( argv[1] , NULL, &hints, &res)) != 0)
{
        fprintf(stderr, "getaddrinfo: %s \n", gai_strerror(status));
        return 2;
}
//cycle through the ���res��� list to find a valid entries
   for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        }
int fd;
fd= socket (res->ai_family, res->ai_socktype, res->ai_protocol);


	//open file for write
	FILE * ret = fopen( output_file , "w" );
	
	//do while loop, run while mp1_read is returning frames
	do {

		char * buf = (char *) malloc(1024);
		
		if (buffer==NULL) 
			{
			exit (1);
			printf("well fuck");
			}
			
			int numbytes = 0 ;  //, tempbytes = 0;
			// 2
			//call
				while (numbytes <1024){
					numbytes = MP1_read (fd, buf, 1024);
				//	numbytes = tempbytes + numbytes;
					}


			//with FD
			//read until error









			///TO DO WHILE LOOP until mp1_read is done.


			///if crc passes check -> enqueue
			//if fail -> continue
			//pass *buf to crc check function

			//break up buf into struct frame

			//child process should sleep while queue is being populated
			//blank frame signals write to stop and terminate child process





			//temp variables
			struct frame * temp = (struct frame * ) malloc(sizeof(struct frame) );

			char Length[2];
			char File_Posn[4];


			int i = 0; 
			int j = 0;

			//manual string copy, begin...

			//Length
					  for (i ; i < 2 ; i++)
						       Length[i] = buf[i];     

					  //bits to ints?
					  temp->length = * (int*) Length;
					  //orrrrr
					  //mp1client.c:163: warning: cast from pointer to integer of different size
					  //temp->length = (int) Length;

					  if ( i == 2) printf("you're doing something right \n");

					  //temp variable with room for null
					  char Data[temp->length+1];

			//file_posn
					  for (i,j=0 ; i < 6 ; i++, j++)
						       File_Posn[j] = buf[i];  

						       //convert file posn to int (see above experiments)
						       temp->file_posn = * (int*) File_Posn; 
			//Data
					  for (i, j=0 ; i < 6+temp->length ; i++, j++)
						       temp->Data[j] = buf[i];  

					  //NULL terminate: use i (incremented last time around for loop, i++, or ++i     
					  temp->Data[i] = '\0'; 
					  
					  
			//enque frame temp
			enqueue (temp);


			//CRC   
			//1016 + 4 + 2 = 1022   
			//map into bits
			//discard first 4 bits
					  for (i= 1022 , j=0; i < 1024 ; i++, j++)
						       temp->CRC_12[j] = buf[i];            
			//crcChecker returns 0 if check failed, 1 if check passed    
			// int check = crcChecker(buf, temp->CRC_12);




			//4 
			//CRC check


	free(buf);
	}while (numbytes) ;

	//5 
		// Write frame
		// if EOF, send empty frame
struct frame * write_frame;


	while (size != 0){
		write_frame = dequeue();	
		
		if(fseek(ret, write_frame->file_posn , SEEK_SET) != 0)
			printf("seek failed");

		if( MP1_fwrite (write_frame->Data, sizeof(char), write_frame->length , ret) != write_frame->length )
			printf("fwrite failed");
			
		free(write_frame);

	}








}

