// rtt - client, select remote or local by supplying corresponding ip addresses
// open a local server which listens and accepts connections, receives data and sends it back
// for remote testing use => ./rtt 204.236.154.188
// for local testing use => ./rtt 127.0.0.1
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
// ping packet size is 56 bytes
#define DATA_SIZE 56
#define N_iterations 10

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

int main( int argc, char *argv[] ) {
    int sockfd;
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8081);
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    int connectStatus; 
    int i;
    u_int64_t duration_per_iteration[N_iterations];
    u_int64_t start_time, end_time;
    u_int64_t avg, var, std_dev;
    
    char buf[DATA_SIZE];
    char msg[DATA_SIZE];
    int bytes_read;
    for (i = 0; i < DATA_SIZE; i++)
        msg[i] = 'a';

    printf("Start\n");
    for(i = 0; i < N_iterations; i++) {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            perror("Client: socket");
            return 1;
        }
        if ((connectStatus = connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))) == -1){
            close(sockfd);
            perror("Client: connect");
            return 1;
        }
        rdtsc();
        // sending data to server and checking the time
        send(sockfd, msg, sizeof(msg), 0);
        bytes_read = recv(sockfd, buf, DATA_SIZE, 0);
        rdtsc1();
        
        close(sockfd);
        if (bytes_read != DATA_SIZE) {
            printf("Packets dropped\n");
            i -= 1;
            continue;
        }
        start_time = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end_time = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        duration_per_iteration[i] = end_time - start_time;
    }
    avg = getAvg(duration_per_iteration, N_iterations);
    var = getVar(duration_per_iteration, avg, N_iterations);
    std_dev = sqrt(var);

    printf("Avg Round Trip Time = %llu\n", avg);
    printf("Standard deviation of Round trip time = %llu\n", std_dev);
    return 0;
    
}