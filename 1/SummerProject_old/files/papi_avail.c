#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "papi.h"

/*
int
papi_print_header( char *prompt, const PAPI_hw_info_t ** hwinfo )
{
	int cnt, mpx;
	struct utsname uname_info;
	PAPI_option_t options;

	if ( ( *hwinfo = PAPI_get_hardware_info(  ) ) == NULL ) {
   		return PAPI_ESYS;
	}

	PAPI_get_opt(PAPI_COMPONENTINFO,&options);

	uname(&uname_info);

	printf( "%s", prompt );
	printf
		( "--------------------------------------------------------------------------------\n" );
	printf( "PAPI version             : %d.%d.%d.%d\n",
			PAPI_VERSION_MAJOR( PAPI_VERSION ),
			PAPI_VERSION_MINOR( PAPI_VERSION ),
			PAPI_VERSION_REVISION( PAPI_VERSION ),
			PAPI_VERSION_INCREMENT( PAPI_VERSION ) );
	printf( "Operating system         : %s %s\n",
		uname_info.sysname, uname_info.release);
	printf( "Vendor string and code   : %s (%d, 0x%x)\n",
			( *hwinfo )->vendor_string,
			( *hwinfo )->vendor,
			( *hwinfo )->vendor );
	printf( "Model string and code    : %s (%d, 0x%x)\n",
			( *hwinfo )->model_string,
			( *hwinfo )->model,
			( *hwinfo )->model );
	printf( "CPU revision             : %f\n", ( *hwinfo )->revision );
	if ( ( *hwinfo )->cpuid_family > 0 ) {
		printf( "CPUID                    : Family/Model/Stepping %d/%d/%d, "
			"0x%02x/0x%02x/0x%02x\n",
			( *hwinfo )->cpuid_family,
			( *hwinfo )->cpuid_model,
			( *hwinfo )->cpuid_stepping,
			( *hwinfo )->cpuid_family,
			( *hwinfo )->cpuid_model,
			( *hwinfo )->cpuid_stepping );
	}
	printf( "CPU Max MHz              : %d\n", ( *hwinfo )->cpu_max_mhz );
	printf( "CPU Min MHz              : %d\n", ( *hwinfo )->cpu_min_mhz );
	printf( "Total cores              : %d\n", ( *hwinfo )->totalcpus );

	if ( ( *hwinfo )->threads > 0 )
		printf( "SMT threads per core     : %d\n", ( *hwinfo )->threads );
	if ( ( *hwinfo )->cores > 0 )
		printf( "Cores per socket         : %d\n", ( *hwinfo )->cores );
	if ( ( *hwinfo )->sockets > 0 )
		printf( "Sockets                  : %d\n", ( *hwinfo )->sockets );
	printf( "Cores per NUMA region    : %d\n", ( *hwinfo )->ncpu );
	printf( "NUMA regions             : %d\n", ( *hwinfo )->nnodes );
	printf( "Running in a VM          : %s\n", ( *hwinfo )->virtualized?
		"yes":"no");
	if ( (*hwinfo)->virtualized) {
           printf( "VM Vendor                : %s\n", (*hwinfo)->virtual_vendor_string);
	}
	cnt = PAPI_get_opt( PAPI_MAX_HWCTRS, NULL );
	mpx = PAPI_get_opt( PAPI_MAX_MPX_CTRS, NULL );
	if ( cnt >= 0 ) {
		printf( "Number Hardware Counters : %d\n",cnt );
	} else {
		printf( "Number Hardware Counters : PAPI error %d: %s\n", cnt, PAPI_strerror(cnt));
	}
	if ( mpx >= 0 ) {
		printf( "Max Multiplex Counters   : %d\n", mpx );
	} else {
		printf( "Max Multiplex Counters   : PAPI error %d: %s\n", mpx, PAPI_strerror(mpx));
	}
	printf("Fast counter read (rdpmc): %s\n",
		options.cmp_info->fast_counter_read?"yes":"no");
	printf( "--------------------------------------------------------------------------------\n" );
	printf( "\n" );
	return PAPI_OK;
}*/


static char *
is_derived( PAPI_event_info_t * info )
{
	if ( strlen( info->derived ) == 0 )
		return ( "No" );
	else if ( strcmp( info->derived, "NOT_DERIVED" ) == 0 )
		return ( "No" );
	else if ( strcmp( info->derived, "DERIVED_CMPD" ) == 0 )
		return ( "No" );
	else
		return ( "Yes" );
}

static void
print_help( char **argv )
{
        printf( "This is the PAPI avail program.\n" );
        printf( "It provides availability and details about PAPI Presets and User-defined Events.\n" );
	printf( "PAPI Preset Event filters can be combined in a logical OR.\n" );
	printf( "Usage: %s [options]\n", argv[0] );
	printf( "Options:\n\n" );
	printf( "General command options:\n" );
	printf( "\t-h, --help       Print this help message\n" );
	printf( "\t-a, --avail      Display only available PAPI preset and user defined events\n" );
	printf( "\t-c, --check      Display only available PAPI preset and user defined events after an availability check\n" );
	printf( "\t-d, --detail     Display detailed information about events\n" );
	printf( "\t-e EVENTNAME     Display detail information about specified event\n" );
	printf( "\nEvent filtering options:\n" );
	printf( "\t--br             Display branch related PAPI preset events\n" );
	printf( "\t--cache          Display cache related PAPI preset events\n" );
	printf( "\t--cnd            Display conditional PAPI preset events\n" );
	printf( "\t--fp             Display Floating Point related PAPI preset events\n" );
	printf( "\t--ins            Display instruction related PAPI preset events\n" );
	printf( "\t--idl            Display Stalled or Idle PAPI preset events\n" );
	printf( "\t--l1             Display level 1 cache related PAPI preset events\n" );
	printf( "\t--l2             Display level 2 cache related PAPI preset events\n" );
	printf( "\t--l3             Display level 3 cache related PAPI preset events\n" );
	printf( "\t--mem            Display memory related PAPI preset events\n" );
	printf( "\t--msc            Display miscellaneous PAPI preset events\n" );
	printf( "\t--tlb            Display Translation Lookaside Buffer PAPI preset events\n" );
	printf( "\n" );
}

static int
parse_unit_masks( PAPI_event_info_t * info )
{
	char *pmask;

	if ( ( pmask = strchr( info->symbol, ':' ) ) == NULL ) {
		return ( 0 );
	}
	memmove( info->symbol, pmask, ( strlen( pmask ) + 1 ) * sizeof ( char ) );
	pmask = strchr( info->long_descr, ':' );
	if ( pmask == NULL )
		info->long_descr[0] = 0;
	else
		memmove( info->long_descr, pmask + sizeof ( char ),
				 ( strlen( pmask ) + 1 ) * sizeof ( char ) );
	return 1;
}

static int
checkCounter (int eventcode)
{
	int EventSet = PAPI_NULL;
	if (PAPI_create_eventset(&EventSet) != PAPI_OK)
		return 0;
	if (PAPI_add_event (EventSet, eventcode) != PAPI_OK)
		return 0;
	if (PAPI_cleanup_eventset (EventSet) != PAPI_OK)
		return 0;
	if (PAPI_destroy_eventset (&EventSet) != PAPI_OK)
		return 0;
	return 1;
}


/*
  Checks whether a preset event is available. If it is available,
  the function returns 1, or 0 otherwise. */

int 
is_preset_event_available(char *name) {

  int  event_code = 0 | PAPI_PRESET_MASK;
  PAPI_event_info_t info;
  int check_counter = 1;


  if (PAPI_enum_event( &event_code, PAPI_ENUM_FIRST ) != PAPI_OK) {
    printf("error!");
    exit(1);
  }

  /* Iterate over all the available preset events and compare them by names. */
  do {
    if ( PAPI_get_event_info( event_code, &info ) == PAPI_OK ) {
      
      if ( info.count ) {
	if ( (check_counter && checkCounter (event_code)) || !check_counter) {
	  if (strcmp(info.symbol, name) == 0)
	    return 1;
	}
      }
    }
  } while (PAPI_enum_event( &event_code, PAPI_PRESET_ENUM_AVAIL ) == PAPI_OK);

  return 0;
}



int
main( int argc, char **argv )
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
   int tot_count = 0;
   int avail_count = 0;
   int deriv_count = 0;
   int check_counter = 0;
   int event_code;

   PAPI_event_info_t n_info;

   /* Parse command line arguments */

   for( args = 1; args < argc; args++ ) {
      if ( strstr( argv[args], "-e" ) ) {
	 print_event_info = 1;
         if( (args+1 >= argc) || ( argv[args+1] == NULL ) || ( strlen( argv[args+1] ) == 0 ) ) {
	    print_help( argv );
	    exit( 1 );
	 }
	 name = argv[args + 1];
      }
      else if ( strstr( argv[args], "-c" ) || strstr (argv[args], "--check") )
      {
	 print_avail_only = PAPI_PRESET_ENUM_AVAIL;
         check_counter = 1;
      }
      else if ( strstr( argv[args], "-a" ))
	 print_avail_only = PAPI_PRESET_ENUM_AVAIL;
      else if ( strstr( argv[args], "-d" ) )
	 print_tabular = 0;
      else if ( strstr( argv[args], "-h" ) ) {
	 print_help( argv );
	 exit( 1 );
      } else if ( strstr( argv[args], "--br" ) )
	 filter |= PAPI_PRESET_BIT_BR;
      else if ( strstr( argv[args], "--cache" ) )
	 filter |= PAPI_PRESET_BIT_CACH;
      else if ( strstr( argv[args], "--cnd" ) )
	 filter |= PAPI_PRESET_BIT_CND;
      else if ( strstr( argv[args], "--fp" ) )
	 filter |= PAPI_PRESET_BIT_FP;
      else if ( strstr( argv[args], "--ins" ) )
	 filter |= PAPI_PRESET_BIT_INS;
      else if ( strstr( argv[args], "--idl" ) )
	 filter |= PAPI_PRESET_BIT_IDL;
      else if ( strstr( argv[args], "--l1" ) )
	 filter |= PAPI_PRESET_BIT_L1;
      else if ( strstr( argv[args], "--l2" ) )
	 filter |= PAPI_PRESET_BIT_L2;
      else if ( strstr( argv[args], "--l3" ) )
	 filter |= PAPI_PRESET_BIT_L3;
      else if ( strstr( argv[args], "--mem" ) )
	 filter |= PAPI_PRESET_BIT_BR;
      else if ( strstr( argv[args], "--msc" ) )
	 filter |= PAPI_PRESET_BIT_MSC;
      else if ( strstr( argv[args], "--tlb" ) )
	 filter |= PAPI_PRESET_BIT_TLB;
   }

   if ( filter == 0 ) {
      filter = ( unsigned int ) ( -1 );
   }

   /* Init PAPI */

   retval = PAPI_library_init( PAPI_VER_CURRENT );
   if ( retval != PAPI_VER_CURRENT ) {
	fprintf(stderr,"Error!  PAPI library mismatch!\n");
	return 1;
   }


	retval = PAPI_set_debug( PAPI_VERB_ECONT );
	if ( retval != PAPI_OK ) {
		fprintf(stderr,"Error with PAPI_set debug!\n");
		return 1;
	}

   /*   retval=papi_print_header("Available PAPI preset and user defined events plus hardware information.\n",
			       &hwinfo );*/
	if ( retval != PAPI_OK ) {
		fprintf(stderr,"Error with PAPI_get_hardware_info!\n");
		return 1;
	}

      /* Code for info on just one event */

      if ( print_event_info ) {

	 if ( PAPI_event_name_to_code( name, &event_code ) == PAPI_OK ) {
	    if ( PAPI_get_event_info( event_code, &info ) == PAPI_OK ) {

	       if ( event_code & PAPI_PRESET_MASK ) {
		  printf( "%-30s%s\n%-30s%#-10x\n%-30s%d\n",
			  "Event name:", info.symbol, "Event Code:",
			  info.event_code, "Number of Native Events:",
			  info.count );
		  printf( "%-29s|%s|\n%-29s|%s|\n%-29s|%s|\n",
			  "Short Description:", info.short_descr,
			  "Long Description:", info.long_descr,
			  "Developer's Notes:", info.note );
		  printf( "%-29s|%s|\n%-29s|%s|\n", "Derived Type:",
			  info.derived, "Postfix Processing String:",
			  info.postfix );

		  for( j = 0; j < ( int ) info.count; j++ ) {
		     printf( " Native Code[%d]: %#x |%s|\n", j,
			     info.code[j], info.name[j] );
		     PAPI_get_event_info( (int) info.code[j], &n_info );
		     printf(" Number of Register Values: %d\n", n_info.count );
		     for( k = 0; k < ( int ) n_info.count; k++ ) {
			printf( " Register[%2d]: %#08x |%s|\n", k,
				n_info.code[k], n_info.name[k] );
		     }
		     printf( " Native Event Description: |%s|\n\n",
			     n_info.long_descr );
		  }

		  if (!is_preset_event_available(name)) {
		    printf("\nPRESET event %s is NOT available on this architecture!\n\n", name);
		  }

	       } else {	 /* must be a native event code */
		  printf( "%-30s%s\n%-30s%#-10x\n%-30s%d\n",
			  "Event name:", info.symbol, "Event Code:",
			  info.event_code, "Number of Register Values:",
			  info.count );
		  printf( "%-29s|%s|\n", "Description:", info.long_descr );
		  for ( k = 0; k < ( int ) info.count; k++ ) {
		      printf( " Register[%2d]: %#08x |%s|\n", k,
			      info.code[k], info.name[k] );
		  }

		  /* if unit masks exist but none are specified, process all */
		  if ( !strchr( name, ':' ) ) {
		     if ( 1 ) {
			if ( PAPI_enum_event( &event_code, PAPI_NTV_ENUM_UMASKS ) == PAPI_OK ) {
			   printf( "\nUnit Masks:\n" );
			   do {
			      retval = PAPI_get_event_info(event_code, &info );
			      if ( retval == PAPI_OK ) {
				 if ( parse_unit_masks( &info ) ) {
				    printf( "%-29s|%s|%s|\n",
					    " Mask Info:", info.symbol,
					    info.long_descr );
				    for ( k = 0; k < ( int ) info.count;k++ ) {
					printf( "  Register[%2d]:  %#08x  |%s|\n",
						k, info.code[k], info.name[k] );
				    }
				 }
			      }
			   } while ( PAPI_enum_event( &event_code,
					  PAPI_NTV_ENUM_UMASKS ) == PAPI_OK );
			}
		     }
		  }
	       }
	    }
	 } else {
	    printf( "Sorry, an event by the name '%s' could not be found.\n"
                    " Is it typed correctly?\n\n", name );
	 }
      } else {

	 /* Print *ALL* Events */

  for (i=0 ; i<2 ; i++) {
	// set the event code to fetch preset events the first time through loop and user events the second time through the loop
	if (i== 0) {
		event_code = 0 | PAPI_PRESET_MASK;
	} else {
		event_code = 0 | PAPI_UE_MASK;
	}

	/* For consistency, always ASK FOR the first event, if there is not one then nothing to process */
	if (PAPI_enum_event( &event_code, PAPI_ENUM_FIRST ) != PAPI_OK) {
		 continue;
	}

	// print heading to show which kind of events follow
	if (i== 0) {
		printf( "================================================================================\n" );
		printf( "  PAPI Preset Events\n" );
		printf( "================================================================================\n" );
	} else {
		printf( "\n");       // put a blank line after the presets before strarting the user events
		printf( "================================================================================\n" );
		printf( "  User Defined Events\n" );
		printf( "================================================================================\n" );
	}

	 if ( print_tabular ) {
	    printf( "    Name        Code    " );
	    if ( !print_avail_only ) {
	       printf( "Avail " );
	    }
	    printf( "Deriv Description (Note)\n" );
	 } else {
	    printf( "%-13s%-11s%-8s%-16s\n |Long Description|\n"
                    " |Developer's Notes|\n |Derived|\n |PostFix|\n"
                    " Native Code[n]: <hex> |name|\n",
		    "Symbol", "Event Code", "Count", "|Short Description|" );
	 }
	 do {
	    if ( PAPI_get_event_info( event_code, &info ) == PAPI_OK ) {
	       if ( print_tabular ) {
	      // if this is a user defined event or its a preset and matches the preset event filters, display its information
		  if ( (i==1) || (filter & info.event_type)) {
		     if ( print_avail_only ) {
		        if ( info.count ) {
                   if ( (check_counter && checkCounter (event_code)) || !check_counter)
                   {
                      printf( "%-13s%#x  %-5s%s",
                         info.symbol,
                         info.event_code,
                         is_derived( &info ), info.long_descr );
                   }
			}
		        if ( info.note[0] ) {
			   printf( " (%s)", info.note );
			}
			printf( "\n" );
		     } else {
			printf( "%-13s%#x  %-6s%-4s %s",
				info.symbol,
				info.event_code,
				( info.count ? "Yes" : "No" ),
				is_derived( &info ), info.long_descr );
			if ( info.note[0] ) {
			   printf( " (%s)", info.note );
			}
			printf( "\n" );
		     }
		     tot_count++;
		     if ( info.count ) {
	            if ((check_counter && checkCounter (event_code)) || !check_counter )
	              avail_count++;
		     }
		     if ( !strcmp( is_derived( &info ), "Yes" ) ) {
			deriv_count++;
		     }
		  }
	       } else {
		  if ( ( print_avail_only && info.count ) ||
		       ( print_avail_only == 0 ) )
	      {
	         if ((check_counter && checkCounter (event_code)) || !check_counter)
	         {
	           printf( "%s\t%#x\t%d\t|%s|\n |%s|\n"
			     " |%s|\n |%s|\n |%s|\n",
			     info.symbol, info.event_code, info.count,
			     info.short_descr, info.long_descr, info.note,
			     info.derived, info.postfix );
	           for ( j = 0; j < ( int ) info.count; j++ ) {
	              printf( " Native Code[%d]: %#x |%s|\n", j,
	              info.code[j], info.name[j] );
	           }
             }
		  }
		  tot_count++;
		  if ( info.count ) {
	         if ((check_counter && checkCounter (event_code)) || !check_counter )
		        avail_count++;
		  }
		  if ( !strcmp( is_derived( &info ), "Yes" ) ) {
		     deriv_count++;
		  }
	       }
	    }
	 } while (PAPI_enum_event( &event_code, print_avail_only ) == PAPI_OK);
  }
      }

	printf( "--------------------------------------------------------------------------------\n" );

	if ( !print_event_info ) {
		if ( print_avail_only ) {
			printf( "Of %d available events, %d ", avail_count, deriv_count );
		} else {
			printf( "Of %d possible events, %d are available, of which %d ",
				tot_count, avail_count, deriv_count );
		}
		if ( deriv_count == 1 ) {
			printf( "is derived.\n\n" );
		} else {
			printf( "are derived.\n\n" );
		}

		if (avail_count==0) {
			printf("No events detected!  Check papi_component_avail to find out why.\n");
			printf("\n");
		}
	}

	return 0;

}
