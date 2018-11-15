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

void put_data(Settings *set, char *key, char *value)
{
	if (strcmp(key, "problems") == 0) {
		(*set).problems = atoi(value);
	} else if (strcmp(key, "score") == 0) {
		int len = strlen(value);
		char *score = malloc(sizeof(char) * (len + 1));
		if (score == NULL)
			allocation_error();
		memset(score, 0, len + 1);
		memcpy(score, value, len);
		(*set).score = score;
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

void print_user_list(char **user_list)
{
	if (user_list == NULL) {
		puts("No users found.");
		exit(1);
	}
	for (char **cur = user_list; *cur; cur++)
		puts(*cur);
}

int main(int argc, char *args[])
{
	char *contest_name = args[1];
	if (contest_name == NULL) {
		puts("Contest name not found.");
		return 1;
	}
	Settings set = init_settings(contest_name);
	print_settings(set);
	char **user_list = get_user_list();
	print_user_list(user_list);
	//free_settings();
	//free_user_list();
	return 0;
}
