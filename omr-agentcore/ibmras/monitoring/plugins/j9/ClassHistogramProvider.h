 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_plugins_j9_classHistogram
#define ibmras_monitoring_plugins_j9_classHistogram

#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"


namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace classhistogram {

const char* getchpVersion();

class ClassHistogramProvider: public ibmras::monitoring::connector::Receiver, public ibmras::monitoring::Plugin {
public:
	ClassHistogramProvider(jvmFunctions functions);
	virtual ~ClassHistogramProvider();
	int startReceiver();
	int stopReceiver();
	void publishConfig();
	void receiveMessage(const std::string &id, uint32 size, void *data);
	static pushsource* registerPushSource(agentCoreFunctions aCF, uint32 provID);
	static uint32 providerID;
	static ClassHistogramProvider* getInstance(jvmFunctions functions);
	static void* getInstance();
private:
	jvmFunctions vmFunctions;

	static monitordata* generateData(uint32 sourceID, const char *dataToSend, int size);
	std::string createHistogramReport();
	unsigned char* hc_alloc(int size);
	void hc_dealloc(unsigned char** buffer);
	//void force2Native(char * str);
	//static jvmtiIterationControl JNICALL updateClassTotals(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data);
};
}
}
}
}
}/* namespace monitoring */
#endif /* ibmras_monitoring_plugins_j9_classHistogram */
