#include <stdbool.h>
#include <stdint.h>

#define PACKET_SIZE 520
#define PAYLOAD_SIZE 512

#define PTYPE_DATA 1
#define PTYPE_ACK 2

#define MIN(a,b) ((a) < (b) ?  (a) : (b))
#define MAX_WIN_SIZE 31
#define MAX_SEQ (1 << 8)

void read_args (int argc, char **argv, char **filename, int *sber, int *splr, int *delay, char **hostname, char **port, int *verbose_flag);

int get_fd (const char *filename, bool write);
void close_fd (int fd);

struct packet {
  uint8_t type_and_window_size;
  uint8_t seq;
  uint16_t len;
  char payload[512];
  uint32_t crc;
} __attribute__((packed));

uint32_t rc_crc32(uint32_t crc, const char *buf, size_t len);
