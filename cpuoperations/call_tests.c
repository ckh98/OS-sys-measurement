#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
unsigned cycles_low2, cycles_high2, cycles_low3, cycles_high3;

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

static __inline__ unsigned long long rdtsc2(void)
{
    __asm__ __volatile__ ("RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t": "=r" (cycles_high2), "=r" (cycles_low2)::
            "%rax", "rbx", "rcx", "rdx");
}

static __inline__ unsigned long long rdtsc3(void)
{
    __asm__ __volatile__ ("RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t": "=r" (cycles_high3), "=r" (cycles_low3)::
            "%rax", "rbx", "rcx", "rdx");
}

// Process with 0 parameters
int foo0() {
    return 0;
}

// Process with 1 parameter
int foo1(int a) {
    return 0;
}

// Process with 2 parameters
int foo2(int a, int b) {
    return 0;
}

// Process with 3 parameters
int foo3(int a, int b, int c) {
    return 0;
}

// Process with 4 parameters
int foo4(int a, int b, int c, int d) {
    return 0;
}

// Process with 5 parameters
int foo5(int a, int b, int c, int d, int e) {
    return 0;
}

// Process with 6 parameters
int foo6(int a, int b, int c, int d, int e, int f) {
    return 0;
}

// Process with 7 parameters
int foo7(int a, int b, int c, int d, int e, int f, int g) {
    return 0;
}

// Returns the average of the array
u_int64_t getAvg(u_int64_t array []) {
    u_int64_t sum = 0;
    u_int64_t n = sizeof(array);
    for (size_t i = 0; i < n; i++) {
        sum += array[i];
    }
    sum = sum / n;
    return sum;
}

// Returns the variance of the array
u_int64_t getVar(u_int64_t array [], u_int64_t avg) {
    u_int64_t sum = 0;
    u_int64_t n = sizeof(array);
    for (size_t i = 0; i < n; i++) {
        sum += pow((array[i] - avg), 2);
    }
    sum = sum / n;
    return sum;
}

//compile -> gcc -o call call_tests.c -lm
//run -> ./call
int main(int argc, char* argv[]) {
    int trials = 10;
    int iterations = 100000;
    uint64_t start, end, cycle_time;
    uint64_t start1, end1, cycle_time1;
    uint64_t* func0 = (uint64_t*)malloc(trials * sizeof(uint64_t));
    uint64_t* func1 = (uint64_t*)malloc(trials * sizeof(uint64_t));
    uint64_t* func2 = (uint64_t*)malloc(trials * sizeof(uint64_t));
    uint64_t* func3 = (uint64_t*)malloc(trials * sizeof(uint64_t));
    uint64_t* func4 = (uint64_t*)malloc(trials * sizeof(uint64_t));
    uint64_t* func5 = (uint64_t*)malloc(trials * sizeof(uint64_t));
    uint64_t* func6 = (uint64_t*)malloc(trials * sizeof(uint64_t));
    uint64_t* func7 = (uint64_t*)malloc(trials * sizeof(uint64_t));

    // uint64_t* test = (uint64_t*)malloc(10 * sizeof(uint64_t));
    // for (size_t i = 0; i < 10; i++) {
    //     test[i] = i*2;
    // }
    //
    // uint64_t avg = getAvg(test);
    // uint64_t var = getVar(test, avg);
    // uint64_t std = sqrt(var);
    // printf("%f\n", (double)avg);
    // printf("%f\n", (double)std);

    // Measuring cycle time
    rdtsc();
    for (size_t i = 0; i < 5; i++) {
        sleep(1);
    }
    rdtsc1();

    start = ( ((u_int64_t) cycles_high << 32) | cycles_low );
    end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
    cycle_time = (end - start) / 5;
    printf("%lu\n", cycle_time);

    for (size_t i = 0; i < 8; i++) {

        // Measuring process with 0 arguments
        if (i == 0) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo0();
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func0[j] = (end - start) / iterations;
            }

            u_int64_t avg0 = getAvg(func0);
            u_int64_t var0 = getVar(func0, avg0);
            u_int64_t std0 = sqrt(var0);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg0);
            printf("Variance: %lu\n", var0);
            printf("Std: %lu\n", std0);

            // for (size_t i = 0; i < 1000; i++) {
            //     printf("%lu\n", func0[i]);
            // }
        }

        // Measuring process with 1 argument
        if (i == 1) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo1(1);
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func1[j] = (end - start) / iterations;
            }

            u_int64_t avg1 = getAvg(func1);
            u_int64_t var1 = getVar(func1, avg1);
            u_int64_t std1 = sqrt(var1);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg1);
            printf("Variance: %lu\n", var1);
            printf("Std: %lu\n", std1);
        }

        // Measuring process with 2 arguments
        if (i == 2) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo2(1, 2);
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func2[j] = (end - start) / iterations;
            }

            u_int64_t avg2 = getAvg(func2);
            u_int64_t var2 = getVar(func2, avg2);
            u_int64_t std2 = sqrt(var2);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg2);
            printf("Variance: %lu\n", var2);
            printf("Std: %lu\n", std2);
        }

        // Measuring process with 3 arguments
        if (i == 3) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo3(1, 2, 3);
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func3[j] = (end - start) / iterations;
            }

            u_int64_t avg3 = getAvg(func3);
            u_int64_t var3 = getVar(func3, avg3);
            u_int64_t std3 = sqrt(var3);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg3);
            printf("Variance: %lu\n", var3);
            printf("Std: %lu\n", std3);
        }

        // Measuring process with 4 arguments
        if (i == 4) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo4(1, 2, 3, 4);
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func4[j] = (end - start) / iterations;
            }

            u_int64_t avg4 = getAvg(func4);
            u_int64_t var4 = getVar(func4, avg4);
            u_int64_t std4 = sqrt(var4);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg4);
            printf("Variance: %lu\n", var4);
            printf("Std: %lu\n", std4);
        }

        // Measuring process with 5 arguments
        if (i == 5) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo5(1, 2, 3, 4, 5);
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func5[j] = (end - start) / iterations;
            }

            u_int64_t avg5 = getAvg(func5);
            u_int64_t var5 = getVar(func5, avg5);
            u_int64_t std5 = sqrt(var5);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg5);
            printf("Variance: %lu\n", var5);
            printf("Std: %lu\n", std5);
        }

        // Measuring process with 6 arguments
        if (i == 6) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo6(1, 2, 3, 4, 5, 6);
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func6[j] = (end - start) / iterations;
            }

            u_int64_t avg6 = getAvg(func6);
            u_int64_t var6 = getVar(func6, avg6);
            u_int64_t std6 = sqrt(var6);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg6);
            printf("Variance: %lu\n", var6);
            printf("Std: %lu\n", std6);
        }

        // Measuring process with 7 arguments
        if (i == 7) {
            for (size_t j = 0; j < trials; j++) {
                rdtsc();
                for (size_t k = 0; k < iterations; k++) {
                    foo7(1, 2, 3, 4, 5, 6, 7);
                }
                rdtsc1();

                start = ( ((uint64_t)cycles_high << 32) | cycles_low );
                end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

                func7[j] = (end - start) / iterations;
            }

            u_int64_t avg7 = getAvg(func7);
            u_int64_t var7 = getVar(func7, avg7);
            u_int64_t std7 = sqrt(var7);

            printf("Passing %ld arguments\n", i);
            printf("Avg # of cycles: %lu\n", avg7);
            printf("Variance: %lu\n", var7);
            printf("Std: %lu\n", std7);
        }
    }

    // ----------------SYSTEM CALL----------------

    uint64_t* sys1 = (uint64_t*)malloc(trials * sizeof(uint64_t));

    // Measuring the system call getpid()
    for (size_t j = 0; j < trials; j++) {
        rdtsc();
        for (size_t k = 0; k < iterations; k++) {
            getpid();
        }
        rdtsc1();

        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

        sys1[j] = (end - start) / iterations;;
    }
    printf("System Call 1\n");
    u_int64_t avgsys1 = getAvg(sys1);
    u_int64_t varsys1 = getVar(sys1, avgsys1);
    u_int64_t stdsys1 = sqrt(varsys1);

    printf("Avg # of cycles: %lu\n", avgsys1);
    printf("Variance: %lu\n", varsys1);
    printf("Std: %lu\n", stdsys1);


    uint64_t* sys2 = (uint64_t*)malloc(trials * sizeof(uint64_t));

    // Measuring the system call getsid()
    for (size_t j = 0; j < trials; j++) {
        rdtsc();
        for (size_t k = 0; k < iterations; k++) {
            getsid();
        }
        rdtsc1();

        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

        sys2[j] = (end - start) / iterations;
    }
    printf("System Call 2\n");
    u_int64_t avgsys2 = getAvg(sys2);
    u_int64_t varsys2 = getVar(sys2, avgsys2);
    u_int64_t stdsys2 = sqrt(varsys2);

    printf("Avg # of cycles: %lu\n", avgsys2);
    printf("Variance: %lu\n", varsys2);
    printf("Std: %lu\n", stdsys2);

    // ----------------MEASUREMENT OVERHEAD----------------

    uint64_t* measure1 = (uint64_t*)malloc(trials * sizeof(uint64_t));

    // Measuring the measurmenet overhead using a process call
    uint64_t unwantedTime;  // The time that we do not want to consider in our measurement
    for (size_t j = 0; j < trials; j++) {
        unwantedTime = 0;
        rdtsc2();
        for (size_t k = 0; k < iterations; k++) {
            rdtsc();
            foo0();
            rdtsc1();

            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

            unwantedTime += end - start;
        }
        rdtsc3();

        start1 = ( ((uint64_t)cycles_high2 << 32) | cycles_low2 );
        end1 = ( ((uint64_t)cycles_high3 << 32) | cycles_low3 );

        measure1[j] = (end1 - start1 - unwantedTime) / iterations;
    }

    printf("Measurement Overhead 1\n");
    u_int64_t avgmea1 = getAvg(measure1);
    u_int64_t varmea1 = getVar(measure1, avgmea1);
    u_int64_t stdmea1 = sqrt(varmea1);

    printf("Avg # of cycles: %lu\n", avgmea1);
    printf("Variance: %lu\n", varmea1);
    printf("Std: %lu\n", stdmea1);


    uint64_t* measure2 = (uint64_t*)malloc(trials * sizeof(uint64_t));

    // Measuring the measurmenet overhead using a system call
    for (size_t j = 0; j < trials; j++) {
        unwantedTime = 0;
        rdtsc2();
        for (size_t k = 0; k < iterations; k++) {
            rdtsc();
            getpid();
            rdtsc1();

            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

            unwantedTime += end - start;
        }
        rdtsc3();

        start1 = ( ((uint64_t)cycles_high2 << 32) | cycles_low2 );
        end1 = ( ((uint64_t)cycles_high3 << 32) | cycles_low3 );

        measure2[j] = (end1 - start1 - unwantedTime) / iterations;
    }

    printf("Measurement Overhead 2\n");
    u_int64_t avgmea2 = getAvg(measure2);
    u_int64_t varmea2 = getVar(measure2, avgmea2);
    u_int64_t stdmea2 = sqrt(varmea2);

    printf("Avg # of cycles: %lu\n", avgmea2);
    printf("Variance: %lu\n", varmea2);
    printf("Std: %lu\n", stdmea2);

    return 0;
}
