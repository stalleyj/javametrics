 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/monitoring/plugins/j9/jmx/rt/RTJMXPullSource.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXSourceManager.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXUtility.h"
#include "ibmras/common/logging.h"
#include <cstring>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {
namespace rt {

IBMRAS_DEFINE_LOGGER("JMXSources");

/* need to be in own namespace so that top level callbacks work with other MX bean data providers */
RTJMXPullSource* src = NULL;

JMXPullSource* getRTPullSource(uint32 id) {
	if(!src) {
		src = new RTJMXPullSource(id);
	}
	return src;
}

monitordata* callback() {
	return src->JMXPullSource::generateData();
}

void complete(monitordata *mdata) {
	src->pullComplete(mdata);
}

uint32 RTJMXPullSource::getSourceID() {
	return CPU;
}

RTJMXPullSource::RTJMXPullSource(uint32 id) : JMXPullSource(id, "Health Center (runtime)") {
}

void RTJMXPullSource::publishConfig() {

}

pullsource* RTJMXPullSource::getDescriptor() {
	pullsource *src = new pullsource();
	src->header.name = "RT";
	src->header.description = "Runtime information";
	src->header.sourceID = RT;
	src->header.capacity = JMXSourceManager::DEFAULT_CAPACITY;
	src->next = NULL;
	src->callback = callback;
	//src->complete = getCallbackComplete();
	src->complete = complete;	/* use default clean up */
	src->pullInterval = 120;
	return src;
}


monitordata* RTJMXPullSource::generateData(JNIEnv* env, jclass* mgtBean) {
	IBMRAS_DEBUG(debug, "Generating JMX Runtime data");
	monitordata* data = new monitordata;
	data->persistent = false;
	data->provID = getProvID();
	data->sourceID = RT;

	jobject mgt = getMXBean(env, mgtBean, "Runtime");
	if(mgt) {
		IBMRAS_DEBUG(debug, "Invoking getClassPath");
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
		char* cp = getString(env, &mgt, "java/lang/management/RuntimeMXBean", "getClassPath");
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
		if(cp) {
			data->size = strlen(cp);
			data->data = cp;
			return data;
		}
	}
	data->size = 0;
	data->data = NULL;
	return data;
}

}	/* end namespace rt */
}	/* end namespace jmx */
}	/* end namespace j9 */
}	/* end namespace plugins */
}	/* end namespace monitoring */
} 	/* end namespace ibmras */
