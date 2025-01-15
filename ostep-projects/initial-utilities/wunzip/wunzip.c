#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char peek(FILE *i_stream)
{
    char c = getc(i_stream);
    ungetc(c, i_stream);

    return c;
}

void translate_entry(FILE *i_stream, FILE *o_stream)
{
    static int ii[1] = {0};
    static char cc[1] = {0};

    fread(&ii, sizeof(int), 1, i_stream);
    fread(&cc, sizeof(char), 1, i_stream);

    for (int i = 0; i < *ii; ++i) putc(*cc, o_stream);
}

FILE *get_o_stream(int argc, char *argv[])
{
    int redir_index = -1;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], ">") == 0) redir_index = i;
    }

    if (redir_index == -1 || redir_index == (argc - 1)) return stdout;

    FILE *o_stream = fopen(argv[redir_index+1], "w");
    
    if (!o_stream) {
        puts("Could not open file");
        exit(EXIT_FAILURE);
    }

    return o_stream;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf("wunzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    FILE *i_stream = NULL;
    FILE *o_stream = get_o_stream(argc, argv);

    int new_argc = (o_stream == stdout) ? argc : (argc-2);

    for (int i = 1; i < new_argc; ++i) {
        i_stream = fopen(argv[i], "rb");

        if (!i_stream) {
            puts("Could not open file");
            exit(EXIT_FAILURE);
        }

        while (peek(i_stream) != EOF) translate_entry(i_stream, o_stream);

        fclose(i_stream);
    }

    return 0;
}
