#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define MAX_PATH_LEN 64

void print_error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

// Return a NULL termiated arr
char **token_arr_init(char *line)
{
    int arr_size = 0;
    int arr_cap = 10;
    char **arr = malloc(sizeof(*arr) * arr_cap);

    char *end = line;
    char *tok = NULL;

    while((tok = strsep(&end, " \t\n"))) {
        if (tok[0] == '\0') continue;
        if (arr_size == arr_cap) {
            arr_cap *= 2;
            char **new_arr = realloc(arr, sizeof(*new_arr) * arr_cap);
            assert(new_arr != NULL && "Buy more RAM!");
            arr = new_arr;
        }
        arr[arr_size++] = strdup(tok);
    }

    arr[arr_size] = NULL;

    return arr;
}

void token_arr_free(char **token_arr)
{
    for(int i = 0; token_arr[i] != NULL; ++i) free(token_arr[i]);
    free(token_arr);
}

char *get_exe_path(char **paths, char *exe_name)
{
    char *buf = NULL;

    for (int i = 0; i < MAX_PATH_LEN && paths[i] != NULL; ++i) {
        buf = malloc(sizeof(*buf) * strlen(paths[i]) * strlen(exe_name) + 2); // 2 for '/' and '\0'
        strcat(buf, paths[i]);
        strcat(buf, "/");
        strcat(buf, exe_name);
        if (access(buf, X_OK) == 0) return buf;
    }

    free(buf);
    return NULL;
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc > 2) {
        print_error();
        exit(1);
    }

    char *line = NULL;
    size_t line_size = 0;

    if (argc == 2) {
        puts("This is batch mode");
        return 0;
    }

    // NOTE: Could become a dynamic arr here, but for the sake of the simplicity just nah
    char *paths[MAX_PATH_LEN] = {"/bin", "/usr/bin"};

    while(1) {
        printf("wish> ");

        int rcv_bytes = getline(&line, &line_size, stdin);
        if (rcv_bytes == EOF) break;
        char **token_arr = token_arr_init(line);
        if (token_arr[0] == NULL) {
            token_arr_free(token_arr);
            continue;
        }

        int child = fork();
        assert(child != -1 && "fork fail");

        if (child == 0) {
            char *exe_path = get_exe_path(paths, token_arr[0]); // Assume first token always be the exe_name
            if (!exe_path) {
                print_error();
                exit(1);
            }
            execv(exe_path, token_arr);
            perror("Execv fail");
        } else {
            waitpid(child, NULL, 0);
            token_arr_free(token_arr);
        }
    }

    free(line);
}
