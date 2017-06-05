 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_plugins_omr_methodlookupprovider_h
#define ibmras_monitoring_plugins_omr_methodlookupprovider_h

#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"


namespace plugins {
namespace omr {
namespace methods {



class MethodLookupProvider: public ibmras::monitoring::connector::Receiver, public ibmras::monitoring::Plugin {
public:
	MethodLookupProvider(omrRunTimeProviderParameters functions);
	virtual ~MethodLookupProvider();
	int startReceiver();
	int stopReceiver();
	void receiveMessage(const std::string &id, uint32 size, void *data);
	static pushsource* registerPushSource(agentCoreFunctions aCF, uint32 provID);
	static uint32 providerID;
	static MethodLookupProvider* getInstance(omrRunTimeProviderParameters oRTPP);
	static void* getInstance();
	void getMethodIDs(std::vector<std::string> &parameters);
	static void sendMethodDictionary(bool persistent);
private:
	omrRunTimeProviderParameters vmData;

	static monitordata* generateData(uint32 sourceID, const char *dataToSend, int size);
};
}
}
}/* namespace monitoring */
#endif /* ibmras_monitoring_plugins_omr_methodlookupprovider_h */
