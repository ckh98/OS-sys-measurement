// Connection overhead - client (remote and local based on ip given at command line)
// For local:
// local => ./connOverhead 127.0.0.1
// remote => ./connOverhead 204.236.154.188
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#define N_iterations 1

unsigned cycles_low, cycles_high, cycles_low1, cycles_high1, cycles_low_close, cycles_high_close, cycles_low1_close, cycles_high1_close;

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


static __inline__ unsigned long long rdtsc_close(void)
{
    __asm__ __volatile__ ("RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t": "=r" (cycles_high_close), "=r" (cycles_low_close)::
            "%rax", "rbx", "rcx", "rdx");
}

static __inline__ unsigned long long rdtsc1_close(void)
{
    __asm__ __volatile__ ("RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t": "=r" (cycles_high1_close), "=r" (cycles_low1_close)::
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

int main(int argc, char *argv[]){
    int sockfd;
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8081);
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    int connectStatus; 
    int i;
    u_int64_t duration_per_iteration_connect[N_iterations], duration_per_iteration_close[N_iterations];
    u_int64_t start_time_connect, end_time_connect, start_time_close, end_time_close;
    u_int64_t avg_connect, avg_close;
    u_int64_t var_connect, std_dev_connect, var_close, std_dev_close;

    printf("Start\n");
    for(i = 0; i < N_iterations; i++){
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            perror("Client: socket");
            return 1;
        }
        // setup time
        rdtsc();
        if ((connectStatus = connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))) == -1){
            close(sockfd);
            perror("Client: connect");
            return 1;
        };
        rdtsc1();

        // tear down time
        rdtsc_close();
        close(sockfd);
        rdtsc1_close();
        start_time_connect = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end_time_connect = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        duration_per_iteration_connect[i] = end_time_connect - start_time_connect;
        start_time_close = ( ((u_int64_t) cycles_high_close << 32) | cycles_low_close );
        end_time_close = ( ((u_int64_t) cycles_high1_close << 32) | cycles_low1_close );
        duration_per_iteration_close[i] = end_time_close - start_time_close;
    }
    printf("Done ...\n");
    avg_connect = getAvg(duration_per_iteration_connect, N_iterations);
    avg_close = getAvg(duration_per_iteration_close, N_iterations);
    var_connect = getVar(duration_per_iteration_connect, avg_connect, N_iterations);
    var_close = getVar(duration_per_iteration_close, avg_close, N_iterations);
    std_dev_connect = sqrt(var_connect);
    std_dev_close = sqrt(var_close);

    printf("Avg connect time = %llu\n", avg_connect);
    printf("Avg close time = %llu\n", avg_close);
    printf("Standard deviation of connect time = %llu\n", std_dev_connect);
    printf("Standard deviation of close time = %llu\n", std_dev_close);

    return 0;
}