#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char *args[])
{
	char *contest_name = args[1];
	if (contest_name == NULL) {
		puts("Contest name not found.");
		return 1;
	}
	Settings set = init_settings(contest_name);
	print_settings(set);
	return 0;
}
