/*
 * MemCountersPlugin.h
 *
 *  Created on: 20 Feb 2016
 *      Author: Admin
 */

#ifndef ibmras_monitoring_plugins_j9_memorycounters_memcountersplugin_h_
#define ibmras_monitoring_plugins_j9_memorycounters_memcountersplugin_h_

#include "ibmras/common/util/memUtils.h"
#include "healthcenter.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "healthcenter.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace memorycounters {

class MemCountersPlugin : public ibmras::monitoring::Plugin, public ibmras::monitoring::connector::Receiver {
public:
	MemCountersPlugin(jvmFunctions jvmF);
	virtual ~MemCountersPlugin();
	static MemCountersPlugin* getPlugin(jvmFunctions jvmF);
	static void* getReceiver();

	static pullsource* registerPullSource(agentCoreFunctions aCF, uint32 provID);
	int startReceiver();
	int stopReceiver();
	monitordata* pullInt();
	void pullcompleteInt(monitordata* data);
	void receiveMessage(const std::string &id, uint32 size, void* data);

private:
	void publishConfig();
	bool isEnabled();
	void setState(const std::string &newState);
	JNIEnv* env;
	jvmFunctions jvmF;
};

monitordata* pullWrapper();
void pullCompleteWrapper(monitordata* data);
int startWrapper();
int stopWrapper();
const char* getMCVersion();

char* getMemoryCounters(JNIEnv *env, jvmFunctions jvmF);

void dealloc_report_lines(char *lines[], int count);
char* join_strings(char *strings[], int count);



}
}
}
}
}



#endif /* MEMCOUNTERSPLUGIN_H_ */
