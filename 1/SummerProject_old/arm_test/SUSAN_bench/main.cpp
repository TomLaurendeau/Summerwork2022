#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <malloc.h>

char* tmpImageBMPHeader;
char* tmpImageDIBHeader;

int susan(int experimentCount, const std::string& resultFile,
	 unsigned char *data, int width, int height, int bit, int sizeDIB, int cores);

int loadImageHeader(std::string filename, int *width, int *height, int* bit, int *sizeDIB){
	std::ifstream file;
	file.open(filename, std::ios::binary);

	//BMP Header
	char* imageBMPHeader = new char[14];
	tmpImageBMPHeader = new char[14];
    
	if(file.is_open()){
		file.read(imageBMPHeader, 14);
		tmpImageBMPHeader = imageBMPHeader;
		*sizeDIB = *(int*)&imageBMPHeader[10]-14;
	}
	else{
		std::cout << "[Debug] Can't load the file!" << std::endl;
		return 0;
	}
    
	//DIB Header
	//std::cout << "[Debug] Size of DIB: " << *sizeDIB << std::endl;
    
	char* imageDIBHeader = new char[*sizeDIB];
	tmpImageDIBHeader = new char[*sizeDIB];
	file.read(imageDIBHeader, *sizeDIB);
	tmpImageDIBHeader = imageDIBHeader;
    
	*width = *(int*)&imageDIBHeader[4];
	*height = *(int*)&imageDIBHeader[8];
	*bit = *(int*)&imageDIBHeader[14];
    
	if(*bit < 24) return 0;

	//std::cout << "[Debug] " << *width << " x " << *height << " with " << *bit << " bits" << std::endl;

	return 1;
}

int loadImage(const std::string &filename, unsigned char* data, int width, int height, int bit, int sizeDIB){
	std::ifstream file;
	file.open(filename, std::ios::binary);

	//BMP Header
	char* imageBMPHeader = new char[14+sizeDIB];

	if(file.is_open()) {
		file.read(imageBMPHeader, 14+sizeDIB);
		//sizeDIB = *(int*)&imageBMPHeader[10]-14;
	}
	else {
		std::cout << "[Debug] Can't load the file!" << std::endl;
		return 0;
	}

	//Bitmap data
	int sizeData = width*height*bit/8;
	char* imageData = new char[sizeData];
	file.read(imageData, sizeData);

	for(int i=0; i<sizeData; ++i) {
		data[i] = imageData[i];
	}

	file.close();
	return 1;
}

int writeImage(const std::string &filename, unsigned char* data, int width, int height, int bit, int sizeDIB){
	std::ofstream file;
	file.open(filename, std::ios_base::binary|std::ios_base::trunc);
	if (file.is_open()) {
		//std::cout << "[Debug] output size DIB: " << sizeDIB << std::endl;

		file.write(tmpImageBMPHeader, 14);
		file.write(tmpImageDIBHeader, sizeDIB);

		file.write(static_cast<char*>(static_cast<void *>(data)), width*height*bit/8);
		file.close();
		return 1;
	} else {
		std::cout << "[Debug] Can't load the file!" << std::endl;
		return 0;
	}
    
	return 1;
}

int main(int argc, char *argv[]) {

     if (mallopt(M_MMAP_THRESHOLD, 0) != 1) 
	{
                   fprintf(stderr, "mallopt() failed");
                   exit(EXIT_FAILURE);
        }
    std::string filename = argv[1];
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != "bmp") {
        std::cout << "Warning: File is not BMP!\n";
    }

	int cores = argc > 2 ? atoi(argv[2]) : 1;
	int count = argc > 3 ? atoi(argv[3]) : 1;
	//std::cout << "cores: " << cores << "\ncount: " << count << "\n";

    int width, height, bit, sizeDIB;

    if (!loadImageHeader(filename, &width, &height, &bit, &sizeDIB)) {
        std::cout << "Error: Unable to load image header!\n";
        return 1;
    }
    unsigned char *data = new unsigned char[width*height*bit/8];
    if (!loadImage(filename, data, width, height, bit, sizeDIB)) {
        std::cout << "Error: Unable to load image!\n";
		return 1;
    }
	
	//auto begin = std::chrono::high_resolution_clock::now();
	susan(count, "", data, width, height, bit, sizeDIB, cores);
	//auto end = std::chrono::high_resolution_clock::now();
	//auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
	std::cout << time << "ns\n";
}
