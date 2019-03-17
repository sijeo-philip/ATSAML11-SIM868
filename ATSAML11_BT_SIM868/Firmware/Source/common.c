#include "common.h"
#include "string.h"

uint8_t convert_string_to_integer(char* number)
{
	uint8_t dec = 0, i;
	uint32_t len = 0;
	len = strlen(number);
	for(i=0; i<len; i++)
	dec = dec * 10 + (number[i]-'0');
	
	return dec;
}


// Function to convert integer to string
void convert_integer_to_string(char* str, uint16_t number)
{
	uint16_t n, len=0, i, rem;
	n = number;
	
	while(n!=0)
	{
		 len++;
		 n /=10;
	}
	
	for(i=0;i<len; i++)
	{
		rem = number % 10;
		number = number /10;
		str[len-(i+1)] = rem + '0';
	}
	str[len] ='\0';
	
}

//Copy string from start marker to end marker.
char* strcpymarker(char* src, char* dest, char startMarker, char endMarker)
{
	char *s, *d;
	uint16_t bytes=0;
	while(*src != startMarker)
	{	
		src++;
		if(*src == '\0')
			return 0;
	}
	s = src;
	src++;
	s++;
	while(*src != endMarker)
	{
  	src++;
		if(*src == '\0')
			return 0;
	}
	d = src;
	while(s!=d)
	{
	 	*dest = *s;
		dest++;
		s++;
		bytes++;
	}
	
		dest++;
		*dest = '\0';
	
	return d;
} // End of function used to parse the strings.
