#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <inttypes.h>

unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;

static __inline__ unsigned long long rdtsc(void)
{
    __asm__ __volatile__ ("RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
            "%rax", "rbx", "rcx", "rdx");
}

static __inline__ unsigned long long rdtsc1(void)
{
    __asm__ __volatile__ ("RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
            "%rax", "rbx", "rcx", "rdx");
}

//compile gcc -o latency access_latency_test.c -lm
//run ./latency
int main(int argc, char const *argv[]) {
    //  Create an int.
    int *r; //memory that will be used to run latency experiments
    int L1cachesize = 20000/sizeof(int); //size of L1 - little less than L1
    int L2cachesize = 80000/sizeof(int); //size of L2 - larger than L1
    int memorysize = 1800000/sizeof(int); //size of memory region - larger than L2
    uint64_t l1_start, l1_end, l2_start, l2_end, mem_start, mem_end; //used for measurement

    r = (int *) malloc(memorysize * sizeof(int)); //allocate mem
    rdtsc(); //start time for cycles
    for (int a = 0; a < memorysize; a+=16) { //access elements in memory
        int access = r[a]; //access memory
    }
    rdtsc1(); //end time for cycles

    mem_start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //beginning cycles
    mem_end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end cycles

    //start time for cycles
    for (int b = 0; b < L1cachesize; b+=16) { //access elements in L1
        if (b == 160) { //wait for first few iterations for cache to run timer
            rdtsc();
        }
        int access = r[b]; //access L1
    }
    rdtsc1(); //end time for cycles

    l1_start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //beginning cycles
    l1_end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end cycles

    //start time for cycles
    rdtsc();
    for (int c = 0; c < L2cachesize; c+=16) { //access elements in L2
        int access = r[rand() % L2cachesize]; //access L2
        //access random to avoid prefetching from L1
    }
    rdtsc1(); //end time for cycles

    free(r); //free memory

    l2_start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //beginning cycles
    l2_end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end cycles

    printf("Memory Time: %lu nanoseconds\n", (mem_end - mem_start)/ ((memorysize/16)*3)); //divide by number of accesses and convert cycles to ns (3 cycles/ns)
    printf("L1 Cache Time: %lu nanoseconds\n", (l1_end - l1_start)/ (((L1cachesize/16)-10)*3)); //divide by number of accesses and convert cycles to ns (3 cycles/ns)
    printf("L2 Cache Time: %lu nanoseconds\n", (l2_end - l2_start)/ ((((L2cachesize)/16))*3)); //divide by number of accesses and convert cycles to ns (3 cycles/ns)
    return 0;
}
