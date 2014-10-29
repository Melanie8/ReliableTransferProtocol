/*
 * Written by :
 * Benoît Legat <benoit.legat@student.uclouvain.be>
 * Mélanie Sedda <melanie.sedda@student.uclouvain.be>
 * October 2014
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define PACKET_SIZE 520
#define PAYLOAD_SIZE 512
#define CRC_SIZE 4

#define SEQNUM_SIZE 8
#define MAX_SEQ (1 << SEQNUM_SIZE)

#define WINDOW_SIZE 5
#define MAX_WIN_SIZE ((1 << WINDOW_SIZE) - 1)

#define PTYPE_DATA 1
#define PTYPE_ACK  2

#define MIN(a,b) ((a) < (b) ?  (a) : (b))
#define MAX(a,b) ((a) < (b) ?  (b) : (a))

/* Reads all the arguments in argv and sets sber, splr, delay,
 * hostname and ports to the corresponding values.
 */
void read_args (int argc, char **argv, char **filename, int *sber, int *splr, int *delay, char **hostname, char **port, int *verbose_flag);

/* Opens filename and returns the file descriptor.
 * The bool write tells if want you to be able to
 * write in this file.
 */
int get_fd (const char *filename, bool write);

/* Modelisation of a packet : contains pointers towards each field of a packet */
struct packet {
  uint8_t type_and_window_size;
  uint8_t seq;
  uint16_t len;
  char payload[512];
  uint32_t crc;
} __attribute__((packed));

/* Closes a file descriptor. */
void close_fd (int fd);

/* Computes the CRC of the header + payload of a packet */
uint32_t crc_packet(const struct packet *pack);

bool valid_ack(struct packet *p);

/* Checks whether the third argument is between the two first one in modulo */
bool between_mod (int, int, int);

/* Computes the index in the window of the sequence number i, 
 * given lastack (x) and lastack_in_window
 */
int index_in_window (int x, int x_in_window, int i);

#endif
