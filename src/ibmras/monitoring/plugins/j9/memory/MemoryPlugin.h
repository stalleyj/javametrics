/*
 * MemoryPlugin.h
 *
 *  Created on: 19 Feb 2016
 *      Author: Admin
 */

#ifndef ibmras_monitoring_plugins_j9_environment_memoryplugin_h
#define ibmras_monitoring_plugins_j9_environment_memoryplugin_h

#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/vm/java/healthcenter.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace memory {

class MemoryPlugin : public ibmras::monitoring::Plugin, public ibmras::monitoring::connector::Receiver {
public:
	MemoryPlugin(jvmFunctions* jvmF);
	static MemoryPlugin* getPlugin(jvmFunctions* jvmF);
	static void* getReceiver();
	virtual ~MemoryPlugin();

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
const char* getMemVersion();

const std::string COMMA = ","; //$NON-NLS-1$
const std::string EQUALS = "="; //$NON-NLS-1$

jlong getProcessPhysicalMemorySize(JNIEnv* env);
jlong getProcessPrivateMemorySize(JNIEnv* env);
jlong getProcessVirtualMemorySize(JNIEnv* env);
jlong getFreePhysicalMemorySize(JNIEnv* env);

std::string getString(JNIEnv* env, const char* cname, const char* mname, const char* signature);
unsigned char* hc_alloc(int size);
void hc_dealloc(unsigned char** buffer);



}
}
}
}
}



#endif /* MEMORYPLUGIN_H_ */
