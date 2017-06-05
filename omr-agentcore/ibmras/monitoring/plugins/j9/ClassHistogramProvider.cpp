 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#if defined(_ZOS)
#define _XOPEN_SOURCE_EXTENDED 1
#undef _ALL_SOURCE
#endif


#include "ibmras/monitoring/Plugin.h"
#include "ibmras/vm/java/healthcenter.h"
#include "ibmras/monitoring/plugins/j9/ClassHistogramProvider.h"
#include "ibmjvmti.h"
#include "jni.h"
#include "jvmti.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/MemoryManager.h"
#include "ibmras/monitoring/plugins/j9/Util.h"

#include "ibmras/monitoring/agent/Agent.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <ctime>

#if defined(WINDOWS)
#include <windows.h>
#include <Psapi.h>
#include <WinBase.h>
#endif

#if defined(LINUX)
#include <sys/time.h>
#endif

#if defined(_ZOS)
#include <sys/time.h>
#endif

#if defined (WINDOWS)
    #define JLONG_FMT_STR "%I64d"
#else
    #define JLONG_FMT_STR "%lld"
#endif

jlong * classSizes;
jlong * classCounts;

static jvmtiIterationControl JNICALL updateClassTotals(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data)
{
    jint *totalHeapUsedValue;
    totalHeapUsedValue = (jint *)user_data;
    (*totalHeapUsedValue)+=size;
    classSizes[class_tag]+=size;
    classCounts[class_tag] ++;
    return JVMTI_ITERATION_CONTINUE;
}



namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace classhistogram {

PUSH_CALLBACK sendClassHistogramData;
uint32 ClassHistogramProvider::providerID = 0;
IBMRAS_DEFINE_LOGGER("ClassHistogram");

const char* chpVersion = "1.0";

void publishConfig() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "capability.class.histogram=on";
	conMan->sendMessage("configuration/classhistogram", msg.length(),
			(void*) msg.c_str());
}

int startReceiver() {
	publishConfig();
	return 0;
}

int stopReceiver() {
	return 0;
}

pushsource* ClassHistogramProvider::registerPushSource(
		agentCoreFunctions aCF, uint32 provID) {
	pushsource *src = new pushsource();
	src->header.name = "classhistogram";
	src->header.description =
			"Provides the class histogram data when requested by the client";
	src->header.sourceID = 0;
	src->next = NULL;
	src->header.capacity = 1048576; /* 1MB bucket capacity */
	ClassHistogramProvider::providerID = provID;
	ibmras::monitoring::plugins::j9::classhistogram::sendClassHistogramData = aCF.agentPushData;

	return src;
}

ClassHistogramProvider::ClassHistogramProvider(jvmFunctions functions) {
	vmFunctions = functions;
	name = "ClassHistogramProvider";
	pull = NULL;
	push = registerPushSource;
	start = plugins::j9::classhistogram::startReceiver;
	stop = plugins::j9::classhistogram::stopReceiver;
	getVersion = getchpVersion;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY) ClassHistogramProvider::getInstance;
	confactory = NULL;

}

ClassHistogramProvider::~ClassHistogramProvider() {
}

const char* getchpVersion() {
	return chpVersion;
}

ClassHistogramProvider* instance = NULL;

ClassHistogramProvider* ClassHistogramProvider::getInstance(
		jvmFunctions functions) {
    // Does the vm have this function available	
    if (!functions.jvmtiRegisterTraceSubscriber == NULL) {
        if (!instance) {
            instance = new ClassHistogramProvider(functions);
        }
    }
	return instance;
}

void* ClassHistogramProvider::getInstance() {
	if (!instance) {
		return NULL;
	}
	return instance;
}

void ClassHistogramProvider::receiveMessage(const std::string &id, uint32 size,
		void *data) {
	if (id == "classhistogram") {
		IBMRAS_DEBUG(debug, "received class histogram request");
		if (!ibmras::monitoring::agent::Agent::getInstance()->readOnly()) {
			std::string data = createHistogramReport();
			char* dataToSend = ibmras::common::util::createAsciiString(data.c_str());
			monitordata *mdata = generateData(0, dataToSend, data.length());
			sendClassHistogramData(mdata);
			ibmras::common::memory::deallocate((unsigned char**)&dataToSend);
			delete mdata;
		}
	}
}




monitordata* ClassHistogramProvider::generateData(uint32 sourceID,
		const char *dataToSend, int size) {
	monitordata* data = new monitordata;
	data->provID = ClassHistogramProvider::providerID;
	data->data = dataToSend;

	if (data->data == NULL) {
		data->size = 0;
	} else {
		data->size = size;
	}
	data->sourceID = sourceID;
	data->persistent = false;
	return data;
}


std::string ClassHistogramProvider::createHistogramReport()
{
    std::stringstream report;
    std::stringstream reportdata;

	int heapUsed=0;
    int rc;
    jclass *classes = NULL;
    jint count;
    int i;
    const char* repfmt = "@@chd@@,%s,"JLONG_FMT_STR","JLONG_FMT_STR"\n";
    char** classNameArray = NULL;

	JNIEnv * env = NULL;

	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (ClassHistogramProvider)", vmFunctions.theVM, false);

    /* Get all the loaded classes */
    rc = vmFunctions.pti->GetLoadedClasses(&count, &classes);
    if ( rc != JVMTI_ERROR_NONE )
    {
    	IBMRAS_DEBUG_1(debug, "cannot GetLoadedClasses: rc = %d", rc);
        goto cleanup;
    }

    classNameArray = (char**)hc_alloc((count) * sizeof(char*));
    if(NULL == classNameArray) {
    	goto cleanup;
    }

    classSizes = (jlong*)hc_alloc(count * sizeof(jlong));
    if(NULL == classSizes) {
    	goto cleanup;
    }

    classCounts = (jlong*)hc_alloc(count * sizeof(jlong));
    if(NULL == classCounts) {
    	goto cleanup;
    }

    for ( i = 0 ; i < count ; i++ )
    {
        char* signature = NULL;
        char* generic = NULL;

        rc = vmFunctions.pti->SetTag(classes[i], i);
        if ( rc != JVMTI_ERROR_NONE )
        {
        	IBMRAS_DEBUG_1(debug, "cannot setTags on classes: rc = %d", rc);
            goto cleanup;
        }

        rc = vmFunctions.pti->GetClassSignature(classes[i], &signature, &generic);
        if ( rc != JVMTI_ERROR_NONE )
        {
        	IBMRAS_DEBUG_1(debug, "cannot get class signatures: rc = %d", rc);
            goto cleanup;
        }

        classNameArray[i] = (char *)hc_alloc(strlen(signature)+1);
    	if(NULL == classNameArray[i]) {
    		goto cleanup;
	    }

        strcpy(classNameArray[i],signature);
        ibmras::common::util::force2Native(classNameArray[i]);

        if (signature)
        {
            hc_dealloc((unsigned char**)&signature);
        }
        if (generic)
        {
            hc_dealloc((unsigned char**)&generic);
        }
        env->DeleteLocalRef(classes[i]);
    }

    /* This returns the JVMTI_ERROR_UNATTACHED_THREAD */
    rc = vmFunctions.pti->IterateOverHeap(JVMTI_HEAP_OBJECT_EITHER,&updateClassTotals,&heapUsed);

    if (rc != JVMTI_ERROR_NONE)
    {
    	IBMRAS_DEBUG_1(debug, "problem iterating over heap, error %d\n",rc);
        goto cleanup;
    }

    char buffer[1000];

    sprintf(buffer,"heapused,%d\n", heapUsed);

    report << buffer;
    for (i=0; i < count; i++)
    {
        sprintf(buffer,repfmt,classNameArray[i], classSizes[i], classCounts[i]);
        hc_dealloc((unsigned char**)&classNameArray[i]);
        report << buffer;
    }

	unsigned long long millisecondsSinceEpoch;
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

	reportdata << "classhistogramreport," << "reportTime," << millisecondsSinceEpoch << "\n" << report.str();

cleanup:
	vmFunctions.theVM->DetachCurrentThread();
    hc_dealloc((unsigned char**)&classes);
    hc_dealloc((unsigned char**)&classCounts);
    hc_dealloc((unsigned char**)&classSizes);
    hc_dealloc((unsigned char**)&classNameArray);


    return reportdata.str();
}


unsigned char* ClassHistogramProvider::hc_alloc(int size)
{
	return ibmras::common::memory::allocate(size);
}


void ClassHistogramProvider::hc_dealloc(unsigned char** buffer)
{
	ibmras::common::memory::deallocate(buffer);
}


}
}
}
}
} /* end namespace methods */








