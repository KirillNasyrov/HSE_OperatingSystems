#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: %s input_file output_file n1 n2\n", argv[0]);
        exit(1);
    }

    char *input_file = argv[1];
    char *output_file = argv[2];
    int n1 = atoi(argv[3]);
    int n2 = atoi(argv[4]);

    if (n1 > n2) {
        printf("Error: n1 should be less than or equal to n2.\n");
        exit(1);
    }

    char buffer[BUFFER_SIZE];

    // Создаем именованные каналы
    char *channel1 = "channel1";
    char *channel2 = "channel2";
    mknod(channel1, 0666, 0);
    mknod(channel2, 0666, 0);

    // Создаем первый дочерний процесс, который читает данные из файла и передает их через канал 1
    pid_t pid1 = fork();
    if (pid1 == 0) {
        int fd_input = open(input_file, O_RDONLY);
        if (fd_input < 0) {
            printf("Error: Cannot open input file.\n");
            exit(1);
        }

        int fd_channel1 = open(channel1, O_WRONLY);
        if (fd_channel1 < 0) {
            printf("Error: Cannot open channel 1.\n");
            exit(1);
        }

        ssize_t bytes_read;
        while ((bytes_read = read(fd_input, buffer, BUFFER_SIZE)) > 0) {
            buffer[bytes_read] = 0;
            write(fd_channel1, buffer, bytes_read + 1);
        }

        close(fd_input);
        close(fd_channel1);

        exit(0);
    }

    // Создаем второй дочерний процесс, который переворачивает данные и передает их через канал 2
    pid_t pid2 = fork();
    if (pid2 == 0) {
        int fd_channel1 = open(channel1, O_RDONLY);
        if (fd_channel1 < 0) {
            printf("Error: Cannot open channel 1.\n");
            exit(1);
        }

        int fd_channel2 = open(channel2, O_WRONLY);
        if (fd_channel2 < 0) {
            printf("Error: Cannot open channel 2.\n");
            exit(1);
        }

        ssize_t bytes_read;
        while ((bytes_read = read(fd_channel1, buffer, BUFFER_SIZE)) > 0) {
            size_t len = strlen(buffer);
            if (n2 > len) {
                n2 = len;
            }
            for (int j = n1; j <= n1 + (n2 - n1) / 2; ++j) {
                char letter = buffer[j];
                buffer[j] = buffer[n2 - (j - n1)];
                buffer[n2 - (j - n1)] = letter;
            }
            write(fd_channel2, buffer, bytes_read);
        }
        close(fd_channel1);
        close(fd_channel2);

        exit(0);
    }
    // Создаем третий дочерний процесс, который записывает данный в файл
    pid_t pid3 = fork();
    if (pid3 == 0) {
        int fd_channel2 = open(channel2, O_RDONLY);
        if (fd_channel2 < 0) {
            printf("Error: Cannot open channel 2.\n");
            exit(1);
        }

        ssize_t bytes_read;
        bytes_read = read(fd_channel2, buffer, BUFFER_SIZE);
        if (bytes_read < 0){
            printf("Can\'t read string from pipe\n");
            exit(-1);
        }
        printf("result, %s\n", buffer);
        int fd_output = open(argv[2], O_WRONLY);
        write(fd_output, buffer, bytes_read);
        close(fd_channel2);
        exit(0);
    }
}
