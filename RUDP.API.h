#define SYN 'S'
#define ACK 'A'
#define SYNACK 'Y'
#define LASTACKIN3 'L'
#define FIN 'F'
#define DATA 'D'
#define TIME_OUT 4
#define MAX_ATTEMPTS 5
#define HEADER_SIZE 16
#define RECEIVER_PORT 8888
#define DATA_SIZE 2 * 1024 * 1024
#define MAX_REQ 100
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define DATA_SIZE 2 * 1024 * 1024
#define MAX_DATA_LENGTH 1024
#include <stdio.h> //
#include <stdlib.h> //
#include <string.h> //
#include <sys/select.h> //
#include <sys/socket.h> //
#include <sys/time.h> //
#include <unistd.h> //
#include <time.h>
#include <arpa/inet.h> //
#include <errno.h> //
#include <netinet/in.h>
/*This header file will declare all the methoods in our rudp protocol.
In this file we also hold the struckt of the rudp socket.
*/

/*Struct for the header of our rudp  packets.
  the packet header will hold:
  1)the length of the data that we are sending to check that all the data arrived.
  2)the checksum of the data to check that the data received correctly.
  3_the flags of the packet, can be:
  a)syn  b)ack  c)synack  d)data  e)fin
*/
typedef struct
{
    size_t length;
    unsigned short int checksum;
    char flags;
} Header;

/**
 * The declaration of the methods of rudp protocol :
 * Note that the full explation are above the functions in api.c.
 */
int rudp_connect(int sock, struct sockaddr_in *addr);

int rudp_socket();

int rudp_send(int sock, Header *packet, size_t packet_size, const struct sockaddr_in *dest_addr);

void rudp_sendack(int sock, struct sockaddr_in *addr);

int rudp_recv(int sock, void *buffer, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

int rudp_close(int sock, const struct sockaddr_in *server_addr);

unsigned short int calculate_checksum(void *data, unsigned int bytes);

char *util_generate_random_data(unsigned int size);

