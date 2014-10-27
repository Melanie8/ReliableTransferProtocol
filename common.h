/*
 * Written by :
 * Benoît Legat <benoit.legat@student.uclouvain.be>
 * Mélanie Sedda <melanie.sedda@student.uclouvain.be>
 * October 2014
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define PACKET_SIZE 520
#define PAYLOAD_SIZE 512
#define SEQNUM_SIZE 8
#define N 256           // pow(2,SEQNUM_SIZE)
#define WINDOW_SIZE 5
#define BUFFER_SIZE 31  // pow(2,WINDOW_SIZE)-1
/* CA SERAIT PEUT-ETRE PAS MAL DE FAIRE DES DEFINE POUR LES TAILLES DE CHAQUE CHAMP EN FAIT POUR QUE CA SOIT PLUS GENERIQUE */

#define PTYPE_DATA 1
#define PTYPE_ACK 2

#define MIN(a,b) ((a) < (b) ?  (a) : (b))
#define MAX(a,b) ((a) < (b) ?  (b) : (a))
#define MAX_WIN_SIZE 31
#define MAX_SEQ (1 << 8)

/* Reads all the arguments in argv and sets sber, splr, delay,
 * hostname and ports to the corresponding values.
 */
void read_args (int argc, char **argv, char **filename, int *sber, int *splr, int *delay, char **hostname, char **port, int *verbose_flag);

/* Opens filename and returns the file descriptor.
 * The bool write tells if want you to be able to
 * write in this file.
 */
int get_fd (const char *filename, bool write);

struct packet {
  uint8_t type_and_window_size;
  uint8_t seq;
  uint16_t len;
  char payload[512];
  uint32_t crc;
} __attribute__((packed));

/* Closes a file descriptor. */
void close_fd (int fd);

/* */
uint32_t rc_crc32(const struct packet *pack);
bool valid_ack(struct packet *p);
