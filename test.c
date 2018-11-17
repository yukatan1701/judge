#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct logfile {
    char * problem;
    int tested;
    int fail_open_dat;
    int fail_open_ans;
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

void put_data(Settings *set, char *key, char *value) {
	if (strcmp(key, "tests") == 0) {
		(*set).tests = atoi(value);
	} else if (strcmp(key, "checker") == 0) {
		int len = strlen(value);
		char *checker = malloc(sizeof(char) * (len + 1));
		if (checker == NULL)
			allocation_error();
		memset(checker, 0, len + 1);
		memcpy(checker, value, len);
		(*set).checker = checker;
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
    sprintf(test, "%s/%d.dat", program_tests, i);
    return test;
}

char * correct_ans_name(int i, char * program_tests) {
    char * test = NULL;
    int len = strlen(program_tests) + 16;
    test = malloc(len * sizeof(char));
    sprintf(test, "%s/%d.ans", program_tests, i);
    return test;
}

char * set_checker(Settings set) {
    int len = strlen(set.checker) + strlen("checkers/") + 1;
    char * checker = malloc(len * sizeof(char));
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

void check_wstat_checker(int wstatus2) {
    if(WEXITSTATUS(wstatus2) == 0) {
        putchar('+');
    }
    if(WEXITSTATUS(wstatus2) == 1) {
        putchar('-');
    }
    if(WEXITSTATUS(wstatus2) == 2) {
        exit(1);
    }
    if(WEXITSTATUS(wstatus2) == 3) {
        putchar('x');
    }
}

void launch_program(int * pipe_chanel, char * program_name, int i, char * program_tests) {
    int fd;
    char * test_name;
    if (fork() == 0) {
        test_name = set_test_name(i, program_tests);
        fd = open(test_name, O_RDWR);
        if (fd < 0) {
            perror("Error open test");
            exit(1);
        }
        if (dup2(fd, 0) < 0) {
            perror("Fail to dup2 reading ");
            exit(1);
        }
        if (dup2(pipe_chanel[1], 1) < 0) {
            perror("Fail dup2 writing");
            exit(1);
        }
        close(pipe_chanel[1]);
        free(test_name);
        if (execl(program_name, program_name, NULL) < 0) {
            perror("Fail to open program");
            exit(1);
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
            perror("Fail to open program");
            exit(2);
        }
    }
}

void check_wstat_prog(int wstatus1) {
    if (WEXITSTATUS(wstatus1) != 0) {
        exit(1);
    }
}

void init_log(Logfile * log, char * program_name) {
    log -> problem = program_name;
    log -> tested = 0;
    log -> fail_open_dat = 0;
    log -> fail_open_ans = 0;
}

void print_log(Logfile log) {
    printf("Problem: %s\n", log.problem);
    printf("tested: %d\n", log.tested);
    printf("fail open test: %d\n", log.fail_open_dat);
    printf("fail open ans %d\n", log.fail_open_ans);
}

void launch_tests(char * program_name, char * program_tests, Settings set) {
    Logfile log;
	init_log(&log, program_name);
    //print_log(log);
    int pipe_chanel[2], wstatus1, wstatus2;
    for (int i = 1; i <= set.tests; i++) {
        pipe(pipe_chanel);
        launch_program(pipe_chanel, program_name, i, program_tests);
        close(pipe_chanel[1]);
        wait(&wstatus1);
        check_wstat_prog(wstatus1);
        char * correct_ans = correct_ans_name(i, program_tests);
        launch_checker(set, pipe_chanel, correct_ans);
        wait(&wstatus2);
        check_wstat_checker(wstatus2);
    }
    puts("");
}



int main(int argc, char *args[]) {
	char * program_name = args[1];
	char * program_tests = args[2];
	if (program_name == NULL) {
		puts("Program name not found.");
		return 1;
	}
	Settings set = init_settings(program_tests);
	//print_log(log);
	//print_settings(set);
	launch_tests(program_name, program_tests, set);
	return 0;
}
