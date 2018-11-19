#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

typedef struct logfile {
    char * correct_ans;
    int status;
} Logfile;

void allocation_error() {
	puts("Failed to allocate memory.");
	exit(2);
}

char * read_ans() {
    int n = 0;
    char * program_ans = NULL;
    char ch;
    ch = getchar();
    while (ch != EOF) {
        program_ans = realloc(program_ans, (n + 1) * sizeof(char));
        program_ans[n] = ch;
        n++;
        ch = getchar();
    }
    program_ans = realloc(program_ans, (n + 1) * sizeof(char));
    program_ans[n] = '\0';
    return program_ans;
}

char * open_ans(char * file_path) {
    int n = 0, fd = open(file_path, O_RDONLY);
    char * file_ans = NULL;
    char ch;
    if (fd < 0) {
        perror("checker: Fail open correct ans");
        exit(2);
    }
    ssize_t read_char = read(fd, &ch, 1);
    while (read_char != 0) {
        file_ans = realloc(file_ans, (n + 1) * sizeof(char));
        file_ans[n] = ch;
        n++;
        read_char = read(fd, &ch, 1);
        if (read_char < 0) {
            perror("Fail: reading ans");
            exit(2);
        }
    }
    file_ans = realloc(file_ans, (n + 1) * sizeof(char));
    file_ans[n] = '\0';
    if (close(fd) < 0) {
        perror("Fail close test");
        exit(2);
    }
    return file_ans;
}

char * set_log_name() {
    char * log_name = NULL;
    struct tm *ptr;
    time_t It;
    It = time(NULL);
    ptr = localtime(&It);
    int len = strlen(asctime(ptr)) + strlen("log.txt") + 1;
    log_name = malloc(len * sizeof(len));
    if (log_name == NULL) {
        allocation_error();
    }
    sprintf(log_name, "%s_checker_log.txt", asctime(ptr));
    return log_name;
}

void write_status(Logfile log, int fd) {
    switch(log.status) {
        case 3:
            write(fd, "Fail to launch program", strlen("Fail to launch program"));
            break;
        case 2:
            write(fd, "file .ans error", strlen("file .ans error"));
            break;
        default:
            write(fd, "OK", strlen("OK"));
            break;
    }
}

void create_log_dir() {
    int mode = S_IRWXU;
	mkdir("checkers_log", mode);
}

void write_log(Logfile log) {
    create_log_dir();
    char * log_name = set_log_name();
    chdir("checkers_log");
    int fd = open(log_name, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    write(fd, "start", 5);
    write(fd, "\n", 1);
    write(fd, "ans file: ", strlen("ans file: "));
    write(fd, log.correct_ans, strlen(log.correct_ans));
    write(fd, "\n", 1);
    write(fd, "status: ", strlen("status: "));
    write_status(log, fd);
    write(fd, "\n", 1);
    write(fd, "finish", strlen("finish"));
    write(fd, "\n", 1);
    write(fd, "\n", 1);
    chdir("..");
}

int main(int argc, char * argv[]) {
    int status = 0;
    char * file_path = argv[1];
    Logfile log;
    log.correct_ans = file_path;
    log.status = 0;
    if (file_path == NULL) {
        perror("Wrong ans file");
        exit(2);
    }
    char * correct_ans, * program_ans;
    correct_ans = open_ans(file_path);
    program_ans = read_ans();
    if ((strlen(correct_ans) - 1) != strlen(program_ans)) {
        status = 1;
    } else {
        for (int i = 0; i < strlen(program_ans); i++) {
            if (correct_ans[i] != program_ans[i]) {
                status = 1;
            }
        }
    }
    if (program_ans[0] == '\0') { // плохой ответ выдала программа
        log.status = 3;
        write_log(log);
        exit(3);
    }
    if(correct_ans[0] == '\0') { // файл с правильным ответом плохой
        log.status = 2;
        write_log(log);
        exit(2);
    }
    write_log(log);
    free(program_ans);
    free(correct_ans);
    return status;
}
