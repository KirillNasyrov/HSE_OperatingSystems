#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <mqueue.h>

#define QUEUE_NAME "/my_queue"

#define SEM_WRITE_NAME "semwrite"
#define SEM_READ_NAME "semread"

struct shared {
    int table[3][10];
    int current_x;
    int current_y;
    bool found;
};

int main(int argc, char **argv) {
    sem_t *semafor_write, *semafor_read;

    char *name = argv[1];

    mqd_t mq;
    char buffer[sizeof(struct shared)];
    struct mq_attr attr;

    // Определение атрибутов очереди сообщений
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(struct shared);
    attr.mq_curmsgs = 0;


    // Создание очереди сообщений
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);

    struct shared data;



    // создаем POSIX-семафор
    semafor_write = sem_open(SEM_WRITE_NAME, O_CREAT | O_RDWR, 0666, 1);
    if (semafor_write == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    semafor_read = sem_open(SEM_READ_NAME, O_CREAT | O_RDWR, 0666, 0);
    if (semafor_read == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }


    // Процесс группы
    for (;;) {
        sleep(1);
        sem_wait(semafor_read); // захватываем семафор


        // Получение структуры из очереди
        mq_receive(mq, buffer, sizeof(struct shared), NULL);
        memcpy(&data, buffer, sizeof(struct shared));


        if (data.found == true) {
            mq_send(mq, (char*)&data, sizeof(struct shared), 0);

            sem_post(semafor_write);

            // освобождаем семафор
            sem_close(semafor_write);
            sem_close(semafor_read);

            sem_unlink(SEM_WRITE_NAME);
            sem_unlink(SEM_READ_NAME);

            // Закрытие очереди сообщений
            mq_close(mq);
            mq_unlink(QUEUE_NAME);

            exit(EXIT_SUCCESS);
        }

        if (data.table[data.current_x][data.current_y] == 100) {
            data.found = true;

            mq_send(mq, (char*)&data, sizeof(struct shared), 0);
            printf("группа <%s> нашла клад в [%d][%d]\n", name, data.current_x, data.current_y);
            fflush(stdout);
            sem_post(semafor_write); // освобождаем семафор

            sem_close(semafor_write);
            sem_close(semafor_read);

            sem_unlink(SEM_WRITE_NAME);
            sem_unlink(SEM_READ_NAME);

            // Закрытие очереди сообщений
            mq_close(mq);
            mq_unlink(QUEUE_NAME);

            exit(EXIT_SUCCESS);
        } else {
            mq_send(mq, (char*)&data, sizeof(struct shared), 0);

            printf("группа <%s> не нашла клад в [%d][%d]\n", name, data.current_x, data.current_y);
            fflush(stdout);
            sem_post(semafor_write); // освобождаем семафор
        }
    }
}
