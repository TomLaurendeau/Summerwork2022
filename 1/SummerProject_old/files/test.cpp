#include <fstream>
#include <iostream>
#include <string>

int main (int argc, char *argv[]){
    remove("papi_avail.txt");
    system("./papi_avail.c");
    FILE *f = popen("/home/jakob/Desktop/Summerwork/a.out", "-a");
    char pwd[10000] = {0};
    std::ofstream outfile("papi_avail.txt");
    while ((fgets(pwd, 10000, f) != NULL))
    {
        outfile << pwd;
    }
    pclose(f); 
}
  