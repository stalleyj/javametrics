 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#include "ibmras/monitoring/Plugin.h"
#include "ibmras/vm/java/healthcenter.h"
#include "ibmras/monitoring/plugins/j9/DumpHandler.h"
#include "ibmjvmti.h"
#include "jni.h"
#include "jvmti.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/MemoryManager.h"

#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/monitoring/plugins/j9/Util.h"


#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include<stdlib.h>

#define JNI_VERSION JNI_VERSION_1_4

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {

uint32 DumpHandler::providerID = 0;

const char* dumpVersion = "1.0";

int startHandler() {
	return 0;
}

int stopHandler() {
	return 0;
}

const char* getDumpVersion() {
	return dumpVersion;
}

DumpHandler::DumpHandler(jvmFunctions functions) {
	vmFunctions = functions;
	name = "Dump Handler";
	pull = NULL;
	push = NULL;
	start = ibmras::monitoring::plugins::j9::startHandler;
	stop = ibmras::monitoring::plugins::j9::stopHandler;
	getVersion = getDumpVersion;
	type = ibmras::monitoring::plugin::none;
	recvfactory = NULL;
	confactory = NULL;
}

DumpHandler::~DumpHandler() {
}

DumpHandler* instance = NULL;

DumpHandler* DumpHandler::getInstance(jvmFunctions functions) {
    // Does the vm have this function available	
    if (!functions.jvmtiRegisterTraceSubscriber == NULL) {
        if (!instance) {
            instance = new DumpHandler(functions);
        }
    }
	return instance;
}

DumpHandler* DumpHandler::getInstance() {
	if (!instance) {
		return NULL;
	}
	return instance;
}

void DumpHandler::requestDumps(const std::vector<std::string> &dumpRequests) {
	if (!ibmras::monitoring::agent::Agent::getInstance()->readOnly()) {
		getInstance()->triggerDumps(dumpRequests);
	}
}

void DumpHandler::triggerDumps(const std::vector<std::string> &dumpRequests) {
	JNIEnv * env = NULL;

	ibmras::monitoring::plugins::j9::setEnv(&env, "Health Center (DumpHandler)", vmFunctions.theVM, false);

	for (std::vector<std::string>::const_iterator it = dumpRequests.begin();
			it != dumpRequests.end(); ++it) {
		const std::string parameter = (*it);
		std::vector < std::string > parts = ibmras::common::util::split(
				parameter, '=');

		// strip dump from end of parameter javadump, systemdump or heapdump
		if (ibmras::common::util::equalsIgnoreCase(parts[1], "true")) {

			std::string type = parts[0].substr(0, (parts[0].length()-4));

			if (vmFunctions.jvmtiTriggerVmDump != 0) {
				char * dumpType = ibmras::common::util::createAsciiString(type.c_str());
				if (dumpType) {
					vmFunctions.jvmtiTriggerVmDump(vmFunctions.pti, dumpType);
				}
				ibmras::common::memory::deallocate((unsigned char**)&dumpType);
			}
		}

	}

	vmFunctions.theVM->DetachCurrentThread();
}

}
}
}
} /* end namespace methods */

