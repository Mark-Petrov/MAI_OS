#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int pipe1[2]; // pipe для передачи данных от родителя к дочернему
    pid_t pid;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s имя_файла\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Создаем pipes
    if (pipe(pipe1) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Создаем дочерний процесс
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Дочерний процесс
        close(pipe1[1]); // Закрываем запись в pipe1

        // Передаем имя файла дочернему процессу
        char filename[256];
        strcpy(filename, argv[1]);

        // Читаем данные от родителя
        char buffer[BUFFER_SIZE];
        while (1) {
            read(pipe1[0], buffer, BUFFER_SIZE);
            if (strcmp(buffer, "exit") == 0) {
                break; // Выход при получении команды exit
            }

            // Обработка чисел
            float sum = 0.0;
            char *token = strtok(buffer, " ");
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
        }

        close(pipe1[0]);
        exit(EXIT_SUCCESS);
    } else { // Родительский процесс
        close(pipe1[0]); // Закрываем чтение из pipe1

        char input[BUFFER_SIZE];
        while (1) {
            printf("Введите числа (или 'exit' для выхода): ");
            fgets(input, BUFFER_SIZE, stdin);
            input[strcspn(input, "\n")] = 0; 

            // Отправляем данные дочернему процессу
            write(pipe1[1], input, strlen(input) + 1);

            if (strcmp(input, "exit") == 0) {
                break; 
            }
        }

        close(pipe1[1]);
        wait(NULL); 
        exit(EXIT_SUCCESS);
    }
}