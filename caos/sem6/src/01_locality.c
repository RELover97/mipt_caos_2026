// Один набор чисел: последовательный обход и обход по случайным указателям.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

int main(int argc, char **argv) 
{
    int n = argc > 1 ? atoi(argv[1]) : 500000;

    if (n < 1 || n > 3000000) {
        fprintf(stderr, "Enter n between 1 and 3000000\n");
        return 1;
    }
    printf("Number of elements is %d\n", n);

    int *array_vals = malloc((size_t)n * sizeof(*array_vals));
    Node *list_nodes = malloc((size_t)n * sizeof(*list_nodes));

    int *list_traversal_order = malloc((size_t) n * sizeof(*list_traversal_order));

    if (!array_vals || !list_nodes || !list_traversal_order) {
        perror("malloc");
        free(array_vals); 
        free(list_nodes); 
        free(list_traversal_order);
        return 1;
    }

    for (int i = 0; i < n; ++i) {
        array_vals[i] = i + 1;
        list_traversal_order[i] = i;
    }

    srand(42);
    // Перемешиваем индексы, но сами узлы хранятся в одном массиве.
    for (int i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int temp = list_traversal_order[i]; 
        list_traversal_order[i] = list_traversal_order[j]; 
        list_traversal_order[j] = temp;
    }

    // узнать размера кэшей процессора
    long l1_dcache = sysconf(_SC_LEVEL1_DCACHE_SIZE);
    long l1_icache = sysconf(_SC_LEVEL1_ICACHE_SIZE);
    (void) l1_icache;
    long l2_cache  = sysconf(_SC_LEVEL2_CACHE_SIZE);
    long l3_cache  = sysconf(_SC_LEVEL3_CACHE_SIZE);

    // найдём наибольшее расстояние между соседними узлами списка
    long biggest_dist = 0;
    long current_dist = 0;
    size_t l1_cache_misses = 0;
    size_t l2_cache_misses = 0;
    size_t l3_cache_misses = 0;
    for (int i = 1; i < n; ++i) {
        current_dist = list_traversal_order[i] - list_traversal_order[i - 1];
        if (current_dist < 0) {
            current_dist = -current_dist;
        }
        if (current_dist > biggest_dist) {
            biggest_dist = current_dist;
        }
        if (current_dist * (long) sizeof(*array_vals) > l1_dcache) {
            l1_cache_misses += 1;
            if (current_dist * (long) sizeof(*array_vals) > l2_cache) {
                l2_cache_misses += 1;
                if (current_dist * (long) sizeof(*array_vals) > l3_cache) {
                    l3_cache_misses += 1;
                }
            }
        }
    }
    size_t biggest_dist_bytes = biggest_dist * sizeof(*array_vals);

    for (int i = 0; i < n; ++i) {
        int k = list_traversal_order[i];
        list_nodes[k].value = array_vals[k];
        list_nodes[k].next = (i + 1 < n) ? &list_nodes[list_traversal_order[i + 1]] : NULL;
    }

    Node *head = &list_nodes[list_traversal_order[0]];

    // Создание структур не включено в замер
    clock_t start = clock();

    uint64_t sum_array = 0;
    for (int rep = 0; rep < 4; ++rep) {
        for (int i = 0; i < n; ++i) {
            sum_array += array_vals[i];
        }
    }
        
    clock_t end_array = clock();

    uint64_t sum_list = 0;
    for (int rep = 0; rep < 4; ++rep) {
        for (Node *p = head; p != NULL; p = p->next)  {
            sum_list += p->value;
        }
    }
        
    clock_t end_list = clock();

    double cpu_time_used_array = ((double) (end_array - start)) / CLOCKS_PER_SEC;
    double cpu_time_used_list = ((double) (end_list - end_array)) / CLOCKS_PER_SEC;

    printf("Array size: %zu bytes = %zu KB\n", (size_t)n * sizeof(*array_vals), ((size_t)n * sizeof(*array_vals)) / 1024);
    printf("n = %d, sizeof(Node) = %zu bytes\n", n, sizeof(Node));
    printf("array: %.3f ms; sum = %llu\n", cpu_time_used_array, (unsigned long long)sum_array);
    printf("list:  %.3f ms; sum = %llu\n", cpu_time_used_list, (unsigned long long)sum_list);
    printf("list slowdown: %.2f %%\n", (cpu_time_used_list / cpu_time_used_array) * 100);
    printf("Biggest distance between list adjacent nodes: %zu bytes = %zu KB\n", biggest_dist_bytes, biggest_dist_bytes / 1024);
    printf("L1 cache: size = %zu KB, misses = %zu\n", l1_dcache / 1024, l1_cache_misses);
    printf("L2 cache: size = %zu KB, misses = %zu\n", l2_cache / 1024, l2_cache_misses);
    printf("L3 cache: size = %zu KB, misses = %zu\n", l3_cache / 1024, l3_cache_misses);

    free(list_traversal_order);
    free(list_nodes);
    free(array_vals);

    return 0;
}
