#include "include_client.h"

int main (int argc, char* argv[]) {
	char* input;
	int len;
	//controllo numero di argomenti
	if(argc != 3){
		printf("Numero di argomenti errato\nSpecificare IP e Porta del server\n");
		return 0;
	}

	//controllo Porta del server
	port = strtol(argv[2], &input, 10);
	if (*input != '\0'){
		printf("Numero di porta del server non valido\n");
		return 0;
	}
	//Salvo l'indirizzo del server specificato
	strcpy(server_addr, argv[1]);


	help();

	strcpy(download_mode, "octet");
	while(1){
		//Cartella in cui verra salvato il file
		strcpy(client_file, CLIENT_FOLDER);

		// Struttura per il server
		struct sockaddr_in srv_addr;

		// Creazione socket
		sd = socket(AF_INET, SOCK_DGRAM, 0);
		error_check(sd);

		// Creazione indirizzo del server
		memset(&srv_addr, 0, sizeof(srv_addr));
		srv_addr.sin_family = AF_INET ;
		srv_addr.sin_port = htons(port);
		inet_pton(AF_INET, server_addr, &srv_addr.sin_addr);


		memset(command, 0 , BUFFER_SIZE);
		printf("\n\nInserisci un comando: ");
		scanf("%s", command);

		//Help
		if(strcmp(command, "!help\0") == 0)
			help();


		//Mode
		else if(strncmp(&command[0], "!mode", 5) == 0)
			mode();


		//GET
		else if(strncmp(&command[0], "!get", 4) == 0){

			char path[BUFFER_SIZE], filename[BUFFER_SIZE];
			memset(filename, 0 ,BUFFER_SIZE);

			//inserimento nome file e nome locale
			scanf("%s", filename);
			printf("\nRichiesta del file %s al server in corso.\n\n", filename);
			memset(path, 0 ,BUFFER_SIZE);
			scanf("%s", path);


			//preparazione messaggio di richiesta
			len = request_message(filename);

			//invio richiesta del file al server
			addrlen = sizeof(srv_addr);
			ret = sendto(sd, buf, len, 0, (struct sockaddr*)&srv_addr, addrlen);
			error_check(ret);


			memset(buf, 0, BUFFER_SIZE);

			//inserisco percorso del file
			strcat(client_file, path);

			if((strcmp(download_mode, "netascii") == 0)){
				file = fopen(client_file, "w");
			} else {
				file = fopen(client_file, "wb");
			}

			if(!file){
				printf("Errore apertura file\n");
				exit(1);
			}

			printf("Trasferimento file in corso. \n\n");


wait_block:

			//attesa della risposta da parte del server
			ret = recvfrom(sd, (char *)buf, BLOCK_SIZE + 4 , 0, (struct sockaddr*)&srv_addr, (socklen_t *)&addrlen);
			error_check(ret);

			nBytes = ret - 4;

			//printf("%s\n", buf + 4);

			memcpy(&opcode, (uint16_t*)buf, 2);
			opcode = ntohs(opcode);


			//ERRORE: Ricevuto messaggio di errore
			if(opcode == 5){
				memcpy(&error_number, (uint16_t*)(buf + 2), 2);
				error_number = ntohs(error_number);
				printf("%s\n\n", buf + 4);
				// 1: file not found    4: Illegal TFTP Operation
				if(error_number == 4 ){
					exit(1);
				}
				//nel caso di file not found il client non termina l'esecuzione


			} else {

				//Scrivo nel file il blocco ricevuto
				if( strcmp(download_mode, "netascii") == 0){
					for(n = 0; n < nBytes ; n++){
						if(buf[n+4]==EOF) goto sending;
						fputc(buf[n + 4], file);
					}
				} else {
					fwrite(&buf[4], sizeof(char), nBytes, file);
				}

sending:
				memcpy(&block_number, (uint16_t*)(buf + 2), 2);
				block_number = ntohs(block_number);

				printf("Ricevuto blocco numero: %d\nDimensione blocco: %d\n",block_number, nBytes);

				//preparo messaggio di ACK
				ACK_message();


				//invio ACK al server ( ho ricevuto il blocco )
				printf("Invio ACK numero %d al server \n\n", block_number);
				ret = sendto(sd, buf, ACK, 0, (struct sockaddr*)&srv_addr, addrlen);
				error_check(ret);

				if( nBytes == BLOCK_SIZE )
					goto wait_block;

				printf("\nSalvataggio %s completato.\n\n", path);

				fclose(file);
				close(sd);
			}

		//Quit
		}else if(strncmp(&command[0], "!quit", 5) == 0){
			close(sd);
			printf("\nClient Terminato\n");
			exit(0);

		//Comando non valido
		} else {
			printf("Comando errato, Inserisci uno dei seguenti comandi\n");
			help();
		}
	}
	return 0;
}
