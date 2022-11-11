#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <inttypes.h>
#include <fcntl.h>

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

// Sequential access measurement
uint64_t seq_time(int file) {
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

    // printf("Size: %ld\n", size);
    // printf("Block Size: %ld\n", block);
    //
    // printf("----Flushing out cache----\n");
    system(SHELLSCRIPT);

    char *buffer = (char *) malloc(size);

    // Reads the entire file
    for (size_t i = 0; i <= nBlocks; i++) {

        // We are only measuring reading the file
        // Sequential Access
        rdtsc();
        ssize_t bytes_read = read(file, buffer, block);
        rdtsc1();

        // If reached end of file or the read somehow failed
        if (bytes_read <= 0) {
            printf("\nREAD FAILED\n");
            break;
        }

        start = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        total_time += end - start;
    }

    free(buffer);
    return total_time / (nBlocks + 1);
}

// Random Access measurement
uint64_t rand_time(int file) {
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

    // printf("Size: %ld\n", size);
    // printf("Block Size: %ld\n", block);
    //
    // printf("----Flushing out cache----\n");
    system(SHELLSCRIPT);

    char *buffer = (char *) malloc(size);

    off_t offset;

    // Reads the entire file
    for (size_t i = 0; i <= nBlocks; i++) {

        // Choose to read from a random block between (0, nBlocks)
        if (nBlocks != 0) {
            offset = (rand() % nBlocks) * block;
        }
        // Edge case
        else {
            offset = 0;
        }

        // Setting offset. SEEK_SET points to beginning of the file, so offset
        // will set the file position to the value of offset
        lseek(file, offset, SEEK_SET);

        // We are only measuring reading the file
        // Random Access
        rdtsc();
        ssize_t bytes_read = read(file, buffer, block);
        rdtsc1();

        // If reached end of file or the read somehow failed
        // Should not reach here
        if (bytes_read <= 0) {
            printf("\nSHOULD NOT HAVE REACHED HERE\n");
            break;
        }

        start = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        total_time += end - start;
    }

    free(buffer);
    return total_time / (nBlocks + 1);
}

//compile -> gcc -o readTime read_file.c -lm
//run -> sudo ./readTime
int main(int argc, char* argv[]) {
    int iterations = 10;
    uint64_t start, end;

    //----------------log7----------------

    // Arrays to store times for each iteration
    uint64_t seq_log7_times[iterations];
    uint64_t rand_log7_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log7 = open("fileread/fileread7.txt", O_RDONLY);

        uint64_t seq_log7 = seq_time(fd_log7);    // sequential access
        uint64_t rand_log7 = rand_time(fd_log7);  // random access
        close(fd_log7);

        seq_log7_times[i] = seq_log7;
        rand_log7_times[i] = rand_log7;
    }

    uint64_t sum_seq_log7, avg_seq_log7, var_seq_log7, dev_seq_log7;
    uint64_t sum_rand_log7, avg_rand_log7, var_rand_log7, dev_rand_log7;
    sum_seq_log7 = 0;
    var_seq_log7 = 0;
    sum_rand_log7 = 0;
    var_rand_log7 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log7 += seq_log7_times[a2]; //find total time to read entire file over iterations
        sum_rand_log7 += rand_log7_times[a2];
    }

    avg_seq_log7 = (double) sum_seq_log7 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log7 = (double) sum_rand_log7 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log7 += pow(((seq_log7_times[a3]) - avg_seq_log7), 2); //total variance
        var_rand_log7 += pow(((rand_log7_times[a3]) - avg_rand_log7), 2); //total variance
    }

    var_seq_log7 = ((double)var_seq_log7 / (double) iterations); //variance over iterations
    var_rand_log7 = ((double)var_rand_log7 / (double) iterations); //variance over iterations
    dev_seq_log7 = sqrt(var_seq_log7); //deviation
    dev_rand_log7 = sqrt(var_rand_log7); //deviation

    printf("\nTimes for file size log7:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log7)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log7)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log7)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log7)/(3));

    //----------------log8----------------

    // Arrays to store times for each iteration
    uint64_t seq_log8_times[iterations];
    uint64_t rand_log8_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log8 = open("fileread/fileread8.txt", O_RDONLY);

        uint64_t seq_log8 = seq_time(fd_log8);    // sequential access
        uint64_t rand_log8 = rand_time(fd_log8);  // random access
        close(fd_log8);

        seq_log8_times[i] = seq_log8;
        rand_log8_times[i] = rand_log8;
    }

    uint64_t sum_seq_log8, avg_seq_log8, var_seq_log8, dev_seq_log8;
    uint64_t sum_rand_log8, avg_rand_log8, var_rand_log8, dev_rand_log8;
    sum_seq_log8 = 0;
    var_seq_log8 = 0;
    sum_rand_log8 = 0;
    var_rand_log8 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log8 += seq_log8_times[a2]; //find total time to read entire file over iterations
        sum_rand_log8 += rand_log8_times[a2];
    }

    avg_seq_log8 = (double) sum_seq_log8 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log8 = (double) sum_rand_log8 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log8 += pow(((seq_log8_times[a3]) - avg_seq_log8), 2); //total variance
        var_rand_log8 += pow(((rand_log8_times[a3]) - avg_rand_log8), 2); //total variance
    }

    var_seq_log8 = ((double)var_seq_log8 / (double) iterations); //variance over iterations
    var_rand_log8 = ((double)var_rand_log8 / (double) iterations); //variance over iterations
    dev_seq_log8 = sqrt(var_seq_log8); //deviation
    dev_rand_log8 = sqrt(var_rand_log8); //deviation

    printf("\nTimes for file size log8:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log8)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log8)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log8)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log8)/(3));

    //----------------log9----------------

    // Arrays to store times for each iteration
    uint64_t seq_log9_times[iterations];
    uint64_t rand_log9_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log9 = open("fileread/fileread9.txt", O_RDONLY);

        uint64_t seq_log9 = seq_time(fd_log9);    // sequential access
        uint64_t rand_log9 = rand_time(fd_log9);  // random access
        close(fd_log9);

        seq_log9_times[i] = seq_log9;
        rand_log9_times[i] = rand_log9;
    }

    uint64_t sum_seq_log9, avg_seq_log9, var_seq_log9, dev_seq_log9;
    uint64_t sum_rand_log9, avg_rand_log9, var_rand_log9, dev_rand_log9;
    sum_seq_log9 = 0;
    var_seq_log9 = 0;
    sum_rand_log9 = 0;
    var_rand_log9 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log9 += seq_log9_times[a2]; //find total time to read entire file over iterations
        sum_rand_log9 += rand_log9_times[a2];
    }

    avg_seq_log9 = (double) sum_seq_log9 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log9 = (double) sum_rand_log9 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log9 += pow(((seq_log9_times[a3]) - avg_seq_log9), 2); //total variance
        var_rand_log9 += pow(((rand_log9_times[a3]) - avg_rand_log9), 2); //total variance
    }

    var_seq_log9 = ((double)var_seq_log9 / (double) iterations); //variance over iterations
    var_rand_log9 = ((double)var_rand_log9 / (double) iterations); //variance over iterations
    dev_seq_log9 = sqrt(var_seq_log9); //deviation
    dev_rand_log9 = sqrt(var_rand_log9); //deviation

    printf("\nTimes for file size log9:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log9)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log9)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log9)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log9)/(3));

    //----------------log10----------------

    // Arrays to store times for each iteration
    uint64_t seq_log10_times[iterations];
    uint64_t rand_log10_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log10 = open("fileread/fileread10.txt", O_RDONLY);

        uint64_t seq_log10 = seq_time(fd_log10);    // sequential access
        uint64_t rand_log10 = rand_time(fd_log10);  // random access
        close(fd_log10);

        seq_log10_times[i] = seq_log10;
        rand_log10_times[i] = rand_log10;
    }

    uint64_t sum_seq_log10, avg_seq_log10, var_seq_log10, dev_seq_log10;
    uint64_t sum_rand_log10, avg_rand_log10, var_rand_log10, dev_rand_log10;
    sum_seq_log10 = 0;
    var_seq_log10 = 0;
    sum_rand_log10 = 0;
    var_rand_log10 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log10 += seq_log10_times[a2]; //find total time to read entire file over iterations
        sum_rand_log10 += rand_log10_times[a2];
    }

    avg_seq_log10 = (double) sum_seq_log10 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log10 = (double) sum_rand_log10 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log10 += pow(((seq_log10_times[a3]) - avg_seq_log10), 2); //total variance
        var_rand_log10 += pow(((rand_log10_times[a3]) - avg_rand_log10), 2); //total variance
    }

    var_seq_log10 = ((double)var_seq_log10 / (double) iterations); //variance over iterations
    var_rand_log10 = ((double)var_rand_log10 / (double) iterations); //variance over iterations
    dev_seq_log10 = sqrt(var_seq_log10); //deviation
    dev_rand_log10 = sqrt(var_rand_log10); //deviation

    printf("\nTimes for file size log10:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log10)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log10)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log10)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log10)/(3));

    //----------------log11----------------

    // Arrays to store times for each iteration
    uint64_t seq_log11_times[iterations];
    uint64_t rand_log11_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log11 = open("fileread/fileread11.txt", O_RDONLY);

        uint64_t seq_log11 = seq_time(fd_log11);    // sequential access
        uint64_t rand_log11 = rand_time(fd_log11);  // random access
        close(fd_log11);

        seq_log11_times[i] = seq_log11;
        rand_log11_times[i] = rand_log11;
    }

    uint64_t sum_seq_log11, avg_seq_log11, var_seq_log11, dev_seq_log11;
    uint64_t sum_rand_log11, avg_rand_log11, var_rand_log11, dev_rand_log11;
    sum_seq_log11 = 0;
    var_seq_log11 = 0;
    sum_rand_log11 = 0;
    var_rand_log11 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log11 += seq_log11_times[a2]; //find total time to read entire file over iterations
        sum_rand_log11 += rand_log11_times[a2];
    }

    avg_seq_log11 = (double) sum_seq_log11 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log11 = (double) sum_rand_log11 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log11 += pow(((seq_log11_times[a3]) - avg_seq_log11), 2); //total variance
        var_rand_log11 += pow(((rand_log11_times[a3]) - avg_rand_log11), 2); //total variance
    }

    var_seq_log11 = ((double)var_seq_log11 / (double) iterations); //variance over iterations
    var_rand_log11 = ((double)var_rand_log11 / (double) iterations); //variance over iterations
    dev_seq_log11 = sqrt(var_seq_log11); //deviation
    dev_rand_log11 = sqrt(var_rand_log11); //deviation

    printf("\nTimes for file size log11:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log11)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log11)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log11)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log11)/(3));

    //----------------log12----------------

    // Arrays to store times for each iteration
    uint64_t seq_log12_times[iterations];
    uint64_t rand_log12_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log12 = open("fileread/fileread12.txt", O_RDONLY);

        uint64_t seq_log12 = seq_time(fd_log12);    // sequential access
        uint64_t rand_log12 = rand_time(fd_log12);  // random access
        close(fd_log12);

        seq_log12_times[i] = seq_log12;
        rand_log12_times[i] = rand_log12;
    }

    uint64_t sum_seq_log12, avg_seq_log12, var_seq_log12, dev_seq_log12;
    uint64_t sum_rand_log12, avg_rand_log12, var_rand_log12, dev_rand_log12;
    sum_seq_log12 = 0;
    var_seq_log12 = 0;
    sum_rand_log12 = 0;
    var_rand_log12 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log12 += seq_log12_times[a2]; //find total time to read entire file over iterations
        sum_rand_log12 += rand_log12_times[a2];
    }

    avg_seq_log12 = (double) sum_seq_log12 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log12 = (double) sum_rand_log12 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log12 += pow(((seq_log12_times[a3]) - avg_seq_log12), 2); //total variance
        var_rand_log12 += pow(((rand_log12_times[a3]) - avg_rand_log12), 2); //total variance
    }

    var_seq_log12 = ((double)var_seq_log12 / (double) iterations); //variance over iterations
    var_rand_log12 = ((double)var_rand_log12 / (double) iterations); //variance over iterations
    dev_seq_log12 = sqrt(var_seq_log12); //deviation
    dev_rand_log12 = sqrt(var_rand_log12); //deviation

    printf("\nTimes for file size log12:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log12)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log12)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log12)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log12)/(3));

    //----------------log13----------------

    // Arrays to store times for each iteration
    uint64_t seq_log13_times[iterations];
    uint64_t rand_log13_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log13 = open("fileread/fileread13.txt", O_RDONLY);

        uint64_t seq_log13 = seq_time(fd_log13);    // sequential access
        uint64_t rand_log13 = rand_time(fd_log13);  // random access
        close(fd_log13);

        seq_log13_times[i] = seq_log13;
        rand_log13_times[i] = rand_log13;
    }

    uint64_t sum_seq_log13, avg_seq_log13, var_seq_log13, dev_seq_log13;
    uint64_t sum_rand_log13, avg_rand_log13, var_rand_log13, dev_rand_log13;
    sum_seq_log13 = 0;
    var_seq_log13 = 0;
    sum_rand_log13 = 0;
    var_rand_log13 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log13 += seq_log13_times[a2]; //find total time to read entire file over iterations
        sum_rand_log13 += rand_log13_times[a2];
    }

    avg_seq_log13 = (double) sum_seq_log13 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log13 = (double) sum_rand_log13 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log13 += pow(((seq_log13_times[a3]) - avg_seq_log13), 2); //total variance
        var_rand_log13 += pow(((rand_log13_times[a3]) - avg_rand_log13), 2); //total variance
    }

    var_seq_log13 = ((double)var_seq_log13 / (double) iterations); //variance over iterations
    var_rand_log13 = ((double)var_rand_log13 / (double) iterations); //variance over iterations
    dev_seq_log13 = sqrt(var_seq_log13); //deviation
    dev_rand_log13 = sqrt(var_rand_log13); //deviation

    printf("\nTimes for file size log13:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log13)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log13)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log13)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log13)/(3));

    //----------------log14----------------

    // Arrays to store times for each iteration
    uint64_t seq_log14_times[iterations];
    uint64_t rand_log14_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log14 = open("fileread/fileread14.txt", O_RDONLY);

        uint64_t seq_log14 = seq_time(fd_log14);    // sequential access
        uint64_t rand_log14 = rand_time(fd_log14);  // random access
        close(fd_log14);

        seq_log14_times[i] = seq_log14;
        rand_log14_times[i] = rand_log14;
    }

    uint64_t sum_seq_log14, avg_seq_log14, var_seq_log14, dev_seq_log14;
    uint64_t sum_rand_log14, avg_rand_log14, var_rand_log14, dev_rand_log14;
    sum_seq_log14 = 0;
    var_seq_log14 = 0;
    sum_rand_log14 = 0;
    var_rand_log14 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log14 += seq_log14_times[a2]; //find total time to read entire file over iterations
        sum_rand_log14 += rand_log14_times[a2];
    }

    avg_seq_log14 = (double) sum_seq_log14 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log14 = (double) sum_rand_log14 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log14 += pow(((seq_log14_times[a3]) - avg_seq_log14), 2); //total variance
        var_rand_log14 += pow(((rand_log14_times[a3]) - avg_rand_log14), 2); //total variance
    }

    var_seq_log14 = ((double)var_seq_log14 / (double) iterations); //variance over iterations
    var_rand_log14 = ((double)var_rand_log14 / (double) iterations); //variance over iterations
    dev_seq_log14 = sqrt(var_seq_log14); //deviation
    dev_rand_log14 = sqrt(var_rand_log14); //deviation

    printf("\nTimes for file size log14:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log14)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log14)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log14)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log14)/(3));

    //----------------log15----------------

    // Arrays to store times for each iteration
    uint64_t seq_log15_times[iterations];
    uint64_t rand_log15_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log15 = open("fileread/fileread15.txt", O_RDONLY);

        uint64_t seq_log15 = seq_time(fd_log15);    // sequential access
        uint64_t rand_log15 = rand_time(fd_log15);  // random access
        close(fd_log15);

        seq_log15_times[i] = seq_log15;
        rand_log15_times[i] = rand_log15;
    }

    uint64_t sum_seq_log15, avg_seq_log15, var_seq_log15, dev_seq_log15;
    uint64_t sum_rand_log15, avg_rand_log15, var_rand_log15, dev_rand_log15;
    sum_seq_log15 = 0;
    var_seq_log15 = 0;
    sum_rand_log15 = 0;
    var_rand_log15 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log15 += seq_log15_times[a2]; //find total time to read entire file over iterations
        sum_rand_log15 += rand_log15_times[a2];
    }

    avg_seq_log15 = (double) sum_seq_log15 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log15 = (double) sum_rand_log15 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log15 += pow(((seq_log15_times[a3]) - avg_seq_log15), 2); //total variance
        var_rand_log15 += pow(((rand_log15_times[a3]) - avg_rand_log15), 2); //total variance
    }

    var_seq_log15 = ((double)var_seq_log15 / (double) iterations); //variance over iterations
    var_rand_log15 = ((double)var_rand_log15 / (double) iterations); //variance over iterations
    dev_seq_log15 = sqrt(var_seq_log15); //deviation
    dev_rand_log15 = sqrt(var_rand_log15); //deviation

    printf("\nTimes for file size log15:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log15)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log15)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log15)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log15)/(3));

    //----------------log16----------------

    // Arrays to store times for each iteration
    uint64_t seq_log16_times[iterations];
    uint64_t rand_log16_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log16 = open("fileread/fileread16.txt", O_RDONLY);

        uint64_t seq_log16 = seq_time(fd_log16);    // sequential access
        uint64_t rand_log16 = rand_time(fd_log16);  // random access
        close(fd_log16);

        seq_log16_times[i] = seq_log16;
        rand_log16_times[i] = rand_log16;
    }

    uint64_t sum_seq_log16, avg_seq_log16, var_seq_log16, dev_seq_log16;
    uint64_t sum_rand_log16, avg_rand_log16, var_rand_log16, dev_rand_log16;
    sum_seq_log16 = 0;
    var_seq_log16 = 0;
    sum_rand_log16 = 0;
    var_rand_log16 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log16 += seq_log16_times[a2]; //find total time to read entire file over iterations
        sum_rand_log16 += rand_log16_times[a2];
    }

    avg_seq_log16 = (double) sum_seq_log16 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log16 = (double) sum_rand_log16 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log16 += pow(((seq_log16_times[a3]) - avg_seq_log16), 2); //total variance
        var_rand_log16 += pow(((rand_log16_times[a3]) - avg_rand_log16), 2); //total variance
    }

    var_seq_log16 = ((double)var_seq_log16 / (double) iterations); //variance over iterations
    var_rand_log16 = ((double)var_rand_log16 / (double) iterations); //variance over iterations
    dev_seq_log16 = sqrt(var_seq_log16); //deviation
    dev_rand_log16 = sqrt(var_rand_log16); //deviation

    printf("\nTimes for file size log16:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log16)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log16)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log16)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log16)/(3));

    //----------------log17----------------

    // Arrays to store times for each iteration
    uint64_t seq_log17_times[iterations];
    uint64_t rand_log17_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log17 = open("fileread/fileread17.txt", O_RDONLY);

        uint64_t seq_log17 = seq_time(fd_log17);    // sequential access
        uint64_t rand_log17 = rand_time(fd_log17);  // random access
        close(fd_log17);

        seq_log17_times[i] = seq_log17;
        rand_log17_times[i] = rand_log17;
    }

    uint64_t sum_seq_log17, avg_seq_log17, var_seq_log17, dev_seq_log17;
    uint64_t sum_rand_log17, avg_rand_log17, var_rand_log17, dev_rand_log17;
    sum_seq_log17 = 0;
    var_seq_log17 = 0;
    sum_rand_log17 = 0;
    var_rand_log17 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log17 += seq_log17_times[a2]; //find total time to read entire file over iterations
        sum_rand_log17 += rand_log17_times[a2];
    }

    avg_seq_log17 = (double) sum_seq_log17 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log17 = (double) sum_rand_log17 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log17 += pow(((seq_log17_times[a3]) - avg_seq_log17), 2); //total variance
        var_rand_log17 += pow(((rand_log17_times[a3]) - avg_rand_log17), 2); //total variance
    }

    var_seq_log17 = ((double)var_seq_log17 / (double) iterations); //variance over iterations
    var_rand_log17 = ((double)var_rand_log17 / (double) iterations); //variance over iterations
    dev_seq_log17 = sqrt(var_seq_log17); //deviation
    dev_rand_log17 = sqrt(var_rand_log17); //deviation

    printf("\nTimes for file size log17:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log17)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log17)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log17)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log17)/(3));

    //----------------log18----------------

    // Arrays to store times for each iteration
    uint64_t seq_log18_times[iterations];
    uint64_t rand_log18_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log18 = open("fileread/fileread18.txt", O_RDONLY);

        uint64_t seq_log18 = seq_time(fd_log18);    // sequential access
        uint64_t rand_log18 = rand_time(fd_log18);  // random access
        close(fd_log18);

        seq_log18_times[i] = seq_log18;
        rand_log18_times[i] = rand_log18;
    }

    uint64_t sum_seq_log18, avg_seq_log18, var_seq_log18, dev_seq_log18;
    uint64_t sum_rand_log18, avg_rand_log18, var_rand_log18, dev_rand_log18;
    sum_seq_log18 = 0;
    var_seq_log18 = 0;
    sum_rand_log18 = 0;
    var_rand_log18 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log18 += seq_log18_times[a2]; //find total time to read entire file over iterations
        sum_rand_log18 += rand_log18_times[a2];
    }

    avg_seq_log18 = (double) sum_seq_log18 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log18 = (double) sum_rand_log18 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log18 += pow(((seq_log18_times[a3]) - avg_seq_log18), 2); //total variance
        var_rand_log18 += pow(((rand_log18_times[a3]) - avg_rand_log18), 2); //total variance
    }

    var_seq_log18 = ((double)var_seq_log18 / (double) iterations); //variance over iterations
    var_rand_log18 = ((double)var_rand_log18 / (double) iterations); //variance over iterations
    dev_seq_log18 = sqrt(var_seq_log18); //deviation
    dev_rand_log18 = sqrt(var_rand_log18); //deviation

    printf("\nTimes for file size log18:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log18)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log18)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log18)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log18)/(3));

    //----------------log19----------------

    // Arrays to store times for each iteration
    uint64_t seq_log19_times[iterations];
    uint64_t rand_log19_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log19 = open("fileread/fileread19.txt", O_RDONLY);

        uint64_t seq_log19 = seq_time(fd_log19);    // sequential access
        uint64_t rand_log19 = rand_time(fd_log19);  // random access
        close(fd_log19);

        seq_log19_times[i] = seq_log19;
        rand_log19_times[i] = rand_log19;
    }

    uint64_t sum_seq_log19, avg_seq_log19, var_seq_log19, dev_seq_log19;
    uint64_t sum_rand_log19, avg_rand_log19, var_rand_log19, dev_rand_log19;
    sum_seq_log19 = 0;
    var_seq_log19 = 0;
    sum_rand_log19 = 0;
    var_rand_log19 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log19 += seq_log19_times[a2]; //find total time to read entire file over iterations
        sum_rand_log19 += rand_log19_times[a2];
    }

    avg_seq_log19 = (double) sum_seq_log19 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log19 = (double) sum_rand_log19 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log19 += pow(((seq_log19_times[a3]) - avg_seq_log19), 2); //total variance
        var_rand_log19 += pow(((rand_log19_times[a3]) - avg_rand_log19), 2); //total variance
    }

    var_seq_log19 = ((double)var_seq_log19 / (double) iterations); //variance over iterations
    var_rand_log19 = ((double)var_rand_log19 / (double) iterations); //variance over iterations
    dev_seq_log19 = sqrt(var_seq_log19); //deviation
    dev_rand_log19 = sqrt(var_rand_log19); //deviation

    printf("\nTimes for file size log19:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log19)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log19)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log19)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log19)/(3));

    //----------------log20----------------

    // Arrays to store times for each iteration
    uint64_t seq_log20_times[iterations];
    uint64_t rand_log20_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log20 = open("fileread/fileread20.txt", O_RDONLY);

        uint64_t seq_log20 = seq_time(fd_log20);    // sequential access
        uint64_t rand_log20 = rand_time(fd_log20);  // random access
        close(fd_log20);

        seq_log20_times[i] = seq_log20;
        rand_log20_times[i] = rand_log20;
    }

    uint64_t sum_seq_log20, avg_seq_log20, var_seq_log20, dev_seq_log20;
    uint64_t sum_rand_log20, avg_rand_log20, var_rand_log20, dev_rand_log20;
    sum_seq_log20 = 0;
    var_seq_log20 = 0;
    sum_rand_log20 = 0;
    var_rand_log20 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log20 += seq_log20_times[a2]; //find total time to read entire file over iterations
        sum_rand_log20 += rand_log20_times[a2];
    }

    avg_seq_log20 = (double) sum_seq_log20 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log20 = (double) sum_rand_log20 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log20 += pow(((seq_log20_times[a3]) - avg_seq_log20), 2); //total variance
        var_rand_log20 += pow(((rand_log20_times[a3]) - avg_rand_log20), 2); //total variance
    }

    var_seq_log20 = ((double)var_seq_log20 / (double) iterations); //variance over iterations
    var_rand_log20 = ((double)var_rand_log20 / (double) iterations); //variance over iterations
    dev_seq_log20 = sqrt(var_seq_log20); //deviation
    dev_rand_log20 = sqrt(var_rand_log20); //deviation

    printf("\nTimes for file size log20:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log20)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log20)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log20)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log20)/(3));

    //----------------log21----------------

    // Arrays to store times for each iteration
    uint64_t seq_log21_times[iterations];
    uint64_t rand_log21_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log21 = open("fileread/fileread21.txt", O_RDONLY);

        uint64_t seq_log21 = seq_time(fd_log21);    // sequential access
        uint64_t rand_log21 = rand_time(fd_log21);  // random access
        close(fd_log21);

        seq_log21_times[i] = seq_log21;
        rand_log21_times[i] = rand_log21;
    }

    uint64_t sum_seq_log21, avg_seq_log21, var_seq_log21, dev_seq_log21;
    uint64_t sum_rand_log21, avg_rand_log21, var_rand_log21, dev_rand_log21;
    sum_seq_log21 = 0;
    var_seq_log21 = 0;
    sum_rand_log21 = 0;
    var_rand_log21 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log21 += seq_log21_times[a2]; //find total time to read entire file over iterations
        sum_rand_log21 += rand_log21_times[a2];
    }

    avg_seq_log21 = (double) sum_seq_log21 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log21 = (double) sum_rand_log21 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log21 += pow(((seq_log21_times[a3]) - avg_seq_log21), 2); //total variance
        var_rand_log21 += pow(((rand_log21_times[a3]) - avg_rand_log21), 2); //total variance
    }

    var_seq_log21 = ((double)var_seq_log21 / (double) iterations); //variance over iterations
    var_rand_log21 = ((double)var_rand_log21 / (double) iterations); //variance over iterations
    dev_seq_log21 = sqrt(var_seq_log21); //deviation
    dev_rand_log21 = sqrt(var_rand_log21); //deviation

    printf("\nTimes for file size log21:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log21)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log21)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log21)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log21)/(3));

    //----------------log22----------------

    // Arrays to store times for each iteration
    uint64_t seq_log22_times[iterations];
    uint64_t rand_log22_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log22 = open("fileread/fileread22.txt", O_RDONLY);

        uint64_t seq_log22 = seq_time(fd_log22);    // sequential access
        uint64_t rand_log22 = rand_time(fd_log22);  // random access
        close(fd_log22);

        seq_log22_times[i] = seq_log22;
        rand_log22_times[i] = rand_log22;
    }

    uint64_t sum_seq_log22, avg_seq_log22, var_seq_log22, dev_seq_log22;
    uint64_t sum_rand_log22, avg_rand_log22, var_rand_log22, dev_rand_log22;
    sum_seq_log22 = 0;
    var_seq_log22 = 0;
    sum_rand_log22 = 0;
    var_rand_log22 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log22 += seq_log22_times[a2]; //find total time to read entire file over iterations
        sum_rand_log22 += rand_log22_times[a2];
    }

    avg_seq_log22 = (double) sum_seq_log22 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log22 = (double) sum_rand_log22 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log22 += pow(((seq_log22_times[a3]) - avg_seq_log22), 2); //total variance
        var_rand_log22 += pow(((rand_log22_times[a3]) - avg_rand_log22), 2); //total variance
    }

    var_seq_log22 = ((double)var_seq_log22 / (double) iterations); //variance over iterations
    var_rand_log22 = ((double)var_rand_log22 / (double) iterations); //variance over iterations
    dev_seq_log22 = sqrt(var_seq_log22); //deviation
    dev_rand_log22 = sqrt(var_rand_log22); //deviation

    printf("\nTimes for file size log22:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log22)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log22)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log22)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log22)/(3));

    //----------------log23----------------

    // Arrays to store times for each iteration
    uint64_t seq_log23_times[iterations];
    uint64_t rand_log23_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log23 = open("fileread/fileread23.txt", O_RDONLY);

        uint64_t seq_log23 = seq_time(fd_log23);    // sequential access
        uint64_t rand_log23 = rand_time(fd_log23);  // random access
        close(fd_log23);

        seq_log23_times[i] = seq_log23;
        rand_log23_times[i] = rand_log23;
    }

    uint64_t sum_seq_log23, avg_seq_log23, var_seq_log23, dev_seq_log23;
    uint64_t sum_rand_log23, avg_rand_log23, var_rand_log23, dev_rand_log23;
    sum_seq_log23 = 0;
    var_seq_log23 = 0;
    sum_rand_log23 = 0;
    var_rand_log23 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log23 += seq_log23_times[a2]; //find total time to read entire file over iterations
        sum_rand_log23 += rand_log23_times[a2];
    }

    avg_seq_log23 = (double) sum_seq_log23 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log23 = (double) sum_rand_log23 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log23 += pow(((seq_log23_times[a3]) - avg_seq_log23), 2); //total variance
        var_rand_log23 += pow(((rand_log23_times[a3]) - avg_rand_log23), 2); //total variance
    }

    var_seq_log23 = ((double)var_seq_log23 / (double) iterations); //variance over iterations
    var_rand_log23 = ((double)var_rand_log23 / (double) iterations); //variance over iterations
    dev_seq_log23 = sqrt(var_seq_log23); //deviation
    dev_rand_log23 = sqrt(var_rand_log23); //deviation

    printf("\nTimes for file size log23:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log23)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log23)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log23)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log23)/(3));

    //----------------log24----------------

    // Arrays to store times for each iteration
    uint64_t seq_log24_times[iterations];
    uint64_t rand_log24_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log24 = open("fileread/fileread24.txt", O_RDONLY);

        uint64_t seq_log24 = seq_time(fd_log24);    // sequential access
        uint64_t rand_log24 = rand_time(fd_log24);  // random access
        close(fd_log24);

        seq_log24_times[i] = seq_log24;
        rand_log24_times[i] = rand_log24;
    }

    uint64_t sum_seq_log24, avg_seq_log24, var_seq_log24, dev_seq_log24;
    uint64_t sum_rand_log24, avg_rand_log24, var_rand_log24, dev_rand_log24;
    sum_seq_log24 = 0;
    var_seq_log24 = 0;
    sum_rand_log24 = 0;
    var_rand_log24 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log24 += seq_log24_times[a2]; //find total time to read entire file over iterations
        sum_rand_log24 += rand_log24_times[a2];
    }

    avg_seq_log24 = (double) sum_seq_log24 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log24 = (double) sum_rand_log24 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log24 += pow(((seq_log24_times[a3]) - avg_seq_log24), 2); //total variance
        var_rand_log24 += pow(((rand_log24_times[a3]) - avg_rand_log24), 2); //total variance
    }

    var_seq_log24 = ((double)var_seq_log24 / (double) iterations); //variance over iterations
    var_rand_log24 = ((double)var_rand_log24 / (double) iterations); //variance over iterations
    dev_seq_log24 = sqrt(var_seq_log24); //deviation
    dev_rand_log24 = sqrt(var_rand_log24); //deviation

    printf("\nTimes for file size log24:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log24)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log24)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log24)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log24)/(3));

    //----------------log25----------------

    // Arrays to store times for each iteration
    uint64_t seq_log25_times[iterations];
    uint64_t rand_log25_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log25 = open("fileread/fileread25.txt", O_RDONLY);

        uint64_t seq_log25 = seq_time(fd_log25);    // sequential access
        uint64_t rand_log25 = rand_time(fd_log25);  // random access
        close(fd_log25);

        seq_log25_times[i] = seq_log25;
        rand_log25_times[i] = rand_log25;
    }

    uint64_t sum_seq_log25, avg_seq_log25, var_seq_log25, dev_seq_log25;
    uint64_t sum_rand_log25, avg_rand_log25, var_rand_log25, dev_rand_log25;
    sum_seq_log25 = 0;
    var_seq_log25 = 0;
    sum_rand_log25 = 0;
    var_rand_log25 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log25 += seq_log25_times[a2]; //find total time to read entire file over iterations
        sum_rand_log25 += rand_log25_times[a2];
    }

    avg_seq_log25 = (double) sum_seq_log25 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log25 = (double) sum_rand_log25 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log25 += pow(((seq_log25_times[a3]) - avg_seq_log25), 2); //total variance
        var_rand_log25 += pow(((rand_log25_times[a3]) - avg_rand_log25), 2); //total variance
    }

    var_seq_log25 = ((double)var_seq_log25 / (double) iterations); //variance over iterations
    var_rand_log25 = ((double)var_rand_log25 / (double) iterations); //variance over iterations
    dev_seq_log25 = sqrt(var_seq_log25); //deviation
    dev_rand_log25 = sqrt(var_rand_log25); //deviation

    printf("\nTimes for file size log25:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log25)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log25)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log25)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log25)/(3));

    //----------------log26----------------

    // Arrays to store times for each iteration
    uint64_t seq_log26_times[iterations];
    uint64_t rand_log26_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log26 = open("fileread/fileread26.txt", O_RDONLY);

        uint64_t seq_log26 = seq_time(fd_log26);    // sequential access
        uint64_t rand_log26 = rand_time(fd_log26);  // random access
        close(fd_log26);

        seq_log26_times[i] = seq_log26;
        rand_log26_times[i] = rand_log26;
    }

    uint64_t sum_seq_log26, avg_seq_log26, var_seq_log26, dev_seq_log26;
    uint64_t sum_rand_log26, avg_rand_log26, var_rand_log26, dev_rand_log26;
    sum_seq_log26 = 0;
    var_seq_log26 = 0;
    sum_rand_log26 = 0;
    var_rand_log26 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log26 += seq_log26_times[a2]; //find total time to read entire file over iterations
        sum_rand_log26 += rand_log26_times[a2];
    }

    avg_seq_log26 = (double) sum_seq_log26 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log26 = (double) sum_rand_log26 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log26 += pow(((seq_log26_times[a3]) - avg_seq_log26), 2); //total variance
        var_rand_log26 += pow(((rand_log26_times[a3]) - avg_rand_log26), 2); //total variance
    }

    var_seq_log26 = ((double)var_seq_log26 / (double) iterations); //variance over iterations
    var_rand_log26 = ((double)var_rand_log26 / (double) iterations); //variance over iterations
    dev_seq_log26 = sqrt(var_seq_log26); //deviation
    dev_rand_log26 = sqrt(var_rand_log26); //deviation

    printf("\nTimes for file size log26:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log26)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log26)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log26)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log26)/(3));

    //----------------log27----------------

    // Arrays to store times for each iteration
    uint64_t seq_log27_times[iterations];
    uint64_t rand_log27_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log27 = open("fileread/fileread27.txt", O_RDONLY);

        uint64_t seq_log27 = seq_time(fd_log27);    // sequential access
        uint64_t rand_log27 = rand_time(fd_log27);  // random access
        close(fd_log27);

        seq_log27_times[i] = seq_log27;
        rand_log27_times[i] = rand_log27;
    }

    uint64_t sum_seq_log27, avg_seq_log27, var_seq_log27, dev_seq_log27;
    uint64_t sum_rand_log27, avg_rand_log27, var_rand_log27, dev_rand_log27;
    sum_seq_log27 = 0;
    var_seq_log27 = 0;
    sum_rand_log27 = 0;
    var_rand_log27 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log27 += seq_log27_times[a2]; //find total time to read entire file over iterations
        sum_rand_log27 += rand_log27_times[a2];
    }

    avg_seq_log27 = (double) sum_seq_log27 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log27 = (double) sum_rand_log27 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log27 += pow(((seq_log27_times[a3]) - avg_seq_log27), 2); //total variance
        var_rand_log27 += pow(((rand_log27_times[a3]) - avg_rand_log27), 2); //total variance
    }

    var_seq_log27 = ((double)var_seq_log27 / (double) iterations); //variance over iterations
    var_rand_log27 = ((double)var_rand_log27 / (double) iterations); //variance over iterations
    dev_seq_log27 = sqrt(var_seq_log27); //deviation
    dev_rand_log27 = sqrt(var_rand_log27); //deviation

    printf("\nTimes for file size log27:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log27)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log27)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log27)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log27)/(3));

    //----------------log28----------------

    // Arrays to store times for each iteration
    uint64_t seq_log28_times[iterations];
    uint64_t rand_log28_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log28 = open("fileread/fileread28.txt", O_RDONLY);

        uint64_t seq_log28 = seq_time(fd_log28);    // sequential access
        uint64_t rand_log28 = rand_time(fd_log28);  // random access
        close(fd_log28);

        seq_log28_times[i] = seq_log28;
        rand_log28_times[i] = rand_log28;
    }

    uint64_t sum_seq_log28, avg_seq_log28, var_seq_log28, dev_seq_log28;
    uint64_t sum_rand_log28, avg_rand_log28, var_rand_log28, dev_rand_log28;
    sum_seq_log28 = 0;
    var_seq_log28 = 0;
    sum_rand_log28 = 0;
    var_rand_log28 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log28 += seq_log28_times[a2]; //find total time to read entire file over iterations
        sum_rand_log28 += rand_log28_times[a2];
    }

    avg_seq_log28 = (double) sum_seq_log28 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log28 = (double) sum_rand_log28 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log28 += pow(((seq_log28_times[a3]) - avg_seq_log28), 2); //total variance
        var_rand_log28 += pow(((rand_log28_times[a3]) - avg_rand_log28), 2); //total variance
    }

    var_seq_log28 = ((double)var_seq_log28 / (double) iterations); //variance over iterations
    var_rand_log28 = ((double)var_rand_log28 / (double) iterations); //variance over iterations
    dev_seq_log28 = sqrt(var_seq_log28); //deviation
    dev_rand_log28 = sqrt(var_rand_log28); //deviation

    printf("\nTimes for file size log28:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log28)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log28)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log28)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log28)/(3));

    //----------------log29----------------

    // Arrays to store times for each iteration
    uint64_t seq_log29_times[iterations];
    uint64_t rand_log29_times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        int fd_log29 = open("fileread/fileread29.txt", O_RDONLY);

        uint64_t seq_log29 = seq_time(fd_log29);    // sequential access
        uint64_t rand_log29 = rand_time(fd_log29);  // random access
        close(fd_log29);

        seq_log29_times[i] = seq_log29;
        rand_log29_times[i] = rand_log29;
    }

    uint64_t sum_seq_log29, avg_seq_log29, var_seq_log29, dev_seq_log29;
    uint64_t sum_rand_log29, avg_rand_log29, var_rand_log29, dev_rand_log29;
    sum_seq_log29 = 0;
    var_seq_log29 = 0;
    sum_rand_log29 = 0;
    var_rand_log29 = 0;
    for (int a2 = 0; a2 < iterations; a2++) {
        sum_seq_log29 += seq_log29_times[a2]; //find total time to read entire file over iterations
        sum_rand_log29 += rand_log29_times[a2];
    }

    avg_seq_log29 = (double) sum_seq_log29 / (double) iterations; //find avg time to read entire file over iterations
    avg_rand_log29 = (double) sum_rand_log29 / (double) iterations; //find avg time to read entire file over iterations

    for (int a3 = 0; a3 < iterations; a3++) {
        var_seq_log29 += pow(((seq_log29_times[a3]) - avg_seq_log29), 2); //total variance
        var_rand_log29 += pow(((rand_log29_times[a3]) - avg_rand_log29), 2); //total variance
    }

    var_seq_log29 = ((double)var_seq_log29 / (double) iterations); //variance over iterations
    var_rand_log29 = ((double)var_rand_log29 / (double) iterations); //variance over iterations
    dev_seq_log29 = sqrt(var_seq_log29); //deviation
    dev_rand_log29 = sqrt(var_rand_log29); //deviation

    printf("\nTimes for file size log29:\n");
    printf("Avg Time for Sequential Access: %lu\n", (avg_seq_log29)/(3));
    printf("Deviation for Sequential Access: %lu\n", (dev_seq_log29)/(3));
    printf("Avg Time for Random Access: %lu\n", (avg_rand_log29)/(3));
    printf("Deviation for Random Access: %lu\n", (dev_rand_log29)/(3));
}
