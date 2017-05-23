/*
 * EnvironmentPlugin.h
 *
 *  Created on: 16 Feb 2016
 *      Author: Admin
 */

#ifndef ibmras_monitoring_plugins_j9_environment_environmentplugin_h
#define ibmras_monitoring_plugins_j9_environment_environmentplugin_h

//#include "ibmras/monitoring/plugins/j9/InternalPlugin.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "healthcenter.h"


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace environment {

class EnvironmentPlugin : public ibmras::monitoring::Plugin, public ibmras::monitoring::connector::Receiver {
public:
	EnvironmentPlugin(jvmFunctions* jvmF);
	static EnvironmentPlugin* getPlugin(jvmFunctions* jvmF);
	static void* getReceiver();
	virtual ~EnvironmentPlugin();

	static pullsource* registerPullSource(agentCoreFunctions aCF, uint32 provID);
	int startReceiver();
	int stopReceiver();
	monitordata* pullInt();
	void pullcompleteInt(monitordata* data);
	void receiveMessage(const std::string &id, uint32 size, void* data);

private:

	void publishConfig();
	JNIEnv* env;
	jvmFunctions* jvmF;
};

//The type definition of the callback functions is typedef monitordata* (*PULL_CALLBACK)(void);
//therefore, the member functions cannot be passed to pullsource->callback. So we define:
monitordata* pullWrapper();
void pullCompleteWrapper(monitordata* data);
int startWrapper();
int stopWrapper();
const char* getEnvVersion();

const std::string reportDumpOptions(jvmFunctions* jvmF);


}
}
}
}
}


#endif /* ENVIRONMENTPLUGIN_H_ */
