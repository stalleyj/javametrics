/*
 * MemoryPlugin.h
 *
 *  Created on: 5 May 2016
 *      Author: Admin
 */

#include "AgentExtensions.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace common {
namespace tprofplugin {

class TprofPlugin {
public:
	static agentCoreFunctions aCF;

	monitordata* OnRequestData();
	void OnComplete(monitordata* data);
	static pullsource* createSource(agentCoreFunctions aCF, uint32 provID);
	static TprofPlugin* getInstance();
	virtual ~TprofPlugin();
	int start();
	int stop();
private:
	uint32 provID;
	static TprofPlugin* instance;
	bool noFailures;

	TprofPlugin(uint32 provID);
	pullsource* createPullSource(uint32 srcid, const char* name);

	static char* NewCString(const std::string& s);
	int64 getTime();


};


extern "C" monitordata* pullWrapper();
extern "C" void pullCompleteWrapper(monitordata* data);

extern "C" {
PLUGIN_API_DECL pullsource* ibmras_monitoring_registerPullSource(agentCoreFunctions aCF, uint32 provID);
PLUGIN_API_DECL int ibmras_monitoring_plugin_init(const char* properties);
PLUGIN_API_DECL int ibmras_monitoring_plugin_start();
PLUGIN_API_DECL int ibmras_monitoring_plugin_stop();
PLUGIN_API_DECL const char* ibmras_monitoring_getVersion();
}
const std::string COMMA = ",";
const std::string EQUALS = "=";


} //tprofplugin
} //common
} //plugins
} //monitoring
} //ibmras


