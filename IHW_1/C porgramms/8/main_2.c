
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

void reverse(char buffer[], int n1, int n2) {
    size_t len = strlen(buffer);
    if (n2 > len) {
        n2 = len;
    }
    for (int j = n1; j <= n1 + (n2 - n1) / 2; ++j) {
        char letter = buffer[j];
        buffer[j] = buffer[n2 - (j - n1)];
        buffer[n2 - (j - n1)] = letter;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <n1> <n2>\n", argv[0]);
        exit(1);
    }

    int n1 = atoi(argv[1]);
    int n2 = atoi(argv[2]);

    if (n1 < 0 || n2 < 0 || n1 > n2) {
        printf("Invalid n1 and n2 values\n");
        exit(1);
    }

    char channel1[] = "channel1";
    char channel2[] = "channel2";
    int fd1, fd2;
    char buffer[5000];

    // Открываем канал для чтения
    fd1 = open(channel1, O_RDONLY);
    
    

    // Читаем данные из канала
    read(fd1, buffer, 5000);

    close(fd1);
    

    reverse(buffer, n1, n2);

    fd2 = open(channel2, O_WRONLY);
    if (fd2 < 0) {
        printf("Error: Cannot open channel 2.\n");
        exit(1);
    }
    printf("внутри второго процесса, %s\n", buffer);
    write(fd2, buffer, 5000);

    close(fd2);
    return 0;
}
