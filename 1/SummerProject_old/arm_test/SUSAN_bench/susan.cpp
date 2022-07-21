#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <vector>
#include <chrono>
#include <pthread.h>
#include <stdint.h>

#include <unistd.h>
#include <cstring> // memcpy

int writeImage(const std::string &filename, unsigned char* data, int width, int height, int bit, int sizeDIB);

struct pointData {
	int cornerResponse;
    int x, y;
};

unsigned char* RGBtoGrayscale(unsigned char *data, int size) {
	unsigned char *out = new unsigned char[size];
	for(int i = 0; i < size; ++i) {
		float a = 0.2126f * data[3*i+2] + 0.7152f * data[3*i+1] + 0.0722f * data[3*i];
		out[i] = a < 0 ? 0 : a > 255 ? 255 : a;
	}
	return out;
}

unsigned char* setup_brightness_lut2(int thresh, int form)
{
int   k;
float temp;

  unsigned char *bp = new unsigned char[516];
  unsigned char *ret = bp;
  bp=bp+258;

  for(k=-256;k<257;k++)
  {
    temp=((float)k)/((float)thresh);
    temp=temp*temp;
    if (form==6)
      temp=temp*temp*temp;
    temp=100.0*exp(-temp);
    *(bp+k)= (unsigned char)temp;
  }
  return ret;
}

unsigned char* setup_brightness_lut(int thresh, int form)
{
	float temp;

	unsigned char *bp= new unsigned char[516];
	unsigned char *p = bp+258;

	for(int k=-256;k<257;k++)
	{
		temp=((float)k)/((float)thresh);
		temp=temp*temp;
		if (form==6)
			temp=temp*temp*temp;
		temp=100.0*exp(-temp);
		p[k] = (unsigned char)temp;
	}
	return bp;
}

void normalize(int *data, int width, int height) {
	int size = width*height;
	int smin = 0, smax = 0;
	for(int i = 0; i < size; ++i) {
		if (data[i] < smin)
			smin = data[i];
		if (data[i] > smax)
			smax = data[i];
	}
	//double scale = (dmax - dmin)*(1./(smax - smin));
	//double shift = dmin - smin*scale;

	for(int i = 0; i < size; ++i) {
		data[i] = (unsigned char)((data[i] - smin)*255./(smax-smin));
		//data[i] = (unsigned char)(data[i] * scale);
	}
}

struct by_cornerResponse { 
    bool operator()(pointData const &left, pointData const &right) { 
        return left.cornerResponse > right.cornerResponse;
    }
};

std::vector<pointData> getMaximaPoints(/*unsigned char*/int *responsedata, int width, int height, float percentage, int filterRange, int suppressionRadius) {
	std::vector<unsigned char> maximaSuppressionMat(width*height);
	std::vector<pointData> points;
	for(int r = 0; r < height; ++r) {
		for(int c = 0; c < width; ++c) {
			if (responsedata[r*width + c] != -1)
				points.push_back(pointData { responsedata[r*width + c], c, r });
		}
	}
	std::sort(points.begin(), points.end(), by_cornerResponse());

	unsigned numberTopPoints = width * height * percentage;
	//std::cout << "Points: " << numberTopPoints << "\n";
	//return std::vector<pointData>(points.begin(), points.begin() + numberTopPoints);

	std::vector<pointData> topPoints;
	unsigned i = 0;
	while (topPoints.size() < numberTopPoints && i < points.size() && points[i].cornerResponse > 0) {
		if (maximaSuppressionMat[points[i].y*width + points[i].x] == 0) {
			for(int r = -suppressionRadius; r <= suppressionRadius; ++r) {
				for(int c = -suppressionRadius; c <= suppressionRadius; ++c) {
					int sx = points[i].x+c;
					int sy = points[i].y+r;

					if (sx >= width || sx < 0
						|| sy >= height || sy < 0)
						continue;

					/*if (sx >= width)
						sx = width - 1;
					else if (sx < 0)
						sx = 0;
					if (sy >= height)
						sy = height - 1;
					else if (sy < 0)
						sy = 0;*/

					maximaSuppressionMat[(sy)*width + (sx)] = 1;
				}
			}

			//points[i].x += 1 + filterRange;
			//points[i].y += 1 + filterRange;
			topPoints.push_back(points[i]);
		}
		++i;
	}
	return topPoints;
}

void markCorners(unsigned char *grayscale, int width, int height, const std::vector<pointData> &points, int num = 0) {
	int tot = num == 0 ? points.size() : num;
	for(auto p : points) {
		if (tot == 0)
			break;
		if (p.y > height - 3 || p.y < 2 
			|| p.x > width - 3 || p.x < 2)
			continue;
		
		grayscale[((p.y)*width + p.x -2)*3] = 
			grayscale[((p.y  )*width + p.x-1)*3] =
			grayscale[((p.y  )*width + p.x  )*3] = 
			grayscale[((p.y  )*width + p.x+1)*3] =
			grayscale[((p.y  )*width + p.x+2)*3] = 

			grayscale[((p.y-2)*width + p.x  )*3] =
			grayscale[((p.y-1)*width + p.x  )*3] =
			grayscale[((p.y+1)*width + p.x  )*3] =
			grayscale[((p.y+2)*width + p.x  )*3] =

			grayscale[((p.y  )*width + p.x-2)*3 + 1] = 
			grayscale[((p.y  )*width + p.x-1)*3 + 1] =
			grayscale[((p.y  )*width + p.x  )*3 + 1] = 
			grayscale[((p.y  )*width + p.x+1)*3 + 1] =
			grayscale[((p.y  )*width + p.x+2)*3 + 1] = 

			grayscale[((p.y-2)*width + p.x  )*3 + 1] =
			grayscale[((p.y-1)*width + p.x  )*3 + 1] =
			grayscale[((p.y+1)*width + p.x  )*3 + 1] =
			grayscale[((p.y+2)*width + p.x  )*3 + 1] = 0;

		grayscale[((p.y)*width + p.x -2)*3 + 2] = 
			grayscale[((p.y  )*width + p.x-1)*3 + 2] =
			grayscale[((p.y  )*width + p.x  )*3 + 2] = 
			grayscale[((p.y  )*width + p.x+1)*3 + 2] =
			grayscale[((p.y  )*width + p.x+2)*3 + 2] = 

			grayscale[((p.y-2)*width + p.x  )*3 + 2] =
			grayscale[((p.y-1)*width + p.x  )*3 + 2] =
			grayscale[((p.y+1)*width + p.x  )*3 + 2] =
			grayscale[((p.y+2)*width + p.x  )*3 + 2] = 255;
		--tot;
	}
}

int globalCores;
int globalWidth, globalHeight;
int globalExperimentCount;
int globalCounter[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char *globalData;
int *globalResponse;
unsigned char *globalLUT;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
bool active = false;

#define  FTOI(a) ( (a) < 0 ? ((int)(a-0.5)) : ((int)(a+0.5)) )
void *forkSUSAN(void *currentCore) {
	constexpr int max_no = 1850;
	int current = (intptr_t)currentCore;
	int executeCount = 0;
	
	int lowerBound = current*globalHeight/globalCores;
	int upperBound = (current+1)*globalHeight/globalCores;
	if (lowerBound < 3)
		lowerBound = 3;

	int *cgx = new int[globalWidth*globalHeight],
		*cgy = new int[globalWidth*globalHeight];

	while (executeCount < globalExperimentCount) {
		globalCounter[current] = 0;
		for(int r = lowerBound; r < upperBound - 3; ++r) {
			for(int c = 3; c < globalWidth - 3; ++c, ++globalCounter[current]) {
				int i = r*globalWidth + c, n = 100;
				unsigned char *p = globalData + (r-3)*globalWidth + (c-1);
				unsigned char *cp = globalLUT + globalData[i];
				/* Iterate mask and use LUT to compare brightness
				- - X X X - -
				- X X X X X -
				X X X X X X X
				X X X O X X X
				X X X X X X X
				- X X X X X -
				- - X X X - - */
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);
				p += globalWidth - 3;

				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);
				p += globalWidth - 5;

				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);
				p += globalWidth - 6;

				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);
				n += 2;
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);
				p += globalWidth - 6;

				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);
				p += globalWidth - 5;

				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);
				p += globalWidth - 3;

				n += *(cp - *p++);
				n += *(cp - *p++);
				n += *(cp - *p);

				if (n < max_no) {
					//globalResponse[i] = max_no - n;
					int x=0, y=0, sq, xx, yy;
					unsigned char cc;
					float divide;
					x=0;y=0;
					p= globalData + (r-3)*globalWidth + c - 1;

					cc=*(cp-*p++);x-=cc;y-=3*cc;
					cc=*(cp-*p++);y-=3*cc;
					cc=*(cp-*p);x+=cc;y-=3*cc;
					p+=globalWidth-3; 

					cc=*(cp-*p++);x-=2*cc;y-=2*cc;
					cc=*(cp-*p++);x-=cc;y-=2*cc;
					cc=*(cp-*p++);y-=2*cc;
					cc=*(cp-*p++);x+=cc;y-=2*cc;
					cc=*(cp-*p);x+=2*cc;y-=2*cc;
					p+=globalWidth-5;

					cc=*(cp-*p++);x-=3*cc;y-=cc;
					cc=*(cp-*p++);x-=2*cc;y-=cc;
					cc=*(cp-*p++);x-=cc;y-=cc;
					cc=*(cp-*p++);y-=cc;
					cc=*(cp-*p++);x+=cc;y-=cc;
					cc=*(cp-*p++);x+=2*cc;y-=cc;
					cc=*(cp-*p);x+=3*cc;y-=cc;
					p+=globalWidth-6;

					cc=*(cp-*p++);x-=3*cc;
					cc=*(cp-*p++);x-=2*cc;
					cc=*(cp-*p);x-=cc;
					p+=2;
					cc=*(cp-*p++);x+=cc;
					cc=*(cp-*p++);x+=2*cc;
					cc=*(cp-*p);x+=3*cc;
					p+=globalWidth-6;

					cc=*(cp-*p++);x-=3*cc;y+=cc;
					cc=*(cp-*p++);x-=2*cc;y+=cc;
					cc=*(cp-*p++);x-=cc;y+=cc;
					cc=*(cp-*p++);y+=cc;
					cc=*(cp-*p++);x+=cc;y+=cc;
					cc=*(cp-*p++);x+=2*cc;y+=cc;
					cc=*(cp-*p);x+=3*cc;y+=cc;
					p+=globalWidth-5;

					cc=*(cp-*p++);x-=2*cc;y+=2*cc;
					cc=*(cp-*p++);x-=cc;y+=2*cc;
					cc=*(cp-*p++);y+=2*cc;
					cc=*(cp-*p++);x+=cc;y+=2*cc;
					cc=*(cp-*p);x+=2*cc;y+=2*cc;
					p+=globalWidth-3;

					cc=*(cp-*p++);x-=cc;y+=3*cc;
					cc=*(cp-*p++);y+=3*cc;
					cc=*(cp-*p);x+=cc;y+=3*cc;

					xx=x*x;
					yy=y*y;
					sq=xx+yy;
					if ( sq > ((n*n)/2) )
					{
						if(yy<xx) {
							divide=(float)y/(float)abs(x);
							sq=abs(x)/x;
							sq=*(cp-globalData[(r+FTOI(divide))*globalWidth+c+sq]) +
							*(cp-globalData[(r+FTOI(2*divide))*globalWidth+c+2*sq]) +
							*(cp-globalData[(r+FTOI(3*divide))*globalWidth+c+3*sq]);
						}
						else {
							divide=(float)x/(float)abs(y);
							sq=abs(y)/y;
							sq=*(cp-globalData[(r+sq)*globalWidth+c+FTOI(divide)]) +
							*(cp-globalData[(r+2*sq)*globalWidth+c+FTOI(2*divide)]) +
							*(cp-globalData[(r+3*sq)*globalWidth+c+FTOI(3*divide)]);
						}

						if(sq>290){
							globalResponse[r*globalWidth+c] = max_no-n;
							cgx[r*globalWidth+c] = (51*x)/n;
							cgy[r*globalWidth+c] = (51*y)/n;
						}
					}
				}
			}
		}
		++executeCount;
	}
	free(cgx);
	free(cgy);
	return NULL;
}



int susan(int experimentCount, const std::string& resultFile, 
	unsigned char *data, int width, int height, int bit, int sizeDIB, int cores)
{
	int size = width*height;
	int filter = 3;

	// Init
	globalData = new unsigned char[size];
	globalResponse = new int[size]();
	globalLUT = setup_brightness_lut(20, 6);
	globalCores = cores;
	globalWidth = width;
	globalHeight = height;
	globalExperimentCount = experimentCount;
	
	// debug arrays
	while (1)
	{
	auto begin = std::chrono::high_resolution_clock::now();
	
	unsigned char *grayscale = new unsigned char[size*3];

	// Step 0: Grayscale
	unsigned char *image = RGBtoGrayscale(data, size);

	globalData = image;
	int *response;
	//std::cout << "Start\n";
	forkSUSAN(0);

	response = globalResponse;
	//normalize(response, width, height);

	unsigned char *resp = new unsigned char[size*bit/8];
	for(int i = 0, j=0; i < size; ++i, j+=3) {
		resp[j] = resp[j+1] = resp[j+2] = response[i];
	}
	//writeImage("./Result/response.bmp", resp, width, height, bit, sizeDIB);

	// Step 4: Find corners
	std::vector<pointData> corners = getMaximaPoints(response, width, height, 0.002f, filter, 5);
	
	//std::cout << "Step 4: " << time << " \xC2\xB5s\n";

	//std::cout << "DONE!\n";

	for(int i = 0; i < size; ++i) {
		grayscale[3*i] = grayscale[3*i+1] = grayscale[3*i+2] = image[i];
	}
	
	markCorners(grayscale, width, height, corners);
	free(image);
        free(grayscale);
	free(resp);
	//writeImage("./Result/corners.bmp", grayscale, width, height, bit, sizeDIB);
	auto end = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
	std::cout << time << "\n";
	}
	return 1;
}

