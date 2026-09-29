#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "mymalloc.h" 

#define NUM_ITERATIONS 50

long get_time_diff(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
}

// Workload 1: malloc() and immediately free() a 1-byte object, 120 times
void workload_1() {
    for (int i = 0; i < 120; i++) {
        void *p = malloc(1);
        free(p);
    }
}

// Workload 2: malloc() 120 1-byte objects, store pointers, then free them
void workload_2() {
    void *ptrs[120];
    
    for (int i = 0; i < 120; i++) {
        ptrs[i] = malloc(1);
    }
    
    for (int i = 0; i < 120; i++) {
        free(ptrs[i]);
    }
}

// Workload 3: Randomly choose between allocating and deallocating.
// Stop when 120 allocations have been performed
void workload_3() {
    void *ptrs[120];       
    int current_loc = 0;   
    int malloc_count = 0; 

    while (malloc_count < 120) {
        int action = rand() % 2; 

        if (action == 0 || current_loc == 0) {
            ptrs[current_loc] = malloc(1);
            current_loc++;
            malloc_count++;
        } else {
            current_loc--;
            free(ptrs[current_loc]);
        }
    }

    while (current_loc > 0) {
        current_loc--;
        free(ptrs[current_loc]);
    }
}

// Workload 4: Fragmentation Stress Test
// Allocates an array of objects, then frees every other object (evens),
// then frees the rest (odds). This creates holes in memory and tests 
// if the allocator correctly coalesces free blocks
void workload_4() {
    void *ptrs[120];
    
    for(int i = 0; i < 120; i++) {
        ptrs[i] = malloc(1); 
    }

    for(int i = 0; i < 120; i+=2) {
        free(ptrs[i]);
    }

    for(int i = 1; i < 120; i+=2) {
        free(ptrs[i]);
    }
}

// Workload 5: Linked List Simulation
// Simulates a data structure usage pattern (allocate nodes, link them, traverse, free)
// This tests allocations of sizes larger than 1 byte (struct size)
struct Node {
    struct Node* next;
    int data;
};

void workload_5() {
    struct Node* head = NULL;
    
    for(int i = 0; i < 100; i++) {
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        if (newNode != NULL) {
             newNode->data = i;
             newNode->next = head;
             head = newNode;
        }
    }

    struct Node* curr = head;
    while(curr != NULL) {
        struct Node* temp = curr;
        curr = curr->next;
        free(temp);
    }
}

int main() {
    struct timeval start, end;
    long total_time;
    
    printf("Memgring Workloads Performance\n");

    // Run Workload 1 
    total_time = 0;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        gettimeofday(&start, NULL);
        workload_1();
        gettimeofday(&end, NULL);
        total_time += get_time_diff(start, end);
    }
    printf("Workload 1 Average Time: %ld microseconds\n", total_time / NUM_ITERATIONS);

    // Run Workload 2 
    total_time = 0;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        gettimeofday(&start, NULL);
        workload_2();
        gettimeofday(&end, NULL);
        total_time += get_time_diff(start, end);
    }
    printf("Workload 2 Average Time: %ld microseconds\n", total_time / NUM_ITERATIONS);

    // Run Workload 3
    total_time = 0;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        gettimeofday(&start, NULL);
        workload_3();
        gettimeofday(&end, NULL);
        total_time += get_time_diff(start, end);
    }
    printf("Workload 3 Average Time: %ld microseconds\n", total_time / NUM_ITERATIONS);

    // Workload 4
    total_time = 0;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        gettimeofday(&start, NULL);
        workload_4();
        gettimeofday(&end, NULL);
        total_time += get_time_diff(start, end);
    }
    printf("Workload 4 Average Time: %ld microseconds\n", total_time / NUM_ITERATIONS);

    // Workload 5
    total_time = 0;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        gettimeofday(&start, NULL);
        workload_5();
        gettimeofday(&end, NULL);
        total_time += get_time_diff(start, end);
    }
    printf("Workload 5 Average Time: %ld microseconds\n", total_time / NUM_ITERATIONS);

    return 0;
}
