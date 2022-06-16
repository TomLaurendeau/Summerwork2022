#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <iostream>
#include <fstream>

std::vector<char*> PAPI_EVENTS;

#define PAPI_DIR "/home/jakob/Desktop/Summerwork/papi_avail"
#define SOURCE_DIR "/home/jakob/Desktop/Summerwork/Resource_graphs/"

void print_counters()
{
	int n = 0;
	char buffer_str[130];
	char command_print_buff[200];
	n = sprintf(buffer_str, "%scounters.txt", "/home/jakob/Desktop/Summerwork/");
	n = sprintf(command_print_buff, "%s -a | awk '{print $1}' > %s", PAPI_DIR, buffer_str);
	//int sys_call = system("/home/root/papi/src/utils/papi_avail -a | awk '{print $1}' > /home/root/Resource_graphs/counters.txt");
	int sys_call = system(command_print_buff);
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int i = 0;
	int found_start = 0;
	char*temp;
	printf("START");
	//n = sprintf(buffer_str, "%scounters.txt", SOURCE_DIR);
	fp = fopen(buffer_str, "r");

	if (fp == NULL)
	{
		exit(EXIT_FAILURE);
	}
	while ((read = getline(&line, &len, fp)) != -1)
	{
		printf("%d: %s",i, line);
		temp=(char*)malloc(sizeof(char)*strlen(line));
		temp = strtok(line, "\n");
		PAPI_EVENTS.push_back((char*)malloc(sizeof(char)*strlen(line)));
		strcpy(PAPI_EVENTS[i], temp);
		i++;
    }
}

int main(int ac, char **av) 
{

	printf("START");
	print_counters();
}
