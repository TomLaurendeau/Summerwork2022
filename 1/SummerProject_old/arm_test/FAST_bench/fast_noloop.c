/**
 * @file main_omp.cpp
 * @brief Hough Transform for HSA HCC.
 * @author Jakob Danielsson <jakob.danielsson@mdh.se>
 * @date 2017-Jan-10
 */






//#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
//#include "/home/root/papi-5.5.1/src/papi.h"
//#include "papi.h"

//#include "fast_cpu.hpp"
//#include "ht_cpu.hpp"
#define  FTOI(a) ( (a) < 0 ? ((int)(a-0.5)) : ((int)(a+0.5)) )
#define  GLOBALCORES 8
#define  FASTthreshold 0.2
#define  N 12
#define SEVEN_SUPP           /* size for non-max corner suppression; SEVEN_SUPP or FIVE_SUPP */
#define MAX_CORNERS   15000

#pragma pack(push,1)
int * cornerIndexes;
int globalFASTcornercount;
/* Windows 3.x bitmap file header */
typedef struct {
    char         filetype[2];   /* magic - always 'B' 'M' */
    unsigned int filesize;
    short        reserved1;
    short        reserved2;
    unsigned int dataoffset;    /* offset in bytes to actual bitmap data */
} file_header;

/* Windows 3.x bitmap full header, including file header */
typedef struct {
    file_header  fileheader;
    unsigned int headersize;
    int          width;
    int          height;
    short        planes;
    short        bitsperpixel;  /* we only support the value 24 here */
    unsigned int compression;   /* we do not support compression */
    unsigned int bitmapsize;
    int          horizontalres;
    int          verticalres;
    unsigned int numcolors;
    unsigned int importantcolors;
} bitmap_header;
#pragma pack(pop)


typedef  struct {int x,y,info, dx, dy, I;} CORNER_LIST[MAX_CORNERS];
unsigned char* imgData;
int Width;
int Height;
int top_points_size;
struct pointData {
    float cornerResponse;
    int x, y;
};

typedef struct {
    int core;
    int alg;
}corePair;

static struct timeval tm1;



void start_time()
{
    gettimeofday(&tm1, NULL);
}

void stop_time()
{
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t = 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
    printf("%llu ms\n", t);
}
struct bitmap
{
    unsigned int width, height;
    unsigned char *pixels;
};
struct bitmap* bmp;
const int bytesPerPixel = 3; /// red, green, blue
const int fileHeaderSize = 14;
const int infoHeaderSize = 40;

void generateBitmapImage(unsigned char *image, int height, int width, char* imageFileName);
unsigned char* createBitmapFileHeader(int height, int width);
unsigned char* createBitmapInfoHeader(int height, int width);
struct pointData * top_points;


unsigned char info[54];
void generateBitmapImage(unsigned char *image, int height, int width, char* imageFileName){

    unsigned char* fileHeader = info;
    unsigned char* infoHeader = createBitmapInfoHeader(height, width);
    unsigned char padding[3] = {0, 0, 0};
    int paddingSize = (4-(width*bytesPerPixel)%4)%4;

    FILE* imageFile = fopen(imageFileName, "wb");

    fwrite(fileHeader, 1, fileHeaderSize, imageFile);
    fwrite(infoHeader, 1, infoHeaderSize, imageFile);

    int i;
    for(i=0; i<height; i++){
        fwrite(image+(i*width*bytesPerPixel), bytesPerPixel, width, imageFile);
        fwrite(padding, 1, paddingSize, imageFile);
    }

    fclose(imageFile);
}

unsigned char* createBitmapFileHeader(int height, int width){
    int fileSize = fileHeaderSize + infoHeaderSize + bytesPerPixel*height*width;

    static unsigned char fileHeader[] = {
        0,0, /// signature
        0,0,0,0, /// image file size in bytes
        0,0,0,0, /// reserved
        0,0,0,0, /// start of pixel array
    };

    fileHeader[ 0] = (unsigned char)('B');
    fileHeader[ 1] = (unsigned char)('M');
    fileHeader[ 2] = (unsigned char)(fileSize    );
    fileHeader[ 3] = (unsigned char)(fileSize>> 8);
    fileHeader[ 4] = (unsigned char)(fileSize>>16);
    fileHeader[ 5] = (unsigned char)(fileSize>>24);
    fileHeader[10] = (unsigned char)(fileHeaderSize + infoHeaderSize);

    return fileHeader;
}

unsigned char* createBitmapInfoHeader(int height, int width){
    static unsigned char infoHeader[] = {
        0,0,0,0, /// header size
        0,0,0,0, /// image width
        0,0,0,0, /// image height
        0,0, /// number of color planes
        0,0, /// bits per pixel
        0,0,0,0, /// compression
        0,0,0,0, /// image size
        0,0,0,0, /// horizontal resolution
        0,0,0,0, /// vertical resolution
        0,0,0,0, /// colors in color table
        0,0,0,0, /// important color count
    };

    infoHeader[ 0] = (unsigned char)(infoHeaderSize);
    infoHeader[ 4] = (unsigned char)(width    );
    infoHeader[ 5] = (unsigned char)(width>> 8);
    infoHeader[ 6] = (unsigned char)(width>>16);
    infoHeader[ 7] = (unsigned char)(width>>24);
    infoHeader[ 8] = (unsigned char)(height    );
    infoHeader[ 9] = (unsigned char)(height>> 8);
    infoHeader[10] = (unsigned char)(height>>16);
    infoHeader[11] = (unsigned char)(height>>24);
    infoHeader[12] = (unsigned char)(1);
    infoHeader[14] = (unsigned char)(bytesPerPixel*8);

    return infoHeader;
}
int glo_h;
int glo_w;
char glo_inf[54];
void FreeBmp()
{
    if (bmp && bmp->pixels)
    {
        bmp->width = 0;
        bmp->height = 0;
        free(bmp->pixels);
        free(bmp);
        bmp = NULL;
    }
}

int LoadBmp(const char *filepath)
{
    bmp = NULL;
    FILE *f = fopen(filepath, "rb");
    if (f)
    {
        bmp = (struct bitmap*)malloc(sizeof(bmp));
        bmp->width = 0;
        bmp->height = 0;
        bmp->pixels = NULL;
        //info[54] = {0};
        fread(info, sizeof(unsigned char), 54, f);
	//memcpy(glo_inf,info,54);
        bmp->width = *(unsigned int *)&info[18];
        bmp->height = *(unsigned int *)&info[22];
	Height = bmp->height;
	Width = bmp->width;
        unsigned int size = Width*Height*3;
        bmp->pixels = (unsigned char *)malloc(size);
        fread(bmp->pixels, sizeof(unsigned char), size, f);
        fclose(f);
	//printf("Loaded image\n");
        return 1;
    }
    return 0;
}

int *r;
int bt=20;
unsigned char *bp;
int max_no_corners=1850;
struct pointData* globalHarriscorners;
CORNER_LIST corner_list;

/*typedef int pthread_barrierattr_t;
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int tripCount;
} pthread_barrier_t;*/

pthread_barrier_t mybarrier;


unsigned char* RGBtoGrayscale(unsigned char *data, int lowerBound, int higherBound, int size) {
	unsigned char *out = (unsigned char*)malloc(sizeof(unsigned char)*size);
	for(int i = lowerBound; i < higherBound; ++i) {
		float a = 0.2126f * data[3*i+2] + 0.7152f * data[3*i+1] + 0.0722f * data[3*i];
		out[i] = a < 0 ? 0 : a > 255 ? 255 : a;
	}
	return out;
}


//Hitta sweetspot när man ska använda mmap
int globalExperimentCount;
int **globalCounter;
unsigned char *globalData;
float *globalResponse;



void normalize(float *data, int width, int height) {
	const int a = 255;
	const int b = 0;

	int size = width*height;
	double smin = 0, smax = 0;
	double dmin = a, dmax = b;
	for(int i = 0; i < size; ++i) {
		if (data[i] < smin)
			smin = data[i];
		if (data[i] > smax)
			smax = data[i];
	}
	double scale = (dmax - dmin)*(1./(smax - smin));

	for(int i = 0; i < size; ++i) {
		data[i] = (unsigned char)(data[i] * scale);
	}
}


int compare(const void * a, const void * b)
{
    struct pointData * tempa = (struct pointData*)a;
    struct pointData * tempb = (struct pointData*)b;
    return ( tempa->cornerResponse < tempb->cornerResponse );
}

struct pointData * getMaximaPoints(float *responsedata, int width, int height, float percentage, int filterRange, int suppressionRadius, int lowerBound, int upperBound)
{
	
	
	unsigned char * maximaSuppressionMat;
	struct pointData * intr_points;
	
	intr_points = (struct pointData *)malloc(sizeof(struct pointData)*width*height);
	maximaSuppressionMat = (unsigned char*)malloc(sizeof(unsigned char)*width*height);
	int point_size = 0;
	
	for(int r = 0; r < height; ++r) {
		for(int c = 0; c < width; ++c)
		{
			
			//points.push_back(pointData { responsedata[r*width + c], c, r });
			intr_points[point_size].cornerResponse = responsedata[r*width + c];
			intr_points[point_size].x = c;
			intr_points[point_size].y = r;
			point_size++;
		}
	}
	
	qsort(intr_points, point_size, sizeof(struct pointData), compare);
	unsigned numberTopPoints = width * height/3 * percentage;

	top_points_size = 0;
	unsigned int i = 0;
	
	
	top_points = (struct pointData *)malloc(sizeof(struct pointData)*numberTopPoints);
	
	//for (int j = 0; j < 100; j++)
	//{
	//i = 0;
	top_points_size = 0;
	//printf("Iterating maxima");
	while (top_points_size < numberTopPoints && i < point_size && intr_points[i].cornerResponse > 20000) 
	{
		if (maximaSuppressionMat[intr_points[i].y*width + intr_points[i].x] == 0) 
		{
			for(int r = -suppressionRadius; r <= suppressionRadius; ++r) 
			{
				
				for(int c = -suppressionRadius; c <= suppressionRadius; ++c) 
				{
					int sx = intr_points[i].x+c;
					int sy = intr_points[i].y+r;
					
					//if (sx >= width || sx < 0 || sy >= height || sy < 0)
					if (sx >= width || sx < 0 || sy >= height || sy < 0)
						continue;
					
					//maximaSuppressionMat[(sy)*width + (sx)] = 1;
					
				}
			}
			
			top_points[top_points_size].cornerResponse = intr_points[i].cornerResponse;
			top_points[top_points_size].x = intr_points[i].x;
			top_points[top_points_size].y = intr_points[i].y;
			top_points_size=top_points_size+1;
			
			
		}
		++i;
		
	
	}
	//printf("toppoints%d\n", top_points_size);
	//}
	//free(intr_points);
	globalHarriscorners = (struct pointData *)malloc(sizeof(struct pointData)*top_points_size);
	for (int i = 0; i < top_points_size; i++)
	{
		globalHarriscorners[i].cornerResponse = top_points[top_points_size].cornerResponse;
		globalHarriscorners[i].x = top_points[top_points_size].x;
		globalHarriscorners[i].y = top_points[top_points_size].y;


	}
	return top_points;
}


void markCorners(unsigned char *grayscale, int width, int height,struct pointData *points) {
	int tot = top_points_size;
	int num = 0;
	for(int i = 0; i < top_points_size;i++) {
		if (tot == 0)
			break;
		if (top_points[i].y > height - 3 || top_points[i].y < 2
			|| top_points[i].x > width - 3 || top_points[i].x < 2)
			continue;



		grayscale[((top_points[i].y)*width + top_points[i].x -2)*3 + 2] =
			grayscale[((top_points[i].y  )*width + top_points[i].x-1)*3 + 2] =
			grayscale[((top_points[i].y  )*width + top_points[i].x  )*3 + 2] =
			grayscale[((top_points[i].y  )*width + top_points[i].x+1)*3 + 2] =
			grayscale[((top_points[i].y  )*width + top_points[i].x+2)*3 + 2] =

			grayscale[((top_points[i].y-2)*width + top_points[i].x  )*3 + 2] =
			grayscale[((top_points[i].y-1)*width + top_points[i].x  )*3 + 2] =
			grayscale[((top_points[i].y+1)*width + top_points[i].x  )*3 + 2] =
			grayscale[((top_points[i].y+2)*width + top_points[i].x  )*3 + 2] = 255;
		--tot;
	}
}
void forkHarris(int currentCore)
{
	int size = Width*Height;
	int filter = 3;
	globalData = (unsigned char*)malloc(sizeof(unsigned char)*size);
	globalResponse = (float*)malloc(sizeof(float)*size);
	unsigned char *grayscale = (unsigned char*)malloc(sizeof(unsigned char)*size*3);
	globalData = imgData;
	float *response;
	int currCore = currentCore;
	int lowerBound = 0;
	int upperBound = Height;
	int localExperimentCount = 0;
	int x2, y2, xy, derivX, derivY;
	double time;
	int tests = 100;
	int testCount = 0;
	
	
	
	for (int exp = 0; exp < 1; exp++)
	{
	for(int r = lowerBound+1; r < upperBound - 1; ++r) {
		for(int c = 1; c < Width - 1; ++c) {
			derivX =
				globalData[(r-1)*Width+(c-1)]
				+ 2*globalData[(r  )*Width+(c-1)]
				+   globalData[(r+1)*Width+(c-1)]
				-   globalData[(r-1)*Width+(c+1)]
				- 2*globalData[(r  )*Width+(c+1)]
				-   globalData[(r+1)*Width+(c+1)];

			derivY =
				globalData[(r-1)*Width+(c-1)]
				+ 2*globalData[(r-1)*Width+(c  )]
				+   globalData[(r-1)*Width+(c+1)]
				-   globalData[(r+1)*Width+(c-1)]
				- 2*globalData[(r+1)*Width+(c  )]
				-   globalData[(r+1)*Width+(c+1)];

			x2 = derivX * derivX;
			y2 = derivY * derivY;
			xy = derivX * derivY;/*19436361094.000000
					       34135657620.000000*/
			globalResponse[r*Width + c] = abs((x2*y2 - xy*xy) - 0.06 * (x2 + y2) * (x2 + y2));

		}
	}
	}
	
	response = globalResponse;
	
	/*for (int i = 0; i < 100; i++)
	{*/
	globalHarriscorners = getMaximaPoints(response, Width, Height, 0.004f, filter, 5, lowerBound, upperBound); //Fixa imorgon, utparameter, globalharriscorners måste vara struct pointData
	//}




	for(int i = 0; i < size; ++i) {
		grayscale[3*i] = grayscale[3*i+1] = grayscale[3*i+2] = globalData[i];
	}

	normalize(response, Width, Height);

	


	//imgData = grayscale;


}
void grayScale(unsigned char * inputMatrix,int lowerBound, int higherBound)
{
    unsigned char * pixelGrayScale;
    int temp = 0;
    pixelGrayScale = (unsigned char*)malloc(sizeof(char)*Width*Height*3);
    
    for(int i=0; i<higherBound+1; i=i+3)
    {
        imgData[i] = (int)(0.2126 * inputMatrix[i+2] + 0.7152 * inputMatrix[i+1] + 0.0722 * inputMatrix[i]);
        imgData[i+1] = (int)(0.2126 * inputMatrix[i+2] + 0.7152 * inputMatrix[i+1] + 0.0722 * inputMatrix[i]);
        imgData[i+2] = (int)(0.2126 * inputMatrix[i+2] + 0.7152 * inputMatrix[i+1] + 0.0722 * inputMatrix[i]);
        //std::cout << "Pixel " << pixelGrayScale[i] << std::endl;
        //std::cout << "i " << i << std::endl;
        temp = i;
    }
    //return pixelGrayScale;

}
void * forkFAST(int currentCore)
{
    int current = (intptr_t)currentCore;
    int currentest = 0;
    int executeCount = 0;
    double timeD = 0.0;
    double convX = 0.0;
    double convY = 0.0;
    double gradientMagnitude = 0.0;
    double gradientAngle = 0.0;
    int lowerThreshold = 100;
    int gradientCoefficient = 300;
    int traversed = 0;
    int corners = 0;
    int higherArray[16];
    int localexperimentCount = 0;
    int tests = 500;
    int testCount = 0;
    float currentMax = 0.0;
    float currentMin = 0.0;
    float point01 = 0.0;
    float point02 = 0.0;
    float point03 = 0.0;
    float point04 = 0.0;
    float point05 = 0.0;
    float point06 = 0.0;
    float point07 = 0.0;
    float point08 = 0.0;
    float point09 = 0.0;
    float point10 = 0.0;
    float point11 = 0.0;
    float point12 = 0.0;
    float point13 = 0.0;
    float point14 = 0.0;
    float point15 = 0.0;
    float point16 = 0.0;
    cornerIndexes = (int*)malloc(sizeof(int)*Width*Height*3);
    unsigned char *grayImg;
    int lowerBound = 0;
    int upperBound = Height;
    
   
    
    //grayScale(imgData, 0, Height*Width*3);
    grayImg = imgData;
    corners = 0;
    
    
   
	globalFASTcornercount = 0; //Måste syncas annars blir resultatet knäppt
        for(int j=lowerBound+3; j < upperBound-1; j++)
        {
		
            for(int i=1; i<Width-1; i++)
            {
                
                
                currentMax = grayImg[j*Width+i]*(1+FASTthreshold);
                currentMin = grayImg[j*Width+i]*(1-FASTthreshold);
                point01 = grayImg[(j-3)*Width+i];
                point02 = grayImg[(j-3)*Width+i+1];
                point03 = grayImg[(j-2)*Width+i+2];
                point04 = grayImg[(j-1)*Width+i+3];
                point05 = grayImg[j*Width+i+3];
                point06 = grayImg[(j+1)*Width+i+3];
                point07 = grayImg[(j+2)*Width+i+2];
                point08 = grayImg[(j+3)*Width+i+1];
                point09 = grayImg[(j+3)*Width+i];
                point10 = grayImg[(j+3)*Width+i-1];
                point11 = grayImg[(j+2)*Width+i-2];
                point12 = grayImg[(j+1)*Width+i-3];
                point13 = grayImg[j*Width+i-3];
                point14 = grayImg[(j-1)*Width+i-3];
                point15 = grayImg[(j-2)*Width+i-2];
                point16 = grayImg[(j-3)*Width+i-1];                   //float whiteValue = pixelR[j*Width+i]+pixelG[j*Width+i]+pixelB[j*Width+i];
		
                int hitCount = 0;
                int shouldContinue = 0;
                
                if (point01 > currentMax || point01 < currentMin){
                    hitCount++;
                    higherArray[0] = 1;
		    
		    

                }
                if (point05 > currentMax || point05 < currentMin){
                    hitCount++;
                    higherArray[4] = 1;
                }
                if (point09 > currentMax || point09 < currentMin){
                    
                    hitCount++;
                    higherArray[8] = 1;
                }
                if (point13 > currentMax || point13 < currentMin){
                    hitCount++;
                    higherArray[12] = 1;
                }
                if (hitCount > 2){
                    if (point02 > currentMax || point02 < currentMin){
                        hitCount++;
                        higherArray[1] = 1;
                    }
                    if (point03 > currentMax || point03 < currentMin){
                        hitCount++;
                        higherArray[2] = 1;
                    }
                    if (point04 > currentMax || point04 < currentMin){
                        hitCount++;
                        higherArray[3] = 1;
                    }
                    if (point06 > currentMax || point06 < currentMin){
                        hitCount++;
                        higherArray[5] = 1;
                    }
                    if (point07 > currentMax || point07 < currentMin){
                        hitCount++;
                        higherArray[6] = 1;
                    }
                    if (point08 > currentMax || point08 < currentMin){
                        hitCount++;
                        higherArray[7] = 1;
                    }
                    
                    if (point10 > currentMax || point10 < currentMin){
                        hitCount++;
                        higherArray[9] = 1;
                    }
                    if (point11 > currentMax || point11 < currentMin){
                        hitCount++;
                        higherArray[10] = 1;
                    }
                    if (point12 > currentMax || point12 < currentMin){
                        hitCount++;
                        higherArray[11] = 1;
                    }
                    
                    if (point14 > currentMax || point14 < currentMin){
                        hitCount++;
                        higherArray[13] = 1;
                    }
                    if (point15 > currentMax || point15 < currentMin){
                        hitCount++;
                        higherArray[14] = 1;
                    }
                    if (point16 > currentMax || point16 < currentMin){
                        hitCount++;
                        higherArray[15] = 1;
                    }
                }
                
                if (hitCount >= N){
                    int tempC = 0;
                    
                    for (int co = 1; co < 16; co++)
                    {
                        if (higherArray[co] - higherArray[co-1] ==  0)
                        {
                            tempC++;
                            //higherArray[co-1] = 0;
                        }
                        else
                        {
                            tempC=0;
                        }
                        higherArray[co-1] = 0;
                    }
                    higherArray[15] = 0;
                    if (tempC >= N)
                    {
			/*printf("Point1 %f \n", point01);
			fflush(stdout);*/
			//printf("x:%d, y:%d\n", j, i);
                        cornerIndexes[globalFASTcornercount] = j*Width+i;
			globalFASTcornercount++;
                    }
                }
            }
	    
		
        }

    
}
/*static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_MONOTONIC, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}*/
int foo(char* input, char *output) {

    //variable dec:
    FILE *fp,*out;
    bitmap_header* hp;
    int n;
    unsigned char *data;
    //Papi variables
    //Open input file:
    fp = fopen(input, "rb");
    if(fp==NULL){
        //cleanup
    }

    //Read the input file headers:
    hp=(bitmap_header*)malloc(sizeof(bitmap_header));
    if(hp==NULL)
        return 3;
    
    n=fread(hp, sizeof(bitmap_header), 1, fp);
    
    fflush(stdout);
    if(n<1){
        //cleanup
    }
    int bit = 24;
    Width = hp->width;
    Height = hp->height;
    //printf("Height %d Width %d", Height, Width);
    //Read the data of the image:
    data = (unsigned char*)malloc(sizeof(unsigned char)*hp->bitmapsize);
    imgData = (unsigned char*)malloc(sizeof(unsigned char)*hp->bitmapsize);
    imgData = data;
    
    
    
    
    if(data==NULL){
        //cleanup
    }

    fseek(fp,sizeof(char)*hp->fileheader.dataoffset,SEEK_SET);
    n=fread(data,sizeof(char),hp->bitmapsize, fp);
    if(n<1){
        //cleanup
    }

    imgData = RGBtoGrayscale(data, 0, Width*Height, Width*Height);
    
    /*forkHarris(1);
    
    unsigned char *resp = (unsigned char *)malloc(sizeof(unsigned char)*bit*Width*Height/8);
    for(int i = 0, j=0; i < Width*Height; ++i, j+=3)
    {
	resp[j] = resp[j+1] = resp[j+2] = imgData[i];
    }
    imgData = resp;
    markCorners(imgData, Width, Height, globalHarriscorners);*/
    
    
    data = imgData;
    struct timespec ts;
    long long unsigned timestamp;
    long long unsigned timestamp_after;
    //timestamp = time_ns(&ts);
    clock_t start = clock();
//forkFAST(1);
	forkFAST(1);
    //forkHarris(1);
    unsigned char *resp = (unsigned char *)malloc(sizeof(unsigned char)*bit*Width*Height/8);
    for(int i = 0, j=0; i < Width*Height; ++i, j+=3)
    {
        resp[j] = resp[j+1] = resp[j+2] = imgData[i];
    }
    imgData = resp;
    //markCorners(imgData, Width, Height, globalHarriscorners);

    //printf("Perf count: %d\n", values[0]);
    data=imgData;
    //timestamp_after = time_ns(&ts);
    clock_t stop = clock();

    double exec = (double)(timestamp_after-timestamp);
    double elapsed = (double)(stop - start) * 1000.0 / (double)CLOCKS_PER_SEC;
    //printf("Time elapsed in ms: %f\n", elapsed);
    //printf("Time elapsed in ns: %f\n", exec);
    //printf("ms: %f\tus: %f\t\n", elapsed, exec);
    
    
    
        //Open output file:
    out = fopen(output, "wb");
    if(out==NULL){
        //cleanup
    }

    n=fwrite(hp,sizeof(unsigned char),sizeof(bitmap_header),out);
    if(n<1){
        //cleanup
    }
    fseek(out,sizeof(char)*hp->fileheader.dataoffset,SEEK_SET);
    n=fwrite(data,sizeof(unsigned char),hp->bitmapsize,out);
    if(n<1){
        //cleanup
    }

    fclose(fp);
    fclose(out);
    free(hp);
    free(data);
    
    return 0;
}

int main(int argc, const char * argv[])
{
    
	int tests = 0;
	
	char * fp = argv[1];
	
	/*if (type == 1)
   	{*/
		//init_counters();
		//fflush(stdout);
	while(1)
	{
		foo(fp, "output.bmp");
	}
    	/*}
	else if (type == 2)
    	{
		receiver();
    	}
    	else if (type ==3)
    	{
		sender();
    	}
    	else if(type==4)
    	{
		net_spam();
    	}*/
	return 0;
}

