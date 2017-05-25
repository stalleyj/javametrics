 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_plugins_j9_methods
#define ibmras_monitoring_plugins_j9_methods

#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/common/port/Lock.h"
#include <set>

#include "jni.h"


namespace ibmras{
namespace monitoring {
namespace plugins {
namespace j9 {
namespace methods {

const char* getmlpVersion(void);

class MethodLookupProvider: public ibmras::monitoring::connector::Receiver, public ibmras::monitoring::Plugin {
public:

	virtual ~MethodLookupProvider();

	static MethodLookupProvider* getInstance(jvmFunctions functions);

	static void* getReceiver();
	void receiveMessage(const std::string &id, uint32 size, void *data);

	static pullsource* registerPullSource(agentCoreFunctions aCF, uint32 provID);



	static monitordata* getData(void);			/* shortcut definition for the pull source callback */
	static void complete(monitordata*);	/* callback to indicate when the data source can free / re-use the memory */

private:
	MethodLookupProvider(jvmFunctions functions);
	static MethodLookupProvider* instance;

	void getAllMethodIDs();
	monitordata* getMethodData(void);

	uint32 providerID;
	jvmFunctions vmFunctions;
	unsigned char* hc_alloc(int size);
	void hc_dealloc(unsigned char** buffer);

	monitordata* generateData(uint32 sourceID, const char *dataToSend, int size, bool persistentData);

	std::set<void*> methodsToLookup;
	ibmras::common::port::Lock methodSetLock;
	bool sendHeader;
	bool initialHeaderSent;

	JNIEnv *env;

	bool getAllMethods;

	bool enabled;
};
}
}
}
}
}/* namespace monitoring */
#endif /* ibmras_monitoring_plugins_j9_methods */
