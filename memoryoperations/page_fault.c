#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

// Creating a buffer of 1000 byte
// 2.5gb = 2.5 * 10^9 bytes
#define BUFFER_SIZE 1000
#define PAGESIZE 4096 //defining the page size as 4096 B (each char is 1 byte)
#define N_iterations 1000
#define FILENAME "temp_file.txt"
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

int main()
{
    // creating a large file (2.5 GB)
    srand(time(0));
    printf("----Creating 2.5 GB file----\n");
    FILE *fp;
    fp = fopen(FILENAME, "w");
    u_int64_t i;

    char buffer[BUFFER_SIZE];
    for(i = 0; i < BUFFER_SIZE; i++){
        buffer[i] = 'A';
    }

    for(i = 0; i < (2500000); i++){
        fseek(fp, (i*BUFFER_SIZE), SEEK_SET);
        fprintf(fp, buffer);
    }

    printf("----Done Creating----\n");
    fclose(fp);
    printf("----Flushing out cache----\n");
    system(SHELLSCRIPT);
    // Add the code to mmaps -> allocate the memory to open the file and then randomly access it

    int open_fd;
    size_t size;
    int status;
    struct stat s;
    const char * mapped;
    u_int64_t duration_per_iteration[N_iterations];
    u_int64_t start_time, end_time;

    open_fd = open (FILENAME, O_RDONLY);
    status = fstat (open_fd, & s);
    if(status < 0){
        perror("Error checking the status of the file ");
        exit(1);
    }
    size = s.st_size;
    printf("Size of the file opened = %lu\n", size);
    // Memory-map the file
    mapped = mmap(0, size, PROT_READ, MAP_FILE|MAP_SHARED, open_fd, 0);
    if(mapped == MAP_FAILED){
        perror("Mmap failed.");
        exit(2);
    }
    // accessing 1000 times
    printf("----starting test by random access of file----\n");
    for(i = 0; i < N_iterations; i++){
        char c;
        int num = (rand() % (size + 1));
        int j;
        rdtsc();
        // Accessing continuos 4096 kb in memory => size of pages in linux
        for(j = num; j <= num+PAGESIZE; j++){
            c = mapped[j];
        }
        rdtsc1();
        start_time = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end_time = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        duration_per_iteration[i] = (end_time - start_time);
    }

    printf("%llu\n",duration_per_iteration[200]);
    printf("----Plotting data----\n");
    char *filename = "output.txt";

    // open the file for writing
    FILE *fpWrite = fopen(filename, "w");
    if (fpWrite == NULL)
    {
        perror("Error opening the file ");
        exit(1);
    }
    for(i = 0; i < N_iterations; i++){
        char str[256];
        sprintf(str, "%llu\n", duration_per_iteration[i]);
        fprintf(fpWrite, str);
    }
    fclose(fpWrite);


    fflush(stdout);
    return(0);
}