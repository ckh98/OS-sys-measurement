#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>


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

u_int64_t getAvg(u_int64_t array [], int len) {
    u_int64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += array[i];
    }
    sum = sum / len;
    return sum;
}

u_int64_t getVar(u_int64_t array [], u_int64_t avg, int len) {
    u_int64_t sum = 0;
    u_int64_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff = array[i] - avg;
        sum = sum + diff*diff;
    }
    sum = sum / len;
    return sum;
}

int main()
{

    int x, N_trials=10;
    u_int64_t avg, var, std_dev, duration_per_iteration[N_trials];
    for(x = 0; x < N_trials; x++){

    char data = 'a'; //one byte data to be passed in the pipes
    char buf1, buf2;
    int child_pid;
    long N_iterations=100000;
    int i = 0;
    int fd[2], r, w;
    int fd2[2], r2, w2;

    u_int64_t start, end, cycle_time;
    u_int64_t start_time, end_time;

    int p_id, p_id2;

    p_id = pipe(fd);
    if (p_id < 0) 
    {
        perror("Pipes ");
        exit(1);
    }
    
    p_id2 = pipe(fd2);
    if (p_id2 < 0) 
    {
        perror("Pipes ");
        exit(1);
    }

    child_pid = fork();
    if (child_pid < 0){
        fprintf(stderr, "Fork Failed");
        return 1;
    }       
    if (child_pid == 0) {
        char ch = 'a';
        for (i = 0; i < N_iterations; i++) {
            r = read(fd[0], (void*) &buf1, 1);
            if(r < 0){
                perror("Read");
                exit(3);
            }
            w2 = write(fd2[1], (void*) &ch, 1);
            if(w2 < 0){
                perror("Write");
                exit(3);
            }
        }
        close(fd[0]);
        close(fd2[1]);      
        return 0;    
        
    }   

        char ch = 'a';
        rdtsc();

        for (i = 0; i < N_iterations; i++) {
            w = write(fd[1], (void*) &ch, 1);
            if(w < 0){
                perror("parent - write");
                exit(2);
            }
            r2 = read(fd2[0], (void*) &buf2, 1);
            if(r2 < 0){
                perror("Parent - read ");
                exit(2);
            }
        }
        rdtsc1();
        start_time = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end_time = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        close(fd[1]);
        close(fd2[0]);
        duration_per_iteration[x] = (end_time - start_time)/(2*N_iterations);

    }

    avg = getAvg(duration_per_iteration, N_trials);
    var = getVar(duration_per_iteration, avg, N_trials);
    std_dev = sqrt(var);

    printf("Average Execution time = %llu clock cycles for %ld trials\n", avg, N_trials);
    printf("Standard deviation of Execution time = %llu clock cycles for %ld trials\n", std_dev, N_trials);

    fflush(stdout);
    return(0);
}

