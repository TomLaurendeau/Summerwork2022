#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include </home/jakob/Desktop/Summerwork/files/papi.h>
//#include "/home/root/papi/src/papi.h"
#include <math.h>
#include <vector>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <papiStdEventDefs.h>
#include <papi.h>

//#define PAPI_DIR "/home/jakob/Desktop/Summerwork/papi_avail"
//#define SOURCE_DIR "/home/jakob/Desktop/Summerwork/"

#define SLEEP_TIME 10000 //
#define TASK_ACTIVE_MS 5000
#define CACHE_LOWEST_SIZE 0
#define VALUE_SIZE 400			  //Samples for correlations
#define TARGET_PERCENTAGE 0.01	  //This can be discussed
#define CORRELATION_THRESHOLD 0.5 //Corr
#define SAMPLING_FREQ 1			  //ms, think about this....---???
#define CACHE_LOW_THRESHOLD 0.3
#define CACHE_HIGH_THRESHOLD 0.65
#define LLC_PC_ON 1
#define REPARTITION_LLC_PC 1
//#define LLM_DEPS 1 //IMplementera Rsquared
#define PART_TEST 0
#define SAMPLES 500
#define CUTOFF_STD_FACTOR 0.7
#define WINDOW_SIZE_DEF 100
#define JUNK_CONTAINER_SIZE 80
#define LLM_SHARK_PART 1
#define PROFILING_PART_SIZE 15 

int main(){
    char* argument_exe[] = {"/home/jakob/Desktop/Summerwork/arm_test/Matmult/matmult", "512", "1", "1", NULL}; // !!! warning
    if( -1 == execve(argument_exe[0], argument_exe, NULL))
		{
			perror("child process execve failed [%m]");
            printf("please retry");
			return -1;            
		}
}