/*
 * FIT VUT 2023 - Projekt předmětu ISA
 * TFTP Klient + Server
 *
 * File: tftp-functions.h
 * Author(s): xpauli08
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <signal.h>
#include <sys/select.h>
#include <netdb.h>

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5
#define OP_OACK 6

typedef struct {
	int opcode;
	char* filepath;
	char* mode;
	int block_number;
	char* data;
	int blocksize;
	char* timeout;
	char* tsize;
	int error_code;
	char* error_message;
	char options[512];
} Packet;

/**
 * @brief Inicializuje strukturu Packet.
 * 
 * @return Packet Vrací inicializovanou strukturu Packet.
 */
Packet init_packet();

/**
 * @brief Analyzuje vstupní data a naplní strukturu Packet.
 * 
 * @param packet Ukazatel na strukturu Packet.
 * @param input Ukazatel na vstupní data.
 * @param input_size Velikost vstupních dat.
 */
void parse_packet(Packet* packet, char* input, int input_size);

/**
 * @brief Vytiskne informace o paketu.
 * 
 * @param packet Ukazatel na strukturu Packet.
 * @param cliaddr Ukazatel na strukturu sockaddr_in klienta.
 * @param servaddr Ukazatel na strukturu sockaddr_in serveru.
 */
void print_packet(Packet* packet, struct sockaddr_in *cliaddr, struct sockaddr_in *servaddr);

/**
 * @brief Uvolní paměť alokovanou pro strukturu Packet.
 * 
 * @param packet Ukazatel na strukturu Packet..
 */
void free_packet(Packet* packet);

/**
 * @brief Vytvoří WRQ/RRQ packet.
 * 
 * @param opcode Kód operace.
 * @param filepath Cesta k souboru.
 * @param mode Režim přenosu.
 * @param option1 První volba.
 * @param value1 Hodnota první volby.
 * @param option2 Druhá volba.
 * @param value2 Hodnota druhé volby.
 * @param option3 Třetí volba.
 * @param value3 Hodnota třetí volby.
 * @param packet Ukazatel na výstupní paket.
 * @return size_t Velikost vytvořeného paketu.
 */
size_t create_rq_packet(uint16_t opcode, char* filepath, char* mode, char* option1, char* value1, char* option2, char* value2, char* option3, char* value3, char** packet);

/**
 * @brief Vytvoří ACK paket.
 * 
 * @param block_num Číslo bloku.
 * @param packet Ukazatel na výstupní paket.
 * @return size_t Velikost vytvořeného paketu.
 */
size_t create_ack_packet(uint16_t block_num, char** packet);

/**
 * @brief Vytvoří DATA paket.
 * 
 * @param block_number Číslo bloku.
 * @param data_length Délka dat.
 * @param data Ukazatel na data.
 * @param packet Ukazatel na výstupní paket.
 * @return size_t Velikost vytvořeného paketu.
 */
size_t create_data_packet(uint16_t block_number, size_t data_length, char* data, char** packet);

/**
 * @brief Vytvoří ERROR paket.
 * 
 * @param error_code Kód chyby.
 * @param error_msg Zpráva o chybě.
 * @param packet Ukazatel na výstupní paket.
 * @return size_t Velikost vytvořeného paketu.
 */
size_t create_error_packet(uint16_t error_code, char* error_msg, char** packet);

/**
 * @brief Odešle ERROR paket.
 * 
 * @param error_code Kód chyby.
 * @param error_msg Zpráva o chybě.
 * @param cliaddr Ukazatel na strukturu sockaddr_in klienta.
 * @param c_len Délka struktury sockaddr_in klienta.
 * @param sockfd Socket file descriptor.
 */
void send_error_packet(uint16_t error_code, char* error_msg, struct sockaddr_in *cliaddr, int c_len, int sockfd);

/**
 * @brief Vytvoří OACK paket.
 * 
 * @param option1 První volba.
 * @param value1 Hodnota první volby.
 * @param option2 Druhá volba.
 * @param value2 Hodnota druhé volby.
 * @param option3 Třetí volba.
 * @param value3 Hodnota třetí volby.
 * @param packet Ukazatel na výstupní paket.
 * @return size_t Velikost vytvořeného paketu.
 */
size_t create_oack_packet(char* option1, char* value1, char* option2, char* value2, char* option3, char* value3, char** packet);

/**
 * @brief Vytiskne chybovou zprávu.
 * 
 * @param err_msg Chybová zpráva.
 */
void print_error(char *err_msg);

/**
 * @brief Zpracuje příchozí TFTP packet.
 * 
 * @param rq_packet Ukazatel na strukturu Packet (WRQ/RRQ).
 * @param cliaddr Ukazatel na strukturu sockaddr_in klienta.
 * @param c_len Délka struktury sockaddr_in klienta.
 */
void handle_tftp_request(Packet* rq_packet, struct sockaddr_in *cliaddr, int c_len);
