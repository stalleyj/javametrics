/*
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_j9_api_appplugin_h
#define ibmras_monitoring_plugins_j9_api_appplugin_h

#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/vm/java/healthcenter.h"


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace api {

const char* getVersionApp();

class AppPlugin: public ibmras::monitoring::connector::Receiver, public ibmras::monitoring::Plugin {
public:
	AppPlugin(jvmFunctions functions);
	virtual ~AppPlugin();
	int startReceiver();
	int stopReceiver();
	void publishConfig();
	void receiveMessage(const std::string &id, uint32 size, void *data);
	static pushsource* registerPushSource(agentCoreFunctions aCF, uint32 provID);
	static uint32 providerID;
	static AppPlugin* getInstance(jvmFunctions functions);
	static void* getInstance();
private:
	jvmFunctions vmFunctions;
	static monitordata* generateData(uint32 sourceID, const char *dataToSend, int size);
	std::string createEvent(std::string type, std::string message);
};

} //api
} //j9
} //plugins
} //monitoring
} //ibmras
#endif /* ibmras_monitoring_plugins_j9_api_appplugin_h */
