// Merges two subarrays of arr[]. 
// First subarray is arr[l..m] 
// Second subarray is arr[m+1..r] 
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <malloc.h>	
int monitor_pid = 0;
long long values[1];
int retval = 0;
int performance = 0;
int temp = 0;
static inline long long unsigned time_ns(struct timespec* const ts) {
  /* We use monotonic to avoid any leap-problems generated by NTP. */
  if (clock_gettime(CLOCK_MONOTONIC, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

void swap (int * array, int index1, int index2) {
	int tempElement = array [index1];
	array [index1] = array [index2];
	array [index2] = tempElement;
}

void insertionSort (int * array, int count) {
	//For each element in the array
	for (int i = 1; i < count; i++) {
		//Move a pointer backwards from that element
		//see if it needs to be swapped with the previous element in the array
		//and swap it as needed
		for (int j = i; j > 0 && array[j] < array[j-1]; j--) {
			swap(array, j, j-1);
			performance++;
		}
	}
}



void merge(int arr[], int l, int m, int r) 
{ 
    int i, j, k; 
    int n1 = m - l + 1; 
    int n2 =  r - m; 
  
    /* create temp arrays */
    int * L = (int*)malloc(sizeof(int)*n1);
    int * R = (int*)malloc(sizeof(int)*n2);
    //int L[n1], R[n2]; 
    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++) 
        L[i] = arr[l + i]; 
    for (j = 0; j < n2; j++) 
        R[j] = arr[m + 1+ j]; 
  
    /* Merge the temp arrays back into arr[l..r]*/
    i = 0; // Initial index of first subarray 
    j = 0; // Initial index of second subarray 
    k = l; // Initial index of merged subarray 
    while (i < n1 && j < n2) 
    { 
        if (L[i] <= R[j]) 
        { 
            arr[k] = L[i]; 
            i++; 
        } 
        else
        { 
            arr[k] = R[j]; 
            j++; 
        } 
	performance++;
        k++; 
    } 
  
    /* Copy the remaining elements of L[], if there 
       are any */
    while (i < n1) 
    { 
        arr[k] = L[i]; 
        i++; 
        k++; 
	performance++;
    } 
  
    /* Copy the remaining elements of R[], if there 
       are any */
    while (j < n2) 
    { 
        arr[k] = R[j]; 
        j++; 
        k++; 
	performance++;
    }
    free(L);
    free(R);
} 
/* l is for left index and r is right index of the 
   sub-array of arr to be sorted */
void mergeSort(int arr[], int l, int r) 
{ 
    if (l < r) 
    { 
        // Same as (l+r)/2, but avoids overflow for 
        // large l and h 
	
        int m = l+(r-l)/2; 
	
 	//printf("m: %d, r: %d l: %d\n", m, l, r);
        // Sort first and second halves 
        mergeSort(arr, l, m); 
        mergeSort(arr, m+1, r); 
        merge(arr, l, m, r); 
	
	fflush(stdout);
    } 
}
int stick_this_thread_to_core(int core_id) {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   if (core_id < 0 || core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();    
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}
  
/* UTILITY FUNCTIONS */
/* Function to print an array */
void printArray(int A[], int size) 
{ 
    int i; 
    for (i=0; i < size; i++) 
        printf("%d ", A[i]); 
    printf("\n"); 
} 
void swapa(int *xp, int *yp) 
{ 
    int temp = *xp; 
    *xp = *yp; 
    *yp = temp; 
} 
  
// A function to implement bubble sort 
void bubbleSort(int arr[], int n) 
{ 
   int i, j; 
   for (i = 0; i < n-1; i++)       
  
       // Last i elements are already in place    
       for (j = 0; j < n-i-1; j++)  
           if (arr[j] > arr[j+1]) 
              swapa(&arr[j], &arr[j+1]); 
} 
/* Driver program to test above functions */
int main(int argc, char** argv) 
{ 
    
    int status;
    int thread_ec;
    int number = 0;
    long long values[1];
    int monitor_pid = 0;
    int perf_id = 0;
    pid_t tid;
    int arr_size = atoi(argv[1]); 
    long long unsigned timestamp;
    long long unsigned timestamp_after;
    int * arr;
    srand(time(NULL));  
    while (1)
    {
	if (mallopt(M_MMAP_THRESHOLD, 0) != 1) {
                   fprintf(stderr, "mallopt() failed");
                   exit(EXIT_FAILURE);
            }

	arr = (int*)malloc(sizeof(int)*arr_size);
    	for (int i = 0; i < arr_size; i++)
    	{
		int r = rand()%1000000; 
		arr[i] = r;
    	}
	insertionSort(arr, arr_size-1);
	printf("Min:%d - Max%d\n",  arr[0], arr[arr_size-1]);
	free(arr);
    }
} 