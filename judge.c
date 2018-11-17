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

typedef struct settings {
	int problems;
	char *score;
} Settings;

typedef struct user_info {
	char *username;
	char *results;
	int sum;
} UserInfo;

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

void print_settings(Settings set)
{
	puts("Current settings:");
	printf("problems: %d\n", set.problems);
	printf("score: %s\n", set.score);
	puts("");
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
		set->problems = atoi(value);
	} else if (strcmp(key, "score") == 0) {
		set->score = copy_string(value);
	}
}

Settings read_settings(FILE *file)
{
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

Settings init_settings(char *contest_name)
{
	puts("Reading settings...");
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
		exit(1);
	}
	Settings set = read_settings(settings_file);
	fclose(settings_file);
	puts("OK.\n");
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
		exit(1);
	}
	free(code_path);
	return user_list;
}

void print_list(UserInfo **user_list)
{
	if (user_list == NULL) {
		puts("No users found. Terminate.");
		exit(1);
	}
	puts("Users:");
	for (int i = 0; user_list[i]; i++)
		printf("%s ", user_list[i]->username);
	puts("\n");
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
		exit(1);
	}
	fprintf(res_file, "%s", answer);
	free(path);
	fclose(res_file);
}

char get_answer(char *contest_name, char *username, char letter)
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
	printf("%s\n", ans);
	//puts(ans);
	//answer_to_file(contest_name, username, ans, letter);
	int plus = 0, minus = 0, x = 0;
	for (int i = 0; i < size; i++) {
		if (ans[i] == '+')
			plus++;
		else if (ans[i] == '-')
			minus++;
		else
			x++;
	}
	free(ans);
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
	//char filename[2] = {0, 0};
	for (int i = 0; i < set.problems; i++) {
		char letter = alphabet[i];
		char *code = generate_code_path(contest_name, username, letter);
		memset(path, 0, path_len + 1);
		sprintf(path, "var/%c", letter);
		//printf("Try to compile: FROM: %s TO: %s\n", code, path);
		printf("Compiling: %c.c... ", letter);
		if (fork() == 0) {
			//filename[0] = letter;
			close(2);
			if (execlp("gcc", "gcc", code, "-o", path, NULL) < 0) {
				perror("Failed to compile user program");
				exit(1);
			}
		}
		int status;
		wait(&status);
		if (WEXITSTATUS(status) != 0) {
			puts("Failed.");
		} else {
			puts("Ok!");
		}
		free(code);
	}
}

void clear_var_dir(const char alphabet[])
{
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
	puts("Checking user programs...");
	int user_num = 0;
	for (UserInfo **cur_user = user_list; *cur_user; cur_user++, user_num++) {
		char *result = malloc(sizeof(char) * (set.problems + 1));
		if (result == NULL)
			allocation_error();
		memset(result, 0, set.problems + 1);
		printf("\nUser: %s\n", (*cur_user)->username);
		compile_user_codes(contest_name, (*cur_user)->username, set, alphabet);
		for (int i = 0; i < set.problems && i < strlen(alphabet) - 1; i++) {
			char *user_file_path = generate_file_path(contest_name, 
				alphabet[i]);
			char *output_path = generate_output_path(contest_name, alphabet[i]);
			int fd[2];
			pipe(fd);
			printf("Running tests for program: %c.c...\n", alphabet[i]);
			printf("(%s) (%s)\n", user_file_path, output_path);
			if (fork() == 0)
			{
				close(fd[0]);
				dup2(fd[1], 1);
				close(fd[1]);
				if (execl("test", "test", user_file_path, output_path, NULL) < 0) {
					perror("Failed to run tests");
					exit(1);
				}
			}
			close(fd[1]);
			dup2(fd[0], 0);
			close(fd[0]);
			int status = 0;
			wait(&status);
			printf("Status: %d\n", status);
			if (WEXITSTATUS(status) != 0) {
				puts("Tests stopped.");
				clear_var_dir(alphabet);
				exit(1);
			}
			char child_answer = get_answer(contest_name, (*cur_user)->username, alphabet[i]);
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

int get_sum(char *result, char *sum_type)
{
	int sum = 0;
	for (int i = 0; result[i]; i++) {
		if (result[i] == '+')
			sum++;
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
	//printf("%s %s\n", name1, name2);
	return -(sum1 - sum2);
}

void calculate_results(UserInfo **user_list, Settings set)
{
	for (int i = 0; user_list[i]; i++) {
		UserInfo *user = user_list[i];
		user->sum = get_sum(user->results, set.score);
	}
}

void generate_results_file(UserInfo **user_list, Settings set, 
	const char alphabet[])
{
	int list_size = get_list_size(user_list);
	qsort(user_list, list_size, sizeof(UserInfo *), comp);
	printf("\nResults:\n");
	FILE *csv = fopen("results.csv", "w");
	if (csv == NULL) {
		perror("Failed to open results.csv");
		exit(1);
	}
	fprintf(csv, "user,");
	printf("%-15s", "user");
	for (int i = 0; i < set.problems; i++) {
		fprintf(csv, "%c,", alphabet[i]);
		printf("%-2c", alphabet[i]);
	}
	fprintf(csv, "Sum\n");
	printf("%-4s\n", "Sum");
	for (int i = 0; user_list[i]; i++) {
		UserInfo *user = user_list[i];
		fprintf(csv, "%s,", user->username);
		printf("%-15s", user->username);
		for (int j = 0; j < set.problems; j++) {
			printf("%-2c", (user->results)[j]);
			fprintf(csv, "%c,", (user->results)[j]);
		}
		printf("%-4d\n", user->sum);
		fprintf(csv, "%d\n", user->sum);
	}
	fclose(csv);
}

void create_directories()
{
	printf("Creating directories...\n");
	int mode = S_IRWXU;
	mkdir("var", mode);
	mkdir("log", mode);
	printf("OK.\n");
}

int main(int argc, char *args[]){
	const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char *contest_name = args[1];
	if (contest_name == NULL) {
		puts("Contest name not found.");
		return 1;
	}
	create_directories();
	Settings set = init_settings(contest_name);
	UserInfo **user_list = get_user_list(contest_name);
	print_settings(set);
	print_list(user_list);
	run_tests(contest_name, user_list, set, alphabet);
	calculate_results(user_list, set);
	generate_results_file(user_list, set, alphabet);
	free_settings(set);
	free_users_info(user_list);
	return 0;
}
