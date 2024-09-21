/*
 * FIT VUT 2023 - Projekt předmětu ISA
 * TFTP Klient + Server
 *
 * File: tftp-functions.c
 * Author(s): xpauli08
 */

#include "tftp-functions.h"

Packet init_packet() {
	Packet packet;

  packet.opcode = -1;
  packet.blocksize = -1;
  packet.error_code = -1;
  packet.block_number = -1;
  packet.filepath = NULL;
  packet.mode = NULL;
  packet.timeout = NULL;
  packet.tsize = NULL;
  packet.error_message = NULL;
	packet.options[0] = '\0';

	return packet;
}

void parse_packet(Packet* packet, char* input, int input_size) {

    // Načtení opcode
    memcpy(&(packet->opcode), input, 2);
    packet->opcode = ntohs(packet->opcode);

		char* option;

    switch (packet->opcode) {
        case 1: // RRQ
        case 2: // WRQ
						char options_buffer[512];

						// Načtení filepath a mode, popř options
            packet->filepath = strdup(input + 2);
            packet->mode = strdup(input + 2 + strlen(packet->filepath) + 1);
            option = input + 2 + strlen(packet->filepath) + 1 + strlen(packet->mode) + 1;
            while (option < input + input_size) {
                char* value = option + strlen(option) + 1;
                if (strcmp(option, "blksize") == 0) {
                    packet->blocksize = atoi(value);
										sprintf(options_buffer, "blksize=%d", atoi(value));
										strcat(packet->options, " ");
										strcat(packet->options, options_buffer);
										
                } else if (strcmp(option, "timeout") == 0) {
                    packet->timeout = strdup(value);
										sprintf(options_buffer, "timeout=%s", value);
										strcat(packet->options, " ");
										strcat(packet->options, options_buffer);
										
                } else if (strcmp(option, "tsize") == 0) {
										packet->tsize = strdup(value);
										sprintf(options_buffer, "tsize=%s", value);
										strcat(packet->options, " ");
										strcat(packet->options, options_buffer);
										
                }
                option = value + strlen(value) + 1;
            }
            break;
        case 3: // DATA
						// Načtení dat a block number
            memcpy(&(packet->block_number), input + 2, 2);
            packet->block_number = ntohs(packet->block_number);
						packet->data = (char *)malloc(input_size - 4);
						memcpy(packet->data, input + 4, input_size - 4);
						break;
				case 4: // ACK
						// Načtení block number
            memcpy(&(packet->block_number), input + 2, 2);
            packet->block_number = ntohs(packet->block_number);
            break;
        case 5: // ERROR
						// Načtení error zprávy a kódu
            memcpy(&(packet->error_code), input + 2, 2);
            packet->error_code = ntohs(packet->error_code);
            packet->error_message = strdup(input + 4);
            break;
        case 6: // OACK
						// Načtení options
            option = input + 2;
            while (option < input + input_size) {
                char* value = option + strlen(option) + 1;
                if (strcmp(option, "blksize") == 0) {
                    packet->blocksize = atoi(value);
                } else if (strcmp(option, "timeout") == 0) {
                    packet->timeout = strdup(value);
                } else if (strcmp(option, "tsize") == 0) {
                    packet->tsize = strdup(value);
                }
                option = value + strlen(value) + 1;
            }
            break;
        default:
            break;
    }
}

void print_packet(Packet* packet, struct sockaddr_in *cliaddr, struct sockaddr_in *servaddr) {
	// Vypsání informací na stderr dle opcode
	switch(packet->opcode){
		case OP_RRQ:
			fprintf(stderr, "RRQ %s:%d \"%s\" %s%s\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port), packet->filepath, packet->mode, packet->options);
			break;
		case OP_WRQ:
			fprintf(stderr, "WRQ %s:%d \"%s\" %s%s\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port), packet->filepath, packet->mode, packet->options);
			break;
		case OP_ACK:
			fprintf(stderr, "ACK %s:%d %d\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port), packet->block_number);
			break;
		case OP_OACK:
			fprintf(stderr, "OACK %s:%d %s\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port), packet->options);
			break;
		case OP_DATA:
			fprintf(stderr, "DATA %s:%d:%d %d\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port), ntohs(servaddr->sin_port), packet->block_number);
			break;
		case OP_ERROR:
			fprintf(stderr, "ERROR %s:%d:%d %d \"%s\"\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port), ntohs(servaddr->sin_port), packet->error_code, packet->error_message);
			break;
	}
}

void free_packet(Packet* packet) {
    if (packet->filepath != NULL) {
        free(packet->filepath);
        packet->filepath = NULL;
    }
    if (packet->mode != NULL) {
        free(packet->mode);
        packet->mode = NULL;
    }
    if (packet->data != NULL) {
        free(packet->data);
        packet->data = NULL;
    }
    if (packet->timeout != NULL) {
        free(packet->timeout);
        packet->timeout = NULL;
    }
    if (packet->tsize != NULL) {
        free(packet->tsize);
        packet->tsize = NULL;
    }
    if (packet->error_message != NULL) {
        free(packet->error_message);
        packet->error_message = NULL;
    }

		packet->options[0] = '\0';
}

size_t create_rq_packet(uint16_t opcode, char* filepath, char* mode, char* option1, char* value1, char* option2, char* value2, char* option3, char* value3, char** packet) {
		// Nastavení opcode
    uint16_t opcode_net = htons(opcode);

		// Nastavení délky dle options
    size_t packet_size = 2 + strlen(filepath) + 1 + strlen(mode) + 1;
    if (strlen(option1) > 0 && strlen(value1) > 0) {
        packet_size += strlen(option1) + 1 + strlen(value1) + 1;
    }
    if (strlen(option2) > 0 && strlen(value2) > 0) {
        packet_size += strlen(option2) + 1 + strlen(value2) + 1;
    }
    if (strlen(option3) > 0 && strlen(value3) > 0) {
        packet_size += strlen(option3) + 1 + strlen(value3) + 1;
    }
    *packet = (char*)malloc(packet_size);

    memcpy(*packet, &opcode_net, sizeof(opcode_net));
    strcpy(*packet + 2, filepath);
    (*packet)[2 + strlen(filepath)] = '\0';

    strcpy(*packet + 2 + strlen(filepath) + 1, mode);
    (*packet)[2 + strlen(filepath) + 1 + strlen(mode)] = '\0';

		// Postupné přidávání options
    int offset = 2 + strlen(filepath) + 1 + strlen(mode) + 1;
    if (strlen(option1) > 0 && strlen(value1) > 0) {
        strcpy(*packet + offset, option1);
        (*packet)[offset + strlen(option1)] = '\0';
        strcpy(*packet + offset + strlen(option1) + 1, value1);
        (*packet)[offset + strlen(option1) + 1 + strlen(value1)] = '\0';
        offset += strlen(option1) + 1 + strlen(value1) + 1;
    }
    if (strlen(option2) > 0 && strlen(value2) > 0) {
        strcpy(*packet + offset, option2);
        (*packet)[offset + strlen(option2)] = '\0';
        strcpy(*packet + offset + strlen(option2) + 1, value2);
        (*packet)[offset + strlen(option2) + 1 + strlen(value2)] = '\0';
        offset += strlen(option2) + 1 + strlen(value2) + 1;
    }
    if (strlen(option3) > 0 && strlen(value3) > 0) {
        strcpy(*packet + offset, option3);
        (*packet)[offset + strlen(option3)] = '\0';
        strcpy(*packet + offset + strlen(option3) + 1, value3);
        (*packet)[offset + strlen(option3) + 1 + strlen(value3)] = '\0';
    }
    return packet_size;
}

size_t create_ack_packet(uint16_t block_num, char** packet) {
    uint16_t opcode_net = htons(OP_ACK);
    uint16_t block_num_net = htons(block_num);
    size_t packet_size = 2 + 2;
    *packet = (char*)malloc(packet_size);

    memcpy(*packet, &opcode_net, sizeof(opcode_net));
    memcpy(*packet + 2, &block_num_net, sizeof(block_num_net));

    return packet_size;
}

size_t create_data_packet(uint16_t block_number, size_t data_length, char* data, char** packet) {
    uint16_t opcode_net = htons(OP_DATA);
    uint16_t block_number_net = htons(block_number);
    size_t packet_size = 2 + 2 + data_length;
    *packet = (char*)malloc(packet_size);

    memcpy(*packet, &opcode_net, sizeof(opcode_net));
    memcpy(*packet + 2, &block_number_net, sizeof(block_number_net));
    memcpy(*packet + 4, data, data_length);

    return packet_size;
}

size_t create_error_packet(uint16_t error_code, char* error_msg, char** packet) {
    uint16_t opcode_net = htons(OP_ERROR);
    uint16_t error_code_net = htons(error_code);
    size_t packet_size = 2 + 2 + strlen(error_msg) + 1;
    *packet = (char*)malloc(packet_size);

    memcpy(*packet, &opcode_net, sizeof(opcode_net));
    memcpy(*packet + 2, &error_code_net, sizeof(error_code_net));
    strcpy(*packet + 4, error_msg);
    (*packet)[4 + strlen(error_msg)] = '\0';

    return packet_size;
}

void send_error_packet(uint16_t error_code, char* error_msg, struct sockaddr_in *cliaddr, int c_len, int sockfd) {
	char* error_packet;
  size_t error_packet_length = create_error_packet(error_code, error_msg, &error_packet);
  sendto(sockfd, error_packet, error_packet_length, 0, (const struct sockaddr *)cliaddr, c_len);
  free(error_packet);
	exit(0);
}

size_t create_oack_packet(char* option1, char* value1, char* option2, char* value2, char* option3, char* value3, char** packet) {
    uint16_t opcode_net = htons(OP_OACK);
    size_t packet_size = 2;
    if (strlen(option1) > 0 && strlen(value1) > 0) {
        packet_size += strlen(option1) + 1 + strlen(value1) + 1;
    }
    if (strlen(option2) > 0 && strlen(value2) > 0) {
        packet_size += strlen(option2) + 1 + strlen(value2) + 1;
    }
    if (strlen(option3) > 0 && strlen(value3) > 0) {
        packet_size += strlen(option3) + 1 + strlen(value3) + 1;
    }
    *packet = (char*)malloc(packet_size);

    memcpy(*packet, &opcode_net, sizeof(opcode_net));
    int offset = 2;
    if (strlen(option1) > 0 && strlen(value1) > 0) {
        strcpy(*packet + offset, option1);
        (*packet)[offset + strlen(option1)] = '\0';
        strcpy(*packet + offset + strlen(option1) + 1, value1);
        (*packet)[offset + strlen(option1) + 1 + strlen(value1)] = '\0';
        offset += strlen(option1) + 1 + strlen(value1) + 1;
    }
    if (strlen(option2) > 0 && strlen(value2) > 0) {
        strcpy(*packet + offset, option2);
        (*packet)[offset + strlen(option2)] = '\0';
        strcpy(*packet + offset + strlen(option2) + 1, value2);
        (*packet)[offset + strlen(option2) + 1 + strlen(value2)] = '\0';
        offset += strlen(option2) + 1 + strlen(value2) + 1;
    }
    if (strlen(option3) > 0 && strlen(value3) > 0) {
        strcpy(*packet + offset, option3);
        (*packet)[offset + strlen(option3)] = '\0';
        strcpy(*packet + offset + strlen(option3) + 1, value3);
        (*packet)[offset + strlen(option3) + 1 + strlen(value3)] = '\0';
    }
    return packet_size;
}

void print_error(char *err_msg) {
	printf("ERROR: %s\n", err_msg);
}

void handle_tftp_request(Packet* rq_packet, struct sockaddr_in *cliaddr, int c_len) {

	// Proměnné
	FILE *file;
	int sockfd;
	struct timeval timeout_value;

	// Vytvoření socketu
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    print_error("socket");
		exit(1);
  }

	// Otevření souboru dle opcode
	if (rq_packet->opcode == OP_RRQ) {
		file = fopen(rq_packet->filepath, "r");
		if (file == NULL) {
			print_error("file not found");
			send_error_packet(1, "File not found", cliaddr, c_len, sockfd);
		}
	} else {
		file = fopen(rq_packet->filepath, "r");
		if (file != NULL) {
			print_error("file already exists");
			fclose(file);
			send_error_packet(6, "File already exists", cliaddr, c_len, sockfd);
		}
		file = fopen(rq_packet->filepath, "wb");
	}

	printf("SERVER: Recieved request: %s '%s' in mode '%s'\n",rq_packet->opcode == OP_RRQ ? "download" : "upload", rq_packet->filepath, rq_packet->mode);

	int used_blocksize;
	int used_timeout;

	// Nastaveni blocksize
	if (rq_packet->blocksize > 0) {
		if (8 <= rq_packet->blocksize && rq_packet->blocksize <= 65464) {
			used_blocksize = rq_packet->blocksize;
		} else {
			used_blocksize = 512;
			rq_packet->blocksize = -1;
		}
	} else {
		used_blocksize = 512;
	}

	// Nastaveni timeout
	if (rq_packet->timeout != NULL) {
		if(1 <= atoi(rq_packet->timeout) && atoi(rq_packet->timeout) <= 256) {
			used_timeout = atoi(rq_packet->timeout);
		} else {
			used_timeout = 5;
			free(rq_packet->timeout);
			rq_packet->timeout = NULL;
		}
	} else {
		used_timeout = 5;
	}

	// Nastaveni timeout_value
	timeout_value.tv_sec = used_timeout;
	timeout_value.tv_usec = 0;

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_value, sizeof(timeout_value));

	if (rq_packet->tsize != NULL) {
		if (rq_packet->opcode == OP_RRQ && atoi(rq_packet->tsize) > 0) {
			free(rq_packet->tsize);
			rq_packet->tsize = NULL;
		}
		if (rq_packet->opcode == OP_WRQ && atoi(rq_packet->tsize) < 1) {
			free(rq_packet->tsize);
			rq_packet->tsize = NULL;
		}
	}

	// Odpověď v případě přijatých options
	if (rq_packet->blocksize > 0 || rq_packet->timeout != NULL || rq_packet->tsize != NULL) {
		char oack_buffer[512];

		char blocksize[32];
		char timeout[32];
		char tsize[32];

		char blocksize_val[32];
		char timeout_val[32];
		char tsize_val[32];

		if (rq_packet->blocksize == -1) {
			strcpy(blocksize, "");
			strcpy(blocksize_val, "");
		} else {
			strcpy(blocksize, "blksize");
			sprintf(blocksize_val, "%d", rq_packet->blocksize);
		}

		if (rq_packet->timeout == NULL) {
			strcpy(timeout, "");
			strcpy(timeout_val, "");
		} else {
			strcpy(timeout, "timeout");
			strcpy(timeout_val, rq_packet->timeout);
		}

		if (rq_packet->tsize == NULL) {
			strcpy(tsize, "");
			strcpy(tsize_val, "");
		} else {
			if (strcmp(rq_packet->tsize, "0") == 0 && rq_packet->opcode == OP_RRQ) {
				strcpy(tsize, "tsize");
			
				struct stat st;
				if (stat(rq_packet->filepath, &st) == 0) {
					sprintf(tsize_val, "%ld", st.st_size);
				}
			} else {
				if (atoi(rq_packet->tsize) > 0 && rq_packet->opcode == OP_WRQ) {
					strcpy(tsize, "tsize");
					strcpy(tsize_val, rq_packet->tsize);
				} else {
					strcpy(tsize, "");
					strcpy(tsize_val, "");
				}
			}
		}

		char* oack_packet;
		size_t oack_packet_length = create_oack_packet(blocksize, blocksize_val, timeout, timeout_val, tsize, tsize_val, &oack_packet);
    if ((sendto(sockfd, oack_packet, oack_packet_length, 0, (const struct sockaddr *)cliaddr, c_len)) < 0)
		{
			print_error("sendto");
			send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
		}
		free(oack_packet);

		struct sockaddr_in servaddr;
		int s_len = sizeof(servaddr);
		getsockname(sockfd, (struct sockaddr*)&servaddr, &s_len);

		if (rq_packet->opcode == OP_RRQ) {
			Packet recv_packet = init_packet();

			size_t packet_size = recvfrom(sockfd, oack_buffer, sizeof(oack_buffer), 0, (struct sockaddr *) cliaddr, &c_len);
			if (packet_size < 0) {
				print_error("recvfrom");
				send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
			}

			parse_packet(&recv_packet, oack_buffer, packet_size);

			print_packet(&recv_packet, cliaddr, &servaddr);

			printf("OPCODE: %d", recv_packet.opcode);
			
			if (recv_packet.opcode != OP_ACK) {
				print_error("expected ACK, recieved other");
				send_error_packet(1, "Invalid TFTP operation", cliaddr, c_len, sockfd);
			}
		}
	}
	
	// Download ze serveru
	if (rq_packet->opcode == OP_RRQ) {
		int rrq_counter;

		Packet recv_packet = init_packet();

		char buffer[used_blocksize+4];
		char data[used_blocksize];
		int data_length;
		int block_number = 0;
		bool last_packet = false;

		// Loop pro odesílání packetů
		while (!last_packet) {

			block_number++;
			data_length = fread(data, 1, used_blocksize, file);

			// Po tomto loop skončí
			if (data_length < used_blocksize) {
				last_packet = true;
			}

			size_t packet_size;

			struct sockaddr_in servaddr;
			int s_len = sizeof(servaddr);

			// For loop pro timeout
			for (rrq_counter = 5; rrq_counter; rrq_counter--) {

				packet_size = 0;

				// Odeslání DATA paketu
				char* data_packet;
				size_t data_packet_length = create_data_packet(block_number, data_length, data, &data_packet);
				if ((sendto(sockfd, data_packet, data_packet_length, 0, (const struct sockaddr *) cliaddr, c_len)) < 0) {
					print_error("sendto");
					send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
				}

				// Získání informací o serveru
				getsockname(sockfd, (struct sockaddr*)&servaddr, &s_len);

				free(data_packet);

				// Přijetí ACK packetu
				packet_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) cliaddr, &c_len);
				if (packet_size < 0 && errno != EAGAIN) {
					print_error("recvfrom");
					send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
				}
				printf("PACKET SIZE: %ld\n", packet_size);
				if (packet_size == -1)
				{
					printf("SERVER: no response, resending packet\n");
					continue;
				}
				else if (packet_size >= 4) {
					printf("How did i get here\n");
					parse_packet(&recv_packet, buffer, packet_size);
					print_packet(&recv_packet, cliaddr, &servaddr);
					break;
				}
			}

			// Time out v případě 5 opakování odeslání packetu
			if (!rrq_counter) {
				printf("SERVER: Timed out.\n");
				exit(0);
			}

			// Reakce na typ paketu
			if (recv_packet.opcode == OP_ERROR) {
				printf("SERVER: Recieved error packet, terminating.\n");
				exit(0);
			} else if (recv_packet.opcode != OP_ACK) {
				print_error("expected ACK, recieved other packet");
				send_error_packet(4, "Invalid TFTP operation", cliaddr, c_len, sockfd);
			}

			if (recv_packet.block_number != block_number) {
				print_error("bad block number in ACK packet");
				send_error_packet(4, "Invalid TFTP operation", cliaddr, c_len, sockfd);
			}
		}
	}

	// Upload na server
	else if (rq_packet->opcode == OP_WRQ) {
		int wrq_counter;
		
		Packet recv_packet = init_packet();

		char buffer[used_blocksize+4];

		int block_number = 0;
		bool last_packet = false;
		
		// Odeslání ACK pouze, pokud nebyl odeslán OACK
		if (rq_packet->blocksize == -1 && rq_packet->timeout == NULL && rq_packet->tsize == NULL) {
			char* ack_packet;
			size_t ack_packet_length = create_ack_packet(block_number, &ack_packet);
			if ((sendto(sockfd, ack_packet, ack_packet_length, 0, (const struct sockaddr *)cliaddr, c_len)) < 0) {
				print_error("sendto");
				send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
			}
			free(ack_packet);
		}

		// Loop pro přijímání dat
		while(!last_packet) {

			size_t packet_size;

			struct sockaddr_in servaddr;
			int s_len = sizeof(servaddr);

			// For loop pro timeout
			for (wrq_counter = 5; wrq_counter; wrq_counter--) {

				// Přijetí data packetu
				packet_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)cliaddr, &c_len);
				if (packet_size < 0  && errno != EAGAIN) {
					print_error("recvfrom");
					send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
				}

				printf("SIZE: %ld\n", packet_size);

				if (packet_size == -1) {
					printf("SERVER: no response, resending packet\n");
					free_packet(&recv_packet);

					// Opětovné odeslání ACK packetu
					char* ack_packet;
					size_t ack_packet_length = create_ack_packet(block_number, &ack_packet);
					if ((sendto(sockfd, ack_packet, ack_packet_length, 0, (const struct sockaddr *)cliaddr, c_len)) < 0) {
						print_error("sendto");
						send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
					}
					free(ack_packet);
				}

				else if (packet_size >= 4) {
					parse_packet(&recv_packet, buffer, packet_size);

					getsockname(sockfd, (struct sockaddr*)&servaddr, &s_len);

					print_packet(&recv_packet, cliaddr, &servaddr);
					break;
				}
			}

			// Timeout v případě 5 opakování odeslání packetu
			if (!wrq_counter) {
				printf("SERVER: Timed out.\n");
				exit(1);
			}

			block_number++;

			// Po tomto loop skončí
			if (packet_size < used_blocksize+4) {
				last_packet = true;
			}

			// Reakce na typ packetu
			if (recv_packet.opcode == OP_ERROR) {
				printf("SERVER: Recieved error packet, terminating.\n");
				close(sockfd);
				exit(0);
			} else if (recv_packet.opcode != OP_DATA) {
				print_error("expected DATA, recieved other packet");
				send_error_packet(4, "Invalid TFTP operation", cliaddr, c_len, sockfd);
			}

			// Zápis do souboru
			if (fwrite(recv_packet.data, 1, packet_size - 4, file) < 0) {
				print_error("write to file");
				send_error_packet(0, "Error opening file", cliaddr, c_len, sockfd);
			}

			// Odeslání ack packetu
			char* ack_packet;
			size_t ack_packet_length = create_ack_packet(block_number, &ack_packet);

			if ((sendto(sockfd, ack_packet, ack_packet_length, 0, (const struct sockaddr *)cliaddr, c_len)) < 0) {
				print_error("sendto");
				send_error_packet(0, "Server side error", cliaddr, c_len, sockfd);
			}
			free(ack_packet);
			free_packet(&recv_packet);
		}
	}

	printf("SERVER: Transfer completed\n");

	fclose(file);
	close(sockfd);
}
