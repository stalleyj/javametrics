/*
  * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */
 
#if defined(_ZOS)
#define _XOPEN_SOURCE_EXTENDED 1
#undef _ALL_SOURCE
#endif

#include "jni.h"
#include "ibmras/monitoring/plugins/j9/api/AppPlugin.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/MemoryManager.h"
#include <string>
#if defined(WINDOWS)
#include <windows.h>
#include <ctime>
#else
#include <sys/time.h>
#endif

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace api {

PUSH_CALLBACK sendAppData;
uint32 AppPlugin::providerID = 0;
IBMRAS_DEFINE_LOGGER("AppPlugin");

const char* appVersion = "1.0";
const std::string genericEvent="genericEvent";

void publishConfig() {
	IBMRAS_DEBUG(debug, "> publishConfig()");
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();

	ibmras::monitoring::connector::ConnectorManager *conMan =
			agent->getConnectionManager();

	std::string msg = "capability.generic.events=on";
	conMan->sendMessage("configuration/genericevents", msg.length(),
			(void*) msg.c_str());
	IBMRAS_DEBUG(debug, "< publishConfig()");
}

int startReceiver() {
	IBMRAS_DEBUG(debug, "> startReceiver()");
	publishConfig();
	IBMRAS_DEBUG(debug, "< startReceiver()");
	return 0;
}

int stopReceiver() {
	IBMRAS_DEBUG(debug, "> stopReceiver()");
	IBMRAS_DEBUG(debug, "< stopReceiver()");
	return 0;
}

pushsource* AppPlugin::registerPushSource(agentCoreFunctions aCF, uint32 provID) {
	IBMRAS_DEBUG(debug, "> registerPushSource()");
	pushsource *src = new pushsource();
	src->header.name = "genericevents";
	src->header.description = "Provides generic events when requested by the client";
	src->header.sourceID = 0;
	src->next = NULL;
	src->header.capacity = 1048576; /* 1MB bucket capacity */
	AppPlugin::providerID = provID;
	ibmras::monitoring::plugins::j9::api::sendAppData = aCF.agentPushData;
	IBMRAS_DEBUG(debug, "< registerPushSource()");
	return src;
}

AppPlugin::AppPlugin(jvmFunctions functions) {
	IBMRAS_DEBUG(debug, "> AppPlugin()");
	vmFunctions = functions;
	name = "GenericEvents";
	pull = NULL;
	push = registerPushSource;
	start = plugins::j9::api::startReceiver;
	stop = plugins::j9::api::stopReceiver;
	getVersion = getVersionApp;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY) AppPlugin::getInstance;
	confactory = NULL;
	IBMRAS_DEBUG(debug, "< AppPlugin()");
}

AppPlugin::~AppPlugin() {
	IBMRAS_DEBUG(debug, "> ~AppPlugin()");
	IBMRAS_DEBUG(debug, "< ~AppPlugin()");
}

const char* getVersionApp() {
	IBMRAS_DEBUG(debug, "> getVersionApp()");
	IBMRAS_DEBUG_1(finest, "< getVersionApp(), returning %s", appVersion);
	return appVersion;
}

AppPlugin* instance = NULL;

AppPlugin* AppPlugin::getInstance(jvmFunctions functions) {
	IBMRAS_DEBUG(debug, "> getInstance(jvmFunctions)");
	if (!instance) {
		instance = new AppPlugin(functions);
	}
	IBMRAS_DEBUG(debug, "< getInstance(jvmFunctions)");
	return instance;
}

void* AppPlugin::getInstance() {
	IBMRAS_DEBUG(debug, "> getInstance()");
	if (!instance) {
		IBMRAS_DEBUG(finest, "< getInstance(), returning NULL");
		return NULL;
	}
	IBMRAS_DEBUG(debug, "< getInstance(), returning instance");
	return instance;
}

void AppPlugin::receiveMessage(const std::string &id, uint32 size, void *data) {
	IBMRAS_DEBUG_1(debug, "> receiveMessage(), id is %s", id.c_str());
		if (id.compare(0,genericEvent.size(),genericEvent) == 0) {
			IBMRAS_DEBUG(fine, "received genericEvent request");
			if (!ibmras::monitoring::agent::Agent::getInstance()->readOnly()) {
				IBMRAS_DEBUG(finest, "storing data");
				std::string eventType = id.substr(genericEvent.size());
				IBMRAS_DEBUG_1(fine, "eventType is %s", eventType.c_str());
				std::string dataString((const char*) data, size);
				dataString = createEvent(eventType,dataString);
				char* dataToSend = ibmras::common::util::createAsciiString(dataString.c_str());
				monitordata *mdata = generateData(0, dataToSend, dataString.length());
				sendAppData(mdata);
				ibmras::common::memory::deallocate((unsigned char**)&dataToSend);
				delete mdata;
			}
		}
	IBMRAS_DEBUG(debug, "< receiveMessage()");
}

std::string AppPlugin::createEvent(std::string type, std::string message) {
	IBMRAS_DEBUG(debug, "> createEvent()");
	std::stringstream reportdata;
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

	reportdata << type << "Event,time," << millisecondsSinceEpoch << ",message," << message;
	IBMRAS_DEBUG(debug, "< createEvent()");
	return reportdata.str();
}

monitordata* AppPlugin::generateData(uint32 sourceID, const char *dataToSend, int size) {
	IBMRAS_DEBUG(debug, "> generateData()");
	monitordata* data = new monitordata;
	data->provID = AppPlugin::providerID;
	data->data = dataToSend;

	if (data->data == NULL) {
		data->size = 0;
	} else {
		data->size = size;
	}
	data->sourceID = sourceID;
	data->persistent = false;
	IBMRAS_DEBUG(debug, "< generateData()");
	return data;
}

extern "C" {
JNIEXPORT void JNICALL
Java_com_ibm_java_diagnostics_healthcenter_agent_dataproviders_api_Event_sendMessage(JNIEnv *env, jobject obj, jstring topic, jbyteArray ident) {
	IBMRAS_DEBUG(debug, "> JNI_sendMessage()");
	const char *s = env->GetStringUTFChars(topic,NULL);
	if (s) {
		jboolean isCopy;
		jbyte* i = env->GetByteArrayElements(ident, &isCopy);
		if(instance) {
			std::string topic_str(ibmras::common::util::createNativeString(s));
			IBMRAS_DEBUG_1(finest, "message topic is %s", topic_str.c_str());
			instance->receiveMessage(topic_str, env->GetArrayLength(ident), (void *)i);
		}
		env->ReleaseStringUTFChars(topic,s);
		env->ReleaseByteArrayElements(ident, i, 0);
	}
	IBMRAS_DEBUG(debug, "< JNI_sendMessage()");
}
}


}//api
}//j9
}//plugins
}//monitoring
}//ibmras


