#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

char got_eof = 0;
/*implementation of the completion of the program by double clicking Ctrl+D at the end of the entered line*/

typedef struct {
    int argc;
    char brackets;
    char** argv;
} command;

typedef struct {
    int cnt;
    command** arg;
} str_pl;

typedef struct {
    char* input;
    char* output;
    char* append;
    str_pl* arg;
} str_cmd;

char* readstr() {
	int cur;
	if ((cur = getchar()) != EOF) {
		if (cur == '\n') {
			char *emptystr = (char*)malloc(2 * sizeof(char));
			emptystr[0] = '\n';
			emptystr[1] = '\0';
			return emptystr;
		}
		unsigned int j = 0, size = 10;
		char *str = (char*)malloc(size * sizeof(char));
		while (cur != '\n' && cur != EOF) {
			str[j] = cur;
			++j;
			if (j >= size) {
				size += 10;
				str = (char*)realloc(str, size * sizeof(char));
			}
			cur = getchar();
		}
		str[j] = '\0';
        if (cur == EOF)
            ++got_eof;
		return str;
	}
	else
		return NULL;
}

char* del_spaces(char *str) {
    unsigned int i = 0, j = 0, i0 = 0, j0 = 0, cut = 0, cnt = 0, n = strlen(str);
    char *line = (char*)malloc((n + 1) * sizeof(char));
    while (str[i] != '\0') {
        if (str[i] == '"') {
            ++cnt;
            i0 = i+1;
            j0 = j+1;
            line[j] = str[i];
            ++j;
            ++i;
            while (str[i] != '\0' && str[i] != '"') {
                line[j] = str[i];
                ++i;
                ++j;
            }
            if (str[i] == '\0') {
                i = i0;
                j = j0;
                cut = 1;
            }
            else {
                ++cnt;
                line[j] = str[i];
                ++i;
                ++j;
            }
        }
        if (str[i] == ' ') {
            while (str[i] == ' ')
                ++i;
            if (str[i] == '\0')
                break;
            --i;
        }
        if (j == 0 && str[i] == ' ')
            ++i;
        if (str[i] != '"' && str[i] != '\0') {
            line[j] = str[i];
            ++j;
            ++i;
        }
    }
    line[j] = '\0';
    if (cnt % 2) {
        free(line);
        free(str);
        return NULL;
    }
    if (cut) {
        char *line1 = (char*)malloc((n + 1) * sizeof(char));
        strncpy(line1, line, j+1);
        free(line);
        free(str);
        return line1;
    }
    free(str);
    return line;
}

int command_is_arg(char *str) {
    if (strcmp(str, ";") == 0 || strcmp(str, "|") == 0 || strcmp(str, "&") == 0 || strcmp(str, "<") == 0 || strcmp(str, ">") == 0 || strcmp(str, "||") == 0 || strcmp(str, "&&") == 0 || strcmp(str, ">>") == 0)
        return 0;
    else
        return 1;
}

command** get_commands(char *str, int *N) {
    unsigned int i = 0, j = 0, k = 0, l = 0, size = 10, quotes = 0, cnt_brackets = 0;
    command **array_of_commands = NULL; 
  L:if (str[i] != '\0') {
        array_of_commands = (command**)realloc(array_of_commands, (j+1) * sizeof(command*));
        array_of_commands[j] = (command*)malloc(sizeof(command));
        array_of_commands[j] -> brackets = 0;
        array_of_commands[j] -> argc = 0;
    }
    while (str[i] != '\0') {
        if (str[i] != ';' && str[i] != '|' && str[i] != '&' && str[i] != '<' && str[i] != '>') {
            if (str[i] != ' ' || quotes) {
                if (str[i] == '"') {
                    if (quotes)
                        quotes = 0;
                    else
                        quotes = 1;
                    ++i;
                    if (str[i] == '\0') {
                        (array_of_commands[j] -> argv)[k][l] = '\0';
                        array_of_commands[j] -> argv = (char**)realloc(array_of_commands[j] -> argv, (k+2) * sizeof(char*));
                        (array_of_commands[j] -> argv)[k+1] = NULL;
                        ++j;
                        break;
                    }
                }
                if (k == 0 && l == 0) {
                    array_of_commands[j] -> argc = 1;
                    array_of_commands[j] -> argv = (char**)malloc(sizeof(char*));
                    if (str[i] == '(') {
                        ++cnt_brackets;
                        ++array_of_commands[j] -> brackets;
                        (array_of_commands[j] -> argv)[0] = (char*)malloc(size * sizeof(char));
                        if (str[i+1] == '\0') {
                            printf("wrong brackets syntax\n");
                            exit(1);
                        }
                        if (str[i+1] == '(')
                            ++cnt_brackets;
                        else if (str[i+1] == ')') {
                            --cnt_brackets;
                            free((array_of_commands[j] -> argv)[0]);
                            free(array_of_commands[j] -> argv);
                            free(array_of_commands[j]);
                            array_of_commands[j] = NULL;
                            ++j;
                            i += 2;
                            if (str[i] == ' ')
                                ++i;
                            goto L;
                        }
                        while (cnt_brackets != 0) {
                            ++i;
                            (array_of_commands[j] -> argv)[0][l] = str[i];
                            if (str[i+1] == '\0') {
                                printf("wrong brackets syntax\n");
                                exit(1);
                            }
                            else if (str[i+1] == '(')
                                ++cnt_brackets;
                            else if (str[i+1] == ')')
                                --cnt_brackets;
                            ++l;
                            if (l+1 >= size) {
                                size += 10;
                                (array_of_commands[j] -> argv)[0] = (char*)realloc((array_of_commands[j] -> argv)[0], size * sizeof(char));
                            }
                        }
                        (array_of_commands[j] -> argv)[0][l] = '\0';
                        ++j;
                        i += 2;
                        if (str[i] == ' ')
                            ++i;
                        l = 0;
                        k = 0;
                        size = 10;
                        goto L;
                    }
                }
                if (l == 0)
                    (array_of_commands[j] -> argv)[k] = (char*)malloc(size * sizeof(char));
                if (str[i] == ')') {
                    printf("wrong brackets syntax\n");
                    exit(1);
                }
                (array_of_commands[j] -> argv)[k][l] = str[i];
		        if (str[i+1] == '(') {
                    (array_of_commands[j] -> argv)[k][l+1] = '\0';
                    array_of_commands[j] -> argv = (char**)realloc(array_of_commands[j] -> argv, (k+2) * sizeof(char*));
                    (array_of_commands[j] -> argv)[k+1] = NULL;
                    ++j;
                    k = 0;
                    l = 0;
                    size = 10;
                    ++i;
                    goto L;
		        }
                if (str[i+1] == '\0') {
                    (array_of_commands[j] -> argv)[k][l+1] = '\0';
                     array_of_commands[j] -> argv = (char**)realloc(array_of_commands[j] -> argv, (k+2) * sizeof(char*));
                    (array_of_commands[j] -> argv)[k+1] = NULL;
                    ++j;
                    break;
                }
                if (str[i+1] != ';' && str[i+1] != '|' && str[i+1] != '&' && str[i+1] != '<' && str[i+1] != '>') {
                    ++l;
                    if (l+1 >= size) {
                        size += 10;
                        (array_of_commands[j] -> argv)[k] = (char*)realloc((array_of_commands[j] -> argv)[k], size * sizeof(char));
                    }
                }
                else {
                    (array_of_commands[j] -> argv)[k][l+1] = '\0';
                    array_of_commands[j] -> argv = (char**)realloc(array_of_commands[j] -> argv, (k+2) * sizeof(char*));
                    (array_of_commands[j] -> argv)[k+1] = NULL;
                    ++j;
                }
            }
            else {
                (array_of_commands[j] -> argv)[k][l] = '\0';
                if (str[i+1] != ';' && str[i+1] != '|' && str[i+1] != '&' && str[i+1] != '<' && str[i+1] != '>') {
                    ++k;
                    ++array_of_commands[j] -> argc;
                    array_of_commands[j] -> argv = (char**)realloc(array_of_commands[j] -> argv, (k+1) * sizeof(char*));
                    l = 0;
                    size = 10;
                }
                else {
                    array_of_commands[j] -> argv = (char**)realloc(array_of_commands[j] -> argv, (k+2) * sizeof(char*));
                    (array_of_commands[j] -> argv)[k+1] = NULL;
                    ++j;
                }
            }
        }
        else {
            if (l != 0 || k != 0) {
                k = 0;
                l = 0;
                size = 10;
                array_of_commands = (command**)realloc(array_of_commands, (j+1) * sizeof(command*));
                array_of_commands[j] = (command*)malloc(sizeof(command));
            }
            array_of_commands[j] -> argv = (char**)malloc(sizeof(char*));
            array_of_commands[j] -> argc = 1;
            array_of_commands[j] -> brackets = 0;
            if (str[i] == '|' && str[i+1] == '|') {
                (array_of_commands[j] -> argv)[0] = (char*)malloc(3 * sizeof(char));
                (array_of_commands[j] -> argv)[0][0] = '|';
                (array_of_commands[j] -> argv)[0][1] = '|';
                (array_of_commands[j] -> argv)[0][2] = '\0';
                ++i;
            }
            else if (str[i] == '&' && str[i+1] == '&') {
                (array_of_commands[j] -> argv)[0] = (char*)malloc(3 * sizeof(char));
                (array_of_commands[j] -> argv)[0][0] = '&';
                (array_of_commands[j] -> argv)[0][1] = '&';
                (array_of_commands[j] -> argv)[0][2] = '\0';
                ++i;
            }
            else if (str[i] == '>' && str[i+1] == '>') {
                (array_of_commands[j] -> argv)[0] = (char*)malloc(3 * sizeof(char));
                (array_of_commands[j] -> argv)[0][0] = '>';
                (array_of_commands[j] -> argv)[0][1] = '>';
                (array_of_commands[j] -> argv)[0][2] = '\0';
                ++i;
            }
            else {
                (array_of_commands[j] -> argv)[0] = (char*)malloc(2 * sizeof(char));
                (array_of_commands[j] -> argv)[0][0] = str[i];
                (array_of_commands[j] -> argv)[0][1] = '\0';
            }
            ++j;
            ++i;
            if (str[i] == ' ')
                ++i;
            goto L;
        }
        ++i;
    }
    *N = j;
    return array_of_commands;
}

str_pl** get_pipeline(command **array_of_commands, int N, int* cnt_p) {
    str_pl** pipeline = NULL;
    int i = 0, j = 0, k = 0;
    while (j < N) {
        pipeline = (str_pl**)realloc(pipeline, (i+1)*sizeof(str_pl*));
        if (array_of_commands[j] == NULL) {
            pipeline[i] = (str_pl*)malloc(sizeof(str_pl));
            pipeline[i] -> arg = NULL;
            pipeline[i] -> cnt = 1;
        }
        else if (!command_is_arg((array_of_commands[j] -> argv)[0]) && strcmp((array_of_commands[j] -> argv)[0], "|") != 0) {
            pipeline[i] = (str_pl*)malloc(sizeof(str_pl));
            pipeline[i] -> arg = array_of_commands + j;
            pipeline[i] -> cnt = 1;
        }
        else if (command_is_arg((array_of_commands[j] -> argv)[0])) {
            k = j;
            while (k < N-1 && strcmp((array_of_commands[k+1] -> argv)[0], "|") == 0)
                k += 2;
            if (k == N) {
                printf("pipeline has another syntax\n");
                exit(1);
            }
            pipeline[i] = (str_pl*)malloc(sizeof(str_pl));
            pipeline[i] -> arg = array_of_commands + j;
            pipeline[i] -> cnt = k-j+1;
            j = k;
        }
        else {
            printf("wrong args\n");
            exit(1);
        }
        ++i;
        ++j;
    }
    *cnt_p = i;
    return pipeline;
}

str_cmd** get_cmd(str_pl **pipeline, int cnt_p, int* cnt_cmd) {
    str_cmd** cmd = NULL;
    int i = 0, j = 0;
    while (j < cnt_p) {
        if (pipeline[j] -> arg == NULL) {
            cmd = (str_cmd**)realloc(cmd, (i+1)*sizeof(str_cmd*));
            cmd[i] = (str_cmd*)malloc(sizeof(str_cmd));
            cmd[i] -> arg = NULL;
            cmd[i] -> input = NULL;
            cmd[i] -> output = NULL;
            cmd[i] -> append = NULL;
            ++i;
        }
        else if (!command_is_arg((((pipeline[j] -> arg)[0]) -> argv)[0]) && strcmp((((pipeline[j] -> arg)[0]) -> argv)[0], "<") != 0 && strcmp((((pipeline[j] -> arg)[0]) -> argv)[0], ">") != 0 && strcmp((((pipeline[j] -> arg)[0]) -> argv)[0], ">>") != 0) {
            cmd = (str_cmd**)realloc(cmd, (i+1)*sizeof(str_cmd*));
            cmd[i] = (str_cmd*)malloc(sizeof(str_cmd));
            cmd[i] -> arg = pipeline[j];
            cmd[i] -> input = NULL;
            cmd[i] -> output = NULL;
            cmd[i] -> append = NULL;
            ++i;
        }
        else if (strcmp((((pipeline[j] -> arg)[0]) -> argv)[0], "<") == 0) {
            if (j > cnt_p - 2 || !command_is_arg((((pipeline[j+1] -> arg)[0]) -> argv)[0]) || j < 1 || !command_is_arg((((pipeline[j-1] -> arg)[0]) -> argv)[0])) {
                printf("wrong args\n");
                exit(1);
            }
            cmd = (str_cmd**)realloc(cmd, (i+1)*sizeof(str_cmd*));
            cmd[i] = (str_cmd*)malloc(sizeof(str_cmd));
            cmd[i] -> arg = pipeline[j-1];
            cmd[i] -> input = (((pipeline[j+1] -> arg)[0]) -> argv)[0];
            if (j < cnt_p - 3  && strcmp((((pipeline[j+2] -> arg)[0]) -> argv)[0], ">") == 0 && command_is_arg((((pipeline[j+3] -> arg)[0]) -> argv)[0])) {
                cmd[i] -> output = (((pipeline[j+3] -> arg)[0]) -> argv)[0];
                cmd[i] -> append = NULL;
                j += 2;
            }
            else if (j < cnt_p - 3  && strcmp((((pipeline[j+2] -> arg)[0]) -> argv)[0], ">>") == 0 && command_is_arg((((pipeline[j+3] -> arg)[0]) -> argv)[0])) {
                cmd[i] -> append = (((pipeline[j+3] -> arg)[0]) -> argv)[0];
                cmd[i] -> output = NULL;
                j += 2;
            }
            else {
                cmd[i] -> output = NULL;
                cmd[i] -> append = NULL;
            }
            ++i;
            ++j;
        }
        else if (strcmp((((pipeline[j] -> arg)[0]) -> argv)[0], ">") == 0) {
            if (j > cnt_p - 2 || !command_is_arg((((pipeline[j+1] -> arg)[0]) -> argv)[0]) || j < 1 || !command_is_arg((((pipeline[j-1] -> arg)[0]) -> argv)[0])) {
                printf("wrong args\n");
                exit(1);
            }
            cmd = (str_cmd**)realloc(cmd, (i+1)*sizeof(str_cmd*));
            cmd[i] = (str_cmd*)malloc(sizeof(str_cmd));
            cmd[i] -> arg = pipeline[j-1];
            cmd[i] -> output = (((pipeline[j+1] -> arg)[0]) -> argv)[0];
            if (j < cnt_p - 3  && strcmp((((pipeline[j+2] -> arg)[0]) -> argv)[0], "<") == 0 && command_is_arg((((pipeline[j+3] -> arg)[0]) -> argv)[0])) {
                cmd[i] -> input = (((pipeline[j+3] -> arg)[0]) -> argv)[0];
                cmd[i] -> append = NULL;
                j += 2;
            }
            else if (j < cnt_p - 3  && strcmp((((pipeline[j+2] -> arg)[0]) -> argv)[0], ">>") == 0 && command_is_arg((((pipeline[j+3] -> arg)[0]) -> argv)[0])) {
                printf("wrong args\n");
                exit(1);
            }
            else {
                cmd[i] -> input = NULL;
                cmd[i] -> append = NULL;
            }
            ++i;
            ++j;
        }
        else if (strcmp((((pipeline[j] -> arg)[0]) -> argv)[0], ">>") == 0) {
            if (j > cnt_p - 2 || !command_is_arg((((pipeline[j+1] -> arg)[0]) -> argv)[0]) || j < 1 || !command_is_arg((((pipeline[j-1] -> arg)[0]) -> argv)[0])) {
                printf("wrong args\n");
                exit(1);
            }
            cmd = (str_cmd**)realloc(cmd, (i+1)*sizeof(str_cmd*));
            cmd[i] = (str_cmd*)malloc(sizeof(str_cmd));
            cmd[i] -> arg = pipeline[j-1];
            cmd[i] -> append = (((pipeline[j+1] -> arg)[0]) -> argv)[0];
            if (j < cnt_p - 3  && strcmp((((pipeline[j+2] -> arg)[0]) -> argv)[0], "<") == 0 && command_is_arg((((pipeline[j+3] -> arg)[0]) -> argv)[0])) {
                cmd[i] -> input = (((pipeline[j+3] -> arg)[0]) -> argv)[0];
                cmd[i] -> output = NULL;
                j += 2;
            }
            else if (j < cnt_p - 3  && strcmp((((pipeline[j+2] -> arg)[0]) -> argv)[0], ">") == 0 && command_is_arg((((pipeline[j+3] -> arg)[0]) -> argv)[0])) {
                printf("wrong args\n");
                exit(1);
            }
            else {
                cmd[i] -> output = NULL;
                cmd[i] -> input = NULL;
            }
            ++i;
            ++j;
        }
        else if ((command_is_arg((((pipeline[j] -> arg)[0]) -> argv)[0]) && j == cnt_p-1) || (command_is_arg((((pipeline[j] -> arg)[0]) -> argv)[0]) && j < cnt_p-1 && strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], "<") != 0 && strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], ">") != 0 && strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], ">>") != 0)) {
            cmd = (str_cmd**)realloc(cmd, (i+1)*sizeof(str_cmd*));
            cmd[i] = (str_cmd*)malloc(sizeof(str_cmd));
            cmd[i] -> arg = pipeline[j];
            cmd[i] -> input = NULL;
            cmd[i] -> output = NULL;
            cmd[i] -> append = NULL;
            ++i;
        }
        else if ((command_is_arg((((pipeline[j] -> arg)[0]) -> argv)[0]) && j == cnt_p-2 && (strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], "<") == 0 || strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], ">") == 0 || strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], ">>") == 0)) || (command_is_arg((((pipeline[j] -> arg)[0]) -> argv)[0]) && j < cnt_p-2 && (strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], "<") == 0 || strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], ">") == 0 || strcmp((((pipeline[j+1] -> arg)[0]) -> argv)[0], ">>") == 0) && !command_is_arg((((pipeline[j+2] -> arg)[0]) -> argv)[0]))) {
            printf("wrong args\n");
            exit(1);
        }
        ++j;
    }
    *cnt_cmd = i;
    return cmd;
}

void dispose(command ***array_of_commands, unsigned int N) {
    unsigned int j, k;
    for (j = 0; j < N; ++j) {
        if ((*array_of_commands)[j] != NULL) {
            for (k = 0; k < (*array_of_commands)[j] -> argc; ++k)
                free(((*array_of_commands)[j] -> argv)[k]);
            free((*array_of_commands)[j] -> argv);
            free((*array_of_commands)[j]);
        }
    }
    free(*array_of_commands);
}

void dispose_pipeline(str_pl ***pipeline, unsigned int cnt_p) {
    unsigned int j;
    for (j = 0; j < cnt_p; ++j)
        free((*pipeline)[j]);
    free(*pipeline);
}

void dispose_cmd(str_cmd ***cmd, unsigned int cnt_cmd) {
    unsigned int j;
    for (j = 0; j < cnt_cmd; ++j)
        free((*cmd)[j]);
    free(*cmd);
}

int main(int argc, char** argv) {
    if (argc > 3) {
        printf("wrong arguments\n");
        return 1;
    }
    int fd1;
    char *str;
    if (argc == 2) {
        if ((fd1 = open(argv[1], O_RDONLY)) != -1) {
            dup2(fd1, 0);
            close(fd1);
        }
        else {
            printf("can't open the file %s\n", argv[1]);
            return 1;
        }
    }
    if (argc == 3) {
        if (strcmp(argv[2], "-c") == 0) { /* write "-c" as the last argument of myshell if u want the argument to be a command */
            str = (char*)malloc((strlen(argv[1]) + 1) * sizeof(char));
            strcpy(str, argv[1]);
            got_eof = 1;
            goto start;
        }
        else {
            printf("wrong arguments\n");
            return 1;
        }
    }
    char buff[100];
    command** array_of_commands;
    str_pl** pipeline;
    str_cmd** cmd;
    int i, j, N = 0, status, d, cnt_p = 0, cnt_cmd = 0;
    pid_t pid, pid2, pid3;
    signal(SIGINT, SIG_IGN);
    while (got_eof == 0) {
        if (argc == 1) {
            if (getcwd(buff, sizeof(buff)) == NULL) {
                printf("error of getcwd");
                exit(1);
            }
            else
                printf("%s@ ", buff);
        }
        if ((str = readstr()) == NULL) {
            printf("\n");
            break;
        }
  start:if ((str = del_spaces(str)) == NULL) {
            printf("wrong quotes!\n");
            free(str);
            exit(1);
        }
        array_of_commands = get_commands(str, &N);
        pipeline = get_pipeline(array_of_commands, N, &cnt_p);
        cmd = get_cmd(pipeline, cnt_p, &cnt_cmd);
        i = 0;
        j = 0;
        while (i < cnt_cmd) {
            if (cmd[i] -> arg != NULL) {
                if (!command_is_arg(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0]) && strcmp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "||") != 0 && strcmp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "&&") != 0) {
                    if (i == 0 || strcmp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "&") == 0) {
                        printf("syntax error\n");
                        free(str);
                        dispose_cmd(&cmd, cnt_cmd);
                        dispose_pipeline(&pipeline, cnt_p);
                        dispose(&array_of_commands, N);
                        exit(1);
                    }
                    ++i;
                    continue;
                }
                else if (command_is_arg(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0]) && (cmd[i] -> arg) -> cnt > 1) {
                    if (i < cnt_cmd-1 && strcmp(((((cmd[i+1] -> arg) -> arg)[0]) -> argv)[0], "&") == 0) {
                        if ((pid = fork()) > 0) {
                            waitpid(pid, &status, 0);
                            ++i;
                        }
                        else if (pid == 0) {
                            if ((pid2 = fork()) > 0) {
                                free(str);
                                dispose_cmd(&cmd, cnt_cmd);
                                dispose_pipeline(&pipeline, cnt_p);
                                dispose(&array_of_commands, N);
                                exit(0);
                            }
                            else if (pid2 == 0) {
                                int dn;
                                if ((dn = open("/dev/null", O_RDONLY)) == -1) {
                                    printf("error with opening /dev/null\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(dn, 0);
                                close(dn);
                                if (cmd[i] -> output != NULL) {
                                    if ((d = open(cmd[i] -> output, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
                                        printf("error when openning file\n");
                                        free(str);
                                        dispose_cmd(&cmd, cnt_cmd);
                                        dispose_pipeline(&pipeline, cnt_p);
                                        dispose(&array_of_commands, N);
                                        exit(1);
                                    }
                                    dup2(d, 1);
                                    close(d);
                                }
                                if (cmd[i] -> append != NULL) {
                                    if ((d = open(cmd[i] -> append, O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1) {
                                        printf("error when openning file\n");
                                        free(str);
                                        dispose_cmd(&cmd, cnt_cmd);
                                        dispose_pipeline(&pipeline, cnt_p);
                                        dispose(&array_of_commands, N);
                                        exit(1);
                                    }
                                    dup2(d, 1);
                                    close(d);
                                }
                                int fd[2];
                                j = 0;
                                while (j < (cmd[i] -> arg) -> cnt) {
                                    pipe(fd);
                                    if ((pid3 = fork()) > 0) {
                                        dup2(fd[0], 0);
                                        close(fd[1]);
                                        close(fd[0]);
                                    }
                                    else if (pid3 == 0) {
                                        if (j != (cmd[i] -> arg) -> cnt - 1)
                                            dup2(fd[1], 1);
                                        close(fd[1]);
                                        close(fd[0]);
                                        if ((((cmd[i] -> arg) -> arg)[j]) -> brackets != 0) {
                                            execlp("./myshell", "./myshell", ((((cmd[i] -> arg) -> arg)[j]) -> argv)[0], "-c", (char*)0);
                                            printf("error happened\n");
                                            free(str);
                                            dispose_cmd(&cmd, cnt_cmd);
                                            dispose_pipeline(&pipeline, cnt_p);
                                            dispose(&array_of_commands, N);
                                            exit(1);
                                        }
                                        else {
                                            execvp(((((cmd[i] -> arg) -> arg)[j]) -> argv)[0], (((cmd[i] -> arg) -> arg)[j]) -> argv);
                                            printf("there are no program with name %s\n", ((((cmd[i] -> arg) -> arg)[j]) -> argv)[0]);
                                            free(str);
                                            dispose_cmd(&cmd, cnt_cmd);
                                            dispose_pipeline(&pipeline, cnt_p);
                                            dispose(&array_of_commands, N);
                                            exit(1);
                                        }
                                    }
                                    else {
                                        printf("fork() didn't work\n");
                                        free(str);
                                        dispose_cmd(&cmd, cnt_cmd);
                                        dispose_pipeline(&pipeline, cnt_p);
                                        dispose(&array_of_commands, N);
                                        exit(1);
                                    }
                                    j += 2;
                                }
                                while (wait(NULL) != -1);
                                free(str);
                                dispose_cmd(&cmd, cnt_cmd);
                                dispose_pipeline(&pipeline, cnt_p);
                                dispose(&array_of_commands, N);
                                return 0;
                            }
                            else {
                                printf("fork() didn't work\n");
                                free(str);
                                dispose_cmd(&cmd, cnt_cmd);
                                dispose_pipeline(&pipeline, cnt_p);
                                dispose(&array_of_commands, N);
                                exit(1);
                            }
                        }
                        else {
                            printf("fork() didn't work\n");
                            free(str);
                            dispose_cmd(&cmd, cnt_cmd);
                            dispose_pipeline(&pipeline, cnt_p);
                            dispose(&array_of_commands, N);
                            exit(1);
                        }
                    }
                    else {
                        if ((pid = fork()) > 0) {
                            waitpid(pid, &status, 0); //waiting for pipeline
                        }
                        else if (pid == 0) {
                            int fd[2];
                            j = 0;
                            signal(SIGINT, SIG_DFL);
                            if (cmd[i] -> input != NULL) {
                                if ((d = open(cmd[i] -> input, O_RDONLY)) == -1) {
                                    printf("error when openning file\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(d, 0);
                                close(d);
                            }
                            if (cmd[i] -> output != NULL) {
                                if ((d = open(cmd[i] -> output, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
                                    printf("error when openning file\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(d, 1);
                                close(d);
                            }
                            if (cmd[i] -> append != NULL) {
                                if ((d = open(cmd[i] -> append, O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1) {
                                    printf("error when openning file\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(d, 1);
                                close(d);
                            }
                            while (j < (cmd[i] -> arg) -> cnt) {
                                pipe(fd);
                                if ((pid2 = fork()) > 0) {
                                    dup2(fd[0], 0);
                                    close(fd[1]);
                                    close(fd[0]);
                                }
                                else if (pid2 == 0) {
                                    if (j != (cmd[i] -> arg) -> cnt - 1)
                                        dup2(fd[1], 1);
                                    close(fd[1]);
                                    close(fd[0]);
                                    if ((((cmd[i] -> arg) -> arg)[j]) -> brackets != 0) {
                                        execlp("./myshell", "./myshell", ((((cmd[i] -> arg) -> arg)[j]) -> argv)[0], "-c", (char*)0);
                                        printf("error happened\n");
                                        free(str);
                                        dispose_cmd(&cmd, cnt_cmd);
                                        dispose_pipeline(&pipeline, cnt_p);
                                        dispose(&array_of_commands, N);
                                        exit(1);
                                    }
                                    else {
                                        execvp(((((cmd[i] -> arg) -> arg)[j]) -> argv)[0], (((cmd[i] -> arg) -> arg)[j]) -> argv);
                                        printf("there are no program with name %s\n", ((((cmd[i] -> arg) -> arg)[j]) -> argv)[0]);
                                        free(str);
                                        dispose_cmd(&cmd, cnt_cmd);
                                        dispose_pipeline(&pipeline, cnt_p);
                                        dispose(&array_of_commands, N);
                                        exit(1);
                                    }
                                }
                                else {
                                    printf("fork() didn't work\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                j += 2;
                            }
                            while (wait(&status) != -1);
                            free(str);
                            dispose_cmd(&cmd, cnt_cmd);
                            dispose_pipeline(&pipeline, cnt_p);
                            dispose(&array_of_commands, N);
                            if (WIFEXITED(status))
                                return 0;
                            else
                                exit(1);
                        }
                        else {
                            printf("fork() didn't work\n");
                            free(str);
                            dispose_cmd(&cmd, cnt_cmd);
                            dispose_pipeline(&pipeline, cnt_p);
                            dispose(&array_of_commands, N);
                            exit(1);
                        }
                    }
                }
                else if (strcmp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "||") == 0) {
                    if (i < cnt_cmd-1 && (command_is_arg(((((cmd[i+1] -> arg) -> arg)[0]) -> argv)[0]) || ((((cmd[i+1] -> arg) -> arg)[0]) -> argv)[0][0] == '(')) {
                        if (WIFEXITED(status) && !WEXITSTATUS(status))
                            ++i;
                    }
                    else {
                        printf("the operation || has another syntax\n");
                        free(str);
                        dispose_cmd(&cmd, cnt_cmd);
                        dispose_pipeline(&pipeline, cnt_p);
                        dispose(&array_of_commands, N);
                        exit(1);
                    }
                }
                else if (strcmp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "&&") == 0) {
                    if (i < cnt_cmd-1 && (command_is_arg(((((cmd[i+1] -> arg) -> arg)[0]) -> argv)[0]) || ((((cmd[i+1] -> arg) -> arg)[0]) -> argv)[0][0] == '(')) {
                        if (!WIFEXITED(status) || WEXITSTATUS(status))
                            ++i;
                    }
                    else {
                        printf("the operation && has another syntax\n");
                        free(str);
                        dispose_cmd(&cmd, cnt_cmd);
                        dispose_pipeline(&pipeline, cnt_p);
                        dispose(&array_of_commands, N);
                        exit(1);
                    }
                }
                else if (i < cnt_cmd && strcmp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "cd") == 0) {
                    if (chdir(((((cmd[i] -> arg) -> arg)[0]) -> argv)[1]) == -1) {   /*change directory forward (to get back use "..") */
                        printf("wrong path\n");
                        if ((pid = fork()) > 0) {
                            waitpid(pid, &status, 0);
                        }
                        else if (pid == 0) {
                            free(str);
                            dispose_cmd(&cmd, cnt_cmd);
                            dispose_pipeline(&pipeline, cnt_p);
                            dispose(&array_of_commands, N);
                            exit(1);
                        }
                        else {
                            printf("error happened\n");
                            free(str);
                            dispose_cmd(&cmd, cnt_cmd);
                            dispose_pipeline(&pipeline, cnt_p);
                            dispose(&array_of_commands, N);
                            exit(1);
                        }
                    }
                    else if ((pid = fork()) > 0) {
                        waitpid(pid, &status, 0);
                    }
                    else if (pid == 0) {
                        free(str);
                        dispose_cmd(&cmd, cnt_cmd);
                        dispose_pipeline(&pipeline, cnt_p);
                        dispose(&array_of_commands, N);
                        return 0;
                    }
                    else {
                        printf("error happened\n");
                        free(str);
                        dispose_cmd(&cmd, cnt_cmd);
                        dispose_pipeline(&pipeline, cnt_p);
                        dispose(&array_of_commands, N);
                        exit(1);
                    }
                }
                else if (i < cnt_cmd && (cmd[i] -> arg) -> cnt == 1) {
                    if (i < cnt_cmd-1 && strcmp(((((cmd[i+1] -> arg) -> arg)[0]) -> argv)[0], "&") == 0) {
                        if ((pid = fork()) > 0) {
                            waitpid(pid, &status, 0);
                            ++i;
                        }
                        else if (pid == 0) {
                            if ((pid2 = fork()) > 0) {
                                free(str);
                                dispose_cmd(&cmd, cnt_cmd);
                                dispose_pipeline(&pipeline, cnt_p);
                                dispose(&array_of_commands, N);
                                exit(0);
                            }
                            else if (pid2 == 0) {
                                int dn;
                                if ((dn = open("/dev/null", O_RDONLY)) == -1) {
                                    printf("error with opening /dev/null\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(dn, 0);
                                close(dn);
                                if (cmd[i] -> output != NULL) {
                                    if ((d = open(cmd[i] -> output, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
                                        printf("error when openning file\n");
                                        free(str);
                                        dispose_cmd(&cmd, cnt_cmd);
                                        dispose_pipeline(&pipeline, cnt_p);
                                        dispose(&array_of_commands, N);
                                        exit(1);
                                    }
                                    dup2(d, 1);
                                    close(d);
                                }
                                if (cmd[i] -> append != NULL) {
                                    if ((d = open(cmd[i] -> append, O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1) {
                                        printf("error when openning file\n");
                                        free(str);
                                        dispose_cmd(&cmd, cnt_cmd);
                                        dispose_pipeline(&pipeline, cnt_p);
                                        dispose(&array_of_commands, N);
                                        exit(1);
                                    }
                                    dup2(d, 1);
                                    close(d);
                                }
                                if ((((cmd[i] -> arg) -> arg)[0]) -> brackets != 0) {
                                    execlp("./myshell", "./myshell", ((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "-c", (char*)0);
                                    printf("error happened\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                else {
                                    execvp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], (((cmd[i] -> arg) -> arg)[0]) -> argv);
                                    printf("there are no program with name %s\n", ((((cmd[i] -> arg) -> arg)[0]) -> argv)[0]);
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                            }
                            else {
                                printf("fork() didn't work\n");
                                free(str);
                                dispose_cmd(&cmd, cnt_cmd);
                                dispose_pipeline(&pipeline, cnt_p);
                                dispose(&array_of_commands, N);
                                exit(1);
                            }
                        }
                        else {
                            printf("fork() didn't work\n");
                            free(str);
                            dispose_cmd(&cmd, cnt_cmd);
                            dispose_pipeline(&pipeline, cnt_p);
                            dispose(&array_of_commands, N);
                            exit(1);
                        }
                    }
                    else {
                        if ((pid = fork()) > 0) {
                            waitpid(pid, &status, 0);
                        }
                        else if (pid == 0) {
                            if (cmd[i] -> input != NULL) {
                                if ((d = open(cmd[i] -> input, O_RDONLY)) == -1) {
                                    printf("error when openning file\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(d, 0);
                                close(d);
                            }
                            if (cmd[i] -> output != NULL) {
                                if ((d = open(cmd[i] -> output, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
                                    printf("error when openning file\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(d, 1);
                                close(d);
                            }
                            if (cmd[i] -> append != NULL) {
                                if ((d = open(cmd[i] -> append, O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1) {
                                    printf("error when openning file\n");
                                    free(str);
                                    dispose_cmd(&cmd, cnt_cmd);
                                    dispose_pipeline(&pipeline, cnt_p);
                                    dispose(&array_of_commands, N);
                                    exit(1);
                                }
                                dup2(d, 1);
                                close(d);
                            }
                            signal(SIGINT, SIG_DFL);
                            if ((((cmd[i] -> arg) -> arg)[0]) -> brackets != 0) {
                                execlp("./myshell", "./myshell", ((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], "-c", (char*)0);
                                printf("error happened\n");
                                free(str);
                                dispose_cmd(&cmd, cnt_cmd);
                                dispose_pipeline(&pipeline, cnt_p);
                                dispose(&array_of_commands, N);
                                exit(1);
                            }
                            else {
                                execvp(((((cmd[i] -> arg) -> arg)[0]) -> argv)[0], (((cmd[i] -> arg) -> arg)[0]) -> argv);
                                printf("there are no program with name %s\n", ((((cmd[i] -> arg) -> arg)[0]) -> argv)[0]);
                                free(str);
                                dispose_cmd(&cmd, cnt_cmd);
                                dispose_pipeline(&pipeline, cnt_p);
                                dispose(&array_of_commands, N);
                                exit(1);
                            }
                        }
                        else {
                            printf("fork() didn't work\n");
                            free(str);
                            dispose_cmd(&cmd, cnt_cmd);
                            dispose_pipeline(&pipeline, cnt_p);
                            dispose(&array_of_commands, N);
                            exit(1);
                        }
                    }
                }
            }
            ++i;
        }
        dispose_cmd(&cmd, cnt_cmd);
        dispose_pipeline(&pipeline, cnt_p);
        dispose(&array_of_commands, N);
        free(str);
    }
    return 0;
}
