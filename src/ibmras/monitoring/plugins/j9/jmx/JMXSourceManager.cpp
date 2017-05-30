 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/monitoring/plugins/j9/jmx/JMXSourceManager.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXUtility.h"
#include "ibmras/common/logging.h"
#include <cstring>
#include <stdlib.h>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {


JMXSourceManager::JMXSourceManager() {
	running = false;
	provid = -1;
	pullsources = NULL;
	vm = NULL;
}

pullsource* JMXSourceManager::registerPullSource(uint32 provID) {
	IBMRAS_DEBUG(fine, "Registering pull sources");
	provid = provID;
	pullsources = new JMXPullSource*[PULL_COUNT];
	pullsources[CPU] = ibmras::monitoring::plugins::j9::jmx::os::getOSPullSource(provID);
	pullsource* src = pullsources[CPU]->getDescriptor();

	return src;
}

int JMXSourceManager::start() {
	IBMRAS_DEBUG(info, "Starting");

	for (uint32 i = 0; i < PULL_COUNT; i++) {
		JMXPullSource* p = pullsources[i];
		if (p) {
			p->publishConfig();
		}
	}
	return 0;
}

int JMXSourceManager::stop() {
	IBMRAS_DEBUG(info, "Stopping");
// Don't delete the sources when we are just stopping
//	for(uint32 i = 0; i < PULL_COUNT; i++) {
//		JMXPullSource* p = pullsources[i];
//		delete p;
//	}
	return 0;
}

uint32 JMXSourceManager::getProvID() {
	return provid;
}

}	/* end namespace jmx */
}	/* end namespace j9 */
}	/* end namespace plugins */
}	/* end namespace monitoring */
} 	/* end namespace ibmras */


