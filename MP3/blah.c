
	  	buffsize = recvfrom(sock, buff, 100, 0,
				   (struct sockaddr *) &saddr, &slen);
	  	if(buffsize == -1)
	  	{
	  	//	printf("error recieving from server\n");
		  	printf( "Recvfrom error: %s\n", strerror( errno ) );
	  	}	
	  	//printf("buff is: %s\n" , buff);
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
		}


	cwnd = 2

	while(1)
  	{
		//Check one cwnd
		for(int i = 0; i < cwnd; i++)
		{
		
			buffsize = recvfrom(sock, buff, 100, 0,(struct sockaddr *) &saddr, &slen);
			if(buffsize == -1)
				printf( "Recvfrom error: %s\n", strerror( errno ) );


			//check new packet
			arq	narq;
			memcpy((void *)&narq, (void *)buff, sizeof(arq));
			newseqnum = ntohl(narq.seqnum);
			int newcwnd = ntohl(narq.cwnd);
			printf("for newseqnum: %u\n" , newseqnum);
								
			datalen = buffsize - sizeof(arq);
			int total =	newseqnum + datalen;
 		 	printf("total: %d\n" , total);
		
		
			//Check the cwnd for each packet received for timeouts
			if(newcwnd != cwnd)
			{
				i = 0;
				cwnd = newcwnd;
				printf("\n\n\n\nNEW CWND: %d\n" , cwnd);
			}

			//correct next sequence number
			if(newseqnum == sendacknum )
			{
				memcpy((void *) &BUFFER, (void *) &datalen, sizeof(unsigned int));
				memcpy((void *) &BUFFER[sizeof(unsigned int)], (void *) &buff[sizeof(arq)], datalen);
				flushbuffer(BUFFER);
				sendacknum += datalen;		
			}
			
			//received file
			else if(total == flen)
			{
			
				memcpy((void *) &BUFFER, (void *) &datalen, sizeof(unsigned int));
				memcpy((void *) &BUFFER[sizeof(unsigned int)], (void *) &buff[sizeof(arq)], datalen);
				flushbuffer(BUFFER);
				for (int i = 0; i < 3; i++)
				{
					printf("Second Final ACK! \n");
					sendack(0);
				}
				sleep(10);
				fclose(fp);
				return 0;
			
			}
		
			//packet not in order
			else
			{
				//send dupack
				for (int i = 0; i < 3; i++)
				{
					printf(" Elese Send DUPACK! \n");
					sendack(sendacknum);
				}
			}
		
	
		}
		
		//ack for CWND
		for (int i = 0; i < 3; i++)
		{
			printf(" Elese Send SACK! \n");
				sendack(sendacknum);
		}
	}

	
