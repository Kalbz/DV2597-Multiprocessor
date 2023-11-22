#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define KILO (1024)
#define MEGA (1024*1024)
#define MAX_ITEMS (64*MEGA)
#define swap(v, a, b) {unsigned tmp; tmp=v[a]; v[a]=v[b]; v[b]=tmp;}
#define MAX_THREADS 32
#define THRESHOLD 1000  // Threshold for switching back to sequential sort
#define SAMPLE_SIZE 25 // Size of the sample for pivot selection

typedef struct {
    int *array;
    int left;
    int right;
} QuickSortArgs;

int active_threads = 0;
int max_threads = 32; // Maximum number of threads to be used
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int compare_ints(const void* a, const void* b) {
    int arg1 = *(const int*)a;
    int arg2 = *(const int*)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

int select_pivot(int arr[], int low, int high) {
    int sample[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; ++i) {
        sample[i] = arr[low + rand() % (high - low)];
    }
    qsort(sample, SAMPLE_SIZE, sizeof(int), compare_ints);
    return sample[SAMPLE_SIZE / 2]; // Return the median of the sample
}

int partition(int arr[], int low, int high) {
    int pivot;
    if (high - low > THRESHOLD) {
        pivot = select_pivot(arr, low, high);
    } else {
        pivot = arr[high];
    }
    
    int i = low - 1;
    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(arr, i, j);
        }
    }
    swap(arr, i + 1, high);
    return (i + 1);
}

void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

void *quick_sort_parallel(void *args) {
    QuickSortArgs *qa = (QuickSortArgs *) args;
    int low = qa->left;
    int high = qa->right;
    int *arr = qa->array;

    if (low < high) {
        int pi = partition(arr, low, high);
        QuickSortArgs args1 = {arr, low, pi - 1};
        QuickSortArgs args2 = {arr, pi + 1, high};

        pthread_t thread;
        int created_thread = 0;

        pthread_mutex_lock(&mutex);
        if (active_threads < max_threads) {
            active_threads++;
            created_thread = 1;
            pthread_mutex_unlock(&mutex);

            pthread_create(&thread, NULL, quick_sort_parallel, &args1);
            quick_sort_parallel(&args2);

            pthread_join(thread, NULL);

            pthread_mutex_lock(&mutex);
            active_threads--;
        }
        pthread_mutex_unlock(&mutex);

        if (!created_thread) {
            quick_sort(arr, low, high);
        }
    }
    return NULL;
}
int main() {
    pthread_mutex_init(&mutex, NULL);

    // Allocate memory for the array
    int *arr = (int *) malloc(MAX_ITEMS * sizeof(int));

    // Initialize array with random numbers
    // This part is now outside the timed section
    for (int i = 0; i < MAX_ITEMS; i++) {
        arr[i] = rand();
    }

    // Setup for parallel quicksort
    QuickSortArgs qa = {arr, 0, MAX_ITEMS - 1};

    // Start timing here, right before sorting
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Begin parallel sorting
    pthread_t thread;
    pthread_create(&thread, NULL, quick_sort_parallel, &qa);
    pthread_join(thread, NULL);

    // Stop timing after sorting is complete
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate and print elapsed time
    double time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("Parallel Quick Sort took %.9f seconds\n", time_taken);

    // Clean up
    free(arr);
    pthread_mutex_destroy(&mutex);

    return 0;
}