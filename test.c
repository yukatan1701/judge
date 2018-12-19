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
    char * problem;
    int test_num;
    int status;
} Logfile;

typedef struct settings {
	int tests;
	char *checker;
} Settings;

void invalid_format() {
	puts("Failed to load settings from file: invalid format.");
	exit(1);
}

void allocation_error() {
	puts("Failed to allocate memory.");
	exit(1);
}

void print_settings(Settings set) {
	printf("count of tests: %d\n", set.tests);
	printf("checker: %s\n", set.checker);
}

char *copy_string(char *value)
{
	int len = strlen(value);
	char *score = malloc(sizeof(char) * (len + 1));
	if (score == NULL)
		allocation_error();
	memset(score, 0, len + 1);
	memcpy(score, value, len);
	return score;
}

void put_data(Settings *set, char *key, char *value)
{
	if (strcmp(key, "tests") == 0) {
		int new_val = atoi(value);
		if (new_val < 1) {
			fprintf(stderr, "Tests count is wrong. Terminate.");
			exit(1);
		} else if (new_val > 10000) {
			fprintf(stderr, "Tests count is wrong (max = 10000). Terminate.");
			exit(1);
		} else {
			set->tests = new_val;
		}
	} else if (strcmp(key, "checker") == 0) {
		set-> checker = copy_string(value);
	}
}

Settings read_settings(FILE *file) {
	Settings set;
	char c = 0;
	while (c != EOF) {
		int i = 0;
		char *key = NULL, *value = NULL;
		char **current_field = &key;
		int was_equal = 0;
		c = fgetc(file);
		while (c != '\n' && c != EOF) {
			if (c == ' ') {
				c = fgetc(file);
				continue;
			}
			if (c == '=') {
				if (was_equal)
					invalid_format();
				was_equal = 1;
				current_field = &value;
				i = 0;
				c = fgetc(file);
				continue;
			}
			*current_field = realloc(*current_field, (i + 2) * sizeof(char));
			if (*current_field == NULL)
				allocation_error();
			(*current_field)[i] = c;
			(*current_field)[i + 1] = 0;
			i++;
			c = fgetc(file);
		}
		if (key == NULL && value == NULL) {
			// empty line
			if (was_equal == 0)
				continue;
			else
				invalid_format();
		}
		if (key == NULL || value == NULL)
			invalid_format();
		put_data(&set, key, value);
		free(key);
		free(value);
	}
	return set;
}

Settings init_settings(char *program_tests) {
	char filename[] = "problem.cfg";
	int len = strlen(program_tests) + strlen(filename) + 2;
	char *path = malloc(len * sizeof(int));
	if (path == NULL)
		allocation_error();
	memset(path, 0, len);
	strcpy(path, program_tests);
	strncat(path, "/", 1);
	strcat(path, filename);
	FILE *settings_file = fopen(path, "r");
	free(path);
	if (!settings_file) {
		perror("Failed to open problem.cfg");
		exit(1);
	}
	Settings set = read_settings(settings_file);
	fclose(settings_file);
	return set;
}

char * set_test_name(int i, char * program_tests) {
    char * test = NULL;
    int len = strlen(program_tests) + 16;
    test = malloc(len * sizeof(char));
    memset(test, 0, len + 1);
    sprintf(test, "%s/%d.dat", program_tests, i);
    return test;
}

char * correct_ans_name(int i, char * program_tests) {
    char * test = NULL;
    int len = strlen(program_tests) + 16;
    test = malloc(len * sizeof(char));
    memset(test, 0, len);
    sprintf(test, "%s/%d.ans", program_tests, i);
    return test;
}

char * set_checker(Settings set) {
    int len = strlen(set.checker) + strlen("checkers/") + 1;
    char * checker = malloc(len * sizeof(char));
    memset(checker, 0, len + 1);
    if (strcmp(set.checker, "checker_int") == 0) {
        sprintf(checker, "checkers/checker_int");
        return checker;
    }
    if (strcmp(set.checker, "checker_byte") == 0) {
        sprintf(checker, "checkers/checker_byte");
        return checker;
    }
    perror("Wrong checker");
    exit(1);
}

void launch_program(int * pipe_chanel, char * program_name, int i, char * program_tests) {
    int fd;
    char * test_name;
    if (fork() == 0) {
        test_name = set_test_name(i, program_tests);
        fd = open(test_name, O_RDWR);
        if (fd < 0) {
            close(pipe_chanel[1]);
            exit(1);
        }
        if (dup2(fd, 0) < 0) {
            perror("Fail to dup2 reading ");
            exit(2);
        }
        if (dup2(pipe_chanel[1], 1) < 0) {
            perror("Fail dup2 writing");
            exit(2);
        }
        close(pipe_chanel[1]);
        free(test_name);
        if (execl(program_name, program_name, NULL) < 0) {
            perror("Fail to open program");
            exit(2);
        }
    }
}

void launch_checker(Settings set, int * pipe_chanel, char * correct_ans) {
    if (fork() == 0) {
        char * checker = set_checker(set);
        close(pipe_chanel[1]);
        if (dup2(pipe_chanel[0], 0) < 0) {
            perror("Fail to dup2 (checker)");
            exit(2);
        }
        if (close(pipe_chanel[0]) < 0) {
            perror("Fail to close");
            exit(2);
        }
        if (execl(checker, checker, correct_ans, NULL)) {
            perror("Fail to open checker");
            exit(2);
        }
    }
}

void init_log(Logfile * log, char * program_name, int i) {
    log -> problem = program_name;
    log -> status = 0;
    log -> test_num = i;
}

void print_log(Logfile log) {
    printf("Problem: %s\n", log.problem);
    printf("tested: %d\n", log.test_num);
    printf("status: %d\n", log.status);
}

void write_num(int a, int fd) {
    char str[20];
    int i = 0;
    while (a != 0) {
        str[i++] = '0' + a % 10;
        a /= 10;
    }
    i--;
    while (i >= 0) {
        write(fd, &str[i], 1);
        i--;
    }
}

// log status = 10: не открылся dat файл
// status = 2: не запустилась программа
// status = 3: проблема с checker / файлом ans
// status = 4: программа не выдала ответ
// status = 5 ; ошибка компиляции

void write_status(Logfile log, int fd) {
    switch(log.status) {
        case 10:
            write(fd, "dat file error", strlen("dat file error"));
            break;
        case 2:
            write(fd, "Fail to launch program", strlen("Fail to launch program"));
            break;
        case 3:
            write(fd, "Checker/file .ans error", strlen("Checker/file .ans error"));
            break;
        case 4:
            write(fd, "Invalid program output", strlen("Invalid program output"));
            break;
        case 5:
            write(fd, "Fail to compile checker", strlen("Fail to compile checker"));
            break;
        default:
            write(fd, "OK", strlen("OK"));
            break;
    }
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
    sprintf(log_name, "%s_log.txt", asctime(ptr));
    return log_name;
}

void write_log(Logfile log, char * log_name) {
    chdir("tests_log");
    int fd = open(log_name, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    write(fd, "start", 5);
    write(fd, "\n", 1);
    write(fd, "problem: ", strlen("problem: "));
    write(fd, log.problem, strlen(log.problem));
    write(fd, "\n", 1);
    write(fd, "test num: ", strlen("test num: "));
    write_num(log.test_num, fd);
    write(fd, "\n", 1);
    write(fd, "status: ", strlen("status: "));
    write_status(log, fd);
    write(fd, "\n", 1);
    write(fd, "finish", strlen("finish"));
    write(fd, "\n", 1);
    write(fd, "\n", 1);
    chdir("..");
}

void check_wstat_checker(int wstatus2, Logfile * log, char * log_name) {
    if(WEXITSTATUS(wstatus2) == 0) {
        putchar('+');
    }
    if(WEXITSTATUS(wstatus2) == 1) {
        putchar('-');
    }
    if(WEXITSTATUS(wstatus2) == 2) { // проблема с checker / файлом ans
        log -> status = 3;
        write_log(*log, log_name);
        exit(1);
    }
    if(WEXITSTATUS(wstatus2) == 3) { // программа не выдала ответ
        putchar('x');
        log -> status = 4;
        exit(0);
    }
}

int check_wstat_prog(int wstatus1, Logfile * log, char * log_name) {
    if (WEXITSTATUS(wstatus1) == 1) { // не открылся .dat файл
        log -> status = 10;
        write_log(*log, log_name);
        exit(10);
    }
    if (WEXITSTATUS(wstatus1) == 2) { // Не запустилась программа
        putchar('x');
        log -> status = 2;
        write_log(*log, log_name);
        exit(0);
    }
    return 0;
}

void launch_tests(char * program_name, char * program_tests, Settings set, char * log_name) {
    Logfile log;
    //print_log(log);
    int pipe_chanel[2], wstatus1, wstatus2;
    for (int i = 1; i <= set.tests; i++) {
        //printf("%d\n", i);
        init_log(&log, program_name, i);
        pipe(pipe_chanel);
        launch_program(pipe_chanel, program_name, i, program_tests);
        wait(&wstatus1);
        close(pipe_chanel[1]);
        check_wstat_prog(wstatus1, &log, log_name);
        char * correct_ans = correct_ans_name(i, program_tests);
        launch_checker(set, pipe_chanel, correct_ans);
        wait(&wstatus2);
        check_wstat_checker(wstatus2, &log, log_name);
        putchar('\n');
        write_log(log, log_name);
    }
    puts("");
}

char * set_checker_name(Settings set) {
    char * name = NULL;
    int len = strlen(set.checker) + strlen(".c") + 1;
    name = malloc(len * sizeof(char));
    if (name == NULL) {
        allocation_error();
    }
    memset(name, 0, len + 1);
    sprintf(name, "%s.c", set.checker);
    return name;
}

void checker_log_fail(char * program_name, char * log_name) {
    Logfile log;
    log.problem = program_name;
    log.status = 5;
    log.test_num = 0;
    write_log(log, log_name);
}

void compile_checker(Settings set, char * program_name, char * log_name) {
    char * name = set_checker_name(set);
    int wstatus;
    if (fork() == 0) {
        chdir("checkers");
        if (execlp("gcc", "gcc", name, "-o", set.checker, NULL) < 0) {
            perror("Failed to compile checker program");
            exit(1);
        }
    }
    wait(&wstatus);
    if (WEXITSTATUS(wstatus) != 0) {
        checker_log_fail(program_name, log_name);
        exit(1);
    }
    free(name);
}

void create_log_dir() {
    int mode = S_IRWXU;
	mkdir("tests_log", mode);
}

int main(int argc, char *args[]) {
	char * program_name = args[1];
	char * program_tests = args[2];
	if (program_name == NULL) {
		puts("Program name not found.");
		return 1;
	}
	if (program_tests == NULL) {
	    puts("Program tests not found.");
		return 1;
	}
	create_log_dir();
	Settings set = init_settings(program_tests);
	//print_log(log);
	//print_settings(set);
	char * log_name = set_log_name();
	compile_checker(set, program_name, log_name);
	launch_tests(program_name, program_tests, set, log_name);
	return 0;
}
