#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <string>

#include "papi.h"


static void
print_help(char **argv)
{
	printf("This is the PAPI avail program.\n");
	printf("It provides availability and details about PAPI Presets and User-defined Events.\n");
	printf("PAPI Preset Event filters can be combined in a logical OR.\n");
	printf("Usage: %s [options]\n", argv[0]);
	printf("Options:\n\n");
	printf("General command options:\n");
	printf("\t-h, --help       Print this help message\n");
	printf("\t-a, --avail      Display only available PAPI preset and user defined events\n");
	printf("\t-c, --check      Display only available PAPI preset and user defined events after an availability check\n");
	printf("\t-d, --detail     Display detailed information about events\n");
	printf("\t-e EVENTNAME     Display detail information about specified event\n");
	printf("\nEvent filtering options:\n");
	printf("\t--br             Display branch related PAPI preset events\n");
	printf("\t--cache          Display cache related PAPI preset events\n");
	printf("\t--cnd            Display conditional PAPI preset events\n");
	printf("\t--fp             Display Floating Point related PAPI preset events\n");
	printf("\t--ins            Display instruction related PAPI preset events\n");
	printf("\t--idl            Display Stalled or Idle PAPI preset events\n");
	printf("\t--l1             Display level 1 cache related PAPI preset events\n");
	printf("\t--l2             Display level 2 cache related PAPI preset events\n");
	printf("\t--l3             Display level 3 cache related PAPI preset events\n");
	printf("\t--mem            Display memory related PAPI preset events\n");
	printf("\t--msc            Display miscellaneous PAPI preset events\n");
	printf("\t--tlb            Display Translation Lookaside Buffer PAPI preset events\n");
	printf("\n");
}

static int
checkCounter(int eventcode)
{
	int EventSet = PAPI_NULL;
	if (PAPI_create_eventset(&EventSet) != PAPI_OK)
		return 0;
	if (PAPI_add_event(EventSet, eventcode) != PAPI_OK)
		return 0;
	if (PAPI_cleanup_eventset(EventSet) != PAPI_OK)
		return 0;
	if (PAPI_destroy_eventset(&EventSet) != PAPI_OK)
		return 0;
	return 1;
}

int main(int argc, char **argv)
{
	int args, i, j, k;
	int retval;
	unsigned int filter = 0;
	int print_event_info = 0;
	char *name = NULL;
	int print_avail_only = PAPI_ENUM_EVENTS;
	int print_tabular = 1;
	PAPI_event_info_t info;
	const PAPI_hw_info_t *hwinfo = NULL;
	int avail_count = 0;
	int check_counter = 0;
	int event_code;

	PAPI_event_info_t n_info;
    
    FILE * inputfile = fopen("/home/jakob/Desktop/Summerwork/executable/papi_avail.txt","wb");

	/* Parse command line arguments */

	for (args = 1; args < argc; args++)
	{
		if (strstr(argv[args], "-e"))
		{
			print_event_info = 1;
			if ((args + 1 >= argc) || (argv[args + 1] == NULL) || (strlen(argv[args + 1]) == 0))
			{
				print_help(argv);
				exit(1);
			}
			name = argv[args + 1];
		}
		else if (strstr(argv[args], "-c") || strstr(argv[args], "--check"))
		{
			print_avail_only = PAPI_PRESET_ENUM_AVAIL;
			check_counter = 1;
		}
		else if (strstr(argv[args], "-a"))
			print_avail_only = PAPI_PRESET_ENUM_AVAIL;
		else if (strstr(argv[args], "-d"))
			print_tabular = 0;
		else if (strstr(argv[args], "-h"))
		{
			print_help(argv);
			exit(1);
		}
		else if (strstr(argv[args], "--br"))
			filter |= PAPI_PRESET_BIT_BR;
		else if (strstr(argv[args], "--cache"))
			filter |= PAPI_PRESET_BIT_CACH;
		else if (strstr(argv[args], "--cnd"))
			filter |= PAPI_PRESET_BIT_CND;
		else if (strstr(argv[args], "--fp"))
			filter |= PAPI_PRESET_BIT_FP;
		else if (strstr(argv[args], "--ins"))
			filter |= PAPI_PRESET_BIT_INS;
		else if (strstr(argv[args], "--idl"))
			filter |= PAPI_PRESET_BIT_IDL;
		else if (strstr(argv[args], "--l1"))
			filter |= PAPI_PRESET_BIT_L1;
		else if (strstr(argv[args], "--l2"))
			filter |= PAPI_PRESET_BIT_L2;
		else if (strstr(argv[args], "--l3"))
			filter |= PAPI_PRESET_BIT_L3;
		else if (strstr(argv[args], "--mem"))
			filter |= PAPI_PRESET_BIT_BR;
		else if (strstr(argv[args], "--msc"))
			filter |= PAPI_PRESET_BIT_MSC;
		else if (strstr(argv[args], "--tlb"))
			filter |= PAPI_PRESET_BIT_TLB;
	}

	if (filter == 0)
	{
		filter = (unsigned int)(-1);
	}

	/* Init PAPI */

	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT)
	{
		fprintf(stderr, "Error!  PAPI library mismatch!\n");
		return 1;
	}

	retval = PAPI_set_debug(PAPI_VERB_ECONT);
	if (retval != PAPI_OK)
	{
		fprintf(stderr, "Error with PAPI_set debug!\n");
		return 1;
	}

	if (retval != PAPI_OK)
	{
		fprintf(stderr, "Error with PAPI_get_hardware_info!\n");
		return 1;
	}

	/* Code for info on just one event */

			/* Print *ALL* Events */

		for (i = 0; i < 2; i++)
		{
			// set the event code to fetch preset events the first time through loop and user events the second time through the loop
			if (i == 0)
			{
				event_code = 0 | PAPI_PRESET_MASK;
			}
			else
			{
				event_code = 0 | PAPI_UE_MASK;
			}

			/* For consistency, always ASK FOR the first event, if there is not one then nothing to process */
			if (PAPI_enum_event(&event_code, PAPI_ENUM_FIRST) != PAPI_OK)
			{
				continue;
			}
			do
			{
				if (PAPI_get_event_info(event_code, &info) == PAPI_OK)
				{
					if (print_tabular)
					{
						// if this is a user defined event or its a preset and matches the preset event filters, display its information
						if ((i == 1) || (filter & info.event_type))
						{
							if (print_avail_only)
							{
									if ((check_counter && checkCounter(event_code)) || !check_counter)
									{
										fprintf(inputfile,"%s\n",info.symbol);
                                        printf("%-13s\n",info.symbol);
									}
							}							
						}
					}
				}
			} while (PAPI_enum_event(&event_code, print_avail_only) == PAPI_OK);
		}
        fclose(inputfile);
        
	return 0;
}
