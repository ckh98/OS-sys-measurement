#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <inttypes.h>
#include <fcntl.h>
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

double seq_time(int file) {
    uint64_t start, end; //start and end times (cycles)
    uint64_t total_time = 0; //total time to read block

    struct stat buf;
    fstat(file, &buf);
    size_t size = buf.st_size;
    blksize_t block = buf.st_blksize;

    uint64_t nBlocks = size / block;
    if (size % block == 0) {
        nBlocks -= 1;
    }
    //printf("%ld\n", size);
    //printf("%ld\n", block);

    char *buffer = (char *) malloc(size); //allocate memory for file

    for (size_t i = 0; i <= nBlocks; i++) { //loop through file blocks

        rdtsc();
        ssize_t bytes_read = read(file, buffer, block); //read block
        rdtsc1();

        // If reached end of file
        if (bytes_read <= 0) {
            //printf("\nSHOULD NOT HAVE REACHED HERE\n");
            break;
        }

        start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //start time
        end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end time
        total_time += end - start; //calculate total cycles to read entire file
    }
    free(buffer); //clear memory
    return total_time / (nBlocks + 1); //calculate avg read I/O for each block
}

blksize_t get_block_size(int file) {
    struct stat buf; //get buffer
    fstat(file, &buf); //passes info about file to buffer like block size
    blksize_t blk = buf.st_blksize; //get block size
    return blk; //return block size
}

//compile gcc -o fcache file_cache_size.c -lm
//run sudo ./fcache

int main(int argc, char const *argv[]) {
    //x axis is size of file and y is avg read I/O time
    //measure 512b, 1k, 4k, 10k, 25k, 50k, 100k, 250k, 500k, 1m, 10m
    int Ntrials = 10;
    //128MB
    uint64_t fd128m_time; //cycles to read 1 block
    uint64_t fd128m_times[Ntrials]; //store array of read I/Os
    for (int a = 0; a < Ntrials; a++) { //calculate read I/Os over iterations
        int fd128m = open("filecaches/filecache128m.txt", O_RDONLY); //open file
        fd128m_time = seq_time(fd128m); //avg block read I/O for file
        close(fd128m); //close file
        fd128m_times[a] = fd128m_time; //store avg block read I/O in array
    }

    uint64_t sum_fd128m, avg_fd128m, var_fd128m, dev_fd128m;
    sum_fd128m = 0; //use to calculate total cycles for all iterations
    var_fd128m = 0; //use to calculate variance for all iterations
    for (int a2 = 0; a2 < Ntrials; a2++) {
        sum_fd128m += fd128m_times[a2]; //increment by cycles for each iteration
    }

    avg_fd128m = (double) sum_fd128m / (double) Ntrials; //calculate avg read I/O across iterations

    for (int a3 = 0; a3 < Ntrials; a3++) {
        var_fd128m += pow(((fd128m_times[a3]) - avg_fd128m), 2); //calculating variance for each iteration
    }
    var_fd128m = ((double)var_fd128m / (double) Ntrials); //variance across iterations
    dev_fd128m = sqrt(var_fd128m); //standard deviation across iterations

    printf("Avg Time for 128 MB in ns: %lu\n", (avg_fd128m)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 128 MB in ns: %lu\n", (dev_fd128m)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    //256MB
    uint64_t fd256m_time; //cycles to read 1 block
    uint64_t fd256m_times[Ntrials]; //store array of read I/Os
    for (int b = 0; b < Ntrials; b++) { //calculate read I/Os over iterations
        int fd256m = open("filecaches/filecache256m.txt", O_RDONLY); //open file
        fd256m_time = seq_time(fd256m); //avg block read I/O for file
        close(fd256m); //close file
        fd256m_times[b] = fd256m_time; //store avg block read I/O in array
    }

    uint64_t sum_fd256m, avg_fd256m, var_fd256m, dev_fd256m;
    sum_fd256m = 0; //use to calculate total cycles for all iterations
    var_fd256m = 0; //use to calculate variance for all iterations
    for (int b2 = 0; b2 < Ntrials; b2++) {
        sum_fd256m += fd256m_times[b2]; //increment by cycles for each iteration
    }

    avg_fd256m = (double) sum_fd256m / (double) Ntrials; //calculate avg read I/O across iterations

    for (int b3 = 0; b3 < Ntrials; b3++) {
        var_fd256m += pow(((fd256m_times[b3]) - avg_fd256m), 2); //calculating variance for each iteration
    }
    var_fd256m = ((double)var_fd256m / (double) Ntrials); //variance across iterations
    dev_fd256m = sqrt(var_fd256m); //standard deviation across iterations

    printf("Avg Time for 256 MB in ns: %lu\n", (avg_fd256m)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 256 MB in ns: %lu\n", (dev_fd256m)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    //512MB
    uint64_t fd512m_time; //cycles to read 1 block
    uint64_t fd512m_times[Ntrials]; //store array of read I/Os
    for (int c = 0; c < Ntrials; c++) { //calculate read I/Os over iterations
        int fd512m = open("filecaches/filecache512m.txt", O_RDONLY); //open file
        fd512m_time = seq_time(fd512m); //avg block read I/O for file
        close(fd512m); //close file
        fd512m_times[c] = fd512m_time; //store avg block read I/O in array
    }

    uint64_t sum_fd512m, avg_fd512m, var_fd512m, dev_fd512m;
    sum_fd512m = 0; //use to calculate total cycles for all iterations
    var_fd512m = 0; //use to calculate variance for all iterations
    for (int c2 = 0; c2 < Ntrials; c2++) {
        sum_fd512m += fd512m_times[c2]; //increment by cycles for each iteration
    }

    avg_fd512m = (double) sum_fd512m / (double) Ntrials; //calculate avg read I/O across iterations

    for (int c3 = 0; c3 < Ntrials; c3++) {
        var_fd512m += pow(((fd512m_times[c3]) - avg_fd512m), 2); //calculating variance for each iteration
    }
    var_fd512m = ((double)var_fd512m / (double) Ntrials); //variance across iterations
    dev_fd512m = sqrt(var_fd512m); //standard deviation across iterations

    printf("Avg Time for 512 MB in ns: %lu\n", (avg_fd512m)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 512 MB in ns: %lu\n", (dev_fd512m)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    //1 GB
    uint64_t fd1g_time; //cycles to read 1 block
    uint64_t fd1g_times[Ntrials]; //store array of read I/Os
    for (int d = 0; d < Ntrials; d++) { //calculate read I/Os over iterations
        int fd1g = open("filecaches/filecache1g.txt", O_RDONLY); //open file
        fd1g_time = seq_time(fd1g); //avg block read I/O for file
        close(fd1g); //close file
        fd1g_times[d] = fd1g_time; //store avg block read I/O in array
    }

    uint64_t sum_fd1g, avg_fd1g, var_fd1g, dev_fd1g;
    sum_fd1g = 0; //use to calculate total cycles for all iterations
    var_fd1g = 0; //use to calculate variance for all iterations
    for (int d2 = 0; d2 < Ntrials; d2++) {
        sum_fd1g += fd1g_times[d2]; //increment by cycles for each iteration
    }

    avg_fd1g = (double) sum_fd1g / (double) Ntrials; //calculate avg read I/O across iterations

    for (int d3 = 0; d3 < Ntrials; d3++) {
        var_fd1g += pow(((fd1g_times[d3]) - avg_fd1g), 2); //calculating variance for each iteration
    }
    var_fd1g = ((double)var_fd1g / (double) Ntrials); //variance across iterations
    dev_fd1g = sqrt(var_fd1g); //standard deviation across iterations

    printf("Avg Time for 1 GB in ns: %lu\n", (avg_fd1g)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 1 GB in ns: %lu\n", (dev_fd1g)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    //2 GB
    uint64_t fd2g_time; //cycles to read 1 block
    uint64_t fd2g_times[Ntrials]; //store array of read I/Os
    for (int e = 0; e < Ntrials; e++) { //calculate read I/Os over iterations
        int fd2g = open("filecaches/filecache2g.txt", O_RDONLY); //open file
        fd2g_time = seq_time(fd2g); //avg block read I/O for file
        close(fd2g); //close file
        fd2g_times[e] = fd2g_time; //store avg block read I/O in array
    }

    uint64_t sum_fd2g, avg_fd2g, var_fd2g, dev_fd2g;
    sum_fd2g = 0; //use to calculate total cycles for all iterations
    var_fd2g = 0; //use to calculate variance for all iterations
    for (int e2 = 0; e2 < Ntrials; e2++) {
        sum_fd2g += fd2g_times[e2]; //increment by cycles for each iteration
    }

    avg_fd2g = (double) sum_fd2g / (double) Ntrials; //calculate avg read I/O across iterations

    for (int e3 = 0; e3 < Ntrials; e3++) {
        var_fd2g += pow(((fd2g_times[e3]) - avg_fd2g), 2); //calculating variance for each iteration
    }
    var_fd2g = ((double)var_fd2g / (double) Ntrials); //variance across iterations
    dev_fd2g = sqrt(var_fd2g); //standard deviation across iterations

    printf("Avg Time for 2 GB in ns: %lu\n", (avg_fd2g)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 2 GB in ns: %lu\n", (dev_fd2g)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    //8 GB
    uint64_t fd8g_time; //cycles to read 1 block
    uint64_t fd8g_start, fd8g_end; //start and end time for cycles
    uint64_t fd8g_times[Ntrials]; //store array of read I/Os
    for (int f = 0; f < Ntrials; f++) { //calculate read I/Os over iterations
        FILE *fp8g = fopen("filecaches/filecache8g.txt", "r"); //open file
        char buff8g[4096]; //open buffer for 4KB read
        rdtsc(); //start timer for cycles
        fread(buff8g, sizeof(buff8g), 1, fp8g); //read 4KB from file
        rdtsc1(); //end timer for cycles
        fd8g_start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //start time
        fd8g_end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end time
        fd8g_times[f] = fd8g_end - fd8g_start; //store cycles for each iteration in array
    }

    uint64_t sum_fd8g, avg_fd8g, var_fd8g, dev_fd8g;
    sum_fd8g = 0; //use to calculate total cycles for all iterations
    var_fd8g = 0; //use to calculate variance for all iterations
    for (int f2 = 0; f2 < Ntrials; f2++) {
        sum_fd8g += fd8g_times[f2]; //increment by cycles for each iteration
    }

    avg_fd8g = (double) sum_fd8g / (double) Ntrials; //calculate avg read I/O across iterations

    for (int f3 = 0; f3 < Ntrials; f3++) {
        var_fd8g += pow(((fd8g_times[f3]) - avg_fd8g), 2); //calculating variance for each iteration
    }
    var_fd8g = ((double)var_fd8g / (double) Ntrials); //variance across iterations
    dev_fd8g = sqrt(var_fd8g); //standard deviation across iterations

    printf("Avg Time for 8 GB in ns: %lu\n", (avg_fd8g)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 8 GB in ns: %lu\n", (dev_fd8g)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    //16 GB
    uint64_t fd16g_time; //cycles to read 1 block
    uint64_t fd16g_start, fd16g_end;
    uint64_t fd16g_times[Ntrials]; //store array of read I/Os
    for (int g = 0; g < Ntrials; g++) { //calculate read I/Os over iterations
        FILE *fp16g = fopen("filecaches/filecache16g.txt", "r"); //open file
        char buff16g[4096]; //open buffer for 4KB read
        rdtsc(); //start timer for cycles
        fread(buff16g, sizeof(buff16g), 1, fp16g); //read 4KB from file
        rdtsc1(); //end timer for cycles
        fd16g_start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //start time
        fd16g_end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end time
        fd16g_times[g] = fd16g_end - fd16g_start; //store cycles for each iteration in array
    }

    uint64_t sum_fd16g, avg_fd16g, var_fd16g, dev_fd16g;
    sum_fd16g = 0; //use to calculate total cycles for all iterations
    var_fd16g = 0; //use to calculate variance for all iterations
    for (int g2 = 0; g2 < Ntrials; g2++) {
        sum_fd16g += fd16g_times[g2]; //increment by cycles for each iteration
    }

    avg_fd16g = (double) sum_fd16g / (double) Ntrials; //calculate avg read I/O across iterations

    for (int g3 = 0; g3 < Ntrials; g3++) {
        var_fd16g += pow(((fd16g_times[g3]) - avg_fd16g), 2); //calculating variance for each iteration
    }
    var_fd16g = ((double)var_fd16g / (double) Ntrials); //variance across iterations
    dev_fd16g = sqrt(var_fd16g); //standard deviation across iterations

    printf("Avg Time for 16 GB in ns: %lu\n", (avg_fd16g)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 16 GB in ns: %lu\n", (dev_fd16g)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    //32 GB
    uint64_t fd32g_time; //cycles to read 1 block
    uint64_t fd32g_start, fd32g_end;
    uint64_t fd32g_times[Ntrials]; //store array of read I/Os
    for (int h = 0; h < Ntrials; h++) { //calculate read I/Os over iterations
        FILE *fp32g = fopen("filecaches/filecache32g.txt", "r"); //open file
        char buff32g[4096]; //open buffer for 4KB read
        rdtsc(); //start timer for cycles
        fread(buff32g, sizeof(buff32g), 1, fp32g); //read 4KB from file
        rdtsc1(); //end timer for cycles
        fd32g_start = ( ((u_int64_t) cycles_high << 32) | cycles_low ); //start time
        fd32g_end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 ); //end time
        fd32g_times[h] = fd32g_end - fd32g_start; //store cycles for each iteration in array
    }

    uint64_t sum_fd32g, avg_fd32g, var_fd32g, dev_fd32g;
    sum_fd32g = 0; //use to calculate total cycles for all iterations
    var_fd32g = 0; //use to calculate variance for all iterations
    for (int h2 = 0; h2 < Ntrials; h2++) {
        sum_fd32g += fd32g_times[h2];
    }

    avg_fd32g = (double) sum_fd32g / (double) Ntrials; //calculate avg read I/O across iterations

    for (int h3 = 0; h3 < Ntrials; h3++) {
        var_fd32g += pow(((fd32g_times[h3]) - avg_fd32g), 2); //calculating variance for each iteration
    }
    var_fd32g = ((double)var_fd32g / (double) Ntrials); //variance across iterations
    dev_fd32g = sqrt(var_fd32g); //standard deviation across iterations

    printf("Avg Time for 32 GB in ns: %lu\n", (avg_fd32g)/(3)); //avg read I/O per block and convert to ns (3 cycles/ns)
    printf("Deviation for 32 GB in ns: %lu\n", (dev_fd32g)/(3));//deviation for each read I/O and convert to ns (3 cycles/ns)

    return 0;
}
