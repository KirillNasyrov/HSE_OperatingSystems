#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAX_BUFFER_SIZE 1024

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
    if (argc != 5) {
        printf("Usage: %s input_file output_file n1 n2\n", argv[0]);
        exit(1);
    }

    int n1 = atoi(argv[3]);
    int n2 = atoi(argv[4]);

    if (n1 < 0 || n2 < 0 || n1 > n2) {
        printf("Invalid n1 and n2 values\n");
        exit(1);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Cannot open file: %s\n", argv[1]);
        exit(1);
    }

    int pipe_fd[2][2]; // 2D array for 2 pipes
    pid_t pid[2];

    // Create 2 pipes
    if (pipe(pipe_fd[0]) == -1 || pipe(pipe_fd[1]) == -1) {
        perror("pipe");
        exit(1);
    }

    int read_bytes;
    // Create 2 child processes
    for (int i = 0; i < 2; i++) {
        pid[i] = fork();

        if (pid[i] < 0) {
            perror("Can\'t fork child");
            exit(1);
        } else if (pid[i] == 0) {
            // Child process
            if (i == 0) {
                // First child process reads from file and writes to first pipe
                char buffer[MAX_BUFFER_SIZE];
                close(pipe_fd[0][0]);
                while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    buffer[read_bytes] = 0;
                    if (write(pipe_fd[0][1], buffer, read_bytes + 1) == -1) {
                        perror("write");
                        exit(1);
                    }
                }
                close(pipe_fd[0][1]);

                // Fisrt child process reads from second pipe and writes to output file
                close(pipe_fd[1][1]);
                read_bytes = read(pipe_fd[1][0], buffer, sizeof(buffer));
                if (read_bytes < 0){
                    printf("Can\'t read string from pipe\n");
                    exit(-1);
                }
                printf("result, %s\n", buffer);
                int fd_output = open(argv[2], O_WRONLY);
                write(fd_output, buffer, read_bytes);
                close(pipe_fd[1][0]);

                exit(0);
            } else if (i == 1) {
                // Second child process reads from first pipe and writes to second pipe
                char buffer[MAX_BUFFER_SIZE];
                close(pipe_fd[0][1]);
                close(pipe_fd[1][0]);
                while ((read_bytes = read(pipe_fd[0][0], buffer, sizeof(buffer))) > 0) {
                    // Reverse the substring from n1 to n2
                    reverse(buffer, n1, n2);

                    if (write(pipe_fd[1][1], buffer, read_bytes) == -1) {
                        perror("write");
                        exit(1);
                    }
                }
                close(pipe_fd[0][0]);
                close(pipe_fd[1][1]);
                exit(0);
            }
        }
    }
}