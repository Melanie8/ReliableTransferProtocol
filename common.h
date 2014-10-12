#include <stdbool.h>

#define BUF_SIZE 100

void read_args (int argc, char **argv, char **filename, int *sber, int *splr, int *delay, char **hostname, char **port, int *verbose_flag);

int get_fd (const char *filename, bool write);
void close_fd (int fd);
