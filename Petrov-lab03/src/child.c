#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFFER_SIZE 1024

int main() {
    float *shared_sum;
    key_t key = IPC_PRIVATE;
    int shm_id = shmget(key, sizeof(float), IPC_CREAT | 0666);

    // Присоединяем общую память
    shared_sum = (float*) shmat(shm_id, NULL, 0);
    if (shared_sum == (float*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    float sum = 0.0;

    // Читаем команды от родительского процесса
    while (read(STDIN_FILENO, buffer, BUFFER_SIZE) > 0) {
        char *token = strtok(buffer, " ");
        while (token != NULL) {
            sum += atof(token);
            token = strtok(NULL, " ");
        }
    }

    // Сохраняем сумму в общей памяти
    *shared_sum = sum;

    // Закрываем память и завершаем дочерний процесс
    shmdt(shared_sum);
    return 0;
}