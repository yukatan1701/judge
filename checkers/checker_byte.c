#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

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
        perror("Fail open test sss");
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
    n++;
    if (close(fd) < 0) {
        perror("Fail close test");
        exit(2);
    }
    return file_ans;
}

int main(int argc, char * argv[]) {
    int status = 0;
    char * file_path = argv[1];
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
    return status;
}
