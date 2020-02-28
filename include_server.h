#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>

#define BUFFER_SIZE			1024
#define BLOCK_SIZE    		512
#define DEFAULT_SIZE		30
#define ACK					4

char buf[BUFFER_SIZE], filename[DEFAULT_SIZE], download_mode[10], error_message[DEFAULT_SIZE], server_file[DEFAULT_SIZE], data[BLOCK_SIZE];
char* input;
int listener, port, ret, addrlen, i, len;
uint16_t opcode, error_number, block_number;
uint8_t endmess = 0;

void error_check(int val){
	if (val < 0){
        perror("Errore: \n");
        exit(1);
	}
}

int illegal_tftp_operation(){
	memset(buf, 0 ,BUFFER_SIZE);
	int len = 0;
	opcode = htons(5);
	memcpy(buf, &opcode, 2);
	len += 2;

	error_number = htons(4);
	memcpy(buf + len, &error_number, 2);
	len += 2;

	strcpy(error_message, "Illegal TFTP operation\n");
	strcpy(buf + len, error_message);
	len += strlen(error_message);

	memcpy(buf + len, &endmess, 1);
	len ++;
	memset(error_message, 0, DEFAULT_SIZE);
	return len;
}

int file_not_found(){
	int len = 0;
	memset(buf, 0, BUFFER_SIZE);
	opcode = htons(5);

	memcpy(buf, &opcode, 2);
	len += 2;

	error_number = htons(1);
	memcpy(buf + len, &error_number, 2);
	len += 2;

	strcpy(error_message, "File Not Found!\n");
	strcpy(buf + len, error_message);
	len += strlen(error_message);

	memcpy(buf + len, &endmess, 1);
	len ++;

	memset(error_message, 0, DEFAULT_SIZE);
	return len;
}

void data_message(int block){
	opcode = htons(3);

	memcpy(buf, &opcode, 2);

	block_number = htons(block);
	memcpy(buf + 2, &block_number, 2);

}
