#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char peek(FILE *i_stream)
{
    char c = getc(i_stream);
    ungetc(c, i_stream);

    return c;
}

char peek_next_stream(char *i_stream_path)
{
    FILE *i_stream = fopen(i_stream_path, "r");

    if (!i_stream) {
        printf("Could not open file\n");
        exit(EXIT_FAILURE);
    }

    char c = getc(i_stream);
    ungetc(c, i_stream);

    fclose(i_stream);

    return c;
}

void write_entry(FILE *o_stream, int n, char c)
{
    fwrite(&n, sizeof(int), 1, o_stream);
    fwrite(&c, sizeof(char), 1, o_stream);
}

FILE *get_o_stream(int argc, char *argv[])
{
    int redir_index = -1;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], ">") == 0) redir_index = i;
    }

    // Indicating that no redirection sign or no output file provided
    if (redir_index == -1 || redir_index == (argc - 1)) return stdout;

    FILE *o_stream = fopen(argv[redir_index+1], "wb");

    if (!o_stream) {
        printf("Could not open file\n");
        exit(EXIT_FAILURE);
    }
    
    return o_stream;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf("wzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    char c = ' ';
    FILE *i_stream = NULL;
    FILE *o_stream = get_o_stream(argc, argv);
    int merge_count = 1;
    int new_argc = (o_stream == stdout) ? argc : (argc - 2);

    for (int i = 1; i < new_argc; ++i) {
        int is_last_stream = i == (new_argc - 1);
        i_stream = fopen(argv[i], "r");

        if (!i_stream) {
            printf("Could not open file\n");
            exit(EXIT_FAILURE);
        }

        while (c = getc(i_stream), c != EOF) {
            char peek_c = peek(i_stream);

            if (peek_c == c) {
                merge_count++;
                continue;
            }

            if (peek_c == EOF && is_last_stream) {
                write_entry(o_stream, merge_count, c);
                continue;
            }

            if (peek_c == EOF && (peek_next_stream(argv[i+1]) == c)) {
                merge_count++;
                continue;
            }

            write_entry(o_stream, merge_count, c);
            merge_count = 1;
        }

        fclose(i_stream);
    }

    fclose(o_stream);
    return 0;
}
