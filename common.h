#include <stdbool.h>
#include <stdint.h>

#define PACKET_SIZE 520
#define PAYLOAD_SIZE 512
#define SEQNUM_SIZE 8
#define WINDOW_SIZE 5
#define BUFFER_SIZE 31 // = 2^WINDOW_SIZE-1
/* CA SERAIT PEUT-ETRE PAS MAL DE FAIRE DES DEFINE POUR LES TAILLES DE CHAQUE CHAMP EN FAIT POUR QUE CA SOIT PLUS GENERIQUE */

#define PTYPE_DATA 1
#define PTYPE_ACK 2

#define MIN(a,b) ((a) < (b) ?  (a) : (b))

void read_args (int argc, char **argv, char **filename, int *sber, int *splr, int *delay, char **hostname, char **port, int *verbose_flag);

int get_fd (const char *filename, bool write);
void close_fd (int fd);


uint32_t rc_crc32(uint32_t crc, const char *buf, size_t len);
