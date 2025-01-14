#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int is_match_pattern(char *line, const char *pattern)
{
    return strstr(line, pattern) ? 1 : 0;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf("wgrep: searchterm [file ...]\n");
        exit(EXIT_FAILURE);
    }

    size_t line_size = 64;
    char *line = malloc(sizeof(*line) * line_size);

    FILE *fp = fopen(argv[1], "r");
    int is_grep_empty_str = (fp != NULL);
    fclose(fp);

    if (is_grep_empty_str) {
        for (int i = 1; i < argc; ++i) {
            fp = fopen(argv[i], "r");
            if (!fp) {
                printf("wgrep: cannot open file\n");
                exit(EXIT_FAILURE);
            }
            while (getline(&line, &line_size, fp) != -1) printf("%s", line);
            fclose(fp);
        }
        goto clean;
    } 

    const char *pattern = argv[1];

    if (argc == 2) { // Grep from stdin
        while (getline(&line, &line_size, stdin) != -1) {
            if (is_match_pattern(line, pattern)) printf("%s", line);
        }
    } else { // Grep form files
        for (int i = 2; i < argc; ++i) {
            fp = fopen(argv[i], "r");
            if (!fp) {
                printf("wgrep: cannot open file\n");
                exit(EXIT_FAILURE);
            }
            while (getline(&line, &line_size, fp) != -1) {
                if (is_match_pattern(line, pattern)) printf("%s", line);
            }
            fclose(fp);
        }
    }

clean:
    free(line);
    return 0;
}
