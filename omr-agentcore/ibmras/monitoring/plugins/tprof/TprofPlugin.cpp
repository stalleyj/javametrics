
#include "TprofPlugin.h"
#include <cstring>
#include <string>
#include <sstream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
//#include <fcntl.h>
#include <iostream>

#if defined(_LINUX)
#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#endif

#if defined(__MACH__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <mach/mach.h>
#endif

#if defined(_WINDOWS)
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <winbase.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#if defined(AIXPPC)
#include <unistd.h>
#include <alloca.h>
#include <procinfo.h>
#include <sys/vminfo.h>
#include <sys/procfs.h>
#include <sys/resource.h>
#include <sys/types.h>
#endif

#define TPROF_PULL_INTERVAL 2
#define DEFAULT_CAPACITY 1024*10
#define MAX_BUFFER_SIZE 0x1000000

extern "C"{
#include "perfutil.h"
#include "bputil_common.h"
#include "perfmisc.h"
char *raw_buffer;
int rawdata_size;
//extern void my_init();
//extern void my_fini();
/*void swtrace_off();
void SetTraceAnonMTE();
int TraceSetMteSize(int);
int TraceInit(int);
int TraceSetMode(int); */

MAPPED_DATA *pmd = NULL;
typedef struct tprof_data_st{
        int pid;
        int tid;
        char *mod_name;
        char *sym_name;
        struct tprof_data_st *next;
}TPROF_DATA_ST;
TPROF_DATA_ST * mypost();
void free_tprof_data_st(TPROF_DATA_ST *);
}


//
// append_kallsyms_and_modules()
// *****************************
//
void swtrace_off(void)
{
   int rc;

   std::cout << "<<<swtrace turning off..." << std::endl; 
   rc = TraceOff();
   std::cout << "<<<swtrace turned off..." << std::endl; 
   switch (rc) {
   case PU_ERROR_BUFFER_FULL:
      fprintf(stderr, "WARNING: Trace was already off\n");
      fprintf(stderr, "Could indicate a buffer full condition\n");
      exit(0);
   case 0:
      break;
   default:
      fprintf(stderr, "ERROR (swtrace): Failed to turn off tracing \n");
      exit(1);
   }

}


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace common {
namespace tprofplugin {

TprofPlugin* TprofPlugin::instance = 0;
agentCoreFunctions TprofPlugin::aCF;

TprofPlugin::TprofPlugin(uint32 provID):
		provID(provID), noFailures(false){
}

TprofPlugin* TprofPlugin::getInstance() {
		return instance;
}

TprofPlugin::~TprofPlugin(){}

int TprofPlugin::start() {
	int rc;
	aCF.logMessage(ibmras::common::logging::debug, ">>>TprofPlugin::start()");
	std::cout << "TOBES    >>>TprofPlugin::start" << std::endl; 
	noFailures = true;
//	my_init();
	std::cout << ">>>after init" << std::endl; 
	// do some initialize work 
	//set buffer size and mte buffer size
	pmd = GetMappedDataAddress();            // Get ptr to DD mapped data
	TraceSetMteSize(0x100000*5);
	SetTraceAnonMTE(0);
	// set mode to continuos mode
	TraceSetMode(COLLECTION_MODE_CONTINUOS);
	TraceInit(0x100000*5);

	// setrate disable and enable 16
	SetProfilerRate(0);

	rc = TraceDisable(0);
	if(rc != 0)
 	{
		std::cout << ">>>ERROR: Failed to disable tracing, Is the trace devicd driver, pidd active?" << std::endl; 
		exit(1);
	}

	rc = TraceEnable(16);
	if(rc != 0)
	{
		std::cout << "ERROR: Failed to enable major code 16" << std::endl;
		exit(1);
	}

	std::cout << "<<<TprofPlugin::initialize()" << std::endl; 
	
	// turn on PI trace
	rc = TraceOn();
	if (rc != 0) {
		std::cout << "ERROR: Failed to turn on tracing" << std::endl;
		if (PU_ERROR_INSTALLING_REQ_HOOKS == rc) {
			fprintf(stderr, "OProfile not installed!!!\n");
			// turn tracing off
			swtrace_off();
		}
		else
			fprintf(stderr, "Is the trace devicd driver, pidd active?\n");
		exit(1);
	}

	std::cout << ">>>TprofPlugin::traceOn" << std::endl; 
	// malloc a buffer
	raw_buffer = (char*) malloc(sizeof(char)*MAX_BUFFER_SIZE);
	sleep(1);
	aCF.logMessage(ibmras::common::logging::debug, "<<<TprofPlugin::start()");
	return 0;
}

int TprofPlugin::stop() {
	aCF.logMessage(ibmras::common::logging::debug, ">>>TprofPlugin::stop()");
	swtrace_off();
	//my_fini();
	aCF.logMessage(ibmras::common::logging::debug, "<<<TprofPlugin::stop()");
	return 0;
}

pullsource* TprofPlugin::createSource(agentCoreFunctions aCF, uint32 provID) {
	aCF.logMessage(ibmras::common::logging::fine, "[tprof] Registering pull source");
	if(!instance) {
		TprofPlugin::aCF = aCF;
		instance = new TprofPlugin(provID);
	}
	return instance->createPullSource(0, "tprof");
}

pullsource* TprofPlugin::createPullSource(uint32 srcid, const char* name) {
		pullsource *src = new pullsource();
		src->header.name = name;
		std::string desc("Description for ");
		desc.append(name);
		src->header.description = NewCString(desc);
		src->header.sourceID = srcid;
		src->next = NULL;
		src->header.capacity = DEFAULT_CAPACITY;
		src->callback = pullWrapper;
		src->complete = pullCompleteWrapper;
		src->pullInterval = TPROF_PULL_INTERVAL; // seconds
		return src;
}

monitordata* TprofPlugin::OnRequestData() {
	monitordata *data = new monitordata;
	data->provID = provID;
	data->size = 0;
	data->data = NULL;

	data->persistent = false;
	data->sourceID = 0;
	
	// call get data size every 2 seconds
	//before getting data, do clean the whole buffer every time
	memset(raw_buffer, 0, sizeof(char)*MAX_BUFFER_SIZE );
	rawdata_size = GetBufferData(raw_buffer, MAX_BUFFER_SIZE);
	if(rawdata_size == -1)
	{
		printf("can't get data from kernel");
//		swtrace_off();
	}

	// calling mypost to get data processed
	TPROF_DATA_ST* head = mypost();
	std::stringstream ss;
	ss << "tprofdatastart" << COMMA << getTime() << "\n";
	while (head)
	{
		TPROF_DATA_ST* temp = head->next;
		if(temp != NULL){
	 		ss << temp->pid<< COMMA;
			ss << temp->tid<< COMMA;
			ss << temp->mod_name<< COMMA;
			ss << temp->sym_name << "\n" ;		
	            free_tprof_data_st(head);
		}

	    head = temp;
	}
	ss << "tprofdataend" << "\n";

	std::string tprofdata = ss.str();

	int len = tprofdata.length();
	char* sval = new char[len + 1];
	if (sval) {
		strcpy(sval, tprofdata.c_str());
		data->size = len;
		data->data = sval;
	}
	return data;
}

void TprofPlugin::OnComplete(monitordata* data) {
	if (data != NULL) {
		if (data->data != NULL) {
			delete[] data->data;
		}
		delete data;
	}
}

/*****************************************************************************
 * CALLBACK WRAPPERS
 *****************************************************************************/

extern "C" monitordata* pullWrapper() {
		return TprofPlugin::getInstance()->OnRequestData();
}

extern "C" void pullCompleteWrapper(monitordata* data) {
	TprofPlugin::getInstance()->OnComplete(data);
}

/*****************************************************************************
 * FUNCTIONS EXPORTED BY THE LIBRARY
 *****************************************************************************/

extern "C" {
pullsource* ibmras_monitoring_registerPullSource(agentCoreFunctions aCF, uint32 provID) {
	aCF.logMessage(ibmras::common::logging::debug, "[tprofdata] Registering pull source");
	std::cout << "TOBES == register pull source" << "\n";
	pullsource *src = TprofPlugin::createSource(aCF, provID);
	return src;
}

int ibmras_monitoring_plugin_init(const char* properties) {
	return 0;
}

int ibmras_monitoring_plugin_start() {
	std::cout << "TOBES 2" << "\n";
	TprofPlugin::aCF.logMessage(ibmras::common::logging::fine, "[tprofdata] Starting");
	TprofPlugin::getInstance()->start();
	return 0;
}

int ibmras_monitoring_plugin_stop() {
	TprofPlugin::aCF.logMessage(ibmras::common::logging::fine, "[tprofdata] Stopping");
	TprofPlugin::getInstance()->stop();
	return 0;
}

const char* ibmras_monitoring_getVersion() {
		return PLUGIN_API_VERSION;
}
}

int64 TprofPlugin::getTime() {
#if defined(_LINUX) || defined(_AIX) || defined(__MACH__) || defined(__APPLE__)
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return ((int64) tv.tv_sec)*1000 + tv.tv_usec/1000;
#elif defined(_WINDOWS)
        LONGLONG time;
       	GetSystemTimeAsFileTime( (FILETIME*)&time );
       	return (int64) ((time - 116444736000000000) /10000);
#elif defined(_S390)
	int64 millisec = MAXPREC() / 8000;
	return millisec;
#else
	return -1;
#endif
}

char* TprofPlugin::NewCString(const std::string& s) {
		char *result = new char[s.length() + 1];
		std::strcpy(result, s.c_str());
		return result;
}


} //TprofPlugin
} //common
} //plugins
} //monitoring
} //ibmras


