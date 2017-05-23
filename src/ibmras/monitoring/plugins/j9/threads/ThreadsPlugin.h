/*
 * ThreadsPlugin.h
 *
 *  Created on: 19 Feb 2016
 *      Author: Admin
 */

#ifndef ibmras_monitoring_plugins_j9_jni_threads_threadsplugin_h
#define ibmras_monitoring_plugins_j9_jni_threads_threadsplugin_h

#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "healthcenter.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace threads {

class ThreadsPlugin : public ibmras::monitoring::Plugin, public ibmras::monitoring::connector::Receiver {
public:
	ThreadsPlugin(jvmFunctions* jvmF);
	static ThreadsPlugin* getPlugin(jvmFunctions* jvmF);
	static void* getReceiver();
	virtual ~ThreadsPlugin();

	static pullsource* registerPullSource(agentCoreFunctions aCF, uint32 provID);
	int startReceiver();
	int stopReceiver();
	void publishConfig();
	monitordata* pullInt();
	void pullcompleteInt(monitordata* data);
	void receiveMessage(const std::string &id, uint32 size, void* data);

private:
	void setState(const std::string &newState);
	bool isEnabled();
	JNIEnv* env;
	jvmFunctions* jvmF;
};

monitordata* pullWrapper();
void pullCompleteWrapper(monitordata* data);
int startWrapper();
int stopWrapper();
const char* getThrVersion();

}
}
}
}
}

#endif /* THREADSPLUGIN_H_ */
