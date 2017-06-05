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
#include "ibmras/common/logging.h"
#include "ibmras/vm/omr/healthcenter.h"
#include "ibmras/monitoring/plugins/omr/NativeMemoryDataProvider.h"
#include "omragent.h"
#include "stdlib.h"
#include <sys/time.h>


#define DEFAULT_CAPACITY 1024	//Capacity of the bucket that will host the pushsource

namespace plugins {
namespace omr {
namespace nativememory {

IBMRAS_DEFINE_LOGGER("NativeMemory");

ibmras::common::port::Lock* lock = new ibmras::common::port::Lock;

uint32 localprovid = 0;
uint32 srcid = 0;

omrRunTimeProviderParameters vmData;

monitordata* NativeMemoryDataProvider::generateData(uint32 srcid) {
	IBMRAS_DEBUG(debug, "generateData start\n");
	monitordata* data = new monitordata;
	data->persistent = false;
	data->provID = plugins::omr::nativememory::localprovid;

	data->data = getNativeMemoryData();
	if (data->data == NULL) {
		data->size = 0;
	} else {
		data->size = strlen(data->data);
	}

	data->sourceID = plugins::omr::nativememory::srcid;
	return data;
}

monitordata* NativeMemoryDataProvider::pullCallback() {
	/**
	 * This method is called by the agent to get the information from the pullsource,
	 * in this case the data is produced from the "generateData" method.
	 */
	IBMRAS_DEBUG(debug, "pullCallback start\n");
	plugins::omr::nativememory::lock->acquire();
	IBMRAS_DEBUG(debug, "Generating data for pull from agent");
	monitordata* data = generateData(srcid);
	plugins::omr::nativememory::lock->release();
	return data;
}



void NativeMemoryDataProvider::pullComplete(monitordata* data) {
	if (data != NULL) {
		if (data->data != NULL) {
			delete[] data->data;
		}
		delete data;
	}
}




pullsource* NativeMemoryDataProvider::registerPullSource(agentCoreFunctions aCF,uint32 provID) {

	IBMRAS_DEBUG(info, "Registering pull sources");
	pullsource *src = new pullsource();
	src->header.name = "nativememory";
	src->header.description = ("This returns the Native Memory data");
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

NativeMemoryDataProvider::NativeMemoryDataProvider(
		omrRunTimeProviderParameters oRTPP) {
	IBMRAS_DEBUG(debug, "NativeMemoryDataProvider constructor\n");
	vmData = oRTPP;
	name = "nativememory";
	pull = registerPullSource;
	start = memstart;
	stop = memstop;
	push = NULL;
	type = ibmras::monitoring::plugin::data;
	confactory = NULL;
	recvfactory = NULL;
}


int NativeMemoryDataProvider::memstart() {

	/**
	 * This method is exposed and will be called by the agent when starting all the plugins, anything
	 * required to start the plugin has to be added here. In this case, there is just a pullsource, it
	 * does not require any kind of initialization.
	 */
	return 0;
}

int NativeMemoryDataProvider::memstop() {
	/**
	 * The stop method will be called by the agent on shutdown, here is where any cleanup has to be done
	 */
	return 0;

}
NativeMemoryDataProvider* instance = NULL;
NativeMemoryDataProvider* NativeMemoryDataProvider::getInstance(omrRunTimeProviderParameters oRTPP) {
	if (!instance) {
		instance = new NativeMemoryDataProvider(oRTPP);
	}
	return instance;
}

NativeMemoryDataProvider* NativeMemoryDataProvider::getInstance() {
	if (!instance) {
		return NULL;
	}
	return instance;
}

/* ====================================== */
/* Native Memory functions                */
/* ====================================== */


char* NativeMemoryDataProvider::getNativeMemoryData()
{

    int err;
    U_64 freePhysicalMemorySize;
    U_64 processVirtualMemorySize;
    U_64 processPrivateMemorySize;
    U_64 processPhysicalMemorySize;
    U_64 totalPhysicalMemorySize = 0; // currently this does not exist as an mbean
    char * report;
	report = new char[200];
	int rc;
	OMR_VMThread *vmThread = NULL;
	unsigned long long millisecondsSinceEpoch;

    IBMRAS_DEBUG(debug, "getNativeMemoryData start\n");

    err = vmData.omrti->BindCurrentThread(vmData.theVm, "HC getNativeMemoryData", &vmThread);

	if (OMR_ERROR_NONE != err) {
		IBMRAS_DEBUG(debug, "getNativeMemoryData exit as unable to bindCurrentThread");
		return NULL;
	}

    /* Get the freePhysicalMemorySize */
	rc = vmData.omrti->GetFreePhysicalMemorySize(vmThread, &freePhysicalMemorySize);
    if (OMR_ERROR_NONE != rc)
    {
    	IBMRAS_DEBUG_1(debug, "Problem calling GetFreePhysicalMemorySize: %d", rc);
       	goto cleanup;
    }
    /* Get the processVirtualMemorySize */
	rc = vmData.omrti->GetProcessVirtualMemorySize(vmThread, &processVirtualMemorySize);
	if (OMR_ERROR_NONE != rc)
	{
		IBMRAS_DEBUG_1(debug, "Problem calling GetProcessVirtualMemorySize: %d", rc);
		goto cleanup;
	}
    /* Get the processPrivateMemorySize */
	rc = vmData.omrti->GetProcessPrivateMemorySize(vmThread, &processPrivateMemorySize);
	if (OMR_ERROR_NONE != rc)
	{
		IBMRAS_DEBUG_1(debug, "Problem calling GetProcessPrivateMemorySize: %d", rc);
		goto cleanup;
	}
    /* Get the processPhysicalMemorySize */
	rc = vmData.omrti->GetProcessPhysicalMemorySize(vmThread, &processPhysicalMemorySize);
	if (OMR_ERROR_NONE != rc)
	{
		IBMRAS_DEBUG_1(debug, "Problem calling GetProcessPhysicalMemorySize: %d", rc);
		goto cleanup;
	}


#if defined(WINDOWS)
	// work out how to get windows time
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	millisecondsSinceEpoch =
	    (unsigned long long)(tv.tv_sec) * 1000 +
	    (unsigned long long)(tv.tv_usec) / 1000;
#endif

	sprintf(report,
//			"MemorySource,%llu,physicalmemory=%llu,privatememory=%llu,virtualmemory=%llu,freephysicalmemory=%llu,totalphysicalmemory=%llu\n",
			"MemorySource,%llu,physicalmemory=%llu,privatememory=%llu,virtualmemory=%llu,freephysicalmemory=%llu\n",
			millisecondsSinceEpoch, processPhysicalMemorySize,
			processPrivateMemorySize, processVirtualMemorySize,
			freePhysicalMemorySize); //, totalPhysicalMemorySize);

	IBMRAS_DEBUG_1(debug, "%s", report);

    cleanup:
    IBMRAS_DEBUG(debug, "in cleanup block\n");
	if (OMR_ERROR_NONE == err) {
    	err = vmData.omrti->UnbindCurrentThread(vmThread);
	}

	if (OMR_ERROR_NONE != err) {
		IBMRAS_DEBUG(debug, "getNativeMemoryData exit as unable to unbindCurrentThread");
		return NULL;
	}

	if (OMR_ERROR_NONE != rc) {
		IBMRAS_DEBUG(debug, "getNativeMemoryData exit as unable to get all the data");
		return NULL;
	}

	IBMRAS_DEBUG(debug, "getNativeMemoryData exit");
	ibmras::common::util::native2Ascii(report);
    return report;
}

}
}
}/*end of namespace plugins::omr::cpu*/

