
/** compile with -std=gnu99 */

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
#include "/home/jakob/Desktop/papi/src/papi.h"
//#include "/home/root/papi/src/papi.h"
#include <math.h>
#include <vector>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#define USE_ARM 1
#define QUEUE_NAME "/performance_counters"
#define PAPI_DIR "/home/jakob/Desktop/papi/src/utils/papi_avail"
#define SOURCE_DIR "/home/jakob/Desktop/Resource_graphs/"
//#define PAPI_DIR "/home/root/papi/src/utils/papi_avail"
//#define SOURCE_DIR "/home/root/Resource_graphs/"
#define SLEEP_TIME 10000 //
#define TASK_ACTIVE_MS 5000

#define VALUE_SIZE 400 //Samples for correlations
#define TARGET_PERCENTAGE 0.01 //This can be discussed
#define CORRELATION_THRESHOLD 0.5 //Corr
#define SAMPLING_FREQ 1 //ms, think about this....---???
#define CACHE_LOW_THRESHOLD 0.3
#define CACHE_HIGH_THRESHOLD 0.65
#define LLC_PC_ON 1
#define REPARTITION_LLC_PC 0
#define LLM_DEPS 1 //IMplementera Rsquared
#define PART_TEST 0
#define SAMPLES 500
#define CUTOFF_STD_FACTOR 0.7
#define WINDOW_SIZE_DEF 100
#define JUNK_CONTAINER_SIZE 80
#define LLM_SHARK_PART 10
#define PROFILING_PART_SIZE 1000
int argument_index = 3;
std::vector<char*> PAPI_EVENTS;
std::vector<int> selected_rows_idx;
std::vector<char*> argument_list;
int x = 0;
int y = 0;
int x2 = 0;
//int EventSet[20];
int argument_count;
int evset1;
int evset2;
int events_set[20];
int Partitions = 8192;
int tasks_in_system;
int mon_pid = 0;
//1. Benchmarks för hur en partition påverkar
//2. Vision algoritm
//3. Coloris jämförelse
//4. "Jailtime" - implementera tidsstämpel innan nästa cache partition kan ske
//sint EventSet = PAPI_NULL;
//CBS - constant bandwidth server
//Jämföra mot Complete cache sharing

static double arithmetic_mean(double* data, int size);
static double mean_of_products(double* data1, double* data2, int size);
static double standard_deviation(double* data, int size);

typedef struct perf_event_counter{
	char * name;
	double correlation;
	double max;
	double min;
	double average;
}PMC;
typedef struct resource_boundness{
	char * name;
	std::vector<PMC> performance_counter;

} profiled_tasks;
profiled_tasks tasks[100];

typedef struct phase {
	int number;
	int correlation;
}phases_def;

typedef struct execution_counter {
	char*name;
	int number_of_phases;
	double median_correlation;
	std::vector<phases_def> phases;
}ev_counter_def;

typedef struct cache_partition_struct {
	int size;
	int start;
	int end;
	int cpu;
}cache_partition;

typedef struct characteristics {
	char * fp;
	char * name;
	int counters;
	int pid;
	double instr_hist[WINDOW_SIZE_DEF];
	double execution_time;
	double current_performance;
	double diff;
	double desired_performance;
	int frequency;
	const char *app_arg[64] = {""};
	std::vector<std::vector<double>> event_counter_sets;
	std::vector<char*> event_name;
	std::vector<double> total_correlations;
	std::vector<std::vector<double>> segment_correlations;
	cache_partition partition;
	double baseline_correlation[16];
	double std_instr;
	double median_instr;

	std::vector<int> break_point_1;
	std::vector<int> break_point_2;

} task_characteristic;





typedef struct task_char {
	double performance[VALUE_SIZE];
	double test_avg;
	double performance_counter[VALUE_SIZE];
	int pid;
	int target_performance;
	int number_of_samples;
	int run;
	double correlation;
	double prev_correlation;
	double previous_performance;
	double avg_performance;
	double avg_cache_miss;
	double execution_time;
	int partition_interval;
	int reached_target;
	int partition_samples;
	int completed_correlations;

	double alive_time;
	double performance_per_cache_miss;
	double cache_miss_contribution;
	double partitioned_history_data[8000]; //HISTORIA för allt data, alltid partitionera en för mycket
	double partitions[8000];
	int peak_performance;
} task_characteristics;


typedef struct partition {
	char * name;
	int start;
	int end;
	int cpu;
	double partitioned_timestamp;
	int cpu_budget;
	task_characteristics app;
} task_partition;

task_partition partitions[100];
task_characteristics applications[100];


double pearson_correlation(double* independent, double* dependent, int size)
{
    double rho;

    // covariance
    double independent_mean = arithmetic_mean(independent, size);
    double dependent_mean = arithmetic_mean(dependent, size);
    double products_mean = mean_of_products(independent, dependent, size);
    double covariance = products_mean - (independent_mean * dependent_mean);

    // standard deviations of independent values
    double independent_standard_deviation = standard_deviation(independent, size);

    // standard deviations of dependent values
    double dependent_standard_deviation = standard_deviation(dependent, size);

    // Pearson Correlation Coefficient
    rho = covariance / (independent_standard_deviation * dependent_standard_deviation);

    return rho;
}

//--------------------------------------------------------
// FUNCTION arithmetic_mean
//--------------------------------------------------------
static double arithmetic_mean(double* data, int size)
{
    double total = 0;

    // note that incrementing total is done within the for loop
    for(int i = 0; i < size; total += data[i], i++);

    return total / size;
}

//--------------------------------------------------------
// FUNCTION mean_of_products
//--------------------------------------------------------
static double mean_of_products(double* data1, double* data2, int size)
{
    double total = 0;

    // note that incrementing total is done within the for loop
    for(int i = 0; i < size; total += (data1[i] * data2[i]), i++);

    return total / size;
}

//--------------------------------------------------------
// FUNCTION standard_deviation
//--------------------------------------------------------
static double standard_deviation(double* data, int size)
{
    double squares[size];

    for(int i = 0; i < size; i++)
    {
        squares[i] = pow(data[i], 2);
    }

    double mean_of_squares = arithmetic_mean(squares, size);
    double mean = arithmetic_mean(data, size);
    double square_of_mean = pow(mean, 2);
    double variance = mean_of_squares - square_of_mean;
    double std_dev = sqrt(variance);

    return std_dev;
}


void initialize_partitions() {

}
int cache_counters[100];

/* Simple loop body to keep things interested. Make sure it gets inlined. */
int stick_this_thread_to_core(int core_id) {
	int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	if (core_id < 0 || core_id >= num_cores) {
		printf("Core out of index");
		return 0;
	}

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core_id, &cpuset);

	pthread_t current_thread = pthread_self();
	return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

static inline int loop(int* __restrict__ a, int* __restrict__ b, int n) {
	unsigned sum = 0;
	for (int i = 0; i < n; ++i)
		if (a[i] > b[i])
			sum += a[i] + 5;
	return sum;
}
static inline long long unsigned time_ns(struct timespec* const ts) {
	/* We use monotonic to avoid any leap-problems generated by NTP. */
	if (clock_gettime(CLOCK_MONOTONIC, ts)) {
		exit(1);
	}
	return ((long long unsigned) ts->tv_sec) * 1000000000LLU
			+ (long long unsigned) ts->tv_nsec;
}

ssize_t format_timeval(struct timeval *tv, char *buf, size_t sz) {
	ssize_t written = -1;
	struct tm *gm = gmtime(&tv->tv_sec);

	if (gm) {
		written = (ssize_t) strftime(buf, sz, "%Y-%m-%d.%H:%M:%S", gm);
		if ((written > 0) && ((size_t) written < sz)) {
			int w = snprintf(buf + written, sz - (size_t) written, ".%06d",
					(int) tv->tv_usec);
			written = (w > 0) ? written + w : -1;
		}
	}
	return written;
}

int temp = 0;
void unmount_fs() {
	char buffer_str[80];
	int n;
	int status;
	n = sprintf(buffer_str, "umount cache_fs");
	status = system(buffer_str);
	n = sprintf(buffer_str, "umount cpu_fs");
	status = system(buffer_str);
	n = sprintf(buffer_str, "umount cpuset_fs");
	status = system(buffer_str);
	//Hur använd budget
}



void create_new_partition_dir(char * part_name, int start, int end) {
	char buffer_str[80];
	int n;
	int status;
	n = sprintf(buffer_str,
			"mkdir /home/jakob/Desktop/cgroups_dir/cache_partition/%s",
			part_name);
	status = system(buffer_str);

	n = sprintf(buffer_str,
			"mkdir /home/jakob/Desktop/cgroups_dir/cpu_partition/%s",
			part_name);
	status = system(buffer_str);

	n = sprintf(buffer_str,
				"mkdir /home/jakob/Desktop/cgroups_dir/cpuset_partition/%s",
				part_name);
		status = system(buffer_str);
	n =
			sprintf(buffer_str,
					"echo %d-%d > /home/jakob/Desktop/cgroups_dir/cache_partition/%s/palloc.bins",
					start, end, part_name);
	status = system(buffer_str);
	n =
				sprintf(buffer_str,"echo 0 > /home/jakob/Desktop/cgroups_dir/cpuset_partition/%s/cpuset.mems", part_name);
		status = system(buffer_str);
}
void alloc_initial_partitions(std::vector<std::vector<task_characteristic>> input_tasks)
{
	//--------------------------------LLM_SHARK partition segment-----------------------
	char buffer_str[80];
	int n;
	int status;
	int monitor_pid = syscall(SYS_gettid);
	n = sprintf(buffer_str,"mkdir /sys/fs/cgroup/tool_part");
	status = system(buffer_str);
	//printf("Made fs");
	//fflush(stdout);
	n = sprintf(buffer_str,"echo 0 > /sys/fs/cgroup/tool_part/cpuset.cpus");
	status = system(buffer_str);
	//printf("Allocated cpuset");
	//fflush(stdout);
	n = sprintf(buffer_str,"echo 0 > /sys/fs/cgroup/tool_part/cpuset.mems");
	status = system(buffer_str);
	//printf("Allocated mems");*/
	//fflush(stdout);
	n = sprintf(buffer_str,"echo 0-10 > /sys/fs/cgroup/tool_part/palloc.bins");
	status = system(buffer_str);
	//printf("Allocated bins");
	//fflush(stdout\n);
	n = sprintf(buffer_str,"echo %d > /sys/fs/cgroup/tool_part/tasks", monitor_pid);
	status = system(buffer_str);
	//printf("Allocated pid");
	//fflush(stdout);
	//--------------------------------LLM_SHARK partition segment-----------------------
	//--------------------------------Creation of the junk container-----------------------
	n = sprintf(buffer_str,"mkdir /sys/fs/cgroup/Junk");
	status = system(buffer_str);
	n = sprintf(buffer_str,"echo 0-3 > /sys/fs/cgroup/Junk/cpuset.cpus");
	status = system(buffer_str);
	n = sprintf(buffer_str,"echo 0 > /sys/fs/cgroup/Junk/cpuset.mems");
	status = system(buffer_str);
	n = sprintf(buffer_str,"echo 11-90 > /sys/fs/cgroup/Junk/palloc.bins"); //Start from 11, since the tool allocates 0-10 already
	status = system(buffer_str);
	//--------------------------------Creation of the junk container-----------------------
	for (unsigned int i = 0; i < input_tasks.size();i++)
	{
		if (input_tasks[i][0].partition.size != 0) //Dont create a folder for applications that belongs to junk
		{
			if (i == 0) //Handle the special case for when the first task is not a part of the junk container
			{
				input_tasks[i][0].partition.start = 90;
				input_tasks[i][0].partition.end = input_tasks[i][0].partition.start+input_tasks[i][0].partition.size;
			}
			else
			{
				input_tasks[i][0].partition.start = input_tasks[i-1][0].partition.end+1;
				input_tasks[i][0].partition.end = input_tasks[i][0].partition.start+input_tasks[i][0].partition.size;
			}
			//---------The actual directory setup
			n = sprintf(buffer_str,"mkdir /sys/fs/cgroup/%s", input_tasks[i][0].name);
			status = system(buffer_str);
			n = sprintf(buffer_str,"echo %d > /sys/fs/cgroup/%s/cpuset.cpus",input_tasks[i][0].partition.cpu, input_tasks[i][0].name);
			status = system(buffer_str);
			n = sprintf(buffer_str,"echo 0 > /sys/fs/cgroup/%s/cpuset.mems", input_tasks[i][0].name);
			status = system(buffer_str);
			n = sprintf(buffer_str,"echo %d-%d > /sys/fs/cgroup/%s/palloc.bins", input_tasks[i][0].partition.start, input_tasks[i][0].partition.end, input_tasks[i][0].name);
			status = system(buffer_str);
			//---------The actual directory setup
			printf("Task: %s\t partition size: %d\t start: %d\t - end:%d \n", input_tasks[i][0].name, input_tasks[i][0].partition.size, input_tasks[i][0].partition.start, input_tasks[i][0].partition.end);
		}
		else
		{
			input_tasks[i][0].partition.start = 11;
			input_tasks[i][0].partition.end = 90;
			input_tasks[i][0].partition.size = 80;
			printf("Task: %s\t partition size: %d\t start: %d\t - end:%d \n", input_tasks[i][0].name, input_tasks[i][0].partition.size, input_tasks[i][0].partition.start, input_tasks[i][0].partition.end);
		}
	}

}

void alloc_prog_partition(int pid)
{
	char buffer_str[80];
	int n;
	int status;
	int monitor_pid = syscall(SYS_gettid);
	n = sprintf(buffer_str,"mkdir /sys/fs/cgroup/profiling_part");
	status = system(buffer_str);
	n = sprintf(buffer_str,"echo 1 > /sys/fs/cgroup/profiling_part/cpuset.cpus");
	status = system(buffer_str);
	n = sprintf(buffer_str,"echo 0 > /sys/fs/cgroup/profiling_part/cpuset.mems");
	status = system(buffer_str);
	n = sprintf(buffer_str,"echo 11-%d > /sys/fs/cgroup/profiling_part/palloc.bins", PROFILING_PART_SIZE);
	status = system(buffer_str);
	n = sprintf(buffer_str,"echo %d > /sys/fs/cgroup/profiling_part/tasks", pid);
	status = system(buffer_str);
}
void setup_partitions() {
	int status;
	FILE * fp;
	char buffer_str[90];
	int n;
	char path[1];
	unmount_fs();
	printf("Setting up initial partitions");
	status = system("echo 0x0007F000 > /sys/kernel/debug/palloc/palloc_mask");
	status = system("echo never > /sys/kernel/mm/transparent_hugepage/enabled");
	status = system("echo 1 > /sys/kernel/debug/palloc/debug_level");
	n =
			sprintf(buffer_str,
					"mount -t cgroup -o palloc cache_fs /home/jakob/Desktop/cgroups_dir/cache_partition/");
	status = system(buffer_str);
	/*n =
			sprintf(buffer_str,
					"mount -t cgroup -o cpu,cpuacct cpu_fs /home/jakob/Desktop/cgroups_dir/cpu_partition/");*/
	status = system(buffer_str);
	n =
				sprintf(buffer_str,
						"mount -t cgroup -o cpuset cpuset_fs /home/jakob/Desktop/cgroups_dir/cpuset_partition/");
	status = system(buffer_str);
	partitions[0].name = "partition1";
	partitions[0].start = 0;
	partitions[0].end = 1;
	partitions[0].cpu = 1;
	partitions[1].name = "partition2";
	partitions[1].start = 100;
	partitions[1].end = 102;
	partitions[1].cpu = 2;
	partitions[2].name = "partition3";
	partitions[2].start = 103;
	partitions[2].end = 104;
	partitions[2].cpu = 2;
	partitions[3].name = "Controller";
	partitions[3].start = 120;
	partitions[3].end = 127;
	partitions[3].cpu = 3;
	status = system("echo 4 > /sys/kernel/debug/palloc/alloc_balance");
	status = system("echo 1 > /sys/kernel/debug/palloc/use_palloc");
	create_new_partition_dir(partitions[0].name, partitions[0].start,
			partitions[0].end);
	create_new_partition_dir(partitions[1].name, partitions[1].start,
			partitions[1].end);
	create_new_partition_dir(partitions[3].name, partitions[3].start,
			partitions[3].end);
	//create_new_partition_dir(partitions[1].name, partitions[1].start, partitions[1].end);
	//create_new_partition_dir(partitions[2].name, partitions[2].start, partitions[2].end);
	//create_new_partition_dir(partitions[3].name, partitions[3].start, partitions[3].end);

}
void bind_task_to_cache_partition(int pid, char*part_name) {
	char pal_string[80];
	char cpu_string[80];
	int n;
	int status = 0;
	//int status = system("echo 0 > /sys/kernel/debug/palloc/use_palloc");
	n =
			sprintf(pal_string,
					"echo %d > /home/jakob/Desktop/cgroups_dir/cache_partition/%s/tasks",
					pid, part_name);
	//n = sprintf(cpu_string, "echo %d > /sys/fs/cgroup/cpuset/%s/tasks", pid, part_name);
	status = system(pal_string);
	n = sprintf(cpu_string,
			"echo %d > /home/jakob/Desktop/cgroups_dir/cpu_partition/%s/tasks",
			pid, part_name);
	status = system(cpu_string);
	n = sprintf(cpu_string,
				"echo %d > /home/jakob/Desktop/cgroups_dir/cpuset_partition/%s/tasks",
				pid, part_name);
		status = system(cpu_string);
	//status = system("echo 1 > /sys/kernel/debug/palloc/use_palloc");
}

void resize_partition(task_partition partition) {
	char buffer_str[90];
	int status;
	int n =
			sprintf(buffer_str,
					"echo %d-%d > /home/jakob/Desktop/cgroups_dir/cache_partition/%s/palloc.bins",
					partition.start, partition.end, partition.name);
	status = system(buffer_str);
}
double avg_exec(double * input) {
	double mean = (input[VALUE_SIZE/2] + input[VALUE_SIZE/2+1])/2;
	return mean;

	/*
	for (int i = 0; i < VALUE_SIZE; i++) {
		mean = mean + input[i];
	}
	mean = mean / VALUE_SIZE;
	return mean;*/
}

void init_papi_stuff1(int monitor_pid) {

	int retval = 0;
	PAPI_option_t opt;
	events_set[tasks_in_system] = PAPI_NULL;
	double ver = PAPI_VER_CURRENT;

	if ((retval = PAPI_create_eventset(&events_set[tasks_in_system])) != PAPI_OK) {
		printf("ERROR: PAPI_create_eventset %d: %s\n", retval,
				PAPI_strerror(retval));
		exit(-1);
	}
	printf("PAPI_assign_eventset_component()\n");
	if ((retval = PAPI_assign_eventset_component(events_set[tasks_in_system], 0))
			!= PAPI_OK) {
		printf("ERROR: PAPI_assign_eventset_component %d: %s\n", retval,
				PAPI_strerror(retval));
		exit(-1);
	}
	memset(&opt, 0x0, sizeof(PAPI_option_t));
	opt.inherit.inherit = PAPI_INHERIT_ALL;
	opt.inherit.eventset = events_set[tasks_in_system];
	/*if (PAPI_set_multiplex(ev_set) != PAPI_OK) //Multiplexing does not work well
	    {
	    	printf("ERROR: papi enable multiplex failed %d: %s\n", retval,
	    	    			PAPI_strerror(retval));
	    	    	exit(-1);
	    }*/
	if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK)
	{ //Most important, the papi inheritance!!!!
		printf("PAPI_set_opt error %d: %s0", retval, PAPI_strerror(retval));
		exit(-1);
	}
	if ((retval = PAPI_add_event(events_set[tasks_in_system], PAPI_TOT_INS))
			!= PAPI_OK) {
		printf("failed to attach L2 misses");
	}
	if ((retval = PAPI_add_event(events_set[tasks_in_system], PAPI_L3_DCA/*PAPI_L3_TCM*/))
			!= PAPI_OK) {
		printf("failed to attach L2 misses");
	}
	if ((retval = PAPI_attach(events_set[tasks_in_system], monitor_pid))
			!= PAPI_OK) {
		printf("PAPI_attach error %d: %s0", retval, PAPI_strerror(retval));
		exit(-1);
	}
	if ((retval = PAPI_start(events_set[tasks_in_system])) != PAPI_OK) {
		printf("PAPI_start error %d: %s0\n", retval, PAPI_strerror(retval));
		exit(-1);
	}
	if (PAPI_reset(events_set[tasks_in_system]) != PAPI_OK) {
		printf("PAPI_reset error %d: %s0\n", retval, PAPI_strerror(retval));
		exit(-1);
	}
}
int sample_count = 0;


static int time_stamp_prog(const char ** argv)
{
    pid_t   my_pid;
    int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;
    char * first = argv[0];
    char * second = argv[1];
    int i = 0;
    int kiddo = 0;
    int kiddo_fork = 0;
    int temp_test = syscall(SYS_gettid);
    printf("Head thread: %d\n", temp_test);
    fflush(stdout);
    if (0 == (my_pid = fork()))
    {

    		i++;

    		fflush(stdout);
    		if (i==1)
    		{

    		}
		kiddo = syscall(SYS_gettid);
		kiddo_fork = my_pid;
		printf("Kid fork id: %d\n", kiddo_fork);
		fflush(stdout);
		printf("Kid thread: %d\n", kiddo);
		fflush(stdout);
	    //alloc_prog_partition(kiddo);
            if (-1 == execve(argv[0], (char **)argv , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;
            }
    }

    while (0 == waitpid(my_pid , &status , WNOHANG)) {

    }
    //printf("%s WEXITSTATUS %d WIFEXITED %d [status %d]\n",
   //         argv[0], WEXITSTATUS(status), WIFEXITED(status), status);

    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
            perror("%s failed, halt system");
            return -1;
    }

    return 0;
}
void pthread_exec_prog()
{

}

std::vector<double> measure_resource_usage(const char **argv, int event)
{
    pid_t   my_pid;
    int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;
    int retval = 0;
    long long values[7];
    PAPI_option_t opt;
    int ev_set = PAPI_NULL;
    int start_prog = 0;
    int measurements=0;
    double ver = PAPI_VER_CURRENT;
	char event_name[20];
    std::vector<double> counter;

    if (retval = PAPI_create_eventset(&ev_set) != PAPI_OK)
    {
    	printf("ERROR: PAPI_create_eventset %d: %s\n", retval,
    			PAPI_strerror(retval));
    	exit(-1);
    }

    printf("PAPI_assign_eventset_component()\n");
    if (retval = PAPI_assign_eventset_component((ev_set), 0) != PAPI_OK)
    {
    	printf("ERROR: PAPI_assign_eventset_component %d: %s\n", retval,
    			PAPI_strerror(retval));
    	exit(-1);
    }
    memset(&opt, 0x0, sizeof(PAPI_option_t));
    opt.inherit.inherit = PAPI_INHERIT_ALL;
    opt.inherit.eventset = ev_set;
    if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK)
    { //Most important, the papi inheritance!!!!
    	printf("PAPI_set_opt error %d: %s0", retval, PAPI_strerror(retval));
    	exit(-1);
    }
    if ((retval = PAPI_add_event(ev_set, event))!= PAPI_OK) //PAPI_TOT_INS
    {
    	printf("failed to attach instructions retired");
    	printf("Wrapper create: %s\n", strerror(errno));
    }


    int i = 0;
    if (0 == (my_pid = fork()))
    {
    		i++;
    		mon_pid = syscall(SYS_gettid);
    		//alloc_prog_partition(mon_pid);
    		fflush(stdout);

    		if (i==1)
    		{
    			//printf("Child pid is: %d\n", mon_pid);
    		}
            if (-1 == execve(argv[0], (char **)argv , NULL)) {
                    perror("child process execve failed [%m]");
                    exit(-1);

            }
    }
    if ((retval = PAPI_attach(ev_set, my_pid)) != PAPI_OK)
    {
        printf("Monitor pid is: %d\n", my_pid);
        printf("Papi error: %s\n", strerror(errno));
        exit(-1);
    }
    if ((retval = PAPI_start(ev_set)) != PAPI_OK)
    {
        printf("PAPI_start error %d: %s0\n", retval, PAPI_strerror(retval));
        exit(-1);
    }
    if (PAPI_reset(ev_set) != PAPI_OK)
    {
        printf("PAPI_reset error %d: %s0\n", retval, PAPI_strerror(retval));
        exit(-1);
    }

    while (0 == waitpid(my_pid , &status , WNOHANG))
    {
    	if ((retval = PAPI_read(ev_set, values)) != PAPI_OK)
    	{
    		printf("Failed to read the events");
    	}
    	PAPI_reset(ev_set);
    	counter.push_back(values[0]);

    	/*if (event_1 == PAPI_L1_TCM)
    	{
    		(*input_task).instr_ret.push_back(values[0]);
    		(*input_task).L1D_cache.push_back(values[1]);
    		(*input_task).L2D_cache.push_back(values[2]);
    		(*input_task).L3_cache.push_back(values[3]);
    	}
    	if (event_1 == PAPI_TLB_DM)
    	{
    		(*input_task).instr_ret2.push_back(values[0]);
    		(*input_task).TLB.push_back(values[1]);
    		(*input_task).BMSP.push_back(values[2]);
    		(*input_task).FLOPS.push_back(values[3]);
    	}*/

    	//printf("%llu\t", values[0]);
    	//fflush(stdout);
    	usleep(10000);


    }
    //printf("%s WEXITSTATUS %d WIFEXITED %d [status %d]\n",
   //         argv[0], WEXITSTATUS(status), WIFEXITED(status), status);

    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
            perror("%s failed, halt system");
            exit(-1);
    }
    PAPI_reset(ev_set);
    PAPI_stop(ev_set, values);
    return counter;
}
static int characterize_program(const char **argv, task_characteristic * input_task, int event_1, int event_2, int event_3)
{
    pid_t   my_pid;
    int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;
    char buffer_str[130];
    int retval = 0;
    long long values[7];
    PAPI_option_t opt;
    int ev_set = PAPI_NULL;
    int start_prog = 0;
    int n = 0;
    int measurements=0;
    double ver = PAPI_VER_CURRENT;
	char event_name[20];
    std::vector<double> instr_ret;
    std::vector<double> counter_1;
    std::vector<double> counter_2;
    std::vector<double> counter_3;

    long long unsigned timestamp_start;
    long long unsigned timestamp_end;
    struct timespec ts;




    if (retval = PAPI_create_eventset(&ev_set) != PAPI_OK)
    {
    	printf("ERROR: PAPI_create_eventset %d: %s\n", retval,
    			PAPI_strerror(retval));

    	exit(-1);
    }

    printf("PAPI_assign_eventset_component()\n");
    if (retval = PAPI_assign_eventset_component((ev_set), 0) != PAPI_OK)
    {
    	printf("ERROR: PAPI_assign_eventset_component %d: %s\n", retval,
    			PAPI_strerror(retval));
    	exit(-1);
    }
    memset(&opt, 0x0, sizeof(PAPI_option_t));
    opt.inherit.inherit = PAPI_INHERIT_ALL;
    opt.inherit.eventset = ev_set;
    if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK)
    { //Most important, the papi inheritance!!!!
    	printf("PAPI_set_opt error %d: %s0", retval, PAPI_strerror(retval));
    	exit(-1);
    }
    if ((retval = PAPI_add_event(ev_set, PAPI_TOT_INS))!= PAPI_OK) //PAPI_TOT_INS
    {
    	printf("failed to attach instructions retired");
    	printf("Wrapper create: %s\n", strerror(errno));
    }
    if ((retval = PAPI_add_event(ev_set, event_1))!= PAPI_OK)
    {
    	printf("failed to attach L1 misses");
    	printf("Wrapper create: %s\n", strerror(errno));
    }
    if ((retval = PAPI_add_event(ev_set, event_2))!= PAPI_OK)
    {
    	printf("failed to attach L2 misses");
    	printf("Wrapper create: %s\n", strerror(errno));
    }
    if ((retval = PAPI_add_event(ev_set, event_3))!= PAPI_OK)
    {
    	printf("failed to attach L3 misses");
    	printf("Wrapper create: %s\n", strerror(errno));
    }

    timestamp_start = (double)time_ns(&ts)*0.000001;
    int i = 0;
    if (0 == (my_pid = fork()))
    {

    		i++;
    		mon_pid = syscall(SYS_gettid);
    		fflush(stdout);

    		if (i==1)
    		{
    			//printf("Child pid is: %d\n", mon_pid);
    		}
            if (-1 == execve(argv[0], (char **)argv , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;

            }
    }
    if ((retval = PAPI_attach(ev_set, my_pid)) != PAPI_OK)
    {
        printf("Monitor pid is: %d\n", my_pid);
        printf("Papi error: %s\n", strerror(errno));
        exit(-1);
    }
    if ((retval = PAPI_start(ev_set)) != PAPI_OK)
    {
        printf("PAPI_start error %d: %s0\n", retval, PAPI_strerror(retval));
        exit(-1);
    }
    if (PAPI_reset(ev_set) != PAPI_OK)
    {
        printf("PAPI_reset error %d: %s0\n", retval, PAPI_strerror(retval));
        exit(-1);
    }

    while (0 == waitpid(my_pid , &status , WNOHANG))
    {
    	if ((retval = PAPI_read(ev_set, values)) != PAPI_OK)
    	{
    		printf("Failed to read the events");
    	}

    	PAPI_reset(ev_set);
    	instr_ret.push_back(values[0]);
    	counter_1.push_back(values[1]);
    	counter_2.push_back(values[2]);
    	counter_3.push_back(values[3]);

    	/*if (event_1 == PAPI_L1_TCM)
    	{
    		(*input_task).instr_ret.push_back(values[0]);
    		(*input_task).L1D_cache.push_back(values[1]);
    		(*input_task).L2D_cache.push_back(values[2]);
    		(*input_task).L3_cache.push_back(values[3]);
    	}
    	if (event_1 == PAPI_TLB_DM)
    	{

    		(*input_task).instr_ret2.push_back(values[0]);
    		(*input_task).TLB.push_back(values[1]);
    		(*input_task).BMSP.push_back(values[2]);
    		(*input_task).FLOPS.push_back(values[3]);
    	}*/

    	//printf("%llu\t", values[0]);
    	//fflush(stdout);
    	//usleep((*input_task).frequency);
    	timestamp_end = (double)time_ns(&ts)*0.000001;

    	double test = timestamp_end - timestamp_start;
    	if (test > TASK_ACTIVE_MS)
    	{
    		kill(my_pid, SIGKILL);
    		printf("Killed process");
    		PAPI_reset(ev_set);
    		PAPI_stop(ev_set, values);
    	}


    	usleep(SLEEP_TIME);

    }
    (*input_task).event_counter_sets.push_back(instr_ret);
    (*input_task).event_counter_sets.push_back(counter_1);
    (*input_task).event_counter_sets.push_back(counter_2);
    (*input_task).event_counter_sets.push_back(counter_3);
    //printf("%s WEXITSTATUS %d WIFEXITED %d [status %d]\n",
   //         argv[0], WEXITSTATUS(status), WIFEXITED(status), status);

    (*input_task).event_name.push_back((char*)malloc(sizeof(char)*strlen("PAPI_TOT_INS")));
        strcpy((*input_task).event_name[0], "PAPI_TOT_INS");

        PAPI_event_code_to_name(event_1, event_name);
        (*input_task).event_name.push_back((char*)malloc(sizeof(char)*strlen(event_name)));
        strcpy((*input_task).event_name[1], event_name);

        PAPI_event_code_to_name(event_2, event_name);
        (*input_task).event_name.push_back((char*)malloc(sizeof(char)*strlen(event_name)));
        strcpy((*input_task).event_name[2], event_name);

        PAPI_event_code_to_name(event_3, event_name);
        (*input_task).event_name.push_back((char*)malloc(sizeof(char)*strlen(event_name)));
        strcpy((*input_task).event_name[3], event_name);

    /*if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
            perror("%s failed, halt system");
            return -1;
    }*/
    PAPI_reset(ev_set);
    PAPI_stop(ev_set, values);
    return 0;
}

double calculateSD(std::vector<double> & data, int length) {
	double sum = 0.0, mean, SD = 0.0;
	double SD_ret;
	double z_score;
	double temp;
	int remove_index = 0;
    int i;
    for (i = 0; i < length; ++i) {
        sum += data[i];
    }
    mean = sum / length;
    for (i = 0; i < length; ++i)
        SD += pow(data[i] - mean, 2);
    SD_ret = sqrt(SD/length);
    data.erase(data.begin());
    /*for (i = 0; i < length; i++)
    {
    	temp = data[i];
    	z_score = (temp - mean)/SD_ret;
    	if (abs(z_score) > 1.65)
    	{
    		data.erase(data.begin()+remove_index);
    		remove_index--;
    		printf("Z score deviation at index: %d\n", i);
    	}
    	remove_index++;
    }*/
    return SD_ret;
}
void filter_warmup(task_characteristic * input)
{
	/*(*input).FLOPS.erase((*input).FLOPS.begin(), (*input).FLOPS.begin()+5);
	(*input).L1D_cache.erase((*input).L1D_cache.begin(), (*input).L1D_cache.begin()+5);
	(*input).L2D_cache.erase((*input).L2D_cache.begin(), (*input).L2D_cache.begin()+5);
	(*input).L3_cache.erase((*input).L3_cache.begin(), (*input).L3_cache.begin()+5);
	(*input).TLB.erase((*input).TLB.begin(), (*input).TLB.begin()+5);
	(*input).instr_ret.erase((*input).instr_ret.begin(), (*input).instr_ret.begin()+5);
	(*input).instr_ret2.erase((*input).instr_ret2.begin(), (*input).instr_ret2.begin()+5);
	(*input).BMSP.erase((*input).BMSP.begin(), (*input).BMSP.begin()+5);*/
}
void correlate(std::vector<int> indexes, task_characteristic * input, char * file_path_input) //fix later
{
	double tot = 0.0;
	double correlation = 0;
	int n;
	int status;
	std::ofstream myfile;
	std::ofstream myfile_2;
	std::ofstream myfile_3;

	int max_perf = (*input).event_counter_sets[0][0];
	int min_perf = (*input).event_counter_sets[0][0];
	char buffer_str[130];


	for (int j = 0; j < input->event_counter_sets[0].size(); j++)
	{
		if (input->event_counter_sets[0][j] > max_perf)
		{
			max_perf = input->event_counter_sets[0][j];
		}
		if (input->event_counter_sets[0][j]<min_perf)
		{
			min_perf = input->event_counter_sets[0][j];
		}
	}

	for (int i = 1; i < input->counters;i++)
	{
		int max_counter = input->event_counter_sets[i][0];
		int min_counter = input->event_counter_sets[i][0];
		int line = 0;
		n = sprintf(buffer_str, "%sdata.txt", SOURCE_DIR);
		myfile.open(buffer_str); //SOURCE DIR????
		n = sprintf(buffer_str, "%ssegments.txt", SOURCE_DIR);
		myfile_2.open(buffer_str);
		n = sprintf(buffer_str, "%sgraph_boundaries.txt", SOURCE_DIR);
		myfile_3.open(buffer_str);


		for (int j = 0; j < input->event_counter_sets[i].size(); j++)
		{
			 if (input->event_counter_sets[i][j] > max_counter)
			 {
				 max_counter = input->event_counter_sets[i][j];
			 }
			 if (input->event_counter_sets[i][j] < min_counter)
			 {
				 min_counter = input->event_counter_sets[i][j];
			 }
		}
		max_perf = max_perf+(max_perf*0.1);
		min_perf = min_perf*0.9;
		max_counter = max_counter+(max_counter*0.1);
		max_counter = max_counter*0.9;
		line = max_perf;
		myfile_3 << std::fixed << max_perf << '\n' << min_perf << '\n' << max_counter << '\n' << min_counter << '\n' << input->event_name[i] << '\n';
		printf("\n");
		for (int j = 0; j < input->event_counter_sets[0].size(); j ++)
		{
			//printf("%d\t%.0f\t%.0f\n", i, performance[i], counter[i]);
			myfile << std::fixed << j << '\t' << input->event_counter_sets[0][j] << '\t' << input->event_counter_sets[i][j] << '\n';

		}
		for (int j = 0; j < indexes.size()-1;j++)
		{

				//printf("%d\t%.0f\t%.0f\n", i, performance[i], counter[i]);
				myfile_2 << std::fixed << indexes[j] << '\t' << 0 << '\t' << indexes [j] << '\t' << line <<'\n';
				std::vector<double> perf_seg (&input->event_counter_sets[0][indexes[j]], &input->event_counter_sets[0][indexes[j+1]]);
				std::vector<double> count_seg (&input->event_counter_sets[i][indexes[j]], &input->event_counter_sets[i][indexes[j+1]]);
				correlation = pearson_correlation (&perf_seg[0], &count_seg[0], perf_seg.size());

				tot += correlation;

				//printf("Segment %d Correlation %f\n", i, correlation);
		}
		myfile_3.close();
		myfile_2.close();
		myfile.close();
		sleep(2);
		//char buffer_str[130];

		/*n = sprintf(buffer_str,
						"/home/jakob/Desktop/Resource_graphs/./commands.sh %s%s/%s",
						SOURCE_DIR,file_path_input,input->event_name[i]);*/
			/*n = sprintf(buffer_str,
							"/home/jakob/Desktop/Resource_graphs/./commands.sh /home/jakob/Desktop/Resource_graphs/test.pdf");*/
		//status = system(buffer_str);
		double avg = tot/indexes.size();
		printf("Performance counter %s average correlation %f\n",input->event_name[i], avg);
	}

}
double correlate_and_plot(std::vector<int> indexes, std::vector<double> performance, std::vector<double> counter, char*counter_name, char * file_path_input, int median, int arg1, int arg2, int arg3, int arg4)
{
	double correlation = 0;
	std::vector<double> perf_seg (&performance[0], &performance[indexes[0]]);
	std::vector<double> count_seg (&counter[0], &counter[indexes[0]]);
	correlation = pearson_correlation (&perf_seg[0], &count_seg[0], perf_seg.size());
	double tot = correlation;
	char data_file[130];
	char segment_file[130];
	char boundary_str[130];
	char keypoints_file[130];
	std::ofstream myfile;
	std::ofstream myfile_2;
	std::ofstream myfile_3;
	std::ofstream myfile_4;
	int max_perf = performance[0];
	int min_perf = performance[0];
	int max_counter = counter[0];
	int min_counter = counter[0];
	int line = 0;
	int n;
	int status;
	double total_corr = 0.0;
	char buffer_str[250];

	/*n = sprintf(buffer_str, "%sdata.txt", SOURCE_DIR);
	myfile.open(buffer_str); //SOURCE DIR????
	n = sprintf(buffer_str, "%ssegments.txt", SOURCE_DIR);
	myfile_2.open(buffer_str);
	n = sprintf(buffer_str, "%sgraph_boundaries.txt", SOURCE_DIR);
	myfile_3.open(buffer_str);*/



	n = sprintf(data_file, "%s%s/%s_data.txt",SOURCE_DIR, file_path_input, counter_name);

	n = sprintf(segment_file, "%s%s/%s_segments.txt",SOURCE_DIR,file_path_input, counter_name);

	n = sprintf(boundary_str, "%s%s/%s_graph_boundaries.txt", SOURCE_DIR, file_path_input, counter_name);

	n = sprintf(keypoints_file, "%s%s/%s_keypoints.txt", SOURCE_DIR, file_path_input, counter_name);



	myfile.open(data_file); //SOURCE DIR????
	myfile_2.open(segment_file);
	myfile_3.open(boundary_str);
	myfile_4.open(keypoints_file);

	fflush(stdout);
	for (int i = 0; i < performance.size(); i++)
	{
		if (performance[i] > max_perf)
		{
			max_perf = performance[i];
		}
		if (performance[i]<min_perf)
		{
			min_perf = performance [i];
		}
	}

	for (int i = 0; i < counter.size(); i++)
	{
		 if (counter[i] > max_counter)
		 {
			 max_counter = counter[i];
		 }
		 if (counter[i] < min_counter)
		 {
			 min_counter = counter[i];
		 }
	}
	max_perf = max_perf+(max_perf*0.1);
	min_perf = min_perf*0.9;
	max_counter = max_counter+(max_counter*0.1);
	max_counter = max_counter*0.9;
	line = max_perf;
	myfile_3 << std::fixed << max_perf << '\n' << min_perf << '\n' << max_counter << '\n' << min_counter << '\n' << counter_name << '\n';
	//printf("\n");
	for (int i = 0; i < performance.size(); i ++)
	{
		//printf("%d\t%.0f\t%.0f\n", i, performance[i], counter[i]);
		myfile << std::fixed << i << '\t' << performance[i] << '\t' << counter[i] << '\n';

	}
	total_corr = pearson_correlation(&performance[0], &counter[0], performance.size());
	for (int i = 0; i < indexes.size()-1;i++)
	{

		//printf("%d\t%.0f\t%.0f\n", i, performance[i], counter[i]);
		std::vector<double> seg_one (&performance[indexes[i]], &performance[indexes[i+1]]);
		std::vector<double> seg_two (&counter[indexes[i]], &counter[indexes[i+1]]);
		correlation = pearson_correlation (&seg_one[0], &seg_two[0], seg_one.size());

		myfile_2 << std::fixed << indexes[i] << '\t' << 0 << '\t' << indexes [i] << '\t' << line << '\t' << correlation << '\n';

		tot += correlation;

	}
	myfile_4 << file_path_input << '\n';
	myfile_4 << median << '\n';
	myfile_4 << arg1 << '\n';
	myfile_4 << arg2 << '\n';
	myfile_4 << arg3 << '\n';
	myfile_4 << arg4 << '\n';
	myfile_4 << counter_name << '\n';
	myfile_4 << file_path_input << '\n';
	myfile_4 << total_corr << '\n';
	myfile_4.close();
	myfile_3.close();
	myfile_2.close();
	myfile.close();
	//char buffer_str[250];
	/*n = sprintf(buffer_str,
				"/home/jakob/Desktop/Resource_graphs/./commands.sh %s%s/%s %d %d %d %d %d %s %s %.4f",
				SOURCE_DIR,file_path_input,counter_name, median, arg1, arg2, arg3, arg4, counter_name, file_path_input, total_corr);*/
	/*n = sprintf(buffer_str,
					"/home/jakob/Desktop/Resource_graphs/./commands.sh /home/jakob/Desktop/Resource_graphs/test.pdf");*/
	//status = system(buffer_str);
	return total_corr;
	//double avg = tot/indexes.size();

	//printf("Performance counter %s average correlation %f\n",counter_name, avg);
}
void merge_graphs(char * file_path)
{
	char buffer_str[200];
	int n;
	int status;
	n = sprintf(buffer_str, "pdfunite %s%s/*.pdf %s%s/merged_graphs.pdf", SOURCE_DIR, file_path, SOURCE_DIR, file_path);
	status = system(buffer_str);

}
double calculate_median(std::vector<double> data)
{
	std::sort(data.begin(), data.end());
	double median;
	int data_size = data.size();
	if (data_size == 0)
	{
		median = (data[data.size()/2]+data[(data.size()/2)+1])/2;
	}
	else
	{
		median = data[data.size()/2];
	}
	return median;
}
std::vector<int> determine_cutoff(std::vector<double> data, double standard_dev, double median)
{
	int start=0;
	int false_positive=0;
	int window_size = 5;
	int cutoff_val = median-standard_dev*CUTOFF_STD_FACTOR;
	std::vector<int> cut_values;

	for (int i = 0; i < data.size()-1; i++)
	{
		if (i==0)
		{
			while (data[i]<cutoff_val)
			{
				i++;
			}
			cut_values.push_back(i);
		}
		else
		{
			if (data[i] < cutoff_val)
			{
				while (data[i]< cutoff_val)
				{
					i++;
					start++;
					if (i > data.size())
					{
						break;
					}
					/*if (!(data[i] < cutoff_val))
					{
						false_positive++;
						while (!(data[i] < cutoff_val))
						{
							i++;
							false_positive++;
							if (false_positive == 5)
							{
								cut_values.push_back(i-false_positive);
								break;
							}
						}
					}
					if (false_positive == 5)
					{
						false_positive=0;
						break;
					}
					false_positive=0;*/
					if (start == window_size)
					{
						cut_values.push_back(i-start);

					}

				}
				start = 0;
			}

		}

	}
	//printf("Cut done");
	return cut_values;
}
char* create_graph_folder()
{
	char buffer_str [80];
	char * date_str;
	date_str = (char*)malloc(sizeof(char)*80);
	int n;
	int status;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(date_str, "graph_%d-%02d-%02d_%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	n = sprintf(buffer_str, "mkdir %s%s", SOURCE_DIR, date_str);
	//n = sprintf(buffer_str, "mkdir ~/Resource_graphs/%s",date_str);
	status = system(buffer_str);
	return date_str;
}

std::vector<task_characteristic> characterise_pid(const char ** my_argv, std::vector<int> performance_counters, int ac)
{

	double timestamp;
	double timestamp_after;
	double execution_time;
	char * path_string;
	int std_1_upper, std_1_lower, std_2_upper, std_2_lower;


	int counter1, counter2, counter3 = PAPI_NULL;
	fflush(stdout);
	std::vector<task_characteristic> measurement_data;

	path_string = create_graph_folder();

	std::vector<int> first_cutoff;
	std::vector<int> second_cutoff;




	if (performance_counters.size() == 1)
	{
		PAPI_event_name_to_code(PAPI_EVENTS[0], &counter1 );
		//output.counters=1;
	}
	else if (performance_counters.size() == 2)
	{
		PAPI_event_name_to_code(PAPI_EVENTS[0], &counter1 );
		PAPI_event_name_to_code(PAPI_EVENTS[1], &counter2 );
		//output.counters=2;
	}

	if (performance_counters.size()>2)
	{
		for (int ev_counter = 0; ev_counter < performance_counters.size(); ev_counter=ev_counter+3)
		{

			task_characteristic output;
			for (int i = 0; i < ac; i++)
			{
				output.app_arg[i] = (char*)malloc(sizeof(char)*64);
				strcpy(output.app_arg[i], my_argv[i]);
			}
			double output_correlation = 0.0;
			output.fp = (char*)malloc(sizeof(char)*strlen(my_argv[0]));

			//Loop to transfer arguments to struct...
			char * temp_name_tok = (char*)malloc(sizeof(my_argv[0]));
			char * temp_name =(char*)malloc(sizeof(my_argv[0]));
			strcpy(temp_name_tok, my_argv[0]);
			char * token = strtok(temp_name_tok, "/");
			while( token != NULL )
			{
				memset(temp_name, 0, sizeof(temp_name));
				int performance_counter = 0;
				strcpy(temp_name, token);
				token = strtok(NULL, "/");
			}

			output.name = (char*)malloc(sizeof(char)*strlen(my_argv[0]));
			strcpy(output.name, temp_name);
			strcpy(output.fp, my_argv[0]);
			output.execution_time = execution_time;
			output.frequency = output.execution_time / SAMPLES;
			printf("Execution time %f", output.execution_time);
			output.counters=4;
			if (ev_counter%3 == 0) //If full counter coverage
			{
				if ( PAPI_event_name_to_code( PAPI_EVENTS[performance_counters[ev_counter]], &counter1 ) != PAPI_OK )
				{
					printf("Wrapper create: %s\n", strerror(errno));
					fflush(stdout);
				}
				if ( PAPI_event_name_to_code( PAPI_EVENTS[performance_counters[ev_counter+1]], &counter2 ) != PAPI_OK )
				{
					printf("Wrapper create: %s\n", strerror(errno));
					fflush(stdout);
				}
				if ( PAPI_event_name_to_code( PAPI_EVENTS[performance_counters[ev_counter+2]], &counter3 ) != PAPI_OK )
				{
					printf("Wrapper create: %s\n", strerror(errno));
					fflush(stdout);
				}
				characterize_program(output.app_arg, &output, counter1, counter2, counter3);
				printf("Sampled all counters, now starting statistics\n");
				fflush(stdout);
				output.std_instr = calculateSD(output.event_counter_sets[0], output.event_counter_sets[0].size());
				output.median_instr = calculate_median(output.event_counter_sets[0]);
				first_cutoff = determine_cutoff(output.event_counter_sets[0], output.std_instr, output.median_instr);
				std_1_upper = output.median_instr+output.std_instr;
				std_1_lower = output.median_instr-output.std_instr;
				std_2_upper = output.median_instr+(output.std_instr*2);
				std_2_lower = output.median_instr-(output.std_instr*2);
				output_correlation = correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[1], output.event_name[1], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				output.total_correlations.push_back(output_correlation);
				output_correlation = correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[2], output.event_name[2], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				output.total_correlations.push_back(output_correlation);
				output_correlation = correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[3], output.event_name[3], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				output.total_correlations.push_back(output_correlation);

				int segments = first_cutoff.size();
				measurement_data.push_back(output);
				//printf("\n%s number of segments: %d", output.name, segments);

			}
			else if (ev_counter%2 == 0) //If two counter coverage
			{
				task_characteristic output;
				output.fp = (char*)malloc(sizeof(char)*strlen(my_argv[0]));
				strcpy(output.fp, my_argv[0]);
				output.execution_time = execution_time;
				output.frequency = output.execution_time / SAMPLES;
				printf("Execution time %f", output.execution_time);
				output.counters=4;
				if ( PAPI_event_name_to_code( PAPI_EVENTS[performance_counters[ev_counter]], &counter1 ) != PAPI_OK )
				{
					printf("Wrapper create: %s\n", strerror(errno));
					fflush(stdout);
				}
				if ( PAPI_event_name_to_code( PAPI_EVENTS[performance_counters[ev_counter+1]], &counter2 ) != PAPI_OK )
				{
					printf("Wrapper create: %s\n", strerror(errno));
					fflush(stdout);
				}
				characterize_program(my_argv, &output, counter1, counter2, NULL);
				output.std_instr = calculateSD(output.event_counter_sets[0], output.event_counter_sets[0].size());
				output.median_instr = calculate_median(output.event_counter_sets[0]);
				first_cutoff = determine_cutoff(output.event_counter_sets[0], output.std_instr, output.median_instr);
				std_1_upper = output.median_instr+output.std_instr;
				std_1_lower = output.median_instr-output.std_instr;
				std_2_upper = output.median_instr+(output.std_instr*2);
				std_2_lower = output.median_instr-(output.std_instr*2);
				correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[1], output.event_name[1], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[2], output.event_name[2], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[3], output.event_name[3], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				int segments = first_cutoff.size();
				printf("\n%s registered %d different segments \n", output.fp, segments);
				break;
			}
			else //If one counter coverage
			{
				task_characteristic output;
				output.fp = (char*)malloc(sizeof(char)*strlen(my_argv[0]));
				strcpy(output.fp, my_argv[0]);
				output.execution_time = execution_time;
				output.frequency = output.execution_time / SAMPLES;
				printf("Execution time %f", output.execution_time);
				output.counters=4;
				if ( PAPI_event_name_to_code( PAPI_EVENTS[performance_counters[ev_counter]], &counter1 ) != PAPI_OK )
				{
					printf("Wrapper create: %s\n", strerror(errno));
					fflush(stdout);
				}
				characterize_program(my_argv, &output, counter1, counter2, NULL);
				output.std_instr = calculateSD(output.event_counter_sets[0], output.event_counter_sets[0].size());
				output.median_instr = calculate_median(output.event_counter_sets[0]);
				first_cutoff = determine_cutoff(output.event_counter_sets[0], output.std_instr, output.median_instr);
				std_1_upper = output.median_instr+output.std_instr;
				std_1_lower = output.median_instr-output.std_instr;
				std_2_upper = output.median_instr+(output.std_instr*2);
				std_2_lower = output.median_instr-(output.std_instr*2);
				correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[1], output.event_name[1], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[2], output.event_name[2], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				correlate_and_plot(first_cutoff, output.event_counter_sets[0], output.event_counter_sets[3], output.event_name[3], path_string, output.median_instr, std_1_upper, std_1_lower, std_2_upper,std_2_lower);
				int segments = first_cutoff.size();
				printf("\n%s registered %d different segments \n", output.fp, segments);
				break;
			}
		}
	}



	//filter_warmup(&output.event_counter_sets);
	//merge_graphs(path_string);

	return measurement_data;

}

void repartition_engine() {

}
void cache_dependencies() {
	int total_cache_misses;
	for (int i = 0; i < tasks_in_system; i++) {
		total_cache_misses +=
				applications[i].performance_counter[applications[i].number_of_samples
						% 50];
	}
	for (int i = 0; i < tasks_in_system; i++) {
		applications[i].cache_miss_contribution =
				applications[i].performance_counter[applications[i].number_of_samples
						% 50] / total_cache_misses;
	}
}
double avg = 0, current_max = 0, current_min = 100, samples = 0, current_total = 0;
void print_all_counters(int size, double * performance, double * cache_misses)
{
	printf("Cache size %d \n", size);
	for (int i = 0; i < VALUE_SIZE; i++)
	{
		printf("%f\t%f\n", performance[i], cache_misses[i]);
	}
}
void* monitor_performance(task_characteristic * input_task)
{
	pid_t   my_pid;
	    int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;
	    char buffer_str[130];
	    int retval = 0;
	    long long values[7];
	    PAPI_option_t opt;
	    int ev_set = PAPI_NULL;
	    int start_prog = 0;
	    int n = 0;
	    int measurements=0;
	    double ver = PAPI_VER_CURRENT;
		char event_name[20];
	    std::vector<double> instr_ret;
	    std::vector<double> counter_1;
	    std::vector<double> counter_2;
	    std::vector<double> counter_3;

	    long long unsigned timestamp_start;
	    long long unsigned timestamp_end;
	    struct timespec ts;




	    if (retval = PAPI_create_eventset(&ev_set) != PAPI_OK)
	    {
	    	printf("ERROR: PAPI_create_eventset %d: %s\n", retval,
	    			PAPI_strerror(retval));

	    	exit(-1);
	    }

	    printf("PAPI_assign_eventset_component()\n");
	    if (retval = PAPI_assign_eventset_component((ev_set), 0) != PAPI_OK)
	    {
	    	printf("ERROR: PAPI_assign_eventset_component %d: %s\n", retval,
	    			PAPI_strerror(retval));
	    	exit(-1);
	    }
	    memset(&opt, 0x0, sizeof(PAPI_option_t));
	    opt.inherit.inherit = PAPI_INHERIT_ALL;
	    opt.inherit.eventset = ev_set;
	    if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK)
	    { //Most important, the papi inheritance!!!!
	    	printf("PAPI_set_opt error %d: %s0", retval, PAPI_strerror(retval));
	    	exit(-1);
	    }
	    if ((retval = PAPI_add_event(ev_set, PAPI_TOT_INS))!= PAPI_OK) //PAPI_TOT_INS
	    {
	    	printf("failed to attach instructions retired");
	    	printf("Wrapper create: %s\n", strerror(errno));
	    }


	    int i = 0;
	    if (0 == (my_pid = fork()))
	    {

	    		i++;
	    		mon_pid = syscall(SYS_gettid);
	    		fflush(stdout);
	    		stick_this_thread_to_core(input_task->partition.cpu);
	    		if (i==1)
	    		{
	    			//printf("Child pid is: %d\n", mon_pid);
	    		}
	            if (-1 == execve(input_task->app_arg[0], input_task->app_arg , NULL)) {
	                    perror("child process execve failed [%m]");
	                    return -1;

	            }
	    }
	    if ((retval = PAPI_attach(ev_set, my_pid)) != PAPI_OK)
	    {
	        printf("Monitor pid is: %d\n", my_pid);
	        printf("Papi error: %s\n", strerror(errno));
	        exit(-1);
	    }
	    if ((retval = PAPI_start(ev_set)) != PAPI_OK)
	    {
	        printf("PAPI_start error %d: %s0\n", retval, PAPI_strerror(retval));
	        exit(-1);
	    }
	    if (PAPI_reset(ev_set) != PAPI_OK)
	    {
	        printf("PAPI_reset error %d: %s0\n", retval, PAPI_strerror(retval));
	        exit(-1);
	    }
	    int count = 0;
	    double window_val;
	    while (0 == waitpid(my_pid , &status , WNOHANG))
	    {
	    	if ((retval = PAPI_read(ev_set, values)) != PAPI_OK)
	    	{
	    		printf("Failed to read the events");
	    	}
	    	input_task->instr_hist[count%(WINDOW_SIZE_DEF)] = values[0];
	    	for (int i = 0; i < WINDOW_SIZE_DEF; i++)
	    	{
	    		window_val+=input_task->instr_hist[i];
	    	}
	    	count++;
	    	if (count > 10)
	    	{
	    		input_task->current_performance=(window_val/10)*0.000001;
	    	}
	    	window_val = 0;
	    	PAPI_reset(ev_set);
	    	//printf("Values %llu", values[0]);



	    	usleep(SLEEP_TIME);
	    	input_task->current_performance = values[0];
	    }

	    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
	            perror("%s failed, halt system\n");
	            return -1;
	    }
	    PAPI_reset(ev_set);
	    PAPI_stop(ev_set, values);
	    return 0;
}
void* monitor_tasks (std::vector<std::vector<task_characteristic>> active_tasks)
{
	int status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;
	char buffer_str[130];
	int retval = 0;
	long long values[7];
	PAPI_option_t opt;
	int ev_set = PAPI_NULL;
	    double ver = PAPI_VER_CURRENT;
		char event_name[20];
	    std::vector<double> instr_ret;
	    std::vector<double> counter_1;
	    std::vector<double> counter_2;
	    std::vector<double> counter_3;

	    long long unsigned timestamp_start;
	    long long unsigned timestamp_end;
	    struct timespec ts;




	    if (retval = PAPI_create_eventset(&ev_set) != PAPI_OK)
	    {
	    	printf("ERROR: PAPI_create_eventset %d: %s\n", retval,
	    			PAPI_strerror(retval));

	    	exit(-1);
	    }

	    printf("PAPI_assign_eventset_component()\n");
	    if (retval = PAPI_assign_eventset_component((ev_set), 0) != PAPI_OK)
	    {
	    	printf("ERROR: PAPI_assign_eventset_component %d: %s\n", retval,
	    			PAPI_strerror(retval));
	    	exit(-1);
	    }
	    memset(&opt, 0x0, sizeof(PAPI_option_t));
	    opt.inherit.inherit = PAPI_INHERIT_ALL;
	    opt.inherit.eventset = ev_set;
	    if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK)
	    { //Most important, the papi inheritance!!!!
	    	printf("PAPI_set_opt error %d: %s0", retval, PAPI_strerror(retval));
	    	exit(-1);
	    }
	    if ((retval = PAPI_add_event(ev_set, PAPI_TOT_INS))!= PAPI_OK) //PAPI_TOT_INS
	    {
	    	printf("failed to attach instructions retired");
	    	printf("Wrapper create: %s\n", strerror(errno));
	    }

	    if ((retval = PAPI_attach(ev_set, active_tasks[0][0].pid)) != PAPI_OK)
	    {
	        printf("Monitor pid is: %d\n", active_tasks[0][0].pid);
	        printf("Papi error: %s\n", strerror(errno));
	        exit(-1);
	    }
	    if ((retval = PAPI_start(ev_set)) != PAPI_OK)
	    {
	        printf("PAPI_start error %d: %s0\n", retval, PAPI_strerror(retval));
	        exit(-1);
	    }
	    if (PAPI_reset(ev_set) != PAPI_OK)
	    {
	        printf("PAPI_reset error %d: %s0\n", retval, PAPI_strerror(retval));
	        exit(-1);
	    }
	    int count = 0;
	    double window_val;
	    while (1)
	    {
	    	if ((retval = PAPI_read(ev_set, values)) != PAPI_OK)
	    	{
	    		printf("Failed to read the events");
	    	}
	    	active_tasks[0][0].instr_hist[count%(WINDOW_SIZE_DEF)] = values[0];
	    	for (int i = 0; i < WINDOW_SIZE_DEF; i++)
	    	{
	    		window_val+=active_tasks[0][0].instr_hist[i];
	    	}
	    	count++;
	    	if (count > 10)
	    	{
	    		active_tasks[0][0].current_performance=(window_val/10)*0.000001;
	    	}
	    	window_val = 0;
	    	PAPI_reset(ev_set);
	    	//printf("Values %llu", values[0]);



	    	usleep(SLEEP_TIME);
	    	//active_tasks[0][0].current_performance = values[0];
	    	printf("Performance %f\n", active_tasks[0][0].current_performance);

	    }

	    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
	            perror("%s failed, halt system\n");
	            return -1;
	    }
	    PAPI_reset(ev_set);
	    PAPI_stop(ev_set, values);
	    return 0;
}
void cache_resize(std::vector<std::vector<task_characteristic>> input_tasks)
{
	int task_size = input_tasks.size();
	int n =0;
	int status=0;
	char buffer_str[130];
	for (int i = 0; i < task_size; i++)
	{
		if (i == 0)
		{
			input_tasks[i][0].partition.start = 90;
			input_tasks[i][0].partition.end = input_tasks[i][0].partition.start+input_tasks[i][0].partition.size;
		}
		else
		{
			input_tasks[i][0].partition.start = input_tasks[i-1][0].partition.end+1;
			input_tasks[i][0].partition.end = input_tasks[i][0].partition.start+input_tasks[i][0].partition.size;
		}
		printf("Task %s size %d performance %f\t", input_tasks[i][0].name, input_tasks[i][0].partition.size, input_tasks[i][0].current_performance);
		n = sprintf(buffer_str,"echo %d-%d > /sys/fs/cgroup/%s/palloc.bins", input_tasks[i][0].partition.start, input_tasks[i][0].partition.end, input_tasks[i][0].name);
		status = system(buffer_str);
		//---------The actual directory setup
		//printf("Task: %s\t partition size: %d\t start: %d\t - end:%d \n", input_tasks[i][0].name, input_tasks[i][0].partition.size, input_tasks[i][0].partition.start, input_tasks[i][0].partition.end);
	}
	printf("\n");
}
int wakeup_process(task_characteristic input_task)
{
	int n = 0;
	char wake_str[30];
	n = sprintf(wake_str,"kill -CONT %d", input_task.pid);
	n = system(wake_str);
	return n;
}
int suspend_process(task_characteristic input_task)
{
	int n = 0;
	char kill_str[30];
	n = sprintf(kill_str,"kill -STOP %d", input_task.pid);
	n = system(kill_str);
	return n;
}
int start_process(task_characteristic input_task)
{
	int my_pid;
	int mon_pid = 0;
	int n = 0;
	int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;
	char kill_str[30];

	if (0 == (my_pid = fork()))
	{
		 mon_pid = syscall(SYS_gettid);

		 fflush(stdout);
		 stick_this_thread_to_core(input_task.partition.cpu);

		 if (-1 == execve(input_task.app_arg[0], input_task.app_arg , NULL))
		 {
			 mon_pid = syscall(SYS_gettid);
			 perror("child process execve failed [%m]");
			 return -1;
		 }
	}
	 if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status))
	 {
		            perror("%s failed, halt system\n");
		            return -1;
	 }

	return my_pid;
}
void assign_to_partition(task_characteristic input_task)
{
	int n = 0;
	int status = 0;
	char buffer_str[80];
	n = sprintf(buffer_str,"echo %d > /sys/fs/cgroup/%s/tasks", input_task.pid, input_task.name);
	status = system(buffer_str);
}

void LLC_PC(std::vector<std::vector<task_characteristic>> input_tasks)
{
	pthread_t forker[3];
	int rc;
	int task_count = 0;
	double threshold = 0.05;
	int count = 0;
	double window_val = 0;
	int lazy_workaround = 10;
	int status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;
	char buffer_str[130];
	int retval = 0;
	long long values[7];
	long long values2[7];
	PAPI_option_t opt[2];
	int ev_set[3];
	ev_set[0] = PAPI_NULL;
	ev_set[1] = PAPI_NULL;
	ev_set[2] = PAPI_NULL;

	input_tasks[0][0].pid = start_process(input_tasks[0][0]);
	assign_to_partition(input_tasks[0][0]);
	input_tasks[1][0].pid = start_process(input_tasks[1][0]);
	assign_to_partition(input_tasks[1][0]);
	input_tasks[2][0].pid = start_process(input_tasks[2][0]);
	assign_to_partition(input_tasks[2][0]);

	printf("Setup all PAPI counter, now sleeping abit...\n");
	/*suspend_process(input_tasks[0][0]);
	suspend_process(input_tasks[1][0]);
	suspend_process(input_tasks[2][0]);
	fflush(stdout);
		usleep(5000000);
		printf("Starting processes again...\n");
		fflush(stdout);

	wakeup_process(input_tasks[0][0]);
	wakeup_process(input_tasks[1][0]);
	wakeup_process(input_tasks[2][0]);
	/*	wakeup_process(input_tasks[1][0]);
		wakeup_process(input_tasks[2][0]);*/
	//monitor_tasks(input_tasks);
	//-------------------------------PAPI INIT STUFF----------------------------------------
	for (int i = 0; i < input_tasks.size(); i++)
	{
		if (retval = PAPI_create_eventset(&ev_set[i]) != PAPI_OK)
		{
			printf("ERROR: PAPI_create_eventset %d: %s\n", retval,
			PAPI_strerror(retval));
			exit(-1);
		}
	}
	for (int i = 0; i < input_tasks.size(); i++)
	{
		if (retval = PAPI_assign_eventset_component((ev_set[i]), 0) != PAPI_OK)
		{
			printf("ERROR: PAPI_assign_eventset_component %d: %s\n", retval,
			PAPI_strerror(retval));
	   		exit(-1);
		}
	}

	for (int i = 0; i < input_tasks.size();i++)
	{
		memset(&opt[i], 0x0, sizeof(PAPI_option_t));
		opt[i].inherit.inherit = PAPI_INHERIT_ALL;
		opt[i].inherit.eventset = ev_set[i];
		if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt[i])) != PAPI_OK)
		{ //Most important, the papi inheritance!!!!
			printf("PAPI_set_opt error %d: %s0", retval, PAPI_strerror(retval));
			exit(-1);
		}
	}

	for (int i = 0; i < input_tasks.size();i++)
	{
		if ((retval = PAPI_add_event(ev_set[i], PAPI_TOT_INS))!= PAPI_OK) //PAPI_TOT_INS
		{
			printf("failed to attach instructions retired");
			printf("PAPI error on TASK %d: %s\n",i, strerror(errno));
		}
	}

	for (int i = 0; i < input_tasks.size();i++)
	{
		if ((retval = PAPI_attach(ev_set[i], input_tasks[i][0].pid)) != PAPI_OK)
		{
		    printf("Papi ATTACH ERROR ON %d-- PAPI ERROR str: %s\n",i, strerror(errno));
		    //exit(-1);
		}
	}

	for (int i = 0; i < input_tasks.size();i++)
	{
		if ((retval = PAPI_start(ev_set[i])) != PAPI_OK)
		{
			printf("PAPI_start error %d: %s0\n", retval, PAPI_strerror(retval));
		}
	}
	for (int i = 0; i < input_tasks.size();i++)
	{
		if (PAPI_reset(ev_set[i]) != PAPI_OK)
		{
			printf("PAPI_reset error %d: %s0\n", retval, PAPI_strerror(retval));
			//exit(-1);
		}
	}

	//-------------------------------PAPI INIT STUFF----------------------------------------
	/*rc = pthread_create(&forker[1], NULL, monitor_performance, (void *)&input_tasks[1][0]);
	rc = pthread_create(&forker[1], NULL, monitor_performance, (void *)&input_tasks[2][0]);*/
	//rc = pthread_create(&forker[2], NULL, monitor_performance, (void *)&input_tasks[2][0]);*/
	printf("Task %s on core %d\n", input_tasks[0][0].name, input_tasks[0][0].partition.cpu);
	printf("Task %s on core %d\n", input_tasks[1][0].name, input_tasks[1][0].partition.cpu);
	printf("Task %s on core %d\n", input_tasks[2][0].name, input_tasks[2][0].partition.cpu);
	while (1)
	{

		task_count = input_tasks.size();

		monitor_performance(&input_tasks[0][0]);
		monitor_performance(&input_tasks[1][0]);
		monitor_performance(&input_tasks[2][0]);
		/*if (input)
		{

		}*/



		for (int i = 0; i < input_tasks.size();i++)
		{
			if ((retval = PAPI_read(ev_set[i], values)) != PAPI_OK)
			{
				printf("Failed to read the events");
			}
			input_tasks[i][0].instr_hist[count%(WINDOW_SIZE_DEF)] = values[0];
			for (int j = 0; j < WINDOW_SIZE_DEF; j++)
			{
				window_val+=input_tasks[i][0].instr_hist[j];
			}
			count++;
			if (count > 100)
			{
				input_tasks[i][0].current_performance=(window_val/WINDOW_SIZE_DEF)*0.000001;
			}
			window_val = 0;
			PAPI_reset(ev_set[i]);
		}


		//printf("Values %llu", values[0]);



			    	//usleep(SLEEP_TIME);
			    	//active_tasks[0][0].current_performance = values[0];
		/*printf("Performance %f\n", input_tasks[0][0].current_performance);
		printf("OTHER %llu", values2[0]);*/
		if ((lazy_workaround % 100) == 0)
		{
			printf("%s perf: %f \t %s perf: %f \t %s perf: %f\n",
							input_tasks[0][0].name, input_tasks[0][0].current_performance,
							input_tasks[1][0].name, input_tasks[1][0].current_performance,
							input_tasks[2][0].name, input_tasks[2][0].current_performance);
			fflush(stdout);
			for (int i = 0; i < task_count; i++)
			{
				/*int lower_threshold = input_tasks[i][0].desired_performance*(1-threshold);
				int upper_threshold = input_tasks[i][0].desired_performance*(1+threshold);*/
				/*if (input_tasks[i][0].current_performance > lower_threshold && input_tasks[i][0].current_performance < upper_threshold)
				{
					input_tasks[i][0].diff = 0;
					printf("Desired performance met for task %s, not doing anything\n", input_tasks[i][0].name);
					//fflush(stdout);
				}*/
				if (input_tasks[i][0].current_performance < input_tasks[i][0].desired_performance)
				{
					input_tasks[i][0].diff = input_tasks[i][0].desired_performance - input_tasks[i][0].current_performance;
					input_tasks[i][0].diff = input_tasks[i][0].diff*-1;
					//printf("Desired performance for task %s not met, diff %f\n", input_tasks[i][0].name, input_tasks[i][0].diff);
				}
				else if (input_tasks[i][0].current_performance > input_tasks[i][0].desired_performance)
				{
					input_tasks[i][0].diff = input_tasks[i][0].desired_performance - input_tasks[i][0].current_performance;
					//int proportion = input_tasks[i][0].desired_performance - input_tasks[i][0].current_performance < input_tasks[i][0].desired_performance;
					//printf("Desired performance for task %s not met\n", input_tasks[i][0].name);
				}
			}
			if (REPARTITION_LLC_PC == 1)
			{
				int low_diff = 0;
				int high_diff = 0;
				int low_idx=1337;
				int high_idx=1337;
				int positive_diff = 13;
				for (int i =0; i < task_count; i++) //Tänk ut något att hantera JUNK
				{
					if (input_tasks[i][0].diff == 0)
					{

					}
					else
					{
						if (high_diff == 0 && low_diff == 0)
						{
							high_diff = input_tasks[i][0].diff;
							low_diff = input_tasks[i][0].diff;
							low_idx = i;
							high_idx = i;
						}
						else
						{
							if (input_tasks[i][0].diff > high_diff)
							{
								high_diff = input_tasks[i][0].diff;
								high_idx=i;
							}
							else if (input_tasks[i][0].diff < low_diff)
							{
								low_diff = input_tasks[i][0].diff;
								low_idx=i;
							}

						}
						//printf("Low idx:%d, high idx:%d", high_idx, low_idx);
					}
				}
				if ((low_idx != 1337) && (high_idx != 1337))
				{
					printf("Low idx:%d, high idx:%d", high_idx, low_idx);
					//double proportion = 10;
					//double proportion = input_tasks[low_idx][0].diff/input_tasks[high_idx][0].diff;
					int proportion = 10;
					if (input_tasks[high_idx][0].partition.size > 80)
					{
						input_tasks[low_idx][0].partition.size += proportion;
						input_tasks[high_idx][0].partition.size -= proportion;

					}
					else
					{
						printf("Cant decrease the size anymore...\n");
					}



					printf("Highest task %d - diff: %f, lowest task %d - diff %f", high_idx,
							input_tasks[high_idx][0].diff, low_idx, input_tasks[low_idx][0].diff);
					//printf("Proportion %f, removing some stuff...\n", proportion );
				}
				if (REPARTITION_LLC_PC ==1)
				{
					cache_resize(input_tasks);
				}
			}

		}



		lazy_workaround++;
		usleep(10000);

	}
}
void partition_control() {
	int scaling_factor = 10;
	int retval;
	long long values[1];
	int use_correlation = 4;
	struct timeval  t;
	char            t_str[64];
	long long unsigned timestamp;
	long long unsigned timestamp_after;
	struct timespec ts;
	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT && retval != 0)
	{
		printf("PAPI library version mismatch. %d: %s\n", retval,
				PAPI_strerror(retval));
		exit(-1);
	}
	int counter = 0;

	while (1)
	{
		double time_interval_star = (double)time_ns(&ts);
		for (int i = 0; i < tasks_in_system; i++)
		{
			if ((retval = PAPI_read(events_set[i], values)) != PAPI_OK)
			{
				printf("Failed to read the events");
			}
			else
			{
				int mod_counter = applications[i].number_of_samples % VALUE_SIZE;
				applications[i].performance[mod_counter] = (double) values[0];
				applications[i].test_avg += applications[i].performance[mod_counter];
				applications[i].performance_counter[mod_counter] = (double) values[1];
				//printf("Performance %f\n", applications[i].performance[mod_counter]);
				applications[i].number_of_samples++;

				PAPI_reset(events_set[i]);
			}
			if (use_correlation == 1) //Correlation based strategy
			{
				if ((applications[i].number_of_samples % VALUE_SIZE) == 0)
				{
					int size_dec = 0;
					double correlation;
					if (applications[i].partition_samples > 3)
					{

						correlation = pearson_correlation(applications[i].partitions, applications[i].partitioned_history_data, applications[i].partition_samples);


						printf("Correlation%f\n", correlation);
					}
					if (correlation < 0.8 && !(applications[i].partition_samples<=5))
					{
						printf("Too low correlation, partition size is%d\n", partitions[i].end);
					}
					else
					{
						size_dec = 1;
						partitions[i].end = partitions[i].end + size_dec;
						applications[i].partition_samples++;
						printf("Insufficient number, increasing size:%d", partitions[i].end);
						resize_partition(partitions[i]);
					}
					applications[i].partitioned_history_data[applications[i].partition_samples] = applications[i].avg_performance;
					applications[i].avg_performance = avg_exec(applications[i].performance);
					applications[i].partitions[applications[i].partition_samples] = partitions[i].end;

				}
			}
			else if (use_correlation == 2) //Performance based
			{
				applications[i].previous_performance = applications[i].avg_performance;
				applications[i].avg_cache_miss = avg_exec(applications[i].performance_counter);

				applications[i].target_performance = applications[i].avg_performance + applications[i].avg_performance*TARGET_PERCENTAGE;
				if (applications[i].previous_performance > applications[i].target_performance)
				{
										//printf("Target performance reached\n");
				}
				applications[i].completed_correlations++;
				int size_dec = 0;
				size_dec = 1;
				partitions[i].end = partitions[i].end + size_dec;
				resize_partition(partitions[i]);
				applications[i].performance_per_cache_miss = applications[i].avg_performance / applications[i].avg_cache_miss;
				applications[i].partitioned_history_data[applications[i].partition_samples] = applications[i].avg_performance;
				applications[i].partitioned_history_data[applications[i].partition_samples] = applications[i].avg_performance;
				applications[i].partition_samples++;
				applications[i].partitions[applications[i].partition_samples] = partitions[i].end;
				applications[i].reached_target = 1;
				partitions[i].partitioned_timestamp = (double)time_ns(&ts)*0.00000001;
				//printf("<--%s Increased cache to %d-%d bins-->\n", partitions[i].name, partitions[i].start, partitions[i].end);
				printf("%f\t%f\t%f\t%f\n",applications[i].avg_performance, applications[i].avg_cache_miss, applications[i].correlation, applications[i].performance_per_cache_miss);
				double time_now = (double)time_ns(&ts)*0.00000001;

				if (counter > 30)
				{
					exit(-1);
				}
			}
			else if (use_correlation == 3) //Cache contribution based
			{
				cache_dependencies();
				for (int j = 0; j < tasks_in_system;j++)
				{
					printf("Pid %d", applications[j].pid);
				}
				if (applications[i].cache_miss_contribution > CACHE_HIGH_THRESHOLD)
				{
					partitions[i].end = partitions[i].end + 1;
				}
				else if (applications[i].cache_miss_contribution < CACHE_LOW_THRESHOLD)
				{
					partitions[i].end = partitions[i].end - 1;
				}
				else
				{

				}
			}
			else if (use_correlation == 4) //Always increase by 1
			{
				//if ((applications[i].number_of_samples % VALUE_SIZE) == 0)
				//{
					//applications[i].avg_performance = avg_exec(applications[i].performance);
					//printf("%f\n", applications[i].avg_performance);

				//}

				/*if ((applications[i].number_of_samples % VALUE_SIZE) == 0)
				{
									int size_dec = 0;
									double correlation;

									rcorrelation = pearson_correlation(applications[i].partitions, applications[i].partitioned_history_data, applications[i].partition_samples);

									size_dec = 72;
									partitions[i].end = partitions[i].end + size_dec;
									applications[i].partition_samples++;
									printf("Insufficient number, i512ncreasing size:%d", partitions[i].end);
									//resize_partition(partitions[i]);
									applications[i].partitioned_history_data[applications[i].partition_samples] = applications[i].avg_performance;
									applications[i].avg_performance = avg_exec(applications[i].performance);
									applications[i].partitions[applications[i].partition_samples] = partitions[i].end;
									printf("Performance %f Correlation%f\n",applications[i].avg_performance, correlation);

					}*/
			}


		}
		//monitor(); --------- REPLACE WITH CHARACTERIZE TASK FROM PID
		double time_interval_end = (double)time_ns(&ts);
		double total_time = (time_interval_end - time_interval_star)*0.01;
		current_total += total_time;
		if (total_time > current_max)
		{
			current_max = total_time;
		}
		if (total_time < current_min)
		{
			current_min = total_time;
		}
		avg = current_total/samples;
		samples++;
		avg, current_max, samples, current_min;
		printf("Min: %f\tMax: %f\tAvg: %f\n", current_min, current_max, avg);
		usleep(SAMPLING_FREQ * 1000);
 	}
}


int perf_counter;
char performance[20];
int pid;
void AR_model()
{

}
char * getline(void)
{
    char * line = malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}
void print_counters()
{
	int n = 0;
	char buffer_str[130];
	char command_print_buff[200];
	n = sprintf(buffer_str, "%scounters.txt", SOURCE_DIR);
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

	//n = sprintf(buffer_str, "%scounters.txt", SOURCE_DIR);
	fp = fopen(buffer_str, "r");

	if (fp == NULL)
	{
		exit(EXIT_FAILURE);
	}
	while ((read = getline(&line, &len, fp)) != -1)
	{
		if (strcmp(line, "Name\n") == 0)
	    {
	   		found_start = 1;
	    }
	    else if (found_start == 1)
	    {
	        if (line[0] == '-')
	       	{
	       		break;
	       	}
	       	else
	       	{
	       		printf("%d: %s",i, line);
	       		temp=(char*)malloc(sizeof(char)*strlen(line));
	       		temp = strtok(line, "\n");
	       		PAPI_EVENTS.push_back((char*)malloc(sizeof(char)*strlen(line)));
	       		strcpy(PAPI_EVENTS[i], temp);
	       		//free(temp);
       			i++;
       			/*if (i % 7)
       			{
       				printf("\n");
       			}*/
	       	}
      	}
    }
}
int main(int ac, char **av) {
	void * status;
	int thread_ec;
	int pids[200];
	int retval = 0;
	int counter = 0;
	int i = 0;
	const char *my_argv[64] = {""};
	std::vector<std::vector<task_characteristic>> task_vector;
	tasks_in_system = 0; //Should be changed to different, depending on LLM or LLCPC
	retval = PAPI_library_init(PAPI_VER_CURRENT);
	stick_this_thread_to_core(0);
	if (retval != PAPI_VER_CURRENT && retval != 0)
	{
		//Plot the median value
		//Investigate when moving hierarchial counters
		//
		printf("PAPI library version mismatch. %d: %s\n", retval,
		PAPI_strerror(retval));
		exit(-1);
	}
	/*if (PAPI_multiplex_init() != PAPI_OK)
	{
		printf("PAPI multiplex fail %d: %s\n", retval,
		PAPI_strerror(retval));
		exit(-1);
	}*/
	if (LLM_DEPS == 1)
	{

		FILE * fp;
		char * line = NULL;
		size_t len = 0;
		ssize_t read;
		char filepath [60];
		memset(filepath,0,60);

		char * counter_str;

		print_counters();
		counter_str = getline();
		char * token = strtok(counter_str, " ");
		while( token != NULL )
		{
			int performance_counter = 0;
			performance_counter = atoi(token);
			selected_rows_idx.push_back(performance_counter);
			token = strtok(NULL, " ");
		}

		printf("Enter program specification file\n");
		strcpy(filepath, getline());
		int str_len = strlen(filepath);
		filepath[str_len-1] = 0;
	    fp = fopen(filepath, "r");

		if (fp == NULL)
		{
			printf("Could not read file");
			exit(EXIT_FAILURE);
		}
		while ((read = getline(&line, &len, fp)) != -1)
		{
			token = strtok(line, " ");
			while( token != NULL )
			{
				my_argv[i] = (char*)malloc(sizeof(char)*64);
				strcpy(my_argv[i],token);
				token = strtok(NULL, " ");
				i++;
			}
			task_vector.push_back(characterise_pid(my_argv, selected_rows_idx, i));
			i=0;
		}
		int cache_miss_index = 0;
		double corr_gen = 0.0;
		int number_corr;
		for (int j = 0; j < task_vector.size();j++)
		{
			for (int t = 0; t <= task_vector[j][0].total_correlations.size(); t++) //j[0] only covers one set of 4 events, to include more, we need an iterator in 0
			{
				/*if (strcmp(task_vector[j][0].event_name[t],"PAPI_L2_DCM") == 0)
				{
					printf("FOUND STUFF");
					fflush(stdout);
					cache_miss_index = t-1; //t-1 to find the cache miss index
					if (task_vector[j][0].total_correlations[t-1] < -0.1) //-1 to take into the consideration that instr_ret is not included
					{
					corr_gen+=task_vector[j][0].total_correlations[t-1];
					}
				}*/
				if (strcmp(task_vector[j][0].event_name[t],"PAPI_L2_DCM") == 0)
				{
					cache_miss_index = t-1; //t-1 to find the cache miss index

				}
			}
			if (task_vector[j][0].total_correlations[cache_miss_index] < -0.1) //-1 to take into the consideration that instr_ret is not included
			{
				corr_gen+=task_vector[j][0].total_correlations[cache_miss_index];
			}
		}
		for (int j = 0; j < task_vector.size(); j++)
		{
			if (task_vector[j][0].total_correlations[cache_miss_index] < -0.1)
			{
				double temp = (task_vector[j][0].total_correlations[cache_miss_index]/corr_gen)*(PROFILING_PART_SIZE-JUNK_CONTAINER_SIZE-LLM_SHARK_PART);
				task_vector[j][0].partition.size = temp;
				printf("Task %d cache partition size: %d\n", j, task_vector[j][0].partition.size);
			}
			else
			{
				task_vector[j][0].partition.size = 0;
				printf("Task %d junk cache partitions\n", j);
			}
			task_vector[j][0].partition.cpu = j+1; //Assume very simplistic cpu assignment, too annoying to deal with right now....
		}

		//Antagonist L3miss -> L3 access

		fclose(fp);

		if (line)
		{
			free(line);
		}
	}
	if (LLC_PC_ON == 1)
	{
		task_vector[0][0].desired_performance = 5;
		task_vector[1][0].desired_performance = 3;
		task_vector[2][0].desired_performance = 3;
		//printf("Press any key to start the partitioning controller");
		//getline();
		int LLM_shark_pid = syscall(SYS_gettid);
		if (REPARTITION_LLC_PC ==1)
		{
			alloc_initial_partitions(task_vector);
		}
		LLC_PC(task_vector);
		//LLC_PC_Static(task_vector);

	}
	//alloc_prog_partition();
	//alloc_tool_partition();
	/*if (LLC_PC_ON == 1)
	{
		setup_partitions();
		int monitor_pid = syscall(SYS_gettid);

		bind_task_to_cache_partition(monitor_pid, "Controller");
		stick_this_thread_to_core(3);
		partition_control();
	}*/


}

