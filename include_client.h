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

#define BUFFER_SIZE	    1024
#define BLOCK_SIZE		512
#define ADDR_LEN		32
#define ACK				4
#define CLIENT_FOLDER	"./ClientFolder/"

char buf[BUFFER_SIZE], command[BUFFER_SIZE], server_addr[ADDR_LEN], client_file[32], data[BLOCK_SIZE];
char download_mode[10];
FILE *file;
int sd, port, ret, n = 0, addrlen, nBytes;
uint16_t opcode, error_number, block_number;
uint8_t endmess = 0;

void error_check(int val){
	if (val < 0){
        perror("Errore: \n");
        exit(1);
	}
}

void help(){
	printf("\n!help --> mostra l'elenco dei comandi\n");
	printf("!mode {txt|bin} --> imposta il modo di trasferimento dei files (testo o binario)\n");
	printf("!get filename nome_locale --> richiede al server il nome del file <filename> e lo salva localmente con il nome <nome_locale>\n");
	printf("!quit --> termina il client\n ");
}

void mode(){
	memset(command, 0 ,BUFFER_SIZE);
	scanf("%s", command);

	if(strncmp(&command[0], "txt", 3) == 0){

		printf("Modalità di trasferimento testuale configurato. \n\n");
		strcpy(download_mode, "netascii");

	} else if(strncmp(&command[0],"bin", 3) == 0){

		printf("Modalità di trasferimento binario configurato. \n\n");
		strcpy(download_mode, "octet");

	} else {
		printf("Modalità selezionata non valida\n");
	}

}


int request_message( char* filename){

	int length = 0;
	memset(buf, 0 ,BUFFER_SIZE);
	opcode = htons(1);
	memcpy(buf, &opcode, 2);
	length += 2;

	strcpy(buf + length, filename);
	length += strlen(filename);

	memcpy(buf + length, &endmess, 1);
	length++;

	strcpy(buf + length, download_mode);
	length += strlen(download_mode);

	memcpy(buf + length, &endmess, 1);
	length++;

	return length;
}

void ACK_message(){
	//lascio inalterato il block number
	memset(buf + 4, 0, BUFFER_SIZE - 4);

	//modifico opcode
	opcode = htons(4);
	memcpy(buf, &opcode, 2);

}
