#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

typedef struct {
    int x;
    int y;
    bool found;
} Coordinates;

int main(int argc, char *argv[])
{
    int sockfd;
    ssize_t numbytes;
    struct hostent *he;
    struct sockaddr_in server_addr; // адрес сервера

    if (argc != 5) {
        fprintf(stderr,"usage: <client hostname> <port> <x> <y>\n");
        exit(1);
    }

    int port = atoi(argv[2]);
    int x = atoi(argv[3]);
    int y = atoi(argv[4]);

    if ((he=gethostbyname(argv[1])) == NULL) {  // получить адрес сервера
        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;    // системный порядок
    server_addr.sin_port = htons(port);  // сетевой порядок
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(server_addr.sin_zero), '\0', 8);  // обнуляем остаток структуры

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    Coordinates recieved_coordinates;

    while (1) {

        char buffer[sizeof(Coordinates)];

        if ((numbytes = recv(sockfd, buffer, sizeof(Coordinates), 0)) == -1) {
            perror("recv");
            exit(1);
        }
        memcpy(&recieved_coordinates, buffer, sizeof(Coordinates));

        if (recieved_coordinates.found) {
            printf("Клад найден\n");
            close(sockfd);
            return 0;
        }

        sleep(4);

        if (recieved_coordinates.x == x && recieved_coordinates.y == y) {
            printf("Группа нашла клад в [%d][%d]\n", recieved_coordinates.x, recieved_coordinates.y);
            recieved_coordinates.found = true;
            memcpy(buffer, &recieved_coordinates, sizeof(Coordinates));
        } else {

            printf("Группа не нашла клад в [%d][%d]\n", recieved_coordinates.x, recieved_coordinates.y);
        }

        if (send(sockfd, buffer, sizeof(Coordinates), 0) == -1) {
            perror("send");
        }

        if (recieved_coordinates.found) {
            close(sockfd);
            return 0;
        }

    }
}