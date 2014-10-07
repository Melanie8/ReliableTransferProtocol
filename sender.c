#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

/* Flag set by ‘--verbose’. */
static int verbose_flag = 0;

int main (int argc, char **argv) {
  int c = -1;
  char *filename = NULL;
  int sber  = 0;
  int splr  = 0;
  int delay = 0;
  char *hostname = NULL;
  char *port = NULL;

  while (1) {
    static struct option long_options[] =
    {
      /* These options set a flag. */
      {"verbose", no_argument,       &verbose_flag, 1},
      {"brief"  , no_argument,       &verbose_flag, 0},
      /* These options don't set a flag.
         We distinguish them by their indices. */
      {"file",    required_argument, NULL, 'f'},
      {"sber",    required_argument, NULL, 'e'},
      {"splr",    required_argument, NULL, 'l'},
      {"delay",   required_argument, NULL, 'd'},
      {NULL, 0, NULL, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    char *endptr = NULL;
    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != NULL)
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;

      case 'f':
        filename = optarg;
        break;

      case 'e':
        sber = strtol(optarg, &endptr, 10);
        if (*optarg == '\0' || *endptr != '\0' || sber < 0 || sber > 1000) {
          fprintf(stderr, "Invalid sber\n");
          exit(1);
        }
        break;

      case 'l':
        splr = strtol(optarg, &endptr, 10);
        if (*optarg == '\0' || *endptr != '\0' || sber < 0 || sber > 1000) {
          fprintf(stderr, "Invalid splr\n");
          exit(1);
        }
        break;

      case 'd':
        delay = strtol(optarg, &endptr, 10);
        if (*optarg == '\0' || *endptr != '\0' || delay < 0) {
          fprintf(stderr, "Invalid delay\n");
          exit(1);
        }
        break;

      case '?':
        /* getopt_long already printed an error message. */
        break;

      default:
        abort();
    }
  }

  /* Instead of reporting ‘--verbose’
     and ‘--brief’ as they are encountered,
     we report the final status resulting from them. */
  if (verbose_flag)
    puts ("verbose flag is set");

  if (optind >= argc) {
    fprintf(stderr, "Missing hostname\n");
    exit(1);
    hostname = argv[optind++];
  } else {
    hostname = argv[optind++];
  }

  if (optind >= argc) {
    fprintf(stderr, "Missing port\n");
    exit(1);
  } else {
    port = argv[optind++];
  }

  if (optind < argc) {
    printf ("unused arguments: ");
    while (optind < argc)
      printf ("%s ", argv[optind++]);
    putchar ('\n');
  }

  exit (0);
}
