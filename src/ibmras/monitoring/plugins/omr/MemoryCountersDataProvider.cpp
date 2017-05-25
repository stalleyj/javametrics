 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#include "ibmras/common/port/ThreadData.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include <string.h>
#include <sstream>
#include "ibmras/common/logging.h"
#include "ibmras/vm/omr/healthcenter.h"
#include "ibmras/monitoring/plugins/omr/MemoryCountersDataProvider.h"
#include "omragent.h"
#include "stdlib.h"
#include <sys/time.h>


#if defined (WINDOWS)
    #define LONG_FMT_STR "%I64d"
#else
    #define LONG_FMT_STR "%lld"
#endif

#define DEFAULT_CAPACITY 1024	//Capacity of the bucket that will host the pushsource

namespace plugins {
namespace omr {
namespace memorycounters {

IBMRAS_DEFINE_LOGGER("MemoryCounters");

ibmras::common::port::Lock* lock = new ibmras::common::port::Lock;

uint32 localprovid = 0;
uint32 srcid = 0;

omrRunTimeProviderParameters vmData;




monitordata* MemoryCountersDataProvider::generateData(uint32 srcid) {
	IBMRAS_DEBUG(debug, "generateData start\n");
	monitordata* data = new monitordata;
	data->persistent = false;
	data->provID = plugins::omr::memorycounters::localprovid;
	data->data = getMemoryCounters();
	if (data->data == NULL) {
		data->size = 0;
	} else {
		data->size = strlen(data->data);
	}
	data->sourceID = srcid;
	return data;
}

monitordata* MemoryCountersDataProvider::pullCallback() {
	/**
	 * This method is called by the agent to get the information from the pullsource,
	 * in this case the mock data is produced from the "generateData" method.
	 */
	IBMRAS_DEBUG(debug, "pullCallback start\n");
	plugins::omr::memorycounters::lock->acquire();
	IBMRAS_DEBUG(debug, "Generating data for pull from agent");
	monitordata* data = generateData(srcid);
	plugins::omr::memorycounters::lock->release();
	return data;
}



void MemoryCountersDataProvider::pullComplete(monitordata* data) {
	if (data != NULL) {
		if (data->data != NULL) {
			delete[] data->data;
		}
		delete data;
	}
}




pullsource* MemoryCountersDataProvider::registerPullSource(agentCoreFunctions aCF,uint32 provID) {

	IBMRAS_DEBUG(info, "Registering pull sources");
	pullsource *src = new pullsource();
	src->header.name = "memorycounters";
	src->header.description = ("This returns the native memory counters breakdown");
	src->header.sourceID = srcid;
	src->header.capacity = (DEFAULT_CAPACITY);
	src->next = NULL;
	src->callback = pullCallback;
	src->complete = pullComplete;
	src->pullInterval = 2;	/* space pull intervals apart for successive calls */
	localprovid = provID;
	IBMRAS_DEBUG(debug, "registerPullSource end\n");
	return src;
}

MemoryCountersDataProvider::MemoryCountersDataProvider(
		omrRunTimeProviderParameters oRTPP) {
	IBMRAS_DEBUG(debug, "MemoryCountersDataProvider constructor\n");
	vmData = oRTPP;
	name = "memorycounters";
	pull = registerPullSource;
	start = memstart;
	stop = memstop;
	push = NULL;
	type = ibmras::monitoring::plugin::data;
	confactory = NULL;
	recvfactory = NULL;
}


int MemoryCountersDataProvider::memstart() {

	/**
	 * This method is exposed and will be called by the agent when starting all the plugins, anything
	 * required to start the plugin has to be added here. In this case, there is just a pullsource, it
	 * does not require any kind of initialization.
	 */
	return 0;
}

int MemoryCountersDataProvider::memstop() {
	/**
	 * The stop method will be called by the agent on shutdown, here is where any cleanup has to be done
	 */
	return 0;

}

MemoryCountersDataProvider* instance = NULL;
MemoryCountersDataProvider* MemoryCountersDataProvider::getInstance(omrRunTimeProviderParameters oRTPP) {
	if (!instance) {
		instance = new MemoryCountersDataProvider(oRTPP);
	}
	return instance;
}

MemoryCountersDataProvider* MemoryCountersDataProvider::getInstance() {
	if (!instance) {
		return NULL;
	}
	return instance;
}
/* ====================================== */
/* Memory Counters functions              */
/* ====================================== */


char* MemoryCountersDataProvider::getMemoryCounters()
{
    I_32 total_categories;
    I_32 written_count = 0;
    OMR_TI_MemoryCategory* categories_buffer = NULL;

    int err;
    I_32 i;
    char * report;
    char *finalReport = NULL;
    char **memcounterarray = NULL;
    char* category_name = NULL;
	OMR_VMThread *vmThread = NULL;

	char* memCounterFormatString = "memcounterinfo, %s, "LONG_FMT_STR", "LONG_FMT_STR", "LONG_FMT_STR", "LONG_FMT_STR", %ld, %ld, %ld\n";
    char buffer[1000];
    IBMRAS_DEBUG(debug, "getMemoryCounters start\n");

	int rc;

	rc = vmData.omrti->BindCurrentThread(vmData.theVm, " HC getMemoryCounters", &vmThread);

	if (OMR_ERROR_NONE != rc) {
		IBMRAS_DEBUG(debug, "getMemoryCounters exit as unable to bindCurrentThread");
		return NULL;
	}


    /* Find out how much categories buffer to allocate */
    err = vmData.omrti->GetMemoryCategories(vmThread,  0, NULL, NULL, &total_categories);
    if (OMR_ERROR_NONE != err)
    {
    	IBMRAS_DEBUG_1(debug, "Problem querying memory categories for total: %d", err);
        goto cleanup;
    }


    categories_buffer = (OMR_TI_MemoryCategory*)malloc(total_categories * sizeof(OMR_TI_MemoryCategory));
    if (categories_buffer == NULL)
    {
        goto cleanup;
    }

    err = vmData.omrti->GetMemoryCategories(vmThread, total_categories, categories_buffer, &written_count, &total_categories);
    if (OMR_ERROR_NONE != err)
    {
    	IBMRAS_DEBUG_1(debug, "Problem querying memory categories for data: %d", err);
        goto cleanup;
    }

    if (written_count < total_categories)
    {
    	IBMRAS_DEBUG_2(debug, "Not enough categories read. Expected %d, got %d", total_categories, written_count);
        goto cleanup;
    }

    memcounterarray = (char**)malloc((written_count) * sizeof(char *));
    if (memcounterarray == NULL)
    {
        goto cleanup;
    }

    for (i = 0; i < written_count; i++)
    {

        long firstChildNumber = 0;
        long nextSiblingNumber = 0;
        long parentNumber = 0;

        if (NULL != categories_buffer[i].firstChild)
        {
            firstChildNumber = (long)categories_buffer[i].firstChild - (long)&categories_buffer[0];
            if (firstChildNumber > 0)
            {
                firstChildNumber = firstChildNumber / sizeof(OMR_TI_MemoryCategory);
            }
        } else
        {
            firstChildNumber = -1;
        }


        if (NULL != categories_buffer[i].nextSibling)
        {
            nextSiblingNumber = (long)categories_buffer[i].nextSibling - (long)&categories_buffer[0];
            if (nextSiblingNumber > 0)
            {
                nextSiblingNumber = nextSiblingNumber / sizeof(OMR_TI_MemoryCategory);
            }
        } else
        {
            nextSiblingNumber = -1;
        }


        if (NULL != categories_buffer[i].parent)
        {
            parentNumber = (long)categories_buffer[i].parent - (long)&categories_buffer[0];
            if (parentNumber > 0)
            {
                parentNumber = parentNumber / sizeof(OMR_TI_MemoryCategory);
            }
        } else
        {
            parentNumber = -1;
        }


        sprintf(buffer, memCounterFormatString, categories_buffer[i].name, categories_buffer[i].liveBytesShallow, categories_buffer[i].liveBytesDeep, categories_buffer[i].liveAllocationsShallow, categories_buffer[i].liveAllocationsDeep, firstChildNumber, nextSiblingNumber, parentNumber);
        memcounterarray[i] = (char *)malloc(strlen(buffer) + 1);
        if (memcounterarray[i] == NULL)
        {
            goto cleanup;
        }
        strcpy(memcounterarray[i], buffer);
    }

    finalReport = join_strings(memcounterarray, i);

    cleanup:
    IBMRAS_DEBUG(debug, "in cleanup block\n");
    hc_dealloc(category_name);
    hc_dealloc((char*)categories_buffer);
    dealloc_report_lines(memcounterarray, written_count);
    hc_dealloc((char*)memcounterarray);


	if (finalReport != NULL) {

		unsigned long long millisecondsSinceEpoch; // = ibmras::common::util::getMilliseconds();
#if defined(WINDOWS)

		SYSTEMTIME st;
		GetSystemTime(&st);

		millisecondsSinceEpoch = time(NULL)*1000+st.wMilliseconds;

#else
		struct timeval tv;
		gettimeofday(&tv, NULL);

		millisecondsSinceEpoch = (unsigned long long) (tv.tv_sec) * 1000
				+ (unsigned long long) (tv.tv_usec) / 1000;
#endif
		char tstamp[20];
		sprintf(tstamp, "%llu", millisecondsSinceEpoch);
		std::stringstream ss;
		ss << "smc\ntime stamp=";
		ss << tstamp << "\n";
		ss << finalReport;
		ss << "emc\n";

		std::string data = ss.str();
			report = reinterpret_cast<char*>(malloc(data.length() + 1));
			if (report) {
				strcpy(report, data.c_str());
			}
	}
	hc_dealloc(finalReport);
	if (OMR_ERROR_NONE == rc) {
		rc = vmData.omrti->UnbindCurrentThread(vmThread);
	}

	IBMRAS_DEBUG(debug, "getMemoryCounters exit");
	ibmras::common::util::native2Ascii(report);
	return report;
}


/************
 * Function to join an array of strings                            *
 * this function allocates memory that must be freed by the caller *
 ************/
char*
MemoryCountersDataProvider::join_strings(char *strings[], int count)
{
    char* str = NULL;             /* Pointer to the joined strings  */
    size_t total_length = 0;      /* Total length of joined strings */
    size_t length = 0;            /* Length of a string             */
    int i = 0;                    /* Loop counter                   */

    /* Find total length of joined strings */
    for (i = 0 ; i<count ; i++)
    {
        if (strings[i] != NULL)
        {
            total_length += strlen(strings[i]);
        }
    }
    ++total_length;     /* For joined string terminator */

    str = (char*)malloc(total_length);  /* Allocate memory for joined strings */
    if (NULL == str)
    {
        return NULL;
    }
    str[0] = '\0';                      /* Empty string we can append to      */

    /* Append all the strings */
    for (i = 0 ; i<count ; i++)
    {
        if (strings[i] != NULL)
        {
            strcat(str, strings[i]);
            length = strlen(str);
        }
    }

    return str;
}

/**********
 * utility function to free pointers in an array that have been malloced *
 **********/
void
MemoryCountersDataProvider::dealloc_report_lines(char *lines[], int count) {
    int i;
    IBMRAS_DEBUG(debug, "> dealloc_report_lines");

    if (lines != NULL)
    {
        /* Need to free the memory for our array */
        for (i=0; i<count; i++)
        {
        	 free(lines[i]);
        }
    }
    IBMRAS_DEBUG(debug, "< dealloc_report_lines");
}

void MemoryCountersDataProvider::hc_dealloc(char* buffer)
{
    int rc;

    if (buffer == NULL)
    {
    	IBMRAS_DEBUG(debug, "hc_dealloc called with null pointer");
        return;
    }
    IBMRAS_DEBUG_1(debug, "hc_dealloc: deallocating %p", *buffer);
     free(buffer);
     buffer = NULL;
}
}
}
}/*end of namespace plugins::omr::memorycounters*/

