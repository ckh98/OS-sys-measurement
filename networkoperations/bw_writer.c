// Writer code for measuring the network bandwidth
// will send data to the reader (1 MB in size)
// Writer will act as a client, which sends the data which the server reads
// Writer has the ip passed as the argument -> determines the server as local or remote
#include <netinet/in.h> //structure for storing address information
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //for socket APIs
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#define BUFSIZE 1048576 // buffer size of 1Mb (client will be sending1 Mb of data)
#define DATA_SIZE 1048576
#define N_iterations 1

int main(int argc, char const* argv[]){
    int sockfd;
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8081);
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    int i;
    char buf[DATA_SIZE];
    char msg[DATA_SIZE];
    int buffSize = DATA_SIZE;
    int bytes_read;
    int connectStatus;
    for (i = 0; i < DATA_SIZE; i++)
        msg[i] = 'a';
    printf("Start sending data now\n");

    // start iteration
    for(i = 0; i < N_iterations; i++) { 
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            perror("Client: socket");
            return 1;
        }
        // setting socket size to be equal to the buffer size (1Mb)
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buffSize, (int)sizeof(buffSize));
        if ((connectStatus = connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))) == -1){
            close(sockfd);
            perror("Client: connect");
            return 1;
        } 
        // send data       
        send(sockfd, msg, sizeof(msg), 0);
        close(sockfd);
    }
    return 0;
}