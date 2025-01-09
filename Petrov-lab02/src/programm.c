#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include <stdio.h> // Временно для отладки

#define MAX_THREADS 16
#define MAX_POINTS 1000

typedef struct {
    double x, y, z;
} Point;

typedef struct {
    Point *points;
    int num_points;
    double max_area;
    pthread_mutex_t *mutex;
    int thread_id;
} ThreadData;

double triangle_area(Point a, Point b, Point c) {
    double ab = sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2));
    double ac = sqrt(pow(c.x - a.x, 2) + pow(c.y - a.y, 2) + pow(c.z - a.z, 2));
    double bc = sqrt(pow(c.x - b.x, 2) + pow(c.y - b.y, 2) + pow(c.z - b.z, 2));
    double s = (ab + ac + bc) / 2;
    double area = sqrt(s * (s - ab) * (s - ac) * (s - bc));
    
    // Отладочный вывод
    // printf("Triangle (A: (%f, %f, %f), B: (%f, %f, %f), C: (%f, %f, %f)) - Area: %f\n",
    //         a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, area);

    return area;
}

void *find_max_triangle_area(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    double max_area = 0.0;

    for (int i = data->thread_id; i < data->num_points; i += MAX_THREADS) {
        for (int j = i + 1; j < data->num_points; j++) {
            for (int k = j + 1; k < data->num_points; k++) {
                double area = triangle_area(data->points[i], data->points[j], data->points[k]);
                if (area > max_area) {
                    max_area = area;
                }
            }
        }
    }

    pthread_mutex_lock(data->mutex);
    if (max_area > data->max_area) {
        data->max_area = max_area;
    }
    pthread_mutex_unlock(data->mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        const char *error_msg = "Usage: <number_of_threads> <input_file>\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads > MAX_THREADS || num_threads < 1) {
        const char *error_msg = "Number of threads should be between 1 and 16\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    Point points[MAX_POINTS]; // Массив фиксированного размера
    int num_points = 0;

    // Открываем файл с входными данными
    int fd = open(argv[2], O_RDONLY);
    if (fd < 0) {
        const char *error_msg = "Error opening input file\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    // Чтение точек из файла
    char buffer[256];
    while (read(fd, buffer, sizeof(buffer)) > 0) {
        char *line = strtok(buffer, "\n");
        while (line) {
            if (num_points >= MAX_POINTS) {
                break;
            }
            
            sscanf(line, "%lf %lf %lf", &points[num_points].x, &points[num_points].y, &points[num_points].z);
            num_points++;
            line = strtok(NULL, "\n");
        }
    }
    close(fd);

    double max_area = 0.0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];

    for (int i = 0; i < num_threads; i++) {
        thread_data[i] = (ThreadData){.points = points, .num_points = num_points, .max_area = 0.0, .mutex = &mutex, .thread_id = i};
        pthread_create(&threads[i], NULL, find_max_triangle_area, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < num_threads; i++) {
        if (thread_data[i].max_area > max_area) {
            max_area = thread_data[i].max_area;
        }
    }

    // Формируем вывод
    char output[100];
    int len = sprintf(output, "Maximum triangle area: %f\n", max_area);
    write(STDOUT_FILENO, output, len);

    pthread_mutex_destroy(&mutex);
    return 0;
}