/*
 * FIT VUT 2023 - Projekt předmětu ISA
 * TFTP Klient + Server
 *
 * File: tftp-server.c
 * Author(s): xpauli08
 */

#include "tftp-functions.h"

void parse_args(int argc, char **argv);

// Globální proměnné pro argumenty
int port = 69;
char *root_dirpath = NULL;
int blocksize = 0;

int main(int argc, char **argv)
{
  // Načtění argumentů
  parse_args(argc, argv);
  
  // Vytvoření socketu
  int sockfd;
  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      print_error("Create socket failed");
      exit(1);
  }

  // Nastavení informací o serveru
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(port);

  // Přiřazení adresy a portu socketu
  if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
      print_error("Bind failed");
      exit(1);
  }

  // Nastavení dirpath
  if (chdir(root_dirpath) < 0) {
    print_error("Dirpath  failed");
    exit(1);
  }

  // Debug informace
  printf("SERVER: Root directory is %s\n", root_dirpath);
  printf("SERVER: Listening on port %d\n", port);

  char wait_buffer[514];

  while (true) {

    Packet packet = init_packet();

    struct sockaddr_in cliaddr;
    int len = sizeof(cliaddr);

    // Přijetí WRQ/RRQ packetů
    int packet_size = recvfrom(sockfd, wait_buffer, sizeof(wait_buffer), 0, (struct sockaddr *)&cliaddr, &len);
    parse_packet(&packet, wait_buffer, packet_size);
    print_packet(&packet, &cliaddr, &servaddr);

    // Fork
    if (packet.opcode == OP_RRQ || packet.opcode == OP_WRQ) {
      if (fork() == 0) {
        handle_tftp_request(&packet, &cliaddr, len);
        exit(0);
      }
    } else {
      // Reakce na jine typy packetu
      print_error("Expected WRQ/RRQ, recieved other packet");
      char* error_packet;
      size_t error_packet_length = create_error_packet(4, "Invalid TFTP Operation", &error_packet);
      sendto(sockfd, error_packet, error_packet_length, 0, (const struct sockaddr *)&cliaddr, len);
      free(error_packet);
    }
  }

  close(sockfd);
  return 0;
}

void parse_args(int argc, char **argv)
{
	// Procházení argumentů
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0) {
      // Kontrola portu
      if (i + 1 < argc) {
        port = atoi(argv[++i]);
				if (port < 1 || port > 65535) {
          print_error("Port out of range");
          exit(1);
        }
      } else {
        fprintf(stderr, "Port unspecified after -p");
        exit(1);
      }
  	} else {
	    // Kontrola více root_dirpath
	    if (root_dirpath != NULL) {
	      print_error("More than one root_dirpath");
	      exit(1);
	    }
	    root_dirpath = argv[i];
    }
  }

  // Kontrolujeme, zda byla specifikována cesta k adresáři
  if (root_dirpath == NULL) {
    print_error("No root dirpath specified");
    exit(1);
  }
}