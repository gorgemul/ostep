#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define VECTOR_INIT_LEN 10

typedef struct {
    char **lines;
    int cap;
    int len;
} Vector;

Vector *init_vector()
{
    Vector *vec = malloc(sizeof(*vec));

    if (!vec) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }

    char **lines = malloc(sizeof(char**) * VECTOR_INIT_LEN);

    if (!lines) {
        free(vec);
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }

    memset(lines, 0, sizeof(char**) * VECTOR_INIT_LEN);
    vec->cap = VECTOR_INIT_LEN;
    vec->len = 0;
    vec->lines = lines;

    return vec;
}

void destroy_vec(Vector *vec)
{
    for (int i = 0; i < vec->len; ++i) {
        free(vec->lines[i]);
    }

    free(vec->lines);
}

void append_vec(Vector *vec, char *line)
{
    if (vec->len == vec->cap) {
        vec->cap *= 2;
        char **new_lines = realloc(vec->lines, vec->cap * sizeof(char**));
        if (!new_lines) {
            destroy_vec(vec); 
            fprintf(stderr, "realloc failed\n");
            exit(EXIT_FAILURE);
        }
        vec->lines = new_lines;
    }

    if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';

    char *saved_line =  malloc(sizeof(*saved_line) * (strlen(line) + 1));

    if (!saved_line) {
        destroy_vec(vec); 
        fprintf(stderr, "realloc failed\n");
        exit(EXIT_FAILURE);
    }

    strcpy(saved_line, line);
    vec->lines[vec->len] = saved_line;
    vec->len++;
}

void print_vec(Vector *vec, FILE *stream)
{
    for (int i = vec->len; i > 0; --i) fprintf(stream, "%s\n", vec->lines[i-1]);
}

int is_same_file(char *file1, char *file2)
{
    struct stat sb1;
    struct stat sb2;

    int r1 = lstat(file1, &sb1);
    int r2 = lstat(file2, &sb2);

    if (r1 == -1 || r2 == -1) return strcmp(file1, file2) == 0;

    return (sb1.st_ino == sb2.st_ino || strcmp(file1, file2) == 0);
}

int main(int argc, char *argv[])
{
    size_t line_size = 256;
    char *line = NULL;

    if (argc == 1) {
        Vector *vec = init_vector();

        while (getline(&line, &line_size, stdin) != -1) append_vec(vec, line);

        print_vec(vec, stdout);

        destroy_vec(vec);
        free(line);
        return 0;
    }

    if (argc > 3) {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 3 && is_same_file(argv[1], argv[2])) {
        fprintf(stderr, "reverse: input and output file must differ\n");
        exit(EXIT_FAILURE);
    }

    FILE *i_stream = fopen(argv[1], "r");
    
    if (!i_stream) {
        fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    FILE *o_stream = (argc == 2) ? stdout : fopen(argv[2], "w");

    if (argc == 3 && !o_stream) {
        fclose(i_stream);
        fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    Vector *vec = init_vector();

    while (getline(&line, &line_size, i_stream) != EOF) append_vec(vec, line);

    print_vec(vec, o_stream);

    fclose(i_stream);
    fclose(o_stream);
    destroy_vec(vec);
    free(line);

    return 0;
}
