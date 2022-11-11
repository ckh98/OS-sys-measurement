#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <inttypes.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    char text[128];
    char *test;
    FILE *ptr = popen("ssh -i '221.pem' ec2-user@ec2-204-236-154-188.us-west-1.compute.amazonaws.com cat /home/ec2-user/cse221/filecaches/filecache_128.txt", "r");

    test = fgets(text, sizeof(text), ptr);
    printf("File Text: %s\n", text);
    printf("fgets return: %s\n\n", test);
    pclose(ptr);


    FILE *ptr2 = popen("ssh -i '221.pem' ec2-user@ec2-204-236-154-188.us-west-1.compute.amazonaws.com cat /home/ec2-user/cse221/filecaches/filecache_128.txt", "r");

    int a = fseek(ptr, 5, SEEK_SET);    // Attempts to offset it by 5

    test = fgets(text, sizeof(text), ptr);
    printf("File Text: %s\n", text);
    printf("fgets return: %s\n", test);
    printf("fseek return: %d\n", a);
    pclose(ptr);


}

// https://stackoverflow.com/questions/31016873/how-to-read-a-file-from-a-remote-device-from-c-program-and-ubuntu-os
