#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define KILO (1024)
#define MEGA (1024*1024)
#define MAX_ITEMS (64*MEGA)
#define MIN_INSERTION 16 // threshold for switching to insertion sort for small subarrays
#define THRESHOLD 1000 // threshold for choosing method for pivot selection
#define SAMPLE_SIZE 25 // sample size for pivot selection
#define PARALLEL_THRESHOLD 100000 // threshold for spawning new threads

typedef struct {
    int *array;
    int left;
    int right;
} QuickSortArgs;

// inline comparison function for qsort
static inline int compare_ints(const void* a, const void* b) {
    int arg1 = *(const int*)a;
    int arg2 = *(const int*)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

static inline int select_pivot(int arr[], int low, int high) {
    int sample[SAMPLE_SIZE];
    int range = high - low;
    for (int i = 0; i < SAMPLE_SIZE; ++i) {
        sample[i] = arr[low + rand() % range];
    }
    qsort(sample, SAMPLE_SIZE, sizeof(int), compare_ints);
    return sample[SAMPLE_SIZE / 2]; // median of sample
}

// inline partition function
static inline int partition(int arr[], int low, int high) {
    int pivot;
    if (high - low > THRESHOLD)
        pivot = select_pivot(arr, low, high);
    else
        pivot = arr[high];

    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            // swap without function call overhead
            unsigned tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
    }
    unsigned tmp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = tmp;
    return i + 1;
}

// use insertion sort for tiny subarrays
static inline void insertion_sort(int arr[], int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void quick_sort(int arr[], int left, int right) {
    if (left < right) {
        if ((right - left + 1) < MIN_INSERTION) {
            insertion_sort(arr, left, right);
            return;
        }
        int pi = partition(arr, left, right);
        quick_sort(arr, left, pi - 1);
        quick_sort(arr, pi + 1, right);
    }
}

void quick_sort_parallel(int arr[], int left, int right);

// thread entry helper for parallel quicksort
void *quick_sort_parallel_helper(void *args) {
    QuickSortArgs *qa = (QuickSortArgs *) args;
    quick_sort_parallel(qa->array, qa->left, qa->right);
    free(qa);
    return NULL;
}

// parallel quicksort function that spawns threads for large subarrays
void quick_sort_parallel(int arr[], int left, int right) {
    if (left >= right)
        return;

    if ((right - left + 1) <= PARALLEL_THRESHOLD) {
        quick_sort(arr, left, right);
        return;
    }

    int pi = partition(arr, left, right);
    pthread_t thread_left, thread_right;
    int created_left = 0, created_right = 0;

    // spawn a thread for the left partition if it is not too small
    if ((pi - 1 - left + 1) > PARALLEL_THRESHOLD) {
        QuickSortArgs *args_left = malloc(sizeof(QuickSortArgs));
        if (!args_left) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        args_left->array = arr;
        args_left->left = left;
        args_left->right = pi - 1;
        if (pthread_create(&thread_left, NULL, quick_sort_parallel_helper, args_left) != 0) {
            free(args_left);
            quick_sort(arr, left, pi - 1);
        } else {
            created_left = 1;
        }
    } else {
        quick_sort(arr, left, pi - 1);
    }

    // spawn a thread for the right partition if it's not too small
    if ((right - (pi + 1) + 1) > PARALLEL_THRESHOLD) {
        QuickSortArgs *args_right = malloc(sizeof(QuickSortArgs));
        if (!args_right) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        args_right->array = arr;
        args_right->left = pi + 1;
        args_right->right = right;
        if (pthread_create(&thread_right, NULL, quick_sort_parallel_helper, args_right) != 0) {
            free(args_right);
            quick_sort(arr, pi + 1, right);
        } else {
            created_right = 1;
        }
    } else {
        quick_sort(arr, pi + 1, right);
    }

    // wait for spawned threads to finish
    if (created_left) {
        pthread_join(thread_left, NULL);
    }
    if (created_right) {
        pthread_join(thread_right, NULL);
    }
}

int main() {
    // allocate memory for array
    int *arr = (int *) malloc(MAX_ITEMS * sizeof(int));
    if (!arr) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // initialize array
    for (int i = 0; i < MAX_ITEMS; i++) {
        arr[i] = rand();
    }

    // start timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // start parallel quicksort
    quick_sort_parallel(arr, 0, MAX_ITEMS - 1);

    // stop timing after sorting completes
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Parallel Quick Sort took %.9f seconds\n", time_taken);

    free(arr);
    return 0;
}
