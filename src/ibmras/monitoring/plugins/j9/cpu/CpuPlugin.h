/*
 * CpuPlugin.h
 *
 *  Created on: 23 Feb 2016
 *      Author: Admin
 */

#ifndef ibmras_monitoring_plugins_j9_cpu_cpuplugin_h
#define ibmras_monitoring_plugins_j9_cpu_cpuplugin_h

#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/vm/java/healthcenter.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace cpu {

class CpuPlugin : public ibmras::monitoring::Plugin, public ibmras::monitoring::connector::Receiver {
public:
	CpuPlugin(jvmFunctions* jvmF);
	static CpuPlugin* getPlugin(jvmFunctions* jvmF);
	static void* getReceiver();
	virtual ~CpuPlugin();

	static pullsource* registerPullSource(agentCoreFunctions aCF, uint32 provID);
	int startReceiver();
	int stopReceiver();
	static void publishConfig();
	monitordata* pullInt();
	void pullcompleteInt(monitordata* data);
	void receiveMessage(const std::string &id, uint32 size, void* data);

private:
	static bool isEnabled();
	static void setState(const std::string &newState);
	char* buildCpuDataLine(jlong tstamp, jdouble processCPULoad, jdouble systemCPULoad);
	JNIEnv* env;
	jvmFunctions* jvmF;
};

monitordata* pullWrapper();
void pullCompleteWrapper(monitordata* data);
int startWrapper();
int stopWrapper();
const char* getCpuVersion();

}//cpu
}//j9
}//plugins
}//monitoring
}//ibmras


#endif /* CPUPLUGIN_H_ */
