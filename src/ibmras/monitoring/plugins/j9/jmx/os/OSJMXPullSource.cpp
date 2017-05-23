/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#include "ibmras/monitoring/plugins/j9/jmx/os/OSJMXPullSource.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXSourceManager.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXUtility.h"
#include "ibmras/monitoring/plugins/j9/jmx/os/legacy/LegacyData.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/common/MemoryManager.h"

#include <cstring>
#include "ibmras/common/logging.h"

using namespace ibmras::monitoring::plugins::j9::jmx::os::legacy;
using namespace ibmras::monitoring::plugins::j9::jmx;

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {
namespace os {

IBMRAS_DEFINE_LOGGER("JMXSources")
;

/* need to be in own namespace so that top level callbacks work with other MX bean data providers */
OSJMXPullSource* src = NULL;
bool enabled = true;
bool methodsChecked = false;

JMXPullSource* getOSPullSource(uint32 id) {
	if (!src) {
		src = new OSJMXPullSource(id);
	}
	return src;
}

monitordata* callback() {
	if (enabled) {
		return src->JMXPullSource::generateData();
	} else {
		return NULL;
	}
}

void complete(monitordata *mdata) {
	src->pullComplete(mdata);
}

uint32 OSJMXPullSource::getSourceID() {
	return CPU;
}

pullsource* OSJMXPullSource::getDescriptor() {
	pullsource *src = new pullsource();
	src->header.name = "cpu";
	src->header.description = "CPU usage";
	src->header.sourceID = CPU;
	src->header.capacity = 10 * 1024;
	src->next = NULL;
	src->callback = callback;
	src->complete = complete;
	src->pullInterval = 2;
	return src;
}

OSJMXPullSource::OSJMXPullSource(uint32 id) :
		JMXPullSource(id, "Health Center (cpu)") {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string enableProp = agent->getAgentProperty("data.cpu");
	if (enableProp == "on" || enableProp == "") {
		enabled = true;
	} else {
		enabled = false;
	}

}

void OSJMXPullSource::publishConfig() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "cpu_subsystem=";
	if (isEnabled()) {
		methodsChecked = false;
		msg += "on";
	} else {
		msg += "off";
	}

	conMan->sendMessage("configuration/cpu", msg.length(), (void*) msg.c_str());
}

bool OSJMXPullSource::isEnabled() {

	return enabled;
}

void OSJMXPullSource::setState(const std::string &newState) {
	enabled = ibmras::common::util::equalsIgnoreCase(newState, "on");
	if (src) {
		src->publishConfig();
	}
}

monitordata* OSJMXPullSource::generateData(JNIEnv* env, jclass* mgtBean) {

	IBMRAS_DEBUG(debug, "Generating JMX CPU data");



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

	monitordata* data = new monitordata;
	data->size = 0;
	data->data = NULL;

#if !defined(_ZOS)
	if (!methodsChecked) {

		if (!getMethod(env, "com/ibm/lang/management/OperatingSystemMXBean",
				"getSystemCpuLoad", "()D")) {
			if (!getMethod(env,
					"com/ibm/lang/management/OperatingSystemMXBean",
					"getProcessCpuLoad", "()D")) {
				IBMRAS_DEBUG(debug, "CPU data not available")
				enabled = false;
				publishConfig();
			}
		}
		methodsChecked = true;
	}

	if (isEnabled()) {

		data->persistent = false;
		data->provID = getProvID();
		data->sourceID = CPU;

		jobject mgt = getMXBean(env, mgtBean, "OperatingSystem");
		if (mgt) {
			IBMRAS_DEBUG(debug, "Getting timestamp");
			jlong tstamp = getTimestamp(env);
			IBMRAS_DEBUG(debug, "Invoking getSystemCpuLoad");

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif

			jdouble systemCPULoad = getDouble(env, &mgt,
					"com/ibm/lang/management/OperatingSystemMXBean",
					"getSystemCpuLoad");

#if defined(_ZOS)
#pragma convert(pop)
#endif

			IBMRAS_DEBUG(debug, "Invoking getProcessCpuLoad : this is only available in Java 7 and later");

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif

			jdouble processCPULoad = getDouble(env, &mgt,
					"com/ibm/lang/management/OperatingSystemMXBean",
					"getProcessCpuLoad");

#if defined(_ZOS)
#pragma convert(pop)
#endif
			IBMRAS_DEBUG_2(debug, "systemCPULoad %f, processCPULoad %f", systemCPULoad, processCPULoad);

			if (processCPULoad >= 0 || systemCPULoad >= 0) {
				IBMRAS_DEBUG(debug, "Constructing CPU data line");
				LegacyData* line = new LegacyData("startCPU", tstamp);
				LegacyDataNumeric<jdouble>* value = new LegacyDataNumeric<
						jdouble>(processCPULoad);
				line->add(value);
				value = new LegacyDataNumeric<jdouble>(systemCPULoad);
				line->add(value);
				char* sval = line->getData();
				IBMRAS_DEBUG_1(debug, "Got CPU data : %s", sval);
				data->size = strlen(sval);
				ibmras::common::util::native2Ascii(sval);
				data->data = sval;
				delete line; /* deleting the line should cascade delete the values it is currently holding THIS CAN BE A MEMORY LEAK*/
				return data;
			}
		}
	}
#endif
	return data;
}

} /* end namespace os */
} /* end namespace jmx */
} /* end namespace j9 */
} /* end namespace plugins */
} /* end namespace monitoring */
} /* end namespace ibmras */
