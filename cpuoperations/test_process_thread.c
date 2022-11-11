#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
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

int process_func() {
    int pid = fork();
    if (pid == -1) {
        return -1;
    }
    if (pid == 0) {
        printf("Run process\n");
        exit(0);
        return 0;
    }
    if (pid > 0) {
        wait(NULL);
    }
    printf("Run process\n");
    return 0;
}

void* thread_func() {
    printf("Run kernel thread\n");
}


//compile -> gcc -pthread -o test test_process_thread.c -lm
//run -> ./test
int main(int argc, char const *argv[]) {
    long Ntrials = 100;
    long iterations = 1000;
    u_int64_t proc_start, proc_end;
    u_int64_t thread_start, thread_end;
    u_int64_t diff_proc, avg_proc, dev_proc;
    u_int64_t sum_proc = 0;
    u_int64_t var_proc = 0;
    u_int64_t procTimes[Ntrials];
    u_int64_t diff_thread, avg_thread, dev_thread;
    u_int64_t sum_thread = 0;
    u_int64_t var_thread = 0;
    u_int64_t threadTimes[Ntrials];
    int reps = 20;
    u_int64_t clocksSecs[reps];
    int Nproc = 0;
    int Nthread = 0;
    u_int64_t start_time, end_time, cycle_time;

    for (int m = 0; m < reps; m++) {
        rdtsc(); //start time for cycles
        sleep(1); //sleep for one second
        rdtsc1(); //end time for cycles
        start_time = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //beginning cycles
        end_time = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end cycles
        cycle_time = (double) (end_time - start_time)/ (double) 1000000; //calculate cycles per microsecond
        clocksSecs[m] = cycle_time; //store cycles in array
    }

    u_int64_t sumCycles = 0;
    u_int64_t avg_cycles_per_msec;
    u_int64_t var_cycles_per_msec;
    u_int64_t dev_cycles_per_msec;
    for (int o = 0; o < reps; o++) {
        sumCycles += clocksSecs[o]; //find total cycles for all iterations
    }
    avg_cycles_per_msec = ((double) sumCycles / (double) reps); //calculate avg cycles per microsecond

    for (int p = 0; p < reps; p++) {
        var_cycles_per_msec += pow(((clocksSecs[p]) - avg_cycles_per_msec), 2); //total variance
    }
    var_cycles_per_msec = (double) var_cycles_per_msec / (double) reps; //variance over iterations
    dev_cycles_per_msec = sqrt(var_cycles_per_msec); //deviation

    // Calculate avg time per iter for N trials
    for (int a = 0; a < Ntrials; a++) {
        rdtsc(); //start time for cycles
        for (int i = 0; i < iterations; i++) { //create and run process over iterations
            process_func();
        }
        rdtsc1(); //end time for cycles
        proc_start = (((u_int64_t) cycles_high << 32) | cycles_low); //beginning cycles
        proc_end = (((u_int64_t) cycles_high1 << 32) | cycles_low1); //end cycles
        diff_proc = ((double) (proc_end - proc_start)/ (double) avg_cycles_per_msec); //calculate cycles to create and run and convert to microseconds
        procTimes[a] = (double) diff_proc / (double) iterations; //find avg time to create and run over iterations
    }

    for (int k = 0; k < Ntrials; k++) {
        sum_proc += procTimes[k]; //calculate total time to create and run process over all trials
    }
    avg_proc = (double) sum_proc / (double) Ntrials; //calculate avg time to create and run process

    for (int d = 0; d < Ntrials; d++) {
        var_proc += pow(((procTimes[d]) - avg_proc), 2); //total variance
    }
    var_proc = ((double)var_proc / (double) Ntrials); //variance over trials
    dev_proc = sqrt(var_proc); //deviation

    // Calculate avg time per iter for N trials
    for (int b = 0; b < Ntrials; b++) {
        rdtsc(); //start time for cycles
        for (int j = 0; j < iterations; j++) { //create and run thread over iterations
            pthread_t thread1;
            pthread_create(&thread1, NULL, &thread_func, NULL);
            pthread_join(thread1, NULL);
        }
        rdtsc1(); //end time for cycles
        thread_start = (((u_int64_t) cycles_high << 32) | cycles_low); //beginning cycles
        thread_end = (((u_int64_t) cycles_high1 << 32) | cycles_low1); //end cycles
        diff_thread = ((double)(thread_end - thread_start) / (double) avg_cycles_per_msec); //calculate cycles to create and run and convert to microseconds
        threadTimes[b] = (double) diff_thread / (double) iterations; //find avg time to create and run over iterations
    }


    for (int l = 0; l < Ntrials; l++) {
        sum_thread += threadTimes[l]; //calculate total time to create and run thread over all trials
    }
    avg_thread = (double) sum_thread / (double) Ntrials; //calculate avg time to create and run thread

    for (int c = 0; c < Ntrials; c++) {
        var_thread += pow(((threadTimes[c]) - avg_thread), 2); //total variance
    }
    var_thread = ((double)var_thread / (double) Ntrials); //variance over trials
    dev_thread = sqrt(var_thread); //deviation

    printf("Average time in microseconds per trial = %lu\n", avg_proc);
    printf("Standard deviation time in microseconds = %lu\n", dev_proc);
    printf("Average time in microseconds per trial = %lu\n", avg_thread);
    printf("Standard deviation time in microseconds = %lu\n", dev_thread);
    printf("1 Microsecond = %ld cycles on average\n", avg_cycles_per_msec);
    printf("Standard deviation of cycles = %ld\n", dev_cycles_per_msec);
    return 0;
}
