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
    struct hostent *host;
    struct sockaddr_in server_addr; // адрес сервера

    if (argc != 3) {
        fprintf(stderr,"usage: <client hostname> <port>\n");
        exit(1);
    }

    int port = atoi(argv[2]);

    if ((host = gethostbyname(argv[1])) == NULL) {  // получить адрес сервера
        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;    // системный порядок
    server_addr.sin_port = htons(port);  // сетевой порядок
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    memset(&(server_addr.sin_zero), '\0', 8);  // обнуляем остаток структуры

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }


    char *message = "app";
    if ((numbytes = send(sockfd, message, strlen(message), 0)) == -1) {
        perror("send");
    } else if (numbytes == 0) {
        printf("Соединение закрыто сервером.\n");
        close(sockfd);
        exit(1);
    }


    Coordinates recieved_coordinates;

    while (1) {

        char buffer[sizeof(Coordinates)];

        if ((numbytes = recv(sockfd, buffer, sizeof(Coordinates), 0)) == -1) {
            close(sockfd);
            perror("recv");
            exit(1);
        } else if (numbytes == 0) {
            printf("Соединение закрыто сервером.\n");
            close(sockfd);
            exit(1);
        }


        memcpy(&recieved_coordinates, buffer, sizeof(Coordinates));

        if (recieved_coordinates.found) {
            printf("Группа нашла клад в [%d][%d]\n", recieved_coordinates.x, recieved_coordinates.y);
            close(sockfd);
            return 0;
        }

        printf("Группа не нашла клад в [%d][%d]\n", recieved_coordinates.x, recieved_coordinates.y);

    }
}