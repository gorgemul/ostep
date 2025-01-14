#include <stdio.h>
#include <stdlib.h>

#define BUFFER_MAX_LEN 512

int main(int argc, char *argv[])
{
    if (argc == 1) return 0;

    FILE *fp = NULL;
    char buf[BUFFER_MAX_LEN] = {0};

    for (int i = 1; i < argc; ++i) {
        fp = fopen(argv[i], "r");
        if (!fp) {
            printf("wcat: cannot open file\n");
            exit(1);
        }
        while (fgets(buf, sizeof(buf), fp)) printf("%s", buf);
        fclose(fp);
    }

    return 0;
}
