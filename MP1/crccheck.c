#define POLYNOMIAL 0x180D  /* 1100000001101 generator polyomial */
#include <stdint.h>
#include <stdio.h>
//#define GEN 1100000001101 C(x) = x^12 + x^11 + x^3 + x^2 + 1



int crcChecker(char *buf, char check[2])
{
	int i, j, sizebit, e;
	int count=0;
	char g[13] = "1100000001101";
	char checksum[12];
	
	
	
	sizebit = 1022 * 8;
	char remainder[12];
	char message[sizebit];
	
	//map frame crc to 12 bits
	for(j = 0; j < 2; j++)
	{
		
		for( i=0; i < 8; i++)
		{			
			checksum[count] = check[j] >> i & 1;
			++count;		
		}
				
	}
	
	//map each bit in buf to fill a slot in char array
	for(j = 0; j < sizebit; j++)
	{
		unsigned char value = *((unsigned char*)buf);			 //dereference buf pointer
		
		for( i=0; i < 8; i++)
		{			
			message[count] = value >> i & 1;
			++count;		
		}
		//ALT: buf[j]
		buf = (unsigned char*)buf++; //increment buf pointer
	}
	
	//append 12 zeros at end of data
	for(int k = count; k<count+12; k++)
	{
		message[k] = '0';
	}
	
	//take first 13 bits at a time and xor them with generator polynomial
	for(e=0;e<13;e++)
	remainder[e]=message[e];

	do{
	//only run the xor function once the MSB of the remainder is a 1
	//if not, it adds the next bit of the message and runs again with the next bit of the remainder
	if(remainder[0]=='1')
	{	//xor function
		for(c = 1;c < 13; c++)
		remainder[c] = (( remainder[c] == g[c])?'0':'1'); 
	}	


	for(c=0;c<12;c++)
	{
		remainder[c]=remainder[c+1];
		remainder[c]=message[e++];
	}
	
	}while(e<=sizebit+12);
	
	for(int l = 0; l < 12; l++)
	{
		if(remainder[l] != checksum[l])
			return 0;
	}
	return 1;  
	
}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

