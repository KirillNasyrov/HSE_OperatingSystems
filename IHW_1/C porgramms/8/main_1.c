
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 5000

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s input_file output_file\n", argv[0]);
        exit(1);
    }

    char *input_file = argv[1];
    char *output_file = argv[2];

    char buffer[BUFFER_SIZE];

    char channel1[] = "channel1";
    char channel2[] = "channel2";
    int fd1, fd2;

    // Создаем именованный канал с правами доступа 0666
    mknod(channel1, 0666, 0);
    mknod(channel2, 0666, 0);

    // Открываем канал для записи
    fd1 = open(channel1, O_WRONLY);

    int fd_input, fd_output;
    if ((fd_input = open(input_file, O_RDONLY)) < 0) {
        printf("Error: Cannot open input file.\n");
        exit(1);
    }

    ssize_t bytes_read;
    if ((bytes_read = read(fd_input, buffer, BUFFER_SIZE)) == -1) {
        printf("Error: Cannot read input file.\n");
        exit(1);
    }
    buffer[bytes_read] = 0;

    // Записываем данные в канал
    write(fd1, buffer, BUFFER_SIZE);

    // Закрываем канал и удаляем его
    close(fd1);

    fd2 = open(channel2, O_RDONLY);
    if (fd2 < 0) {
        printf("Error: Cannot open channel 2.\n");
        exit(1);
    }
    char buffer2[BUFFER_SIZE];

    // Читаем данные из канала
    bytes_read = read(fd2, buffer2, 5000);
    printf("внутри первого процесса, %s\n", buffer2);
    fd_output = open(output_file, O_WRONLY);
    write(fd_output, buffer2, bytes_read);
    close(fd2);
    return 0;
}