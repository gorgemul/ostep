#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

int main(void)
{
    int pfds[2] = {0};
    char buf[256] = {0};
    int bytes_recv;

    if (pipe(pfds) == -1) {
        perror("pipe");
        exit(1);
    }

    switch (fork()) {
    case -1:
        perror("fork");
        exit(1);
    case 0:
        if (close(pfds[0]) == -1) {
            perror("close");
            exit(1);
        }

        printf("Writing to pipe...\n");
        strcat(buf, "Hello from write end\n");

        if (write(pfds[1], &buf, strlen(buf)) == -1) {
            perror("write");
            exit(1);
        }

        if (close(pfds[1]) == -1) {
            perror("close");
            exit(1);
        }

        exit(0);
    default:
        break;
    }

    switch (fork()) {
    case -1:
        perror("fork");
        exit(1);
    case 0:
        if (close(pfds[1]) == -1) {
            perror("close");
            exit(1);
        }

        printf("Reading from pipe...\n");
        
        if ((bytes_recv = read(pfds[0], &buf, sizeof(buf))) == -1) {
            perror("read");
            exit(1);
        }

        printf("recv(%d): %s", bytes_recv, buf);
        
        if (close(pfds[0]) == -1) {
            perror("close");
            exit(1);
        }

        exit(0);
    default:
        break;
    }

    close(pfds[0]);
    close(pfds[1]);

    wait(NULL);
    wait(NULL);

    return 0;
}
