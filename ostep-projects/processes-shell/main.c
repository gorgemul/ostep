#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

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

void node_dump(Node *head)
{
    while (head) {
        printf("Node data: %s\n", head->data);
        head = head->next;
    }
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

char *get_exe_path(Node *path, char *exe_name)
{
    char *buf = NULL;

    while (path) {
        buf = malloc(sizeof(*buf) * strlen(path->data) * strlen(exe_name) + 2); // 2 for '/' and '\0'
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

void exec_util_program(char **token_arr, Node *path)
{
    int child = fork();
    assert(child != -1 && "fork fail");

    if (child == 0) {
        char *exe_path = get_exe_path(path, token_arr[0]); // Assume first token always be the exe_name
        if (!exe_path) {
            print_error();
            exit(1);
        }
        execv(exe_path, token_arr);
        perror("Execv fail");
    } else {
        waitpid(child, NULL, 0);
    }
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

    bool running = true;

    Node *path = NULL;
    node_add_tail(&path, "/bin"); // Initial path

    while(running) {
        printf("wish> ");

        int rcv_bytes = getline(&line, &line_size, stdin);
        if (rcv_bytes == EOF) break;

        char **token_arr = token_arr_init(line);
        if (token_arr[0] == NULL) {
            token_arr_free(token_arr);
            continue;
        }

        Cmd cmd = get_cmd_type(token_arr[0]);

        switch (cmd) {
        case BUILT_IN_EXIT:
            running = token_arr[1] != NULL; // if exit has other params, then fail to exit
            if (running) print_error();
            break;
        case BUILT_IN_CD:
            break;
        case BUILT_IN_PATH:
            node_free(path);
            path = NULL;
            if (token_arr[1] == NULL) break;
            for (int i = 1; token_arr[i] != NULL; ++i) node_add_tail(&path, token_arr[i]);
            break;
        case UTIL:
            exec_util_program(token_arr, path);
            break;
        }

        token_arr_free(token_arr);
    }

    free(line);
}
