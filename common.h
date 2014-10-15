#include <stdbool.h>

#define PACKET_SIZE 520
#define PAYLOAD_SIZE 512

#define PTYPE_DATA 1
#define PTYPE_ACK 2

void read_args (int argc, char **argv, char **filename, int *sber, int *splr, int *delay, char **hostname, char **port, int *verbose_flag);

int get_fd (const char *filename, bool write);
void close_fd (int fd);


uint32_t rc_crc32(uint32_t crc, const char *buf, size_t len);
