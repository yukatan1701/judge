#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#define WHITE   "\033[1;37m"
#define YELLOW	"\033[1;33m"
#define RED		"\033[1;31m"
#define CYAN	"\033[1;36m"
#define GREEN   "\033[1;32m"
#define RESET   "\033[0m"

// TODO: settings parsing

typedef struct settings {
	int problems;
	char *score;
	int factor;
} Settings;

typedef struct user_info {
	char *username;
	char *results;
	int sum;
} UserInfo;

typedef struct problem_stat {
	char *username;
	char problem;
	int tested;
	int failed;
	char *results;
} Stat;

char *log_path = NULL;

void invalid_format()
{
	puts("Failed to load settings from file: invalid format.");
	exit(1);
}

void allocation_error()
{
	puts("Failed to allocate memory.");
	exit(1);
}

char *make_log_path(char *time)
{
	if (time == NULL) {
		puts("Failed to get current time. Terminate.");
		exit(1);
	}
	char log_dir[] = "log/";
	char base[] = "judge_";
	char ext[] = ".log";
	int len = strlen(log_dir) + strlen(time) + strlen(base) + strlen(ext);  
	char *new_time = malloc(sizeof(char) * (len + 1));
	if (new_time == NULL)
		allocation_error();
	memset(new_time, 0, len + 1);
	for (int i = 0; time[i] != 0; i++) {
		if (time[i] == ' ')
			time[i] = '_';
		else if (time[i] == '\n')
			time[i] = 0;
	}
	sprintf(new_time, "%s%s%s%s", log_dir, base, time, ext);
	return new_time;
}

void set_log_path()
{
	time_t cur_time = time(NULL);
	char *time = ctime(&cur_time);
	log_path = make_log_path(time);
}

void init_log()
{
	set_log_path();
	int fd = open(log_path, O_CREAT|O_EXCL|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd < 0) {
		perror("Failed to create log file");
		exit(1);
	}
	close(fd);
}

FILE *open_log()
{	
	FILE *log = fopen(log_path, "a+");
	if (log == NULL) {
		perror("Failed to open log file");
		exit(1);
	}
	return log;
}

void write_log(char *message)
{
	FILE *log = open_log();
	fprintf(log, "%s\n", message);
	fclose(log);
}

void print_settings(Settings set)
{
	FILE *log = open_log();
	fprintf(log, "Current settings:\n");
	fprintf(log, "problems: %d\n", set.problems);
	fprintf(log, "score: %s\n", set.score);
	fprintf(log, "factor: %d\n\n", set.factor);
	fclose(log);
}

void print_settings_terminal(Settings set)
{
	printf("Current settings:\n");
	printf("problems: %d\n", set.problems);
	printf("score: %s\n", set.score);
	printf("factor: %d\n\n", set.factor);
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
	if (strcmp(key, "problems") == 0) {
		int new_val = atoi(value);
		if (new_val < 1) {
			puts("Problems count is too short (min = 1). Terminate.");
			write_log("Problems count is too short.");
			exit(1);
		} else if (new_val > 26) {
			puts("Problems count is too long (max = 26). Terminate.");
			write_log("Problems count is too long.");
			exit(1);
		} else {
			set->problems = new_val;
		}
	} else if (strcmp(key, "score") == 0) {
		set->score = copy_string(value);
	} else if (strcmp(key, "factor") == 0) {
		int new_val = atoi(value);
		if (new_val < 1) {
			puts("Factor is too short (min = 1). Terminate.");
			write_log("Problems count is too short.");
			exit(1);
		} else if (new_val > 1000) {
			puts("Factor is too long (max = 1000). Terminate.");
			write_log("Problems count is too long.");
			exit(1);
		} else {
			set->factor = new_val;
		}
	}
}

Settings read_settings(FILE *file)
{
	Settings set;
	char c = 0;
	int byte_len = 0;
	while (c != EOF) {
		//printf("[%d %d]", byte_len, c);
		int i = 0;
		char *key = NULL, *value = NULL;
		char **current_field = &key;
		int was_equal = 0;
		c = fgetc(file);
		if (c == ' ') {
			continue;
		}
		if ((c == EOF || c == '\n') && byte_len == 0) {
			invalid_format();
		}
		byte_len++;
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
	if (byte_len == 0) {
		invalid_format();
	}
	return set;
}

Settings init_settings(char *contest_name)
{
	write_log("Reading settings...");
	char filename[] = "global.cfg";
	int len = strlen(contest_name) + strlen(filename) + 2;
	char *path = malloc(len * sizeof(int));
	if (path == NULL)
		allocation_error();
	memset(path, 0, len);
	sprintf(path, "%s/%s", contest_name, filename);
	FILE *settings_file = fopen(path, "r");
	free(path);
	if (!settings_file) {
		perror("Failed to open global.cfg");
		write_log("Failed to open global.cfg.");
		exit(1);
	}
	Settings set = read_settings(settings_file);
	fclose(settings_file);
	write_log("OK.\n");
	return set;
}

UserInfo **get_user_list(char *contest_name)
{
	UserInfo **user_list = NULL;
	int len = 0;
	DIR *dir;
	struct dirent *ent;
	int code_path_len = strlen(contest_name) + sizeof("/code");
	char *code_path = malloc((code_path_len + 1) * sizeof(char));
	memset(code_path, 0, code_path_len + 1);
	sprintf(code_path, "%s/code", contest_name);
	if (code_path == NULL)
		allocation_error();
	if ((dir = opendir(code_path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type != DT_DIR)
				continue;
			if (strcmp(ent->d_name, "..") != 0 && 
				strcmp(ent->d_name, ".") != 0) {
				user_list = realloc(user_list, (len + 2) * sizeof(UserInfo *));
				if (user_list == NULL)
					allocation_error();
				UserInfo *info = malloc(sizeof(UserInfo *));
				if (info == NULL)
					allocation_error();
				info->username = ent->d_name;
				info->results = NULL;
				user_list[len] = info;
				user_list[len + 1] = NULL;
				len++;
			}
		}
	} else {
		perror("User codes not found");
		write_log("User codes not found.");
		exit(1);
	}
	free(code_path);
	return user_list;
}

void print_stat(Stat stat)
{
	printf("%sstart test%s\n", YELLOW, RESET);
	printf("  %suser%s: %s\n", CYAN, RESET, stat.username);
	printf("  %sproblem%s: %c\n", CYAN, RESET, stat.problem);
	printf("  %stested%s: %d\n", CYAN, RESET, stat.tested);
	printf("  %sfailed%s: %d\n", CYAN, RESET,  stat.failed);
	printf("  %sresults%s: %s\n", CYAN, RESET, stat.results);
	printf("%sstop test%s\n\n", YELLOW, RESET);

	FILE *log = open_log();
	fprintf(log, "start test\n");
	fprintf(log, "  user: %s\n", stat.username);
	fprintf(log, "  problem: %c\n", stat.problem);
	fprintf(log, "  tested: %d\n", stat.tested);
	fprintf(log, "  failed: %d\n", stat.failed);
	fprintf(log, "  results: %s\n", stat.results);
	fprintf(log, "stop test\n\n");
	fclose(log);
}

void print_list(UserInfo **user_list)
{
	if (user_list == NULL) {
		puts("No users found. Terminate.");
		exit(1);
	}
	printf("%sUsers:%s\n", GREEN, RESET);
	write_log("Users:");
	FILE *log = open_log();
	for (int i = 0; user_list[i]; i++) {
		printf("%s ", user_list[i]->username);
		fprintf(log, "%s ", user_list[i]->username);
	}
	puts("\n");
	fclose(log);
}

char *generate_file_path(char *contest_name, char letter)
{
	static const int filename_len = 1;
	static const int slashes_count = 1;
	int user_path_len = strlen("var") + filename_len + slashes_count;
	char *user_file_path = malloc(sizeof(char) * (user_path_len + 1));
	if (user_file_path == NULL)
		allocation_error();
	memset(user_file_path, 0, user_path_len + 1);
	sprintf(user_file_path, "var/%c", letter);
	return user_file_path;
}

char *generate_code_path(char *contest_name, char *username, char letter)
{	
	static const int slashes_count = 3;
	static const int filename_len = 3;
	int tests_path_len = strlen(contest_name) + strlen("code") + 
		strlen(username) + filename_len + slashes_count + 1;
	char *code_path = malloc(sizeof(char) * (tests_path_len + 1));
	if (code_path == NULL)
		allocation_error();
	memset(code_path, 0, tests_path_len + 1);
	sprintf(code_path, "%s/code/%s/%c.c", contest_name, username, letter);
	return code_path;
}

char *generate_output_path(char *contest_name, char letter)
{	
	static const int slashes_count = 2;
	int tests_path_len = strlen(contest_name) + strlen("tests") + 1 + 
		slashes_count + 1;
	char *output_path = malloc(sizeof(char) * (tests_path_len + 1));
	if (output_path == NULL)
		allocation_error();
	memset(output_path, 0, tests_path_len + 1);
	sprintf(output_path, "%s/tests/%c", contest_name, letter);
	return output_path;
}

void answer_to_file(char *contest_name, char *username, char *answer, 
                    char letter)
{
	static const int slashes_count = 3;
	static const char res[] = "results.txt";
	int len = strlen(contest_name) + strlen(username) + strlen("code") + 
		slashes_count + strlen(res) + 2;
	char *path = malloc(sizeof(char) * (len + 1));
	memset(path, 0, len + 1);
	sprintf(path, "%s/code/%s/%c_%s", contest_name, username, letter, res);
	FILE *res_file = fopen(path, "w");
	if (res_file == NULL) {
		perror("Failed to open results.txt");
		write_log("Failed to open results.txt");
		exit(1);
	}
	fprintf(res_file, "%s", answer);
	free(path);
	fclose(res_file);
}

char get_answer(char *contest_name, char *username, char letter, Stat *stat, 
                int wstat)
{
	char *ans = NULL;
	int size = 0;
	char ch = 0;
	while ((ch = getchar()) != EOF) {
		if (ch == '\n')
			continue;
		ans = realloc(ans, size + 2);
		if (ans == NULL)
			allocation_error();
		ans[size] = ch;
		ans[size + 1] = 0;
		size++;
	}
	/* OK */
	if (wstat == 10) {
		size >>= 1;
		memset(ans + size, 0, size);
	}
	if (size == 0) {
		printf("WARNING: No test entries for problem %c.\n", letter);
		FILE *log = open_log();
		fprintf(log, "WARNING: No test entries for problem %c.\n", letter);
		fclose(log);
		ans = malloc(2);
		if (ans == NULL)
			allocation_error();
		ans[0] = 'x';
		ans[1] = 0;
		size++;
	}
	answer_to_file(contest_name, username, ans, letter);
	int plus = 0, minus = 0, x = 0;
	stat->tested = 0;
	stat->failed = 0;
	for (int i = 0; i < size; i++) {
		stat->tested++;
		if (ans[i] == '+')
			plus++;
		else if (ans[i] == '-')
			minus++;
		else {
			x++;
		}
	}
	stat->failed = x;
	stat->results = ans;
	if (x != 0)
		return 'x';
	if (plus != 0 && minus == 0)
		return '+';
	return '-';
}

void compile_user_codes(char *contest_name, char *username, Settings set, 
                        const char alphabet[])
{
	const int path_len = sizeof("var/") + sizeof("");
	char path[path_len + 1];
	for (int i = 0; i < set.problems; i++) {
		char letter = alphabet[i];
		char *code = generate_code_path(contest_name, username, letter);
		memset(path, 0, path_len + 1);
		sprintf(path, "var/%c", letter);
		FILE *log = open_log();
		fprintf(log, "Compiling: %c.c... ", letter);
		fclose(log);
		if (fork() == 0) {
			close(2);
			if (execlp("gcc", "gcc", code, "-o", path, NULL) < 0) {
				perror("Failed to compile user program");
				write_log("Failed to compile user program.");
				exit(1);
			}
		}
		int status;
		wait(&status);
		if (WEXITSTATUS(status) != 0) {
			write_log("Failed.");
		} else {
			write_log("Ok!");
		}
		free(code);
	}
}

void clear_var_dir(const char alphabet[])
{
	write_log("Clearing binary files...");
	char path[257];
	memset(path, 0, 257);
	for (int i = 0; i < strlen(alphabet); i++) {
		sprintf(path, "var/%c", alphabet[i]);
		unlink(path);
	}	
}

void run_tests(char *contest_name, UserInfo **user_list, Settings set, 
               const char alphabet[])
{
	write_log("\n\nChecking user programs...");
	int user_num = 0;
	for (UserInfo **cur_user = user_list; *cur_user; cur_user++, user_num++) {
		char *result = malloc(sizeof(char) * (set.problems + 1));
		if (result == NULL)
			allocation_error();
		memset(result, 0, set.problems + 1);
		//printf("\nUser: %s\n", (*cur_user)->username);
		FILE *log = open_log();	
		fprintf(log, "\nUser: %s\n", (*cur_user)->username);
		fclose(log);
		compile_user_codes(contest_name, (*cur_user)->username, set, alphabet);
		Stat stat = {(*cur_user)->username, 0, 0, 0};
		for (int i = 0; i < set.problems && i < strlen(alphabet) - 1; i++) {
			stat.problem = alphabet[i];
			char *user_file_path = generate_file_path(contest_name, 
				alphabet[i]);
			char *output_path = generate_output_path(contest_name, alphabet[i]);
			int fd[2];
			pipe(fd);
			FILE *log = open_log();
			fprintf(log, "Running tests for program: %c.c...\n", alphabet[i]);
			fprintf(log, "From: %s | To: %s\n", user_file_path, output_path);
			fclose(log);
			if (fork() == 0)
			{
				close(fd[0]);
				dup2(fd[1], 1);
				close(fd[1]);
				if (execl("test", "test", user_file_path, output_path, NULL) < 0) {
					perror("Failed to run tests");
					write_log("Failed to run tests.");
					exit(1);
				}
			}
			close(fd[1]);
			dup2(fd[0], 0);
			close(fd[0]);
			int status = 0;
			wait(&status);
			log = open_log();
			fprintf(log, "Status: %d\n", WEXITSTATUS(status));
			fclose(log);
			int wstat = WEXITSTATUS(status);
			//printf("Status: %d\n", wstat);
			if (wstat != 0) {
				puts("Tests stopped.");
				write_log("Tests stopped.");
				if (wstat == 10) {
					puts("WARNING: Some test is broken. Next tests will be ignored.");
					write_log("Some test is broken. Next tests will be ignored.");
				} else { 
					clear_var_dir(alphabet);
					exit(1);
				}
			}
			char child_answer = get_answer(contest_name, (*cur_user)->username, 
			                    alphabet[i], &stat, wstat);
			print_stat(stat);
			free(stat.results);
			result[i] = child_answer;
			free(user_file_path);
			free(output_path);
		}
		(*cur_user)->results = result;
		clear_var_dir(alphabet);
	}
}

void free_settings(Settings set)
{
	if (set.score)
		free(set.score);
	set.score = NULL;
}

void free_users_info(UserInfo **user_list)
{
	for (int i = 0; user_list[i]; i++) {
		UserInfo *user = user_list[i];
		free(user->results);
		free(user);
	}
	free(user_list);
}

int get_sum(char *result, Settings set)
{
	char *sum_type = set.score;
	int mult = set.factor;
	int sum = 0;
	if (strcmp(sum_type, "sum") == 0) {
		for (int i = 0; result[i]; i++) {
			if (result[i] == '+')
				sum++;
		}
	} else if (strcmp(sum_type, "diff") == 0) {
		for (int i = 0; result[i]; i++) {
			if (result[i] == '+')
				sum += mult * (i + 1);
		}
	} else {
		puts("WARNING: score type is undefined. Default (sum) type will be used.");
		write_log("WARNING: score type is undefined. Default (sum) type will be used.");
		for (int i = 0; result[i]; i++) {
			if (result[i] == '+')
				sum++;
		}
	}
	return sum;
}

int get_list_size(UserInfo **user_list)
{
	int size = 0;
	while(user_list[size])
		size++;
	return size;
}

int comp(const void *void_one, const void *void_two)
{
	UserInfo **one = (UserInfo **) void_one;
	UserInfo **two = (UserInfo **) void_two;
	int sum1 = (*one)->sum;
	int sum2 = (*two)->sum;
	return -(sum1 - sum2);
}

void calculate_results(UserInfo **user_list, Settings set)
{
	for (int i = 0; user_list[i]; i++) {
		UserInfo *user = user_list[i];
		user->sum = get_sum(user->results, set);
	}
}

void generate_results_file(UserInfo **user_list, Settings set, 
                           const char alphabet[])
{
	int list_size = get_list_size(user_list);
	qsort(user_list, list_size, sizeof(UserInfo *), comp);
	printf("\n%s--------Results:--------%s\n", CYAN, RESET);
	FILE *csv = fopen("results.csv", "w");
	if (csv == NULL) {
		write_log("Failed to open results.csv");
		perror("Failed to open results.csv");
		exit(1);
	}
	fprintf(csv, "user,");
	printf("%s%-15s", GREEN, "user");
	for (int i = 0; i < set.problems; i++) {
		fprintf(csv, "%c,", alphabet[i]);
		printf("%-2c", alphabet[i]);
	}
	fprintf(csv, "Sum\n");
	printf("%-4s%s\n", "Sum", RESET);
	for (int i = 0; user_list[i]; i++) {
		UserInfo *user = user_list[i];
		fprintf(csv, "%s,", user->username);
		printf("%s%-15s%s", YELLOW, user->username, RESET);
		for (int j = 0; j < set.problems; j++) {
			printf("%-2c", (user->results)[j]);
			fprintf(csv, "%c,", (user->results)[j]);
		}
		printf("%-4d\n", user->sum);
		fprintf(csv, "%d\n", user->sum);
	}
	fclose(csv);
	write_log("results.csv is created.");
	putchar('\n');
}

void create_directories()
{
	write_log("Creating directories...");
	int mode = S_IRWXU;
	int res;
	errno = 0;
	if ((res = mkdir("var", mode)) != 0) {
		if (errno != EEXIST) {
			perror("Failed to create var directory");
			write_log("Failed to create var directory.");
			exit(1);
		}
	}
	errno = 0;
	if ((res = mkdir("log", mode)) != 0) {
		if (errno != EEXIST) {
			perror("Failed to create log directory");
			write_log("Failed to create log directory.");
			exit(1);
		}
	}
	write_log("OK.\n");
}

void say_hello()
{
	const int width = 80;
	char text[] = "Welcome to Judge!";
	int left = (width - sizeof(text)) / 2;
	putchar('\n');
	for (int i = 0; i < width; i++)
		putchar('-');
	puts("");
	for (int i = 0; i < left; i++)
		putchar(' ');
	printf("%sWelcome to %sJudge!%s\n", YELLOW, RED, RESET);
	for (int i = 0; i < width; i++)
		putchar('-');
	puts("\n");
}

int main(int argc, char *args[]){
	const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char *contest_name = args[1];
	if (contest_name == NULL) {
		puts("Contest name not found.");
		return 1;
	}
	init_log();
	write_log("Contest: ");
	write_log(contest_name);
	write_log("");
	create_directories();
	Settings set = init_settings(contest_name);
	UserInfo **user_list = get_user_list(contest_name);
	say_hello();
	print_settings(set);
	//print_settings_terminal(set);
	print_list(user_list);
	run_tests(contest_name, user_list, set, alphabet);
	calculate_results(user_list, set);
	generate_results_file(user_list, set, alphabet);
	free_settings(set);
	free_users_info(user_list);
	write_log("End.");
	return 0;
}
