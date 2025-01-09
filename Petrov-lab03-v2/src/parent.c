#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define SHM_SIZE 1024 // размер общей памяти

// Функция для вывода ошибок в стандартный поток ошибок
void print_error(const char *msg) {
    write(2, msg, strlen(msg)); // 2 - файловый дескриптор для stderr
}

// Функция для записи данных в файл
void write_to_file(const char *filename, const char *data) {
    int file = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (file != -1) {
        write(file, data, strlen(data));
        close(file);
    } else {
        print_error("Ошибка при открытии файла\n");
    }
}

// Функция для преобразования дробного числа в строку
void format_float(char *buffer, size_t size, float sum) {
    int integer_part = (int)sum; // целая часть
    int fractional_part = (int)((sum - integer_part) * 100 + 0.5); // округление до 2-х десятичных
    
    int len = 0;
    
    // Добавляем "Сумма: "
    const char *prefix = "Сумма: ";
    for (size_t i = 0; i < strlen(prefix) && len < size - 1; i++) {
        buffer[len++] = prefix[i];
    }
    
    // Добавляем целую часть
    if (integer_part == 0 && len < size - 1) {
        buffer[len++] = '0';
    } else {
        // Преобразуем целую часть в строку
        char int_buffer[10]; // Достаточно для 32-битного int
        int int_len = 0;

        // Формируем строку целой части в обратном порядке
        if (integer_part < 0) {
            integer_part = -integer_part;
            buffer[len++] = '-';
        }

        do {
            int_buffer[int_len++] = (integer_part % 10) + '0';
            integer_part /= 10;
        } while (integer_part > 0);

        // Добавляем целую часть в правильном порядке
        for (int j = int_len - 1; j >= 0 && len < size - 1; j--) {
            buffer[len++] = int_buffer[j];
        }
    }

    // Добавляем дробную часть, если есть место
    if (len < size - 3) { // 3 для "00\n"
        buffer[len++] = '.';
        buffer[len++] = (fractional_part / 10) + '0'; // Первая цифра дробной части
        buffer[len++] = (fractional_part % 10) + '0'; // Вторая цифра дробной части
        buffer[len++] = '\n'; // Символ новой строки
        buffer[len] = '\0'; // Завершаем строку
    } else {
        print_error("Ошибка форматирования строки\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_error("Использование: <имя_файла>\n");
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];
    int shmid;
    char *shm_ptr;

    // Создаем сегмент общей памяти
    shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        print_error("Ошибка при создании сегмента общей памяти\n");
        exit(EXIT_FAILURE);
    }

    // Порождаем дочерний процесс
    pid_t pid = fork();
    if (pid < 0) {
        print_error("Ошибка при вызове fork\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Дочерний процесс
        shm_ptr = shmat(shmid, NULL, 0);
        if (shm_ptr == (char *)(-1)) {
            print_error("Ошибка при подключении к сегменту общей памяти\n");
            exit(EXIT_FAILURE);
        }

        while (1) {
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

                // Формируем строку для записи в файл
                char result[50];
                format_float(result, sizeof(result), sum);
                write_to_file(filename, result);

                // Очищаем содержимое общей памяти
                memset(shm_ptr, 0, SHM_SIZE);
            }
        }
        shmdt(shm_ptr);
        exit(EXIT_SUCCESS);
    } else { // Родительский процесс
        shm_ptr = shmat(shmid, NULL, 0);
        if (shm_ptr == (char *)(-1)) {
            print_error("Ошибка при подключении к сегменту общей памяти\n");
            exit(EXIT_FAILURE);
        }

        char input[SHM_SIZE];
        while (1) {
            const char *prompt = "Введите числа (или 'exit' для выхода): ";
            write(1, prompt, strlen(prompt)); // 1 - файловый дескриптор для stdout

            read(0, input, SHM_SIZE); // 0 - файловый дескриптор для stdin
            input[strcspn(input, "\n")] = 0; // Удаляем символ новой строки

            // Записываем данные в общую память
            strncpy(shm_ptr, input, SHM_SIZE);

            if (strcmp(input, "exit") == 0) { // Выход при получении команды exit
                break;
            }
        }

        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        wait(NULL); // Ожидаем завершения дочернего процесса
        exit(EXIT_SUCCESS);
    }
}