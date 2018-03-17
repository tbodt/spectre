#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "utils.h"

__attribute__((section(".rodata.transmit"), aligned(0x10000))) const char transmit_space[0x20000] = {1};
__asm__(".text\n.globl gadget\ngadget:\n"
        "movzbl (%rdx), %eax\n"
        "shl $9, %eax\n"
        "movl transmit_space(%eax), %eax\n"
        "retq\n");
__attribute__((constructor)) void init() {
    for (int i = 0; i < sizeof(transmit_space)/0x1000; i++)
        *(volatile char *) &transmit_space[i*0x1000];
}

void connection_thread(FILE *client) {
    char command[100];
    while (fgets(command, sizeof(command), client) != NULL) {
        long a, b;
        sscanf(command, "%ld %ld\n", &a, &b);
        fprintf(client, "%ld\n", a + b);
    }
    fclose(client);
}

char secret[100];

int main(int argc, char *const argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s port secret\n", argv[0]);
        return 0;
    }
    strcpy(secret, argv[2]);

    int sock = socket(PF_INET, SOCK_STREAM, 0); check("socket");
    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)); check("setsockopt");
    struct addrinfo *bind_addr;
    getaddrinfo("localhost", argv[1], NULL, &bind_addr); check("getaddrinfo");
    bind(sock, bind_addr->ai_addr, bind_addr->ai_addrlen); check("bind");
    listen(sock, 10); check("listen");
    printf("listening on port %s\n", argv[1]);
    for (;;) {
        struct sockaddr addr;
        socklen_t addr_len;
        int client_fd = accept(sock, &addr, &addr_len); check("accept");
        FILE *client = fdopen(client_fd, "r+"); check("fdopen");
        pthread_t thread;
        pthread_create(&thread, NULL, (void*(*)(void*)) connection_thread, client);
        pthread_detach(thread);
    }
}
