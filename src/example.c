#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
 
#ifndef   NI_MAXHOST
#define   NI_MAXHOST 1025
#endif
 
int main(int argc, char **argv)
{
    struct addrinfo *result;
    struct addrinfo *res;
    struct addrinfo *hint = NULL;
    int error;
 
    if (argc > 1) {
      hint = (struct addrinfo *) malloc(sizeof(struct addrinfo));
      hint->ai_flags = 0;
      hint->ai_family = AF_INET6; // IPv6
      hint->ai_socktype = 0;
      hint->ai_protocol = IPPROTO_UDP;
      hint->ai_addrlen = 0;
      hint->ai_addr = NULL;
      hint->ai_canonname = NULL;
      hint->ai_next = NULL;
    }

    /* resolve the domain name into a list of addresses */
    error = getaddrinfo("www.example.com", NULL, hint, &result);
    if (error != 0)
    {   
        if (error == EAI_SYSTEM)
        {
            perror("getaddrinfo");
        }
        else
        {
            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
        }   
        exit(EXIT_FAILURE);
    }   
 
    /* loop over all returned results and do inverse lookup */
    printf("family IPv4 %d IPv6 %d any %d\n", AF_INET, AF_INET6, AF_UNSPEC);
    printf("socket STREAM %d DGRAM %d\n", SOCK_STREAM, SOCK_DGRAM);
    for (res = result; res != NULL; res = res->ai_next)
    {   
        char hostname[NI_MAXHOST];
 
        printf("f%d fam%d sock%d prot%d %d %d %d ai_canonname:%s\n", res->ai_flags, res->ai_family, res->ai_socktype, res->ai_protocol, res->ai_addrlen, res->ai_addr->sa_family, *(res->ai_addr->sa_data), res->ai_canonname);
        error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
        if (error != 0)
        {
            fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
            continue;
        }
        if (*hostname != '\0')
            printf("hostname: %s\n", hostname);
    }   
 
    freeaddrinfo(result);
    return 0;
}
