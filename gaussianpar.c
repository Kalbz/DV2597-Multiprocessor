#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MATRIX_SIZE 2048
#define NUM_THREADS 32

double matrix[MATRIX_SIZE][MATRIX_SIZE];
double b[MATRIX_SIZE];
double y[MATRIX_SIZE];
int maxnum = 15;
char* Init = "rand";
int PRINT = 0;

pthread_barrier_t barrier;

void initialize_matrix_and_b() {
    printf("\nsize      = %dx%d ", MATRIX_SIZE, MATRIX_SIZE);
    printf("\nmaxnum    = %d \n", maxnum);
    printf("Init      = %s \n", Init);
    printf("Initializing matrix...");

    srand(69); // Initialize random number generator
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if (i == j)
                matrix[i][j] = (double)(rand() % maxnum) + 5.0; // Diagonal dominance
            else
                matrix[i][j] = (double)(rand() % maxnum) + 1.0;
        }
        b[i] = 2.0; // Initialize vector b
        y[i] = 0;   // Initialize vector y
    }

    printf("done \n\n");
    if (PRINT == 1) {
        // Add function to print the matrix and vectors here, similar to the sequential version
    }
}

void print_matrix() {
    printf("Matrix A:\n");
    for (int i = 0; i < MATRIX_SIZE; i++) {
        printf("[");
        for (int j = 0; j < MATRIX_SIZE; j++)
            printf(" %5.2f,", matrix[i][j]);
        printf("]\n");
    }
    printf("Vector b:\n[");
    for (int j = 0; j < MATRIX_SIZE; j++)
        printf(" %5.2f,", b[j]);
    printf("]\n");
    printf("Vector y:\n[");
    for (int j = 0; j < MATRIX_SIZE; j++)
        printf(" %5.2f,", y[j]);
    printf("]\n\n");
}


void* gaussian_elimination_thread(void* arg) {
    int thread_id = ((int)arg);

    for (int k = 0; k < MATRIX_SIZE - 1; k++) {
        if (thread_id == 0) {
            // Division step (executed by a single thread)
            for (int j = k + 1; j < MATRIX_SIZE; j++) {
                matrix[k][j] /= matrix[k][k];
            }
            y[k] = b[k] / matrix[k][k];
            matrix[k][k] = 1;
        }

        pthread_barrier_wait(&barrier);

        // Calculate the number of rows remaining to be processed
        int rowsRemaining = MATRIX_SIZE - (k + 1);

        // Calculate the number of rows per thread, rounding up to distribute any remainder
        int rowsPerThread = (rowsRemaining + NUM_THREADS - 1) / NUM_THREADS;

        // Calculate the start and end indices for this thread
        int start = k + 1 + rowsPerThread * thread_id;
        int end = start + rowsPerThread;

        // Adjust end for the last thread or if the calculated end exceeds MATRIX_SIZE
        if (thread_id == NUM_THREADS - 1 || end > MATRIX_SIZE) {
            end = MATRIX_SIZE;
        }
        for (int i = start; i < end; i++) {
            for (int j = k + 1; j < MATRIX_SIZE; j++) {
                matrix[i][j] -= matrix[i][k] * matrix[k][j];
            }
            b[i] -= matrix[i][k] * y[k];
            matrix[i][k] = 0;
        }

        pthread_barrier_wait(&barrier);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    initialize_matrix_and_b();

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        pthread_create(&threads[i], NULL, gaussian_elimination_thread, (void*)&thread_args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);

    // Final steps...
        if (PRINT == 1) {
        print_matrix();
    }
    return 0;
}