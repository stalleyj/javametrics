/*
 * ThreadsPlugin.cpp
 *
 *  Created on: 19 Feb 2016
 *      Author: Admin
 */

#include <jni.h>
#include "ibmras/monitoring/plugins/j9/threads/ThreadsPlugin.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/MemoryManager.h"
#include "ibmras/monitoring/plugins/j9/Util.h"
#include <cstring>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace threads {

IBMRAS_DEFINE_LOGGER("threadsplugin")
;

ThreadsPlugin* instance = NULL;
static uint32 provID;
bool enabled;

ThreadsPlugin::ThreadsPlugin(jvmFunctions* jvmF) :
				env(NULL), jvmF(jvmF) {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::ThreadsPlugin");
	name = "threads";
	pull = registerPullSource;
	push = NULL;
	start = startWrapper;
	stop = stopWrapper;
	getVersion = getThrVersion;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY)ThreadsPlugin::getReceiver;
	confactory = NULL;
	IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::ThreadsPlugin");
}

ThreadsPlugin::~ThreadsPlugin() {
}

ThreadsPlugin* ThreadsPlugin::getPlugin(jvmFunctions* jvmF) {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::getPlugin");
	if(!instance) {
		instance = new ThreadsPlugin(jvmF);
	}
	return instance;
}

void* ThreadsPlugin::getReceiver() {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::getReceiver");
	if(!instance) {
		IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::getReceiver[NULL]");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::getReceiver[OK]");
	return (Receiver*)instance;
}


pullsource* ThreadsPlugin::registerPullSource(agentCoreFunctions aCF,uint32 prov) {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::registerPullSource");
	pullsource* src = new pullsource();

	src->header.name = "threads";
	provID = prov;
	src->header.description = "Threads information";
	src->header.sourceID = 0;
	src->header.capacity = 32 * 1024;
	src->next = NULL;
	src->callback = pullWrapper;
	src->complete = pullCompleteWrapper;
	src->pullInterval = 30;
	IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::registerPullSource");
	return src;
}

/***************************************************************************************
 * Receiver-related methods:
 * *************************************************************************************/

void ThreadsPlugin::publishConfig() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "threads_subsystem=";
	if (isEnabled()) {
		msg += "on";
	} else {
		msg += "off";
	}

	conMan->sendMessage("configuration/threads", msg.length(),
			(void*) msg.c_str());
}

void ThreadsPlugin::receiveMessage(const std::string& id, uint32 size, void* data) {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::receiveMessage");

	std::string message((const char*) data, size);
	if (id == "threads") {
			std::size_t found = message.find(',');
			std::string command = message.substr(0, found);
			std::string rest = message.substr(found + 1);

			ibmras::monitoring::plugins::j9::threads::ThreadsPlugin::setState(
					command);
	}
	IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::receiveMessage");
}

int ThreadsPlugin::startReceiver() {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::startReceiver");
	publishConfig();
	IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::startReceiver");
	return 0;
}

int ThreadsPlugin::stopReceiver() {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::stopReceiver");
	IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::stopReceiver");
	return 0;
}

void ThreadsPlugin::setState(const std::string &newState) {
	enabled = ibmras::common::util::equalsIgnoreCase(newState, "on");
	if (instance) {
		instance->publishConfig();
	}
}

bool ThreadsPlugin::isEnabled() {
	return enabled;
}

/***************************************************************************************
 * Data-production/data-transfer related methods:
 * *************************************************************************************/

monitordata* ThreadsPlugin::pullInt() {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::pullInt");
	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (threads)", jvmF->theVM, true);
	if(!env) {
		IBMRAS_DEBUG(warning, "The JNI env was not set up");
		IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::pullInt[JNIenv=NULL]");
		return NULL;
	}
	monitordata* data = new monitordata;
	data->size = 0;
	data->data = NULL;

	if (isEnabled()) {
		data->persistent = false;
		data->provID = provID;
		data->sourceID = 0;
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
		std::string cp =
				ibmras::monitoring::plugins::j9::getString(env,
						"com/ibm/java/diagnostics/healthcenter/agent/dataproviders/threads/ThreadDataProvider",
						"getJMXData", "()Ljava/lang/String;");
#if defined(_ZOS)
#pragma convert(pop)
#endif
		int len = cp.length();
		char* sval = reinterpret_cast<char*>(ibmras::monitoring::plugins::j9::hc_alloc(len + 1));
		if (sval) {
			strcpy(sval, cp.c_str());
		}
		if (sval) {
			data->size = len;
			data->data = sval;
		}
	}
	return data;
}

void ThreadsPlugin::pullcompleteInt(monitordata* data) {
	IBMRAS_DEBUG(debug, ">>>ThreadsPlugin::pullcompleteInt");
	if (data) {
			ibmras::monitoring::plugins::j9::hc_dealloc((unsigned char**) (&(data->data)));
			delete data;
			IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::pullcompleteInt[data deleted]");
	} else {
		if (env) {
			IBMRAS_DEBUG_1(debug, "Detaching thread %s", name.c_str());
			jvmF->theVM->DetachCurrentThread();
			env = NULL;
			IBMRAS_DEBUG(debug, "<<<ThreadsPlugin::pullcompleteInt[thread detached]");
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
	std::string enableProp = agent->getAgentProperty("data.threads");

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

const char* getThrVersion() {
	return "1.0";
}

}//threads
}//j9
}//plugins
}//monitoring
}//ibmras
