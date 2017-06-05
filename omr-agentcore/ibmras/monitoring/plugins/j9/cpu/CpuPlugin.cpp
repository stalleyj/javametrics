/*
 * CpuPlugin.cpp
 *
 *  Created on: 23 Feb 2016
 *      Author: Admin
 */
#include "ibmras/monitoring/plugins/j9/cpu/CpuPlugin.h"
#include "ibmras/common/logging.h"
#include "ibmras/monitoring/plugins/j9/Util.h"
#include "ibmras/vm/java/healthcenter.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/monitoring/plugins/j9/Util.h"
#include "ibmras/common/util/strUtils.h"
#include <iostream>


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace cpu {

IBMRAS_DEFINE_LOGGER("cpuplugin")
;

CpuPlugin* instance = NULL;
bool enabled;
bool methodsChecked = false;
static uint32 provID;
static const char* DELIMITER = "@#";
static const char* EOL = "\n";

CpuPlugin::CpuPlugin(jvmFunctions* jvmF) :
				env(NULL), jvmF(jvmF) {
	IBMRAS_DEBUG(debug, ">>>CpuPlugin::CpuPlugin");
	name = "cpu";
	pull = registerPullSource;
	push = NULL;
	start = startWrapper;
	stop = stopWrapper;
	getVersion = getCpuVersion;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY)CpuPlugin::getReceiver;
	confactory = NULL;
	IBMRAS_DEBUG(debug, "<<<CpuPlugin::CpuPlugin");

}

CpuPlugin::~CpuPlugin() {
}

CpuPlugin* CpuPlugin::getPlugin(jvmFunctions* jvmF) {
	IBMRAS_DEBUG(debug, ">>>CpuPlugin::getPlugin");
	if(!instance) {
		instance = new CpuPlugin(jvmF);
	}
	return instance;
}

void* CpuPlugin::getReceiver() {
	IBMRAS_DEBUG(debug, ">>>CpuPlugin::getReceiver");
	if(!instance) {
		IBMRAS_DEBUG(debug, "<<<CpuPlugin::getReceiver[NULL]");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "<<<CpuPlugin::getReceiver[OK]");
	return (Receiver*)instance;//It is casted here to avoid any issues with the way the compiler offsets the members of the classes
}

pullsource* CpuPlugin::registerPullSource(agentCoreFunctions aCF,uint32 prov) {
	IBMRAS_DEBUG(debug, ">>>CpuPlugin::registerPullSource");
	pullsource* src = new pullsource();

	src->header.name = "cpu";
	provID = prov;
	src->header.description = "CPU usage";
	src->header.sourceID = 0;
	src->header.capacity = 10 * 1024;
	src->next = NULL;
	src->callback = pullWrapper;
	src->complete = pullCompleteWrapper;
	src->pullInterval = 2;
	IBMRAS_DEBUG(debug, "<<<CpuPlugin::registerPullSource");
	return src;
}

/***************************************************************************************
 * Receiver-related methods:
 * *************************************************************************************/

int CpuPlugin::startReceiver() {
	IBMRAS_DEBUG(debug, ">>>CpuPlugin::startReceiver");
	publishConfig();
	IBMRAS_DEBUG(debug, "<<<CpuPlugin::startReceiver");
	return 0;
}

int CpuPlugin::stopReceiver() {
	IBMRAS_DEBUG(debug, ">>>CpuPlugin::stopReceiver");
	IBMRAS_DEBUG(debug, "<<<CpuPlugin::stopReceiver");
	return 0;
}

void CpuPlugin::publishConfig() {
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

void CpuPlugin::receiveMessage(const std::string &id, uint32 size, void* data) {
	std::string message((const char*) data, size);
	if (id == "cpu") {
		std::size_t found = message.find(',');
		std::string command = message.substr(0, found);
		std::string rest = message.substr(found + 1);

		setState(command);
	}
}

bool CpuPlugin::isEnabled() {

	return enabled;
}



void CpuPlugin::setState(const std::string &newState) {
	enabled = ibmras::common::util::equalsIgnoreCase(newState, "on");
	if (instance) {
		instance->publishConfig();
	}
}

/***************************************************************************************
 * Data-production/data-transfer related methods:
 * *************************************************************************************/

monitordata* CpuPlugin::pullInt() {

	IBMRAS_DEBUG(debug, ">>>CpuPlugin::pullInt");
	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (cpu)", jvmF->theVM, true);
	if(!env) {
		IBMRAS_LOG(warning, "The JNI env was not set up");
		IBMRAS_DEBUG(debug, "<<<CpuPlugin::pullInt[JNIenv=NULL]");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "Generating jmx CPU data");

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif

		jclass mgtBean = env->FindClass("java/lang/management/ManagementFactory");

#if defined(_ZOS)
#pragma convert(pop)
#endif

	if (!mgtBean) {
		IBMRAS_DEBUG(warning,  "!Failed to find ManagementFactory class");
		IBMRAS_DEBUG(debug, "<<<CpuPlugin::pullInt");
		return NULL;
	}
	IBMRAS_DEBUG(debug,  "Found management class");

	monitordata* data = new monitordata;
	data->size = 0;
	data->data = NULL;

#if !defined(_ZOS)
	if (!methodsChecked) {

		if (!ibmras::monitoring::plugins::j9::getMethod(env, "com/ibm/lang/management/OperatingSystemMXBean",
				"getSystemCpuLoad", "()D")) {
			if (!ibmras::monitoring::plugins::j9::getMethod(env,
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
		data->provID = provID;
		data->sourceID = 0;

		jobject mgt = ibmras::monitoring::plugins::j9::getMXBean(env, &mgtBean, "OperatingSystem");
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

				char* sval = buildCpuDataLine(tstamp, processCPULoad, systemCPULoad);
				IBMRAS_DEBUG_1(debug, "Got CPU data : %s", sval);
				data->size = strlen(sval);
				ibmras::common::util::native2Ascii(sval);
				data->data = sval;
				IBMRAS_DEBUG(debug, "<<<CpuPlugin::pullInt[OK]");
				return data;
			}
		}
	}
#endif
	return data;
}

char* CpuPlugin::buildCpuDataLine(jlong tstamp, jdouble processCPULoad, jdouble systemCPULoad) {
	std::string sdata;
	sdata.append("startCPU");
	sdata.append(DELIMITER);
	sdata.append(ibmras::common::itoa(tstamp));
	sdata.append(DELIMITER);
	jdouble value = processCPULoad;
	sdata.append(ibmras::common::itoa(processCPULoad));
	sdata.append(DELIMITER);
	value = systemCPULoad;
	sdata.append(ibmras::common::itoa(systemCPULoad));
	sdata.append(EOL);

	size_t len = sdata.size() + 1;	/* add space for null terminator */
	char* p = new char[len];
	if(p) {
		sdata.copy((char*)p, len - 1, 0);
		*(p + len - 1) = '\0';
		return p;			/* the memory will be free'd when the monitor data is cleaned up */
	}
	return NULL;
}


void CpuPlugin::pullcompleteInt(monitordata* data) {
	IBMRAS_DEBUG(debug, ">>>CpuPlugin::pullcompleteInt");
	if (data) {
			IBMRAS_DEBUG(debug, "data != null");
			delete data->data;
			delete data;
			IBMRAS_DEBUG(debug, "<<<CpuPlugin::pullcompleteInt[data deleted]");
	} else {
		if (env) {
			IBMRAS_DEBUG(debug, "env != null");
			IBMRAS_DEBUG_1(debug, "Detaching thread %s", name.c_str());
			jvmF->theVM->DetachCurrentThread();
			env = NULL;
			IBMRAS_DEBUG(debug, "<<<CpuPlugin::pullcompleteInt[thread detached]");
		}
	}
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

	ibmras::monitoring::agent::Agent* agent = ibmras::monitoring::agent::Agent::getInstance();
	std::string enableProp = agent->getAgentProperty("data.cpu");

	if (enableProp == "on" || enableProp == "") {
		enabled = true;
		instance->startReceiver();
	} else {
		enabled = false;
		instance->stopReceiver();
	}
	instance->publishConfig();
	return 0;
}

int stopWrapper() {
	return instance->stopReceiver();
}

const char* getCpuVersion() {
	return "1.0";
}

}//cpu
}//j9
}//plugins
}//monitoring
}//ibmras
