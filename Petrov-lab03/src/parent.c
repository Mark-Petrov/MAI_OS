#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define SHM_SIZE 1024 // размер общей памяти

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s имя_файла\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];
    int shmid;
    char *shm_ptr;

    // Создаем сегмент общей памяти
    shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Порождаем дочерний процесс
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Дочерний процесс
        // Прикрепляем сегмент общей памяти
        shm_ptr = shmat(shmid, NULL, 0);
        if (shm_ptr == (char *)(-1)) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        while (1) {
            // Ждем данные от родителя
            if (strlen(shm_ptr) > 0) {
                if (strcmp(shm_ptr, "exit") == 0) {
                    break; // Выход при получении команды exit
                }

                // Обработка чисел
                float sum = 0.0;
                char *token = strtok(shm_ptr, " ");
                while (token != NULL) {
                    sum += atof(token);
                    token = strtok(NULL, " ");
                }

                // Записываем результат в файл
                FILE *file = fopen(filename, "a");
                if (file != NULL) {
                    fprintf(file, "Сумма: %.2f\n", sum);
                    fclose(file);
                } else {
                    perror("fopen");
                }

                // Очищаем содержимое общей памяти
                memset(shm_ptr, 0, SHM_SIZE);
            }
        }
        // Отключаем сегмент общей памяти
        shmdt(shm_ptr);
        exit(EXIT_SUCCESS);
    } else { // Родительский процесс
        // Прикрепляем сегмент общей памяти
        shm_ptr = shmat(shmid, NULL, 0);
        if (shm_ptr == (char *)(-1)) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        char input[SHM_SIZE];
        while (1) {
            printf("Введите числа (или 'exit' для выхода): ");
            fgets(input, SHM_SIZE, stdin);
            input[strcspn(input, "\n")] = 0; // Удаляем символ новой строки

            // Записываем данные в общую память
            strncpy(shm_ptr, input, SHM_SIZE);

            if (strcmp(input, "exit") == 0) {
                break; // Выход при получении команды exit
            }
        }

        // Отключаем сегмент общей памяти и удаляем его
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        wait(NULL); // Ожидаем завершения дочернего процесса
        exit(EXIT_SUCCESS);
    }
}