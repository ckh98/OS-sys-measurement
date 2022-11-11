#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>

#define SHELLSCRIPT "\
echo 3 > /proc/sys/vm/drop_caches;"

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

static int *two_agg_time; //store value for total time to read blocks for 2 processes
static int *four_agg_time; //store value for total time to read blocks for 4 processes
static int *eight_agg_time; //store value for total time to read blocks for 8 processes
static int *sixteen_agg_time; //store value for total time to read blocks for 16 processes

double seq_time(int file) {
    uint64_t start, end;
    uint64_t total_time = 0;

    // Sanity check file specifications
    struct stat buf;
    fstat(file, &buf);
    size_t size = buf.st_size;
    blksize_t block = buf.st_blksize;

    // Number of blocks needed to go through the full file
    uint64_t nBlocks = size / block;
    if (size % block == 0) {
        nBlocks -= 1;
    }

    //printf("Size: %ld\n", size);
    //printf("Block Size: %ld\n", block);

    //printf("----Flushing out cache----\n");
    system(SHELLSCRIPT);

    char *buffer = (char *) malloc(size);

    // Reads the entire file
    for (size_t i = 0; i <= 0; i++) { //read 1 block

        // We are only measuring reading the file
        // Sequential Access
        rdtsc();
        ssize_t bytes_read = read(file, buffer, block); //read block from file
        rdtsc1();

        // If reached end of file or the read somehow failed
        if (bytes_read <= 0) {
            //printf("\nREAD FAILED\n");
            break;
        }

        start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //start time
        end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end time
        total_time += end - start; //calculate total time to read block
    }

    free(buffer); //free buffer
    return total_time; //return time to read 1 block
}

blksize_t get_block_size(int file) {
    struct stat buf; //get buffer
    fstat(file, &buf); //passes info about file to buffer like block size
    blksize_t blk = buf.st_blksize; //get block size
    return blk; //return block size
}

//compile gcc -o cont contention.c -lm
//run sudo ./cont

int main(int argc, char const *argv[]) {
    // create many processes accessing different files
    //x is number of processes and y is avg read time
    int Ntrials = 100; //number of iterations
    //One process
    uint64_t fd1_time;
    uint64_t fd1_start, fd1_end;
    uint64_t one_proc_times[Ntrials];
    for (int a = 0; a < Ntrials; a++) {
        int fd1 = open("contention/contention1.txt", O_RDONLY); //open file
        fd1_time = seq_time(fd1); //get read time for 1 block
        close(fd1); //close file
        one_proc_times[a] = fd1_time; //store read time in array
    }

    uint64_t sum_one_proc, avg_one_proc, var_one_proc, dev_one_proc;
    sum_one_proc = 0;
    var_one_proc = 0;
    for (int a2 = 0; a2 < Ntrials; a2++) {
        sum_one_proc += one_proc_times[a2]; //calculate total read time over iterations
    }

    avg_one_proc = (double) sum_one_proc / (double) Ntrials; //find avg over iterations

    for (int a3 = 0; a3 < Ntrials; a3++) {
        var_one_proc += pow(((one_proc_times[a3]) - avg_one_proc), 2); //calculate total variance
    }
    var_one_proc = ((double)var_one_proc / (double) Ntrials); //variance per iterations
    dev_one_proc = sqrt(var_one_proc); //get deviation

    printf("Avg time for 1 Process in ns: %lu\n", (avg_one_proc)/(3)); //convert cycles to ns (3 cycles/ns)
    printf("Deviation for 1 Process in ns: %lu\n", (dev_one_proc)/(3)); //convert cycles to ns (3 cycles/ns)


    //Two Processes
    two_agg_time = mmap(NULL, sizeof *two_agg_time, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    uint64_t two_proc_times[Ntrials];
    for (int b = 0; b < Ntrials; b++) {
        *two_agg_time = 0;
        //reset time for running all processes-calculate this every iteration
        //Two Processes
        uint64_t fd2_1_time, fd2_2_time;
        int pid2 = fork();
        //1 fork for 2 processes
        if (pid2 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd2_1 = open("contention/contention1.txt", O_RDONLY); //open file
            fd2_1_time = seq_time(fd2_1); //get read time for 1 block
            close(fd2_1); //close file
            *two_agg_time += fd2_1_time; //add time to run all processes
            //printf("Process 1: %lu\n", fd2_1_time);
        }
        else if (pid2 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd2_2 = open("contention/contention2.txt", O_RDONLY); //open file
            fd2_2_time = seq_time(fd2_2); //get read time for 1 block
            close(fd2_2); //close file
            *two_agg_time += fd2_2_time; //add time to run all processes
            //printf("Process 2: %lu\n", fd2_2_time);
            exit(0);
        }

        //printf("Total Time: %lu\n", *two_agg_time);
        two_proc_times[b] = (*two_agg_time) / 2; //store avg read time per process in array
    }


    uint64_t sum_two_proc, avg_two_proc, var_two_proc, dev_two_proc;
    sum_two_proc = 0;
    var_two_proc = 0;
    for (int b2 = 0; b2 < Ntrials; b2++) {
        sum_two_proc += two_proc_times[b2]; //calculate total read time over iterations
    }

    avg_two_proc = (double) sum_two_proc / (double) Ntrials; //find avg over iterations

    for (int b3 = 0; b3 < Ntrials; b3++) {
        var_two_proc += pow(((two_proc_times[b3]) - avg_two_proc), 2); //calculate total variance
    }
    var_two_proc = ((double)var_two_proc / (double) Ntrials); //variance per iterations
    dev_two_proc = sqrt(var_two_proc); //get deviation

    printf("Avg time for 2 Processes in ns: %lu\n", (avg_two_proc)/3); //convert cycles to ns (3 cycles/ns)
    printf("Deviation for 2 Processes in ns: %lu\n", (dev_two_proc)/3); //convert cycles to ns (3 cycles/ns)


    //Four Processes
    uint64_t fd4_1_time, fd4_2_time, fd4_3_time, fd4_4_time;
    four_agg_time = mmap(NULL, sizeof *four_agg_time, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    uint64_t four_proc_times[Ntrials];
    for (int c = 0; c < Ntrials; c++) {
        *four_agg_time = 0;
        //reset time for running all processes-calculate this every iteration
        int pid4_1 = fork();
        int pid4_2 = fork();
        //2 forks for 4 processes

        if (pid4_1 > 0 && pid4_2 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd4_1 = open("contention/contention1.txt", O_RDONLY); //open file
            fd4_1_time = seq_time(fd4_1); //get read time for 1 block
            close(fd4_1); //close file
            *four_agg_time += fd4_1_time; //add time to run all processes
            //printf("Process 1: %lu\n", fd4_1_time);
        }
        else if (pid4_1 == 0 && pid4_2 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd4_2 = open("contention/contention2.txt", O_RDONLY); //open file
            fd4_2_time = seq_time(fd4_2); //get read time for 1 block
            close(fd4_2); //close file
            *four_agg_time += fd4_2_time; //add time to run all processes
            //printf("Process 2: %lu\n", fd4_2_time);
            exit(0);
        }
        else if (pid4_1 > 0 && pid4_2 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd4_3 = open("contention/contention3.txt", O_RDONLY); //open file
            fd4_3_time = seq_time(fd4_3); //get read time for 1 block
            close(fd4_3); //close file
            *four_agg_time += fd4_3_time; //add time to run all processes
            //printf("Process 3: %lu\n", fd4_3_time);
            exit(0);
        }
        else if (pid4_1 == 0 && pid4_2 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd4_4 = open("contention/contention4.txt", O_RDONLY); //open file
            fd4_4_time = seq_time(fd4_4); //get read time for 1 block
            close(fd4_4); //close file
            *four_agg_time += fd4_4_time; //add time to run all processes
            //printf("Process 4: %lu\n", fd4_4_time);
            exit(0);
        }
        //printf("Total Time: %lu\n", *four_agg_time);
        four_proc_times[c] = (*four_agg_time) / 4; //store avg read time per process in array
    }

    uint64_t sum_four_proc, avg_four_proc, var_four_proc, dev_four_proc;
    sum_four_proc = 0;
    var_four_proc = 0;
    for (int c2 = 0; c2 < Ntrials; c2++) {
        sum_four_proc += four_proc_times[c2]; //calculate total read time over iterations
    }

    avg_four_proc = (double) sum_four_proc / (double) Ntrials; //find avg over iterations

    for (int c3 = 0; c3 < Ntrials; c3++) {
        var_four_proc += pow(((four_proc_times[c3]) - avg_four_proc), 2); //calculate total variance
    }
    var_four_proc = ((double)var_four_proc / (double) Ntrials); //variance per iterations
    dev_four_proc = sqrt(var_four_proc); //get deviation

    printf("Avg time for 4 Processes in ns: %lu\n", (avg_four_proc)/3); //convert cycles to ns (3 cycles/ns)
    printf("Deviation for 4 Processes in ns: %lu\n", (dev_four_proc)/3); //convert cycles to ns (3 cycles/ns)


    //Eight Processes
    uint64_t fd8_1_time, fd8_2_time, fd8_3_time, fd8_4_time;
    uint64_t fd8_5_time, fd8_6_time, fd8_7_time, fd8_8_time;
    eight_agg_time = mmap(NULL, sizeof *eight_agg_time, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    uint64_t eight_proc_times[Ntrials];
    for (int d = 0; d < Ntrials; d++) {
        *eight_agg_time = 0;
        //reset time for running all processes-calculate this every iteration

        int pid8_1 = fork();
        int pid8_2 = fork();
        int pid8_3 = fork();
        //3 forks for 8 processes

        if (pid8_1 > 0 & pid8_2 > 0 && pid8_3 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_1 = open("contention/contention1.txt", O_RDONLY); //open file
            fd8_1_time = seq_time(fd8_1); //get read time for 1 block
            close(fd8_1); //close file
            *eight_agg_time += fd8_1_time; //add time to run all processes
            //printf("Process 1: %lu\n", fd8_1_time);
        }
        else if (pid8_1 == 0 && pid8_2 > 0 && pid8_3 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_2 = open("contention/contention2.txt", O_RDONLY); //open file
            fd8_2_time = seq_time(fd8_2); //get read time for 1 block
            close(fd8_2); //close file
            *eight_agg_time += fd8_2_time; //add time to run all processes
            //printf("Process 2: %lu\n", fd8_2_time);
            exit(0);
        }
        else if (pid8_1 > 0 && pid8_2 == 0 && pid8_3 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_3 = open("contention/contention3.txt", O_RDONLY); //open file
            fd8_3_time = seq_time(fd8_3); //get read time for 1 block
            close(fd8_3); //close file
            *eight_agg_time += fd8_3_time; //add time to run all processes
            //printf("Process 3: %lu\n", fd8_3_time);
            exit(0);
        }
        else if (pid8_1 > 0 && pid8_2 > 0 && pid8_3 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_4 = open("contention/contention4.txt", O_RDONLY); //open file
            fd8_4_time = seq_time(fd8_4); //get read time for 1 block
            close(fd8_4); //close file
            *eight_agg_time += fd8_4_time; //add time to run all processes
            //printf("Process 4: %lu\n", fd8_4_time);
            exit(0);
        }
        else if (pid8_1 > 0 && pid8_2 == 0 && pid8_3 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_5 = open("contention/contention5.txt", O_RDONLY); //open file
            fd8_5_time = seq_time(fd8_5); //get read time for 1 block
            close(fd8_5); //close file
            *eight_agg_time += fd8_5_time; //add time to run all processes
            //printf("Process 5: %lu\n", fd8_5_time);
            exit(0);
        }
        else if (pid8_1 == 0 && pid8_2 == 0 && pid8_3 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_6 = open("contention/contention6.txt", O_RDONLY); //open file
            fd8_6_time = seq_time(fd8_6); //get read time for 1 block
            close(fd8_6); //close file
            *eight_agg_time += fd8_6_time; //add time to run all processes
            //printf("Process 6: %lu\n", fd8_6_time);
            exit(0);
        }
        else if (pid8_1 == 0 && pid8_2 > 0 && pid8_3 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_7 = open("contention/contention7.txt", O_RDONLY); //open file
            fd8_7_time = seq_time(fd8_7); //get read time for 1 block
            close(fd8_7); //close file
            *eight_agg_time += fd8_7_time; //add time to run all processes
            //printf("Process 7: %lu\n", fd8_7_time);
            exit(0);
        }
        else if (pid8_1 == 0 && pid8_2 == 0 && pid8_3 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd8_8 = open("contention/contention8.txt", O_RDONLY); //open file
            fd8_8_time = seq_time(fd8_8); //get read time for 1 block
            close(fd8_8); //close file
            *eight_agg_time += fd8_8_time; //add time to run all processes
            //printf("Process 8: %lu\n", fd8_8_time);
            exit(0);
        }
        //printf("Total Time: %lu\n", *eight_agg_time);
        eight_proc_times[d] = (*eight_agg_time) / 8; //store avg read time per process in array
    }

    uint64_t sum_eight_proc, avg_eight_proc, var_eight_proc, dev_eight_proc;
    sum_eight_proc = 0;
    var_eight_proc = 0;
    for (int d2 = 0; d2 < Ntrials; d2++) {
        sum_eight_proc += eight_proc_times[d2]; //calculate total read time over iterations
    }

    avg_eight_proc = (double) sum_eight_proc / (double) Ntrials; //find avg over iterations

    for (int d3 = 0; d3 < Ntrials; d3++) {
        var_eight_proc += pow(((eight_proc_times[d3]) - avg_eight_proc), 2); //calculate total variance
    }
    var_eight_proc = ((double)var_eight_proc / (double) Ntrials); //variance per iterations
    dev_eight_proc = sqrt(var_eight_proc); //get deviation

    printf("Avg time for 8 Processes in ns: %lu\n", (avg_eight_proc)/3); //convert cycles to ns (3 cycles/ns)
    printf("Deviation for 8 Processes in ns: %lu\n", (dev_eight_proc)/3); //convert cycles to ns (3 cycles/ns)


    //Sixteen Processes
    uint64_t fd16_1_time, fd16_2_time, fd16_3_time, fd16_4_time;
    uint64_t fd16_5_time, fd16_6_time, fd16_7_time, fd16_8_time;
    uint64_t fd16_9_time, fd16_10_time, fd16_11_time, fd16_12_time;
    uint64_t fd16_13_time, fd16_14_time, fd16_15_time, fd16_16_time;
    sixteen_agg_time = mmap(NULL, sizeof *sixteen_agg_time, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    uint64_t sixteen_proc_times[Ntrials];
    for (int e = 0; e < Ntrials; e++) {
        *sixteen_agg_time = 0;
        //reset time for running all processes-calculate this every iteration

        int pid16_1 = fork();
        int pid16_2 = fork();
        int pid16_3 = fork();
        int pid16_4 = fork();
        //4 forks for 16 processes

        if (pid16_1 > 0 & pid16_2 > 0 && pid16_3 > 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_1 = open("contention/contention1.txt", O_RDONLY); //open file
            fd16_1_time = seq_time(fd16_1); //get read time for 1 block
            close(fd16_1); //close file
            *sixteen_agg_time += fd16_1_time; //add time to run all processes
            //printf("Process 1: %lu\n", fd16_1_time);
        }
        else if (pid16_1 == 0 && pid16_2 > 0 && pid16_3 > 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_2 = open("contention/contention2.txt", O_RDONLY); //open file
            fd16_2_time = seq_time(fd16_2); //get read time for 1 block
            close(fd16_2); //close file
            *sixteen_agg_time += fd16_2_time; //add time to run all processes
            //printf("Process 2: %lu\n", fd16_2_time);
            exit(0);
        }
        else if (pid16_1 > 0 && pid16_2 == 0 && pid16_3 > 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_3 = open("contention/contention3.txt", O_RDONLY); //open file
            fd16_3_time = seq_time(fd16_3); //get read time for 1 block
            close(fd16_3); //close file
            *sixteen_agg_time += fd16_3_time; //add time to run all processes
            //printf("Process 3: %lu\n", fd16_3_time);
            exit(0);
        }
        else if (pid16_1 == 0 && pid16_2 == 0 && pid16_3 > 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_4 = open("contention/contention4.txt", O_RDONLY); //open file
            fd16_4_time = seq_time(fd16_4); //get read time for 1 block
            close(fd16_4); //close file
            *sixteen_agg_time += fd16_4_time; //add time to run all processes
            //printf("Process 4: %lu\n", fd16_4_time);
            exit(0);
        }
        else if (pid16_1 > 0 && pid16_2 > 0 && pid16_3 == 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_5 = open("contention/contention5.txt", O_RDONLY); //open file
            fd16_5_time = seq_time(fd16_5); //get read time for 1 block
            close(fd16_5); //close file
            *sixteen_agg_time += fd16_5_time; //add time to run all processes
            //printf("Process 5: %lu\n", fd16_5_time);
            exit(0);
        }
        else if (pid16_1 > 0 && pid16_2 > 0 && pid16_3 > 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_6 = open("contention/contention6.txt", O_RDONLY); //open file
            fd16_6_time = seq_time(fd16_6); //get read time for 1 block
            close(fd16_6); //close file
            *sixteen_agg_time += fd16_6_time; //add time to run all processes
            //printf("Process 6: %lu\n", fd16_6_time);
            exit(0);
        }
        else if (pid16_1 > 0 && pid16_2 == 0 && pid16_3 == 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_7 = open("contention/contention7.txt", O_RDONLY); //open file
            fd16_7_time = seq_time(fd16_7); //get read time for 1 block
            close(fd16_7); //close file
            *sixteen_agg_time += fd16_7_time; //add time to run all processes
            //printf("Process 7: %lu\n", fd16_7_time);
            exit(0);
        }
        else if (pid16_1 > 0 && pid16_2 == 0 && pid16_3 == 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_8 = open("contention/contention8.txt", O_RDONLY); //open file
            fd16_8_time = seq_time(fd16_8); //get read time for 1 block
            close(fd16_8); //close file
            *sixteen_agg_time += fd16_8_time; //add time to run all processes
            //printf("Process 8: %lu\n", fd16_8_time);
            exit(0);
        }
        else if (pid16_1 > 0 & pid16_2 == 0 && pid16_3 > 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_9 = open("contention/contention9.txt", O_RDONLY); //open file
            fd16_9_time = seq_time(fd16_9); //get read time for 1 block
            close(fd16_9); //close file
            *sixteen_agg_time += fd16_9_time; //add time to run all processes
            //printf("Process 9: %lu\n", fd16_9_time);
            exit(0);
        }
        else if (pid16_1 > 0 && pid16_2 > 0 && pid16_3 == 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_10 = open("contention/contention10.txt", O_RDONLY); //open file
            fd16_10_time = seq_time(fd16_10); //get read time for 1 block
            close(fd16_10); //close file
            *sixteen_agg_time += fd16_10_time; //add time to run all processes
            //printf("Process 10: %lu\n", fd16_10_time);
            exit(0);
        }
        else if (pid16_1 == 0 && pid16_2 > 0 && pid16_3 > 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_11 = open("contention/contention11.txt", O_RDONLY); //open file
            fd16_11_time = seq_time(fd16_11); //get read time for 1 block
            close(fd16_11); //close file
            *sixteen_agg_time += fd16_11_time; //add time to run all processes
            //printf("Process 11: %lu\n", fd16_11_time);
            exit(0);
        }
        else if (pid16_1 == 0 && pid16_2 > 0 && pid16_3 == 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_12 = open("contention/contention12.txt", O_RDONLY); //open file
            fd16_12_time = seq_time(fd16_12); //get read time for 1 block
            close(fd16_12); //close file
            *sixteen_agg_time += fd16_12_time; //add time to run all processes
            //printf("Process 12: %lu\n", fd16_12_time);
            exit(0);
        }
        else if (pid16_1 == 0 && pid16_2 > 0 && pid16_3 == 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_13 = open("contention/contention13.txt", O_RDONLY); //open file
            fd16_13_time = seq_time(fd16_13); //get read time for 1 block
            close(fd16_13); //close file
            *sixteen_agg_time += fd16_13_time; //add time to run all processes
            //printf("Process 13: %lu\n", fd16_13_time);
            exit(0);
        }
        else if (pid16_1 == 0 && pid16_2 == 0 && pid16_3 > 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_14 = open("contention/contention14.txt", O_RDONLY); //open file
            fd16_14_time = seq_time(fd16_14); //get read time for 1 block
            close(fd16_14); //close file
            *sixteen_agg_time += fd16_14_time; //add time to run all processes
            //printf("Process 14: %lu\n", fd16_14_time);
            exit(0);
        }
        else if (pid16_1 == 0 && pid16_2 == 0 && pid16_3 == 0 && pid16_4 > 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_15 = open("contention/contention15.txt", O_RDONLY); //open file
            fd16_15_time = seq_time(fd16_15); //get read time for 1 block
            close(fd16_15); //close file
            *sixteen_agg_time += fd16_15_time; //add time to run all processes
            //printf("Process 15: %lu\n", fd16_15_time);
            exit(0);
        }
        else if (pid16_1 == 0 && pid16_2 == 0 && pid16_3 == 0 && pid16_4 == 0) {
            while(wait(NULL) > 0) { //wait for all children to execute
                printf("");
            }
            int fd16_16 = open("contention/contention16.txt", O_RDONLY); //open file
            fd16_16_time = seq_time(fd16_16); //get read time for 1 block
            close(fd16_16); //close file
            *sixteen_agg_time += fd16_16_time; //add time to run all processes
            //printf("Process 16: %lu\n", fd16_16_time);
            exit(0);
        }
        //printf("Total Time: %lu\n", *sixteen_agg_time);
        sixteen_proc_times[e] = (*sixteen_agg_time) / 16; //store avg read time per process in array
    }

    uint64_t sum_sixteen_proc, avg_sixteen_proc, var_sixteen_proc, dev_sixteen_proc;
    sum_sixteen_proc = 0;
    var_sixteen_proc = 0;
    for (int e2 = 0; e2 < Ntrials; e2++) {
        sum_sixteen_proc += sixteen_proc_times[e2]; //calculate total read time over iterations
    }

    avg_sixteen_proc = (double) sum_sixteen_proc / (double) Ntrials; //find avg over iterations

    for (int e3 = 0; e3 < Ntrials; e3++) {
        var_sixteen_proc += pow(((sixteen_proc_times[e3]) - avg_sixteen_proc), 2); //calculate total variance
    }
    var_sixteen_proc = ((double)var_sixteen_proc / (double) Ntrials); //variance per iterations
    dev_sixteen_proc = sqrt(var_sixteen_proc); //get deviation

    printf("Avg time for 16 Processes in ns: %lu\n", (avg_sixteen_proc)/3); //convert cycles to ns (3 cycles/ns)
    printf("Deviation for 16 Processes in ns: %lu\n", (dev_sixteen_proc)/3); //convert cycles to ns (3 cycles/ns)

    return 0;
}
