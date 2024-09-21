/*
 * FIT VUT 2023 - Projekt předmětu ISA
 * TFTP Klient + Server
 *
 * File: tftp-client.c
 * Author(s): xpauli08
 */

#include "tftp-functions.h"

void parse_args(int argc, char **argv);

// Globální proměnné pro vstupní argumenty
char *hostname = NULL;
int port = 69;
char *filepath = NULL;
char *dest_filepath = NULL;

int main(int argc, char **argv) {

  // Načtení argumentů
	parse_args(argc, argv);

  int initial_opcode;

  // Nastavení opcode pomocí argumentů
  if (filepath == NULL) {
    printf("CLIENT: Uploading file to server\n");
    initial_opcode = OP_WRQ;
  } else {
    printf("CLIENT: Downloading file from server\n");
    initial_opcode = OP_RRQ;
  }

  // Vytvoření socketu
	int sockfd;
	struct sockaddr_in init_addr, servaddr;
  int s_len = sizeof(servaddr);

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    print_error("socket");
		exit(1);
  }

	// Nastavení informací o serveru
	init_addr.sin_family = AF_INET;
	init_addr.sin_port = htons(port);
	struct hostent *host_info;
  struct in_addr addr;

  // Zjištění IPv4 adresy z hostname
  host_info = gethostbyname(hostname);
  if (host_info == NULL) {
    print_error("Unknown host");
    exit(1);
  }

  // Nastavení IPv4 adresy
  addr.s_addr = *((unsigned long *) host_info->h_addr_list[0]);
  init_addr.sin_addr.s_addr = addr.s_addr;

  printf("CLIENT: Requesting on %s:%d\n", inet_ntoa(init_addr.sin_addr), ntohs(init_addr.sin_port));

  // Mód octet
  char* mode = "octet";

  // // Nastavení proměnných pro timeout
  // struct timeval timeout_value;
  // timeout_value.tv_sec = 5;
	// timeout_value.tv_usec = 0;

  // // Nastavení timeoutu
  // setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_value, sizeof(timeout_value));

  char buffer[516];

  int block_number = 0;

  // Odeslání WRQ/RRQ packetu
  char* request_packet;
  size_t request_packet_length = create_rq_packet(initial_opcode, dest_filepath, mode, "", "", "", "", "", "", &request_packet);
  sendto(sockfd, request_packet, request_packet_length, 0, (const struct sockaddr *)&init_addr, sizeof(init_addr));
  free(request_packet);

  // Download ze serveru
  if (initial_opcode == OP_RRQ) {
    
    FILE *file;
    file = fopen(filepath, "wb");

    bool last_packet = false;

    // Loop pro načítání packetů
    while (!last_packet) {

      Packet recv_packet = init_packet();

      block_number++;

      size_t packet_size;

      // Přijetí (snad) DATA packetu
      packet_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &s_len);
      if (packet_size < 0  && errno != EAGAIN) {
        print_error("recvfrom");
        send_error_packet(0, "Client side error", &servaddr, s_len, sockfd);
      }

      // Vyskočení z cyklu
      if (packet_size < 516) {
        last_packet = true;
        printf("CLIENT: Last packet recieved\n");
      }

      // Zjištění informací o klientovi
      struct sockaddr_in cliaddr;
			int c_len = sizeof(cliaddr);

      getsockname(sockfd, (struct sockaddr*)&cliaddr, &c_len);

      // Rozparsování a vypsání informací o packetu
      parse_packet(&recv_packet, buffer, packet_size);
      print_packet(&recv_packet, &servaddr, &cliaddr);

      // Reakce na typ packetu
			if (recv_packet.opcode == OP_ERROR) {
				printf("CLIENT: Recieved error packet, terminating.\n");
				close(sockfd);
				exit(0);
			} else if (recv_packet.opcode != OP_DATA) {
				print_error("expected DATA, recieved other packet");
				send_error_packet(4, "Invalid TFTP operation", &servaddr, s_len, sockfd);
			}

      // Zápis do souboru
			if (fwrite(recv_packet.data, 1, packet_size - 4, file) < 0) {
				print_error("write to file");
				send_error_packet(0, "Error writing to file", &servaddr, s_len, sockfd);
			}

      // Odeslání ACK packetu
      char* ack_packet;
      int block_num = block_number;
      size_t ack_packet_length = create_ack_packet(block_num, &ack_packet);

      sendto(sockfd, ack_packet, ack_packet_length, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
      free(ack_packet);
      
      free_packet(&recv_packet);
    }
    fclose(file);

  // Upload na server
  } else if (initial_opcode == OP_WRQ) {

    bool last_packet = false;

    Packet recv_packet = init_packet();

    size_t packet_size;

    // Přijetí ACK packetu
    packet_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &s_len);
    if (packet_size < 0  && errno != EAGAIN) {
      print_error("recvfrom");
      send_error_packet(0, "Client side error", &servaddr, s_len, sockfd);
    }

    struct sockaddr_in cliaddr;
	  int c_len = sizeof(cliaddr);
    getsockname(sockfd, (struct sockaddr*)&cliaddr, &c_len);

    // Rozparsování a vypsání packetu
    parse_packet(&recv_packet, buffer, packet_size);
    print_packet(&recv_packet, &servaddr, &cliaddr);

    // Reakce na typ packetu
		if (recv_packet.opcode == OP_ERROR) {
		  printf("CLIENT: Recieved error packet, terminating.\n");
			close(sockfd);
			exit(0);
		} else if (recv_packet.opcode != OP_ACK) {
			print_error("expected ACK, recieved other packet");
			send_error_packet(4, "Invalid TFTP operation", &servaddr, s_len, sockfd);
		}

    // Loop pro odesílání packetů
    while (!last_packet) {

      block_number++;

      // Načtení dat ze stdin
      char data[512];
      size_t data_length = fread(data, 1, sizeof(data), stdin);

      if (data_length < 512) {
        last_packet = true;
      }

      // Odeslání data packetu
      char* data_packet;
      size_t data_packet_length = create_data_packet(block_number, data_length, data, &data_packet);
      if ((sendto(sockfd, data_packet, data_packet_length, 0, (const struct sockaddr *) &servaddr, s_len)) < 0) {
        print_error("sendto");
        send_error_packet(0, "Server side error", &servaddr, s_len, sockfd);
      }

      // Získání informací o klientovi
      getsockname(sockfd, (struct sockaddr*)&cliaddr, &s_len);
      free(data_packet);

      // Přijetí ACK packetu
      packet_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &s_len);
      if (packet_size < 0  && errno != EAGAIN) {
        print_error("recvfrom");
        send_error_packet(0, "Client side error", &servaddr, s_len, sockfd);
      }

      struct sockaddr_in cliaddr;
      int c_len = sizeof(cliaddr);
      getsockname(sockfd, (struct sockaddr*)&cliaddr, &c_len);

      // Rozparsování a vypsání informací o packetu
      parse_packet(&recv_packet, buffer, packet_size);
      print_packet(&recv_packet, &servaddr, &cliaddr);

      // Reakce na typ packetu
      if (recv_packet.opcode == OP_ERROR) {
        printf("CLIENT: Recieved error packet, terminating.\n");
        close(sockfd);
        exit(0);
      } else if (recv_packet.opcode != OP_ACK) {
        print_error("expected ACK, recieved other packet");
        send_error_packet(4, "Invalid TFTP operation", &servaddr, s_len, sockfd);
      }
      
      free_packet(&recv_packet);
    }
  }

  // Konec
  printf("CLIENT: Transfer completed\n");
  close(sockfd);
	return 0;
}

void parse_args(int argc, char **argv)
{

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      if (i + 1 < argc) {
        hostname = argv[++i];
      } else {
        fprintf(stderr, "Chyba: Nebyla specifikována žádná IP adresa/doménový název po přepínači -h.\n");
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(argv[i], "-p") == 0) {
      if (i + 1 < argc) {
        port = atoi(argv[++i]);
        if (port < 1 || port > 65535) {
          fprintf(stderr, "Chyba: Specifikovaný port je mimo platný rozsah (1-65535).\n");
          exit(EXIT_FAILURE);
        }
      } else {
        fprintf(stderr, "Chyba: Nebyl specifikován žádný port po přepínači -p.\n");
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(argv[i], "-f") == 0) {
      if (i + 1 < argc) {
        filepath = argv[++i];
      } else {
        fprintf(stderr, "Chyba: Nebyla specifikována žádná cesta ke stahovanému souboru po přepínači -f.\n");
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(argv[i], "-t") == 0) {
      if (i + 1 < argc) {
        dest_filepath = argv[++i];
      } else {
        fprintf(stderr, "Chyba: Nebyla specifikována žádná cesta, pod kterou bude soubor uložen po přepínači -t.\n");
        exit(EXIT_FAILURE);
      }
    } else {
      fprintf(stderr, "Chyba: Neznámý argument '%s'.\n", argv[i]);
      exit(EXIT_FAILURE);
    }
  }

  if (hostname == NULL) {
    fprintf(stderr, "Chyba: Nebyla specifikována IP adresa/doménový název.\n");
    exit(EXIT_FAILURE);
  }

  if (dest_filepath == NULL) {
    fprintf(stderr, "Chyba: Nebyla specifikována cesta, pod kterou bude soubor uložen.\n");
    exit(EXIT_FAILURE);
  }
}