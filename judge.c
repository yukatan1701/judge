#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

typedef struct settings {
	int problems;
	char *score;

} Settings;

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
	printf("problems: %d\n", set.problems);
	printf("score: %s\n", set.score);
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
		(*set).problems = atoi(value);
	} else if (strcmp(key, "score") == 0) {
		(*set).score = copy_string(value);
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
			//putchar(c);
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
		//printf("%s (=) %s\n", key, value);
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
	char filename[] = "global.cfg";
	int len = strlen(contest_name) + strlen(filename) + 2;
	char *path = malloc(len * sizeof(int));
	if (path == NULL)
		allocation_error();
	memset(path, 0, len);
	strcpy(path, contest_name);
	strncat(path, "/", 1);
	strcat(path, filename);
	//puts(path);
	FILE *settings_file = fopen(path, "r");
	free(path);
	if (!settings_file) {
		perror("Failed to open global.cfg");
		exit(1);
	}
	Settings set = read_settings(settings_file);
	fclose(settings_file);
	return set;
}

char **get_user_list()
{
	char **user_list = NULL;
	int len = 0;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("contest/code")) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (strcmp(ent->d_name, "..") != 0 && 
				strcmp(ent->d_name, ".") != 0) {
				user_list = realloc(user_list, (len + 2) * sizeof(char *));
				user_list[len] = ent->d_name;
				user_list[len + 1] = NULL;
				len++;
			}
		}
	} else {
		perror("User codes not found");
		exit(1);
	}
	return user_list;
}

void print_list(char **user_list)
{
	if (user_list == NULL) {
		puts("No users found.");
		exit(1);
	}
	for (int i = 0; user_list[i]; i++)
		puts(user_list[i]);
}

char *generate_file_path(char *contest_name, char *username, char letter)
{
	static const int filename_len = 3;
	static const int slashes_count = 2;
	int user_path_len = strlen(contest_name) + strlen(username) + 
		filename_len + slashes_count;
	char *user_file_path = malloc(sizeof(char) * (user_path_len + 1));
	if (user_file_path == NULL)
		allocation_error();
	memset(user_file_path, 0, user_path_len + 1);
	strcpy(user_file_path, contest_name);
	strcat(user_file_path, "/");
	strcat(user_file_path, username);
	strcat(user_file_path, "/");
	strncat(user_file_path, &letter, 1);
	strcat(user_file_path, ".c");
	return user_file_path;
}

char *generate_output_path(char *contest_name, char letter)
{	
	static const char test_dir[] = "tests";
	static const int slashes_count = 2;
	int tests_path_len = strlen(contest_name) + strlen(test_dir) + slashes_count + 1;
	char *output_path = malloc(sizeof(char) * (tests_path_len + 1));
	if (output_path == NULL)
		allocation_error();
	memset(output_path, 0, tests_path_len + 1);
	strcpy(output_path, contest_name);
	strcat(output_path, "/");
	strcat(output_path, test_dir);
	strcat(output_path, "/");
	strncat(output_path, &letter, 1);
	return output_path;
}

void free_buffer()
{
	char ch;
	while ((ch = getchar()) != EOF)
		;
}

char **run_tests(char *contest_name, char **user_list, Settings set, const char alphabet[])
{
	char **results_list = NULL;
	int user_num = 0;
	for (char **cur_user = user_list; *cur_user; cur_user++, user_num++) {
		char *result = malloc(sizeof(char) * (set.problems + 1));
		if (result == NULL)
			allocation_error();
		memset(result, 0, set.problems + 1);
		for (int i = 0; i < set.problems && i < strlen(alphabet) - 1; i++) {
			char *user_file_path = generate_file_path(contest_name, *cur_user, alphabet[i]);
			char *output_path = generate_output_path(contest_name, alphabet[i]);
			int fd[2];
			pipe(fd);
			if (fork() == 0)
			{
				close(fd[0]);
				dup2(fd[1], 1);
				close(fd[1]);
				char *tmp = malloc(10);
				memset(tmp, 0, 9);
				tmp[0] = (char) i;
				if (execl("test_tmp", tmp, user_file_path, output_path, NULL) < 0) {
					perror("Failed to run tests");
					exit(1);
				}
			}
			close(fd[1]);
			dup2(fd[0], 0);
			close(fd[0]);
			int status = 0;
			wait(&status);
			if (WEXITSTATUS(status) != 0) {
				puts("Tests stopped.");
				exit(1);
			}
			char child_answer = getchar();
			result[i] = child_answer;
			free_buffer();
		}
		results_list = realloc(results_list, (user_num + 2) * sizeof(char *));
		results_list[user_num] = result;
		results_list[user_num + 1] = NULL;
	}
	return results_list;
}

void free_settings(Settings set)
{
	if (set.score)
		free(set.score);
	set.score = NULL;
}

void generate_results_file(char **user_list, char **results_list, Settings set, const char alphabet[])
{
	FILE *csv = fopen("results.csv", "w");
	if (csv == NULL) {
		perror("Failed to open results.csv");
		exit(1);
	}
	fprintf(csv, "user,");
	for (int i = 0; i < set.problems; i++)
		fprintf(csv, "%c,", alphabet[i]);
	fprintf(csv, "Sum\n");
	for (int i = 0; user_list[i]; i++) {
		fprintf(csv, "%s,", user_list[i]);
		for (int j = 0; j < set.problems; j++)
			fprintf(csv, "%c,", results_list[i][j]);
		fprintf(csv, "%d\n", -1);
	}
	fclose(csv);
}

int main(int argc, char *args[]){
	const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char *contest_name = args[1];
	if (contest_name == NULL) {
		puts("Contest name not found.");
		return 1;
	}
	Settings set = init_settings(contest_name);
	print_settings(set);
	char **user_list = get_user_list();
	print_list(user_list);
	char **results_list = run_tests(contest_name, user_list, set, alphabet);
	print_list(results_list);
	generate_results_file(user_list, results_list, set, alphabet);
	free_settings(set);
	free(user_list);
	//TODO: free_results_list
	return 0;
}
