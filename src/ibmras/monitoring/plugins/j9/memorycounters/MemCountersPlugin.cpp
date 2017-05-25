/*
 * MemCountersPlugin.cpp
 *
 *  Created on: 20 Feb 2016
 *      Author: Admin
 */

#if defined(_ZOS)
#define _XOPEN_SOURCE_EXTENDED 1
#undef _ALL_SOURCE
#endif

#include <jni.h>
#include "ibmras/monitoring/plugins/j9/memorycounters/MemCountersPlugin.h"
#include "ibmras/common/logging.h"
#include "ibmjvmti.h"

#include "ibmras/monitoring/agent/Agent.h"

#include <cstring>
#include <string>
#include <sstream>
#include <ctime>
#include <stdio.h>
#include "ibmras/common/util/memUtils.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/util/sysUtils.h"
#include "ibmras/common/MemoryManager.h"
#include <jvmti.h>
#include "ibmras/monitoring/plugins/j9/Util.h"


#if defined(WINDOWS)
#define JLONG_FMT_STR "%I64d"
#else /* Unix platforms */
#define JLONG_FMT_STR "%lld"
#endif

#if defined(WINDOWS)
#include <windows.h>
#include <Psapi.h>
#elif defined(LINUX)
#include <sys/time.h>
#elif defined(_ZOS)
#include <sys/time.h>
#endif


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace memorycounters {

IBMRAS_DEFINE_LOGGER("memorycountersplugin")
;

MemCountersPlugin* instance = NULL;
static uint32 provID;
bool enabled = true;

MemCountersPlugin::MemCountersPlugin(jvmFunctions jvmF) :
				env(NULL), jvmF(jvmF) {
	IBMRAS_DEBUG(debug, ">>>MemCountersPlugin::MemCountersPlugin");
	name = "memorycounters";
	pull = registerPullSource;
	push = NULL;
	start = startWrapper;
	stop = stopWrapper;
	getVersion = getMCVersion;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY)MemCountersPlugin::getReceiver;
	confactory = NULL;
	IBMRAS_DEBUG(debug, "<<<MemCountersPlugin::MemCountersPlugin");
}
MemCountersPlugin::~MemCountersPlugin() {
}

MemCountersPlugin* MemCountersPlugin::getPlugin(jvmFunctions jvmF) {
	IBMRAS_DEBUG(debug, ">>>MemCountersPlugin::getPlugin");
    // Does the vm have this function available	
	if (!jvmF.jvmtiRegisterTraceSubscriber == NULL) {
        if(!instance) {
            instance = new MemCountersPlugin(jvmF);
        }
    }
	return instance;
}

void* MemCountersPlugin::getReceiver() {
	IBMRAS_DEBUG(debug, ">>>MemCountersPlugin::getReceiver");
	if(!instance) {
		IBMRAS_DEBUG(debug, "<<<MemCountersPlugin::getReceiver[NULL]");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "<<<MemCountersPlugin::getReceiver[OK]");
	return (Receiver*)instance;
}

pullsource* MemCountersPlugin::registerPullSource(agentCoreFunctions aCF,uint32 prov) {
	IBMRAS_DEBUG(debug, ">>>MemCountersPlugin::registerPullSource");
	pullsource* src = new pullsource();

	src->header.name = "memorycounters";
	provID = prov;
	src->header.description = "Memory counters information";
	src->header.sourceID = 0;
	src->header.capacity = 8 * 1024;
	src->next = NULL;
	src->callback = pullWrapper;
	src->complete = pullCompleteWrapper;
	src->pullInterval = 5;
	IBMRAS_DEBUG(debug, "<<<MemCountersPlugin::registerPullSource");
	return src;
}

/***************************************************************************************
 * Receiver-related methods:
 * *************************************************************************************/

void MemCountersPlugin::publishConfig() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "memorycounters_subsystem=";
	if (isEnabled()) {
		msg += "on";
	} else {
		msg += "off";
	}
	conMan->sendMessage("configuration/memorycounters", msg.length(),
			(void*) msg.c_str());
}

void MemCountersPlugin::receiveMessage(const std::string& id, uint32 size, void* data) {

	std::string message((const char*) data, size);

	if (id == "memorycounters") {
		std::size_t found = message.find(',');
		std::string command = message.substr(0, found);
		std::string rest = message.substr(found + 1);

		ibmras::monitoring::plugins::j9::memorycounters::MemCountersPlugin::setState(
				command);
	}
}

void MemCountersPlugin::setState(const std::string &newState) {
	enabled = ibmras::common::util::equalsIgnoreCase(newState, "on");
	if (instance) {
		instance->publishConfig();
	}
}

int MemCountersPlugin::startReceiver() {
	publishConfig();
	return 0;
}

int MemCountersPlugin::stopReceiver() {
	return 0;
}

bool MemCountersPlugin::isEnabled() {
	return enabled;
}

/***************************************************************************************
 * Data-production/data-transfer related methods:
 * *************************************************************************************/

monitordata* MemCountersPlugin::pullInt() {
	IBMRAS_DEBUG(debug, ">>>MemCountersPlugin::pullInt");

	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (memorycounters)", jvmF.theVM, true);

	if(!env) {
		IBMRAS_DEBUG(warning, "The JNI env was not set up");
		IBMRAS_DEBUG(debug, "<<<MemCountersPlugin::pullInt[JNIenv=NULL]");
		return NULL;
	}

	monitordata* data = new monitordata;
	data->size = 0;
	data->data = NULL;

	if (isEnabled()) {
		data->persistent = false;
		data->provID = provID;
		data->sourceID = 0;

		data->data = getMemoryCounters(env, jvmF);
		if (data->data) {
			data->size = strlen(data->data);
		}
	}
	IBMRAS_DEBUG(debug, "<<<MemCountersPlugin::pullInt");
	return data;
}

void MemCountersPlugin::pullcompleteInt(monitordata* mdata) {
	if (mdata) {
		hc_dealloc((unsigned char**) (&(mdata->data)));
		delete mdata;
	} else {
		if (env) {
			IBMRAS_DEBUG_1(debug, "Detaching thread %s", name.c_str());
			jvmF.theVM->DetachCurrentThread();
			env = NULL;
		}
	}
}

char* getMemoryCounters(JNIEnv *env, jvmFunctions jvmF) {
	IBMRAS_DEBUG(debug, ">>>..j9::memorycounters::getMemoryCounters");
	jint total_categories;
	jint written_count = 0;
	jvmtiMemoryCategory* categories_buffer = NULL;
	char* report = NULL;
	jvmtiError err;
	jint i;
	char *finalReport = NULL;
	char **memcounterarray = NULL;
	char* category_name = NULL;
	jvmFunctions tdpp = jvmF;
	if (!enabled) {
		return NULL;
	}

	if (tdpp.jvmtiGetMemoryCategories == NULL) {
		enabled = false;
		IBMRAS_DEBUG(debug, "<<<..j9::memorycounters::getMemoryCounters[jvmtiGetMemoryCategories == NULL]");
		return NULL;
	}


	const char* memCounterFormatString =
			"memcounterinfo, %s, "JLONG_FMT_STR", "JLONG_FMT_STR", "JLONG_FMT_STR", "JLONG_FMT_STR", %ld, %ld, %ld\n";
	char buffer[1000];

	/* Find out how much categories buffer to allocate */
	err = tdpp.jvmtiGetMemoryCategories(tdpp.pti,
			COM_IBM_GET_MEMORY_CATEGORIES_VERSION_1, 0, NULL, NULL,
			&total_categories);
	if (JVMTI_ERROR_NONE != err) {
		goto cleanup;
	}

	categories_buffer =
			(jvmtiMemoryCategory*) ibmras::monitoring::plugins::j9::hc_alloc(
					total_categories * sizeof(jvmtiMemoryCategory));
	if (categories_buffer == NULL) {
		goto cleanup;
	}

	err = tdpp.jvmtiGetMemoryCategories(tdpp.pti,
			COM_IBM_GET_MEMORY_CATEGORIES_VERSION_1, total_categories,
			categories_buffer, &written_count, &total_categories);
	if (JVMTI_ERROR_NONE != err) {
		goto cleanup;
	}

	if (written_count < total_categories) {
		goto cleanup;
	}

	memcounterarray = (char**) ibmras::monitoring::plugins::j9::hc_alloc(
			(written_count) * sizeof(char *));
	if (memcounterarray == NULL) {
		goto cleanup;
	}

	for (i = 0; i < written_count; i++) {

		long firstChildNumber = 0;
		long nextSiblingNumber = 0;
		long parentNumber = 0;

		if (NULL != categories_buffer[i].firstChild) {
			firstChildNumber = (long) categories_buffer[i].firstChild
					- (long) &categories_buffer[0];
			if (firstChildNumber > 0) {
				firstChildNumber = firstChildNumber
						/ sizeof(jvmtiMemoryCategory);
			}
		} else {
			firstChildNumber = -1;
		}

		if (NULL != categories_buffer[i].nextSibling) {
			nextSiblingNumber = (long) categories_buffer[i].nextSibling
					- (long) &categories_buffer[0];
			if (nextSiblingNumber > 0) {
				nextSiblingNumber = nextSiblingNumber
						/ sizeof(jvmtiMemoryCategory);
			}
		} else {
			nextSiblingNumber = -1;
		}

		if (NULL != categories_buffer[i].parent) {
			parentNumber = (long) categories_buffer[i].parent
					- (long) &categories_buffer[0];
			if (parentNumber > 0) {
				parentNumber = parentNumber / sizeof(jvmtiMemoryCategory);
			}
		} else {
			parentNumber = -1;
		}

#if defined(_ZOS)
	char* catName = ibmras::common::util::createNativeString(categories_buffer[i].name);
#else
	const char* catName = categories_buffer[i].name;
#endif

		sprintf(buffer, memCounterFormatString, catName,
				categories_buffer[i].liveBytesShallow,
				categories_buffer[i].liveBytesDeep,
				categories_buffer[i].liveAllocationsShallow,
				categories_buffer[i].liveAllocationsDeep, firstChildNumber,
				nextSiblingNumber, parentNumber);
#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&catName);
#endif

		memcounterarray[i] =
				(char *) ibmras::monitoring::plugins::j9::hc_alloc(
						strlen(buffer) + 1);
		if (memcounterarray[i] == NULL) {
			goto cleanup;
		}
		strcpy(memcounterarray[i], buffer);
	}

	finalReport = ibmras::monitoring::plugins::j9::memorycounters::join_strings(
			memcounterarray, i);

	cleanup: ibmras::monitoring::plugins::j9::hc_dealloc(
			(unsigned char**) &category_name);
	ibmras::monitoring::plugins::j9::hc_dealloc(
			(unsigned char**) &categories_buffer);
	ibmras::monitoring::plugins::j9::memorycounters::dealloc_report_lines(memcounterarray,
			written_count);
	ibmras::monitoring::plugins::j9::hc_dealloc(
			(unsigned char**) &memcounterarray);

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
		report = reinterpret_cast<char*>(hc_alloc(data.length() + 1));
		if (report) {
			strcpy(report, data.c_str());
		}
	}

	ibmras::monitoring::plugins::j9::hc_dealloc(
			(unsigned char**) &finalReport);
	ibmras::common::util::native2Ascii(report);
	IBMRAS_DEBUG(debug, "<<<..j9::memorycounters::getMemoryCounters");
	return report;
}

/***************************************************************************************
 * Memory and string manipulation related utility methods:
 * *************************************************************************************/

/**********
 * utility function to free pointers in an array that have been malloced
 **********/
void dealloc_report_lines(char *lines[], int count) {
	int i;
	//IBMRAS_DEBUG(debug,  "> dealloc_report_lines");

	if (lines != NULL) {
		/* Need to free the memory for our array */
		for (i = 0; i < count; i++) {
			ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) &lines[i]);
		}
	}
	//IBMRAS_DEBUG(debug,  "< dealloc_report_lines");
}

/************
 * Function to join an array of strings                            *
 * this function allocates memory that must be freed by the caller *
 ************/
char* join_strings(char *strings[], int count) {
	char* str = NULL; /* Pointer to the joined strings  */
	size_t total_length = 0; /* Total length of joined strings */
	int i = 0; /* Loop counter                   */

	/* Find total length of joined strings */
	for (i = 0; i < count; i++) {
		if (strings[i] != NULL) {
			total_length += strlen(strings[i]);
		}
	}
	++total_length; /* For joined string terminator */

	str = (char*) ibmras::monitoring::plugins::j9::hc_alloc(total_length); /* Allocate memory for joined strings */
	if (NULL == str) {
		return NULL;
	}
	str[0] = '\0'; /* Empty string we can append to      */

	/* Append all the strings */
	for (i = 0; i < count; i++) {
		if (strings[i] != NULL) {
			strcat(str, strings[i]);
		}
	}

	return str;
}

/*-------------------------------------------------------------------------------------
 * These are the namespace functions that are used to avoid the restrictions imposed
 * by the defined typedefs for callback functions. Non-static member function pointers
 * have a different prototype than the one generically typedef'd in the headers.
 *-----------------------------------------------------------------------------------*/

monitordata* pullWrapper() {
		return instance->pullInt();
}

void pullCompleteWrapper(monitordata* data) {
	instance->pullcompleteInt(data);
}

int startWrapper() {
	return instance->startReceiver();
}

int stopWrapper() {
	return instance->stopReceiver();
}

const char* getMCVersion() {
	return "1.0";
}


}//memorycounters
}//j9
}//plugins
}//monitoring
}//ibmras
