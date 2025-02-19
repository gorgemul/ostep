#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <fcntl.h>

typedef enum {
    BUILT_IN_EXIT,
    BUILT_IN_CD,
    BUILT_IN_PATH,
    UTIL,
} Cmd;

typedef struct Node {
    char *data;
    struct Node *next;
} Node;

Node *node_create(char *data)
{
    Node *new_node = malloc(sizeof(*new_node));
    assert(new_node != NULL && "Buy more RAM");

    new_node->data = strdup(data);
    // Format the path
    int data_len = strlen(new_node->data);
    if (new_node->data[data_len-1] == '/') new_node->data[data_len-1] = '\0';

    new_node->next = NULL;
    return new_node;
}

void node_add_tail(Node **head, char *data)
{
    Node *new_node = node_create(data);
    if (*head == NULL) {
        *head = new_node;
        return;
    }

    Node *temp = *head;
    while (temp->next) temp = temp->next;
    temp->next = new_node;
}

void node_free(Node *head)
{
    Node *temp = NULL;
    while (head) {
        temp = head;
        head = head->next;
        free(temp->data);
        free(temp);
    }
}

void print_error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

// Return a NULL termiated arr
// NOTE: Kinda problematic, could have a better way to add new element in the token array, maybe use dynamic array here
char **token_arr_init(char *line)
{
    int arr_size = 0;
    int arr_cap = 128;
    char **arr = malloc(sizeof(*arr) * arr_cap);
    assert(arr != NULL && "Buy more RAM");

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

        char *end = strpbrk(tok, ">&");
        if (!end) {
            arr[arr_size++] = strdup(tok);
            continue;
        }

        char *begin = tok;

        while (end) {
            int tok_buf_len = end - begin;
            if (tok_buf_len > 0) {
                char *tok_buf = malloc(sizeof(*tok_buf) * (tok_buf_len+1));
                memcpy(tok_buf, begin, tok_buf_len);
                tok_buf[tok_buf_len] = '\0';
                arr[arr_size++] = tok_buf;
            }

            char *operator_buf = malloc(sizeof(*operator_buf) * 2);

            memcpy(operator_buf, end, 1);
            operator_buf[1] = '\0';
            arr[arr_size++] = operator_buf;

            begin = end + 1;
            end = strpbrk(begin, ">&");
        }

        if (begin[0] != '\0') arr[arr_size++] = strdup(begin);
    }

    arr[arr_size] = NULL;

    return arr;
}

char **token_arr_copy(char **token_arr, int start_index, int n)
{
    int copy_count = 0;
    char **new_arr = malloc(sizeof(*new_arr) * (n+1)); // 1 for the NULL terminator
    assert(new_arr != NULL && "Buy more RAM");
    while (copy_count < n) new_arr[copy_count++] = strdup(token_arr[start_index++]);
    new_arr[n] = NULL;

    return new_arr;
}

void token_arr_free(char **token_arr)
{
    for(int i = 0; token_arr[i] != NULL; ++i) free(token_arr[i]);
    free(token_arr);
}

char *get_exe_path(Node *path, char *exe_name)
{
    char *buf = NULL;

    while (path) {
        buf = malloc(sizeof(*buf) * strlen(path->data) * strlen(exe_name) + 2); // 2 for '/' and '\0'
        assert(buf != NULL && "Buy more RAM");
        strcat(buf, path->data);
        strcat(buf, "/");
        strcat(buf, exe_name);
        if (access(buf, X_OK) == 0) return buf;
        path = path->next;    
    }

    free(buf);
    return NULL;
}

Cmd get_cmd_type(char *token)
{
    if (strcmp(token, "exit") == 0) return BUILT_IN_EXIT;
    if (strcmp(token, "cd") == 0) return BUILT_IN_CD;
    if (strcmp(token, "path") == 0) return BUILT_IN_PATH;

    return UTIL;
}

// If not exist redirection sign, return -1
int get_redirection_sign_index(char **token_arr)
{
    for (int i = 0; token_arr[i] != NULL; ++i) {
        if (strcmp(token_arr[i], ">") == 0) return i;
    }

    return -1;
}

int get_token_arr_len(char **arr)
{
    int sum = 0;
    for (int i = 0; arr[i] != NULL; ++i) sum++;
    return sum;
}

void char_arr_dump(char **arr)
{
    for (int i = 0; arr[i] != NULL; ++i) {
        printf("%d item: %s\n", i+1, arr[i]);
    }
}

int get_ampersand_index_arr_len(int *arr)
{
    int sum = 0;
    for (int i = 0; arr[i] != -1; ++i) sum++;
    return sum;
}

bool is_parallel_exec(char **token_arr)
{
    for (int i = 0; token_arr[i] != NULL; ++i) {
        if (token_arr[i][0] == '&') return true;
    }

    return false;
}

bool valid_parallel_program(int *ampersand_arr)
{
    int ampersand_arr_len = get_ampersand_index_arr_len(ampersand_arr);
    
    int current_index = -1;
    int next_index = -1;
    for (int i = 0; i < ampersand_arr_len - 1; ++i) {
        current_index = ampersand_arr[i];
        next_index = ampersand_arr[i+1];
        if (next_index - current_index == 1) return false;
    }
    
    return true;
}

// -1 terminated int arr
int *get_ampersand_index_arr(char **token_arr)
{
    int cap = 10;
    int len = 0;
    int *arr = malloc(sizeof(*arr) * cap);
    assert(arr != NULL && "Buy more RAM");
    for (int i = 0; token_arr[i] != NULL; ++i) {
        if (token_arr[i][0] != '&') continue;
        if (len == cap) {
            cap *= 2;
            int *new_arr = realloc(arr, sizeof(*new_arr) * cap);
            assert(new_arr != NULL && "Buy more RAM");
            arr = new_arr;
        }
        arr[len++] = i;
    }

    arr[len] = -1; 
    
    return arr;
}

void program_runner(char **token_arr, Node *path)
{
    char *exe_path = get_exe_path(path, token_arr[0]); // Assume first token always be the exe_name
    if (!exe_path) {
        print_error();
        exit(1);
    }
    int redirection_sign_index = get_redirection_sign_index(token_arr);
    if (redirection_sign_index == -1) {
        execv(exe_path, token_arr);
    } else {
        bool not_valid =
            token_arr[redirection_sign_index+1] == NULL ||    // has multiple redirection sign
            token_arr[redirection_sign_index+1][0] == '>' ||  // has no output file
            token_arr[redirection_sign_index+2] != NULL;      // has multiple output file

        if (not_valid) {
            print_error();
            exit(1);
        }
        int output_file = open(token_arr[redirection_sign_index+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(output_file, STDOUT_FILENO);
        dup2(STDOUT_FILENO, STDERR_FILENO);
        execv(exe_path, token_arr_copy(token_arr, 0, redirection_sign_index));
    }
    perror("Execv fail");
}

void parallel_exec_util_program(char **token_arr, int *ampersand_arr, Node *path)
{
    int token_arr_len = get_token_arr_len(token_arr);
    int parallel_num = get_ampersand_index_arr_len(ampersand_arr) + 1;
    int *children = malloc(sizeof(*children) * parallel_num);
    assert(children != NULL && "Buy more RAM");

    for (int i = 0; i < parallel_num; ++i) {
        int child = fork();
        assert(child != -1 && "fork fail");

        bool first_child = (i == 0);
        bool last_child = (i == parallel_num - 1);

        if (child == 0) {
            int program_index = -1;
            int argv_len = 0;
            if (first_child) {
                program_index = 0;
                argv_len = ampersand_arr[0] - program_index;
            } else if (last_child) {
                program_index = ampersand_arr[i-1]+1;
                argv_len = token_arr_len - program_index;
            } else {
                program_index = ampersand_arr[i-1]+1;
                argv_len = ampersand_arr[i] - ampersand_arr[i-1] - 1;
            }
            if (argv_len == 0) exit(0);
            program_runner(token_arr_copy(token_arr, program_index, argv_len), path);
        }
        children[i] = child;
    }

    for (int i = 0; i < parallel_num; ++i) waitpid(children[i], NULL, 0);
    
    free(children);
}

void exec_util_program(char **token_arr, Node *path)
{
    int child = fork();
    assert(child != -1 && "fork fail");

    if (child == 0) {
        program_runner(token_arr, path);
    } else {
        waitpid(child, NULL, 0);
    }
}

void exec_program(char **token_arr, Node **path, bool *running)
{
    Cmd cmd = get_cmd_type(token_arr[0]);
    int *ampersand_index_arr = NULL; 

    switch (cmd) {
    case BUILT_IN_EXIT:
        *running = token_arr[1] != NULL; // if exit has other params, then fail to exit
        if (*running) print_error();
        break;
    case BUILT_IN_CD:
        if (token_arr[2] != NULL) { // only accpet one param: cd <path>
            print_error();
            break;
        }
        if (chdir(token_arr[1]) == -1) print_error();
        break;
    case BUILT_IN_PATH:
        node_free(*path);
        *path = NULL;
        if (token_arr[1] == NULL) break;
        for (int i = 1; token_arr[i] != NULL; ++i) node_add_tail(path, token_arr[i]);
        break;
    case UTIL:
        if (*path == NULL) {
            print_error();
            break;
        }
        if (is_parallel_exec(token_arr)) {
            ampersand_index_arr = get_ampersand_index_arr(token_arr);
            if (!valid_parallel_program(ampersand_index_arr)) break;

            parallel_exec_util_program(token_arr, ampersand_index_arr, *path);
        } else {
            exec_util_program(token_arr, *path);
        }
        break;
    }

    token_arr_free(token_arr);
    free(ampersand_index_arr);
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        print_error();
        exit(1);
    }

    char *line = NULL;
    size_t line_size = 0;

    bool running = true;
    Node *path = NULL;
    node_add_tail(&path, "/bin"); // Initial path

    // batch mode
    if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        if (!f) {
            print_error();
            exit(1);
        }

        while ((getline(&line, &line_size, f)) != EOF) {
            char **token_arr = token_arr_init(line);

            if (token_arr[0] == NULL) {
                token_arr_free(token_arr);
                continue;
            }

            running = false;
            exec_program(token_arr, &path, &running);
        }

        if (path) node_free(path);
        free(line);
        fclose(f);

        return 0;
    }

    // interactive mode
    while (running) {
        printf("wish> ");

        int rcv_bytes = getline(&line, &line_size, stdin);
        if (rcv_bytes == EOF) break;

        char **token_arr = token_arr_init(line);
        if (token_arr[0] == NULL) {
            token_arr_free(token_arr);
            continue;
        }

        exec_program(token_arr, &path, &running);
    }

    if (path) node_free(path);
    free(line);

    return 0;
}
