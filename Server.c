#include "include_server.h"

int main (int argc, char* argv[]) {

	//controllo numero argomenti
	if(argc != 3){
		printf("Numero di argomenti errato\nSpecificare Porta e Directory\n\n");
		return 0;
	}

	//controllo Porta
	port = strtol(argv[1], &input, 10);
	if (*input != '\0'){
		printf("Numero di porta non valido\n");
		return 0;
	}

	pid_t pid;
	struct sockaddr_in my_addr, connecting_addr;

	//Creazione Socket
	listener = socket(AF_INET, SOCK_DGRAM, 0);
	error_check(listener);

	printf("Socket di ascolto %d creato.\n\n", listener);


	memset(&my_addr, 0, sizeof(my_addr));
	memset(&connecting_addr, 0, sizeof(connecting_addr));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	//bind tra socket e indirizzo del server
	ret = bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr));
	error_check(ret);

	printf("Server in attesa di richieste. \n\n");
	while(1){
		strcpy(server_file, argv[2]);
		//attesa di richieste

		addrlen = sizeof(connecting_addr);
		ret = recvfrom(listener, (char *)buf, BUFFER_SIZE, 0, (struct sockaddr *)&connecting_addr, (socklen_t *)&addrlen);

		memcpy(&opcode, (uint16_t*)buf, 2);
		opcode = ntohs(opcode);

		memset(download_mode, 0, 10);
		sprintf(filename, "%s", buf + 2);
		sprintf(download_mode, "%s", buf + (strlen(filename) + 3));


		printf("Richiesta ricevuta!\nOpcode: %d\nNome del file: %s\nModalità di trasferimento: %s\n\n", opcode, filename, download_mode);

		//richiesta non valida
		if(opcode != 1){
			printf("Richiesta effettuata non valida.\n\n");

			memset(buf, 0 ,BUFFER_SIZE);
			len = illegal_tftp_operation();

			//invio messaggio di errore
			ret = sendto(listener, buf, len, 0,(struct sockaddr*)&connecting_addr, addrlen);
			error_check(ret);
			printf("Messaggio di errore inviato.\n\n");
			return 1;
		}


		pid = fork();

		if(pid == 0){
			//processo figlio

			FILE *file;
			//n -> conta byte inviati, block_counter -> conta numero blocco attuale
			int n = 4, block_counter = 1, send_socket;

			//creazione socket per servire la richiesta
			send_socket = socket(AF_INET, SOCK_DGRAM, 0);
			error_check(send_socket);

			strcat(server_file, filename);
			memset(buf, 0, BUFFER_SIZE);
			//trasferimento in formato testuale ( "netascii" )
			if(strcmp(download_mode, "netascii") == 0){

				if((file = fopen(server_file, "r")) == NULL){
					printf("File Inesistente\n");
					//invia errore
					len = file_not_found();
					ret = sendto(send_socket, buf, len, 0,(struct sockaddr*)&connecting_addr, addrlen);
					error_check(ret);
					printf("Messaggio di errore inviato.\n\n");
					exit(1);
				}

send_block_text:

				buf[n] = fgetc(file);
				n++;

				if(n == BLOCK_SIZE + 4 || feof(file)){
					//preparo il messaggio
TXT_block:
					data_message(block_counter);

					//printf("%s\n", buf + 4);	  // test
					//printf("\n\n%c\n\n", buf[n-1]);		//test

					//invio blocco
					ret = sendto(send_socket, buf, n, 0,(struct sockaddr*)&connecting_addr, addrlen);
					error_check(ret);

					printf("Blocco numero %d inviato.\n", block_counter);
					memset(buf, 0, BUFFER_SIZE);

					//attendo ack dal client
					ret = recvfrom(send_socket, (char *)buf, ACK, 0, (struct sockaddr*)&connecting_addr, (socklen_t *)&addrlen);
					error_check(ret);

					//Controlli sul messaggio di ACK
					memcpy(&opcode, (uint16_t *)buf, 2);
					opcode = ntohs(opcode);
					if(opcode != 4){
						printf("Il messaggio ricevuto non era un messaggio di ACK\n\n");
						exit(1);
					}
					memcpy(&block_number, (uint16_t *)(buf + 2), 2);
					block_number = ntohs(block_number);

					if(block_number != block_counter){
						printf("L'ACK ricevuto contiene un 'block_number' errato\n\n");
						exit(1);
					}

					printf("ACK numero %d Ricevuto correttamente\n\n", block_counter);

					block_counter ++;

					//File inviato completamente ma il blocco dati è di 512 bytes quindi devo inviare
					//un altro Data packet con blocco dati vuoto (0 bytes)
					if(n == BLOCK_SIZE + 4 && feof(file)){
						memset(buf, 0 ,BUFFER_SIZE);
						n = 4;
						goto TXT_block;
					}
					n = 4;
				}

				if(!feof(file))
					goto send_block_text;

				memset(buf, 0, BUFFER_SIZE);
				fclose(file);



			//trasferimento in formato binario ( "octet" )
			} else if(strcmp(download_mode, "octet") == 0){

				if((file = fopen(server_file, "rb")) == NULL){
					printf("File Inesistente\n");
					//invia errore
					len = file_not_found();
					ret = sendto(send_socket, buf, len, 0,(struct sockaddr*)&connecting_addr, addrlen);
					error_check(ret);
					printf("Messaggio di errore inviato.\n\n");
					exit(1);
				}


				long file_size;
				fseek (file , 0 , SEEK_END);
				file_size = ftell(file) ;
				rewind (file);

send_block_binary:

				memset(buf, 0 ,BUFFER_SIZE);

				if(file_size >= BLOCK_SIZE){
					fread(&buf[4], sizeof(char), BLOCK_SIZE, file);
					file_size -= BLOCK_SIZE; // 0 --> devo inviare un altro blocco vuoto
					n = BLOCK_SIZE;
				} else {
					fread(&buf[4], sizeof(char), file_size, file);
					n = file_size;
					file_size = -1; //-1 --> file terminato
				}
				// printf("%s\n\n", buf + 4); 		//test

BIN_block:
				//preparo il blocco da inviare
				data_message(block_counter);

					//invio blocco
				ret = sendto(send_socket, buf, n + 4, 0,(struct sockaddr*)&connecting_addr, addrlen);
				error_check(ret);
				printf("Blocco numero %d inviato.\n", block_counter);
				memset(buf, 0, BUFFER_SIZE);

				//attendo ACK dal client
				ret = recvfrom(send_socket, (char *)buf, ACK, 0, (struct sockaddr*)&connecting_addr, (socklen_t *)&addrlen);
				error_check(ret);

				memcpy(&opcode, (uint16_t *)buf, 2);
				opcode = ntohs(opcode);
				if(opcode != 4){
					printf("Il messaggio ricevuto non era un messaggio di ACK\n\n");
					exit(1);
				}
				memcpy(&block_number, (uint16_t *)(buf + 2), 2);
				block_number = ntohs(block_number);

				if(block_number != block_counter){
					printf("L'ACK ricevuto contiene un 'block_number' errato\n\n");
					exit(1);
				}
				printf("ACK numero %d Ricevuto correttamente\n\n", block_counter);
				block_counter ++;

				//se file_size == 0 vuol dire che il blocco dati precedente era di 512 bytes quindi devo inviare
				//un altro blocco dati con 0 bytes
				if(file_size == 0){
						memset(buf, 0 ,BUFFER_SIZE);
						file_size = -1;
						n = 0;
						goto BIN_block;
					}
				if(file_size > 0)
					goto send_block_binary;

				memset(buf, 0, BUFFER_SIZE);
				fclose(file);
			}

			printf("Messaggio inviato correttamente\n\n\n");
			close(send_socket);
			exit(0);

		} else if (pid > 0){

		}
	}
	return 0;
}
