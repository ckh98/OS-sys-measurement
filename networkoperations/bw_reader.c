// Reader code for measuring the network bandwidth
// will read data sent by the sender (1 MB in size)
// Reader will act as a server, which reads the data which the client sends
#include <netinet/in.h> //structure for storing address information
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //for socket APIs
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SA struct sockaddr
#define BUFSIZE 1048576 // buffer size of 1Mb (client will be sending1 Mb of data)
#define N_iterations 1

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

int main() {
    int i;
    u_int64_t duration_per_iteration[N_iterations];
    u_int64_t start_time, end_time;
    u_int64_t avg;
    // create the socket
    int servSockD = socket(AF_INET, SOCK_STREAM, 0);
    if (servSockD == -1){
        perror("Server : socket ");
        return 1;
    }
    printf("Socket created!\n");
    // server address
    struct sockaddr_in servAddr, cli;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8081);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    // bind the socket to ip and port
    if (bind(servSockD, (struct sockaddr*)&servAddr, sizeof(servAddr)) !=0 ){
        perror("Socket:bind failed");
        return 1;
    }
    printf("Socket binded successfully\n");
    // listen for connections
    if (listen(servSockD, 1) < 0){
        perror("Server : listen");
        return 1;
    }
    printf("Server listening!\n");

    for(i = 0; i < N_iterations; i++) {
        int len;
        len = sizeof(cli);
        // integer to hold client socket.
        int clientSocket = accept(servSockD, (SA*) &cli, &len);
        if (clientSocket < 0){
            perror("Server : accept failed");
            return 1;
        }

        // start time
        char buff[BUFSIZE];
        int received, sent;
        rdtsc();
        while (1) {
            received = read(clientSocket, buff, sizeof(buff));
            if (received <= 0){
                perror("Server reading data error");
                break;
            }
        }
        // end time
        rdtsc1();
    }
        close(servSockD);
        start_time = ( ((u_int64_t) cycles_high << 32) | cycles_low );
        end_time = ( ((u_int64_t) cycles_high1 << 32) | cycles_low1 );
        duration_per_iteration[i] = end_time - start_time;
    avg = getAvg(duration_per_iteration, N_iterations);
    printf("Avg duration time for reading 1Mb data= %llu\n", avg);
    
    return 0;

}