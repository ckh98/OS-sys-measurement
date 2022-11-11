#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

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

// Return the average of the array
u_int64_t getAvg(u_int64_t array []) {
    u_int64_t sum = 0;
    u_int64_t n = sizeof(array);
    for (size_t i = 0; i < n; i++) {
        sum += array[i];
    }
    sum = sum / n;
    return sum;
}

// Return the variance of the array
u_int64_t getVar(u_int64_t array [], u_int64_t avg) {
    u_int64_t sum = 0;
    u_int64_t n = sizeof(array);
    for (size_t i = 0; i < n; i++) {
        sum += pow((array[i] - avg), 2);
    }
    sum = sum / n;
    return sum;
}

//compile -> gcc -o memBand mem_bandwidth.c -lm
//run -> ./memBand
int main(int argc, char* argv[]) {
    int size = 10000000;    // 10000000 bytes = 10 megabytes
    int trials = size / 20;
    int *mem = (int*)malloc(size * sizeof(int));    // Allocates size bytes of memory
    int temp = 0;
    u_int64_t writeTime [size / 20];
    u_int64_t readTime [size / 20];
    uint64_t start, end;
    u_int64_t writeSum = 0;
    u_int64_t readSum = 0;

    // ----------------WRITING----------------

    // Using loop unrolling: Writing 20 bytes each loop
    for (size_t i = 0; i < trials; i += 20) {
        rdtsc();
        mem[i] = 0;
        mem[i+1] = 0;
        mem[i+2] = 0;
        mem[i+3] = 0;
        mem[i+4] = 0;
        mem[i+5] = 0;
        mem[i+6] = 0;
        mem[i+7] = 0;
        mem[i+8] = 0;
        mem[i+9] = 0;
        mem[i+10] = 0;
        mem[i+11] = 0;
        mem[i+12] = 0;
        mem[i+13] = 0;
        mem[i+14] = 0;
        mem[i+15] = 0;
        mem[i+16] = 0;
        mem[i+17] = 0;
        mem[i+18] = 0;
        mem[i+19] = 0;
        rdtsc1();

        start = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        writeTime[i] = (end - start);
        writeSum += (end - start);
    }

    u_int64_t writeAvg = getAvg(writeTime);
    u_int64_t writeVar = getVar(writeTime, writeAvg);
    u_int64_t writeStd = sqrt(writeVar);

    printf("Write Time\n");
    printf("Avg Bandwidth: %lu\n", writeAvg);
    printf("Bandwidth std: %lu\n", writeStd);
    printf("Total time: %lu\n\n", writeSum / 3);

    // ----------------READING----------------

    // Using loop unrolling: Reading 20 bytes each loop
    for (size_t i = 0; i < trials; i+=20) {
        rdtsc();
        temp = mem[i];
        temp = mem[i+1];
        temp = mem[i+2];
        temp = mem[i+3];
        temp = mem[i+4];
        temp = mem[i+5];
        temp = mem[i+6];
        temp = mem[i+7];
        temp = mem[i+8];
        temp = mem[i+9];
        temp = mem[i+10];
        temp = mem[i+11];
        temp = mem[i+12];
        temp = mem[i+13];
        temp = mem[i+14];
        temp = mem[i+15];
        temp = mem[i+16];
        temp = mem[i+17];
        temp = mem[i+18];
        temp = mem[i+19];
        rdtsc1();

        start = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        readTime[i] = (end - start);
        readSum += end - start;
    }

    u_int64_t readAvg = getAvg(readTime);
    u_int64_t readVar = getVar(readTime, readAvg);
    u_int64_t readStd = sqrt(readVar);

    printf("Read Time\n");
    printf("Avg Bandwidth: %lu\n", readAvg);
    printf("Bandwidth std: %lu\n", readStd);
    printf("Total time: %lu\n", readSum / 3);

    free(mem);
}
