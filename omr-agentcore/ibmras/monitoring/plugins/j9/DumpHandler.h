 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_j9_dumphandler_h
#define ibmras_monitoring_plugins_j9_dumphandler_h

#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"

namespace ibmras{
namespace monitoring {
namespace plugins {
namespace j9 {

const char* getDumpVersion();

class DumpHandler: public ibmras::monitoring::Plugin {
public:
	DumpHandler(jvmFunctions functions);
	virtual ~DumpHandler();
	int startHandler();
	int stopHandler();
	static uint32 providerID;
	static DumpHandler* getInstance(jvmFunctions functions);
	static DumpHandler* getInstance();
	static void requestDumps(const std::vector<std::string> &dumpRequests);
	void triggerDumps(const std::vector<std::string> &dumpRequests);
private:
	jvmFunctions vmFunctions;

};

}
}
}
}/* namespace monitoring */
#endif /* ibmras_monitoring_plugins_j9_dumphandler_h */
