 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/monitoring/plugins/j9/jmx/JMXPullSource.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXUtility.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/MemoryManager.h"
#include "ibmras/common/util/strUtils.h"
#include <cstring>
#include <stdlib.h>


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {

JavaVM* vm = NULL;

JMXPullSource::JMXPullSource(uint32 id, const std::string& providerName) :
	provID(id), env(NULL), name(providerName) {
}

monitordata* JMXPullSource::generateError(char* msg) {
	monitordata* data = new monitordata;
	data->provID = getProvID();
	data->sourceID = getSourceID();
	data->size = strlen(msg);
	data->data = msg;
	data->persistent = false;
	return data;
}


monitordata* JMXPullSource::generateData() {

	if (!env) {

		int errcode = ibmras::monitoring::plugins::j9::setEnv(&env, name, vm, true);

		if (errcode != JNI_OK) {
			return NULL;
		}

		IBMRAS_DEBUG_1(debug, "Attached thread %s", name.c_str());
	}

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif

	jclass clazz = env->FindClass("java/lang/management/ManagementFactory");

#if defined(_ZOS)
#pragma convert(pop)
#endif

	if (!clazz) {
		IBMRAS_DEBUG(warning,  "!Failed to find ManagementFactory class");
		return NULL;
	}
	IBMRAS_DEBUG(debug,  "Found management class");
	monitordata* data = generateData(env, &clazz);
	return data;
}

void JMXPullSource::pullComplete(monitordata* mdata) {
	if (mdata) {
		ibmras::monitoring::plugins::j9::jmx::complete(mdata);
	} else {
		if (env) {
			IBMRAS_DEBUG_1(debug, "Detaching thread %s", name.c_str());
			vm->DetachCurrentThread();
			env = NULL;
		}
	}
}
JMXPullSource::~JMXPullSource() {
}


}	/* end namespace jmx */
}	/* end namespace j9 */
}	/* end namespace plugins */
}	/* end namespace monitoring */
} 	/* end namespace ibmras */


