#gcc partitionpool.c wrapper.h wrapper.c pearsoncorrelation.c pearsoncorrelation.h -lm -pthread -lrt -o pool /home/jakob/Desktop/papi/src/libpapi.a
gcc bubblesort.c -pthread -o mergesort
cp mergesort ../Benchmark_tests
#gcc monitor.c wrapper.h wrapper.c -pthread -O0 -o testing -pthread -lrt /home/jakob/home/jakob/src/papi/src/libpapi.a
#gcc matmult_leech.c wrapper.h wrapper.c -pthread -O0 -o leecher -pthread -lrt /home/jakob/Desktop/papi_lib/papi/src/libpapi.a
