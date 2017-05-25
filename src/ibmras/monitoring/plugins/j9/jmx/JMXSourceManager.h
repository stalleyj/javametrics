 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_plugins_j9_jmx_jmxsourcemanager_h
#define ibmras_monitoring_plugins_j9_jmx_jmxsourcemanager_h

/* declarations for the plugin that will provide OS metrics */
#include <jni.h>
#include "ibmras/common/port/ThreadData.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMX.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXPullSource.h"
#include <cstdio>
#include <string>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {

/*
 * The JMXSourceManager provides the registration point for the agent.
 * It also controls stopping and starting any configured pull sources.
 */

class JMXSourceManager {
public:
	JMXSourceManager();
	int start();
	int stop();
	pullsource* registerPullSource(uint32 provID);
	static const uint32 DEFAULT_CAPACITY = 1024;
	uint32 getProvID();
private:
	uint32 provid;
	bool running;
	JMXPullSource** pullsources;
};

}	/* end namespace jmx */
}	/* end namespace j9 */
}	/* end namespace plugins */
}	/* end namespace monitoring */
}	/* end namespace ibmras */


#endif /* ibmras_monitoring_plugins_j9_jmx_jmxsourcemanager_h */
