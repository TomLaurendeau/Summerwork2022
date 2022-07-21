#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "papi.h"
#include <fstream>
#include <iostream>
#include <string>
using namespace std;
int system (const char *command);

int main(int argc, char **argv)
{
    remove("papi_avail.txt");
    FILE *f = popen("./papi_avail", "r");
    char pwd[10000] = {0};
    char papi[5] = {0};
    std::ofstream outfile("papi_avail.txt");
    while ((fgets(pwd, 10000, f) != NULL))
    {
        for (int i = 0; i < 5; i++)
        {
            papi[i] = pwd[i];
        }
        if ((papi[0] == 80) && 
            (papi[1] == 65) &&
            (papi[2] == 80) && 
            (papi[3] == 73) && 
            (papi[4] == 95))
        {
            //outfile << "PAPI_";
            for (int i = 0; i < 100; i++)
            {
                if (pwd[i] == 32)
                    break;
                outfile << pwd[i];
            }
            outfile << "\n";
        }
    }
    pclose(f);
}
