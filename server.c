#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(void) {
    int s_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
        .sin_addr   = {.s_addr = INADDR_ANY},
    };
    int addrlen = sizeof(addr);

    int ret = bind(s_fd, (struct sockaddr *)&addr, sizeof(addr));
    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t recv = recvfrom(s_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (recv > 0) {
            buffer[recv] = '\0';
            printf("Received message: %s\n", buffer);

            const char *reply = "Message received";
            sendto(s_fd, reply, strlen(reply), 0, (struct sockaddr *)&addr, addrlen);
        }
    }

    close(s_fd);

    return 0;
}
