 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_plugins_j9_jmx_jmxpullsource_h
#define ibmras_monitoring_plugins_j9_jmx_jmxpullsource_h

/* declarations for the plugin that will provide OS metrics */
#include <jni.h>
#include "ibmras/common/port/ThreadData.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMX.h"
#include <cstdio>
#include <string>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {

/*
 * This enum corresponds to the source IDs for each of the JMX data providers.
 */
enum PullSourceIDs {
	CPU,
	PULL_COUNT, // Pull Count here so as not to use RT for now
	RT
};

/*
 * This is the pure virtual class that specific JMX pull sources need to inherit from.
 * When the agent calls back for data, you need to attach the current agent thread to
 * the JVM using the JNI->attachThread call otherwise you cannot see the running Java code.
 * This is why all agent calls come into the superclass generateData() method, where this
 * attachment is performed before calling the subclass specific generateData(JNIEnv*, jclass*) method.
 * It will also locate the ManagementFactory class (which is the second parameter). Because
 * the VM thread will change over time you cannot cache the JNIEnv or other values.
 */

class JMXPullSource {
public:
	JMXPullSource(uint32 provID, const std::string& providerName);
	virtual ~JMXPullSource();										/* destructor for clean up operations */

	virtual pullsource* getDescriptor() {return NULL;};			/* descriptor for this pull source */
	uint32 getProvID() {return provID; }						/* JMX provider ID */
	virtual uint32 getSourceID() = 0;							/* source ID, overridden by the subclass, and corresponds to the enum entries */
	monitordata* generateData();								/* where the agent will call into */
	void pullComplete(monitordata* mdata);
	monitordata* generateError(char* msg);						/* Wrap an error message to send back */
	virtual void publishConfig() = 0;							/* sub classes must implement to publish their configuration details */
protected:
	virtual monitordata* generateData(JNIEnv* env, jclass* mgtBean) = 0 ;		/* sub classes must implement this to get the right JNI env to use to retrieve data */
	uint32 provID;					/* JMX provider ID assigned by the agent */
	JNIEnv* env;
	std::string name;
};


}	/* end namespace jmx */
}	/* end namespace j9 */
}	/* end namespace plugins */
}	/* end namespace monitoring */
}	/* end namespace ibmras */


#endif /* ibmras_monitoring_plugins_j9_jmx_jmxpullsource_h */
