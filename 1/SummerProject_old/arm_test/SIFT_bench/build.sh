g++ feature_extract.cpp common.h ezsift.h ezsift.cpp image.h image_utility.h image_utility.cpp timer.h vvector.h -lrt -pthread -o sift
g++ sift_noloop.cpp common.h ezsift.h ezsift.cpp image.h image_utility.h image_utility.cpp timer.h vvector.h -lrt -pthread -o sift_noloop
cp sift ../Benchmark_tests
