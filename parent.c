#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <math.h>

#define BUFFER_SIZE 1024

// Функция для преобразования числа с плавающей запятой в строку
void float_to_string(float value, char* buffer, int precision) {
    int int_part = (int)value;
    float fractional_part = value - int_part;

    // Сначала добавляем целую часть
    int len = 0;
    if (int_part == 0) {
        buffer[len++] = '0';
    } else {
        int temp = int_part;
        if (temp < 0) {
            buffer[len++] = '-';
            temp = -temp;
        }
        int digits[10]; // Храним цифры целой части
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = temp % 10;
            temp /= 10;
        }
        // Заполняем буфер целыми цифрами в правильном порядке
        for (int i = digit_count - 1; i >= 0; i--) {
            buffer[len++] = digits[i] + '0';
        }
    }

    // Добавляем дробную часть
    if (precision > 0) {
        buffer[len++] = '.'; // Добавляем десятичную точку
        for (int i = 0; i < precision; i++) {
            fractional_part *= 10;
            int fractional_digit = (int)fractional_part;
            buffer[len++] = fractional_digit + '0';
            fractional_part -= fractional_digit;
        }
    }

    buffer[len] = '\0'; // Завершаем строку
}

void write_to_file(const char *filename, const char *data, size_t len) {
    int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (fd != -1) {
        write(fd, data, len); // Пишем данные в файл
        close(fd);
    } else {
        const char *error_msg = "Ошибка при открытии файла\n";
        write(2, error_msg, strlen(error_msg)); // Записываем сообщение об ошибке в stderr
    }
}

int main(int argc, char *argv[]) {
    int pipe1[2]; // pipe для передачи данных от родителя к дочернему
    pid_t pid;

    if (argc != 2) {
        const char *error_msg = "Использование: <имя_файла>\n";
        write(2, error_msg, strlen(error_msg)); // Печатаем сообщение об ошибке
        exit(EXIT_FAILURE);
    }

    // Создаем pipes
    if (pipe(pipe1) == -1) {
        const char *error_msg = "Ошибка при создании pipe\n";
        write(2, error_msg, strlen(error_msg));
        exit(EXIT_FAILURE);
    }

    // Создаем дочерний процесс
    pid = fork();
    if (pid < 0) {
        const char *error_msg = "Ошибка при fork\n";
        write(2, error_msg, strlen(error_msg));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Дочерний процесс
        close(pipe1[1]); // Закрываем запись в pipe1

        char buffer[BUFFER_SIZE];
        while (1) {
            ssize_t bytesRead = read(pipe1[0], buffer, BUFFER_SIZE);
            if (bytesRead <= 0) {
                break; // Ошибка или конец потока
            }

            buffer[bytesRead] = '\0'; // Завершаем строку
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

            // Формируем строку результата
            char result[50];
            float_to_string(sum, result, 2); // Преобразуем сумму в строку
            strcat(result, "\n"); // Добавляем новую строку

            write_to_file(argv[1], result, strlen(result)); // Записываем результат в файл
        }

        close(pipe1[0]);
        exit(EXIT_SUCCESS);
    } else { // Родительский процесс
        close(pipe1[0]); // Закрываем чтение из pipe1

        char input[BUFFER_SIZE];
        while (1) {
            const char *prompt = "Введите числа (или 'exit' для выхода): ";
            write(1, prompt, strlen(prompt)); // 1 - файловый дескриптор для stdout

            ssize_t bytesRead = read(0, input, BUFFER_SIZE); // 0 - файловый дескриптор для stdin
            if (bytesRead <= 0) {
                break; // Ошибка или конец ввода
            }

            input[bytesRead - 1] = '\0'; // Удаляем символ новой строки в конце (если есть)

            // Отправляем данные дочернему процессу
            write(pipe1[1], input, strlen(input) + 1);

            if (strcmp(input, "exit") == 0) {
                break;
            }
        }

        close(pipe1[1]);
        wait(NULL); // Ожидаем завершения дочернего процесса
        exit(EXIT_SUCCESS);
    }
}