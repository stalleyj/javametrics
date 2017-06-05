/*
 * EnvironmentPlugin.cpp
 *
 *  Created on: 16 Feb 2016
 *      Author: Admin
 */

#include <jni.h>
#include "ibmras/monitoring/plugins/j9/environment/EnvironmentPlugin.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/monitoring/plugins/j9/DumpHandler.h"
#include <stdio.h>

#include "ibmras/common/MemoryManager.h"
#include "ibmras/vm/java/healthcenter.h"
#include <cstring>
#include <stdlib.h>
#include <string>
#include <stdint.h>
#include <sstream>
#include <vector>
#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/monitoring/plugins/j9/Util.h"
#include "ibmras/common/port/Process.h"


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace environment {

IBMRAS_DEFINE_LOGGER("environmentplugin")
;


EnvironmentPlugin* instance = NULL;
static uint32 provID;

EnvironmentPlugin::EnvironmentPlugin(jvmFunctions* jvmF) :
				env(NULL), jvmF(jvmF) {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::EnvironmentPlugin");
	name = "environment";
	pull = registerPullSource;
	push = NULL;
	start = startWrapper;
	stop = stopWrapper;
	getVersion = getEnvVersion;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY)EnvironmentPlugin::getReceiver;
	confactory = NULL;
	IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::EnvironmentPlugin");
}
EnvironmentPlugin::~EnvironmentPlugin() {
}

EnvironmentPlugin* EnvironmentPlugin::getPlugin(jvmFunctions* jvmF) {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::getPlugin");
	if(!instance) {
		instance = new EnvironmentPlugin(jvmF);
	}
	return instance;
}

void* EnvironmentPlugin::getReceiver() {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::getReceiver");
	if(!instance) {
		IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::getReceiver[NULL]");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::getReceiver[OK]");
	return (Receiver*)instance;//It is casted here to avoid any issues with the way the compiler offsets the members of the classes
}

pullsource* EnvironmentPlugin::registerPullSource(agentCoreFunctions aCF,uint32 prov) {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::registerPullSource");
	pullsource* src = new pullsource();

	src->header.name = "environment";
	provID = prov;
	src->header.description = "Environment information";
	src->header.sourceID = 0;
	src->header.capacity = 32 * 1024;
	src->next = NULL;
	src->callback = pullWrapper;
	src->complete = pullCompleteWrapper;
	src->pullInterval = 1200;
	IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::registerPullSource");
	return src;
}

/***************************************************************************************
 * Receiver-related methods:
 * *************************************************************************************/

void EnvironmentPlugin::publishConfig() {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::publishConfig");
	ibmras::monitoring::agent::Agent* agent =
				ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "environment_subsystem=on\ncapability.dump.heap=on\ncapability.dump.system=on\ncapability.dump.java=on";
	conMan->sendMessage("configuration/environment", msg.length(),
			(void*) msg.c_str());
	IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::publishConfig");
}

void EnvironmentPlugin::receiveMessage(const std::string& id, uint32 size, void* data) {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::receiveMessage");

	std::string message((const char*) data, size);
	if (id == "environment") {
		std::size_t found = message.find(',');
		std::string command = message.substr(0, found);
		std::string rest = message.substr(found + 1);
		std::vector < std::string > parameters = ibmras::common::util::split(
				rest, ',');

		if (ibmras::common::util::equalsIgnoreCase(command, "set")) {
			ibmras::monitoring::plugins::j9::DumpHandler::requestDumps (parameters);
		}
	}
	IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::receiveMessage");
}

int EnvironmentPlugin::startReceiver() {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::startReceiver");
	publishConfig();
	IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::startReceiver");
	return 0;
}

int EnvironmentPlugin::stopReceiver() {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::stopReceiver");
	IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::stopReceiver");
	return 0;
}

/***************************************************************************************
 * Data-production/data-transfer related methods:
 * *************************************************************************************/
//This will be the internal pull function called by the callback function provided off-class
monitordata* EnvironmentPlugin::pullInt() {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::pullInt");

	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (environment)", jvmF->theVM, true);

	if(!env) {
		IBMRAS_DEBUG(warning, "The JNI env was not set up");
		IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::pullInt[JNIenv=NULL]");
		return NULL;
	}

	IBMRAS_DEBUG(debug, "Generating Environment data");
	monitordata* data = new monitordata;
	data->persistent = false;
	data->provID = provID;
	data->sourceID = 0;
	#if defined(_ZOS)
	#pragma convert("ISO8859-1")
	#endif
		std::string cp = ibmras::monitoring::plugins::j9::getString(env, "com/ibm/java/diagnostics/healthcenter/agent/dataproviders/environment/EnvironmentDataProvider", "getJMXData", "()Ljava/lang/String;");
	#if defined(_ZOS)
	#pragma convert(pop)
		IBMRAS_DEBUG_1(debug, "cp String = %s", cp.c_str());
		char* envString = ibmras::common::util::createNativeString(cp.c_str());
	#else
		const char* envString = cp.c_str();
	#endif

		std::stringstream ss;

		if (envString) {
			ss << envString;
			ss<<"\n";
		}
	#if defined(_ZOS)
		ibmras::common::memory::deallocate((unsigned char**)&envString);
	#endif

		ss << "native.library.date=" << ibmras::monitoring::agent::Agent::getBuildDate() << "\n";
		ss << "pid=" << ibmras::common::port::getProcessId() << "\n";

		const std::string vmd = reportDumpOptions(jvmF);
		ss<<vmd;

		std::string envdata = ss.str();

		char* asciiEnv = ibmras::common::util::createAsciiString(envdata.c_str());

		if(asciiEnv) {
			data->size = strlen(asciiEnv);
			data->data = asciiEnv;
		} else {
			data->size = 0;
			data->data = NULL;
		}
	IBMRAS_DEBUG_1(debug, "<<<EnvironmentPlugin::pullInt[datasize=%d]", data->size);
	return data;

}

void EnvironmentPlugin::pullcompleteInt(monitordata* data) {
	IBMRAS_DEBUG(debug, ">>>EnvironmentPlugin::pullcompleteInt");
	if (data) {
			ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) (&(data->data)));
			delete data;
			IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::pullcompleteInt[data deleted]");
	} else {
		if (env) {
			IBMRAS_DEBUG_1(debug, "Detaching thread %s", name.c_str());
			jvmF->theVM->DetachCurrentThread();
			env = NULL;
			IBMRAS_DEBUG(debug, "<<<EnvironmentPlugin::pullcompleteInt[thread detached]");
		}
	}
}

const std::string reportDumpOptions(jvmFunctions* tdpp) {

	const::std::string options = ibmras::monitoring::plugins::j9::Util::queryVmDump(tdpp->jvmtiQueryVmDump, tdpp->pti);
	std::vector<std::string> sv = ibmras::common::util::split(options, '\n');
	std::stringstream ss;

	ss << "dump.options=";
	int i = 0;
	for(std::vector<std::string>::iterator it = sv.begin(); it != sv.end(); ++it){
		if(i != 0){
			ss << "@@@";
		}
		ss << *it;
		i++;
	}
	ss << '\n';

	return ss.str();
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

const char* getEnvVersion() {
	return "1.0";
}

}//environment
}//j9
}//plugins
}//monitoring
}//ibmras



