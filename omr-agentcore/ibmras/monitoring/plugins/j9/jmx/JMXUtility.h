 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_plugins_j9_jmx_jmxutility_h
#define ibmras_monitoring_plugins_j9_jmx_jmxutility_h

/* declarations for the plugin that will provide OS metrics */
#include <jni.h>
#include "ibmras/common/port/ThreadData.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMX.h"
#include "ibmras/common/logging.h"
#include <cstdio>
#include <string>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {

extern IBMRAS_DECLARE_LOGGER;

namespace os {
	JMXPullSource* getOSPullSource(uint32 id);		/* forward declaration of OS bean provider */
}

namespace rt {
	JMXPullSource* getRTPullSource(uint32 id);		/* forward declaration of RT bean provider */
}


extern JavaVM* vm;

extern void complete(monitordata* data);	/* default clean-up for generated data */

/*
 * Utility functions that can be used by individual pull sources
 * to get data via JMX
 */

/* get a named MX bean from the ManagementFactory class - will need enhancing if call something that is XYXBeans rather than XYZBean */
jobject getMXBean(JNIEnv* env, jclass* mgtBean, const char* name);
/* get a double from an object instance method */
jdouble getDouble(JNIEnv* env, jobject* obj, const char* cname, const char* mname, jclass* jc = NULL);
/* get a long from an object instance method */
jlong getLong(JNIEnv* env, jobject* obj, const char* cname, const char* mname, jclass* jc = NULL);
/* get a string from an object instance method */
char* getString(JNIEnv* env, jobject* obj, const char* cname, const char* mname, jclass* jc = NULL);
/*
 * Gets a method id. If the jclass parameter is NULL then it will be treated as an instance
 * method when querying the JVM. If the method is static then the resultant jclass parameter
 * will be populated with the class to subsequently use to invoke the method
 */
jmethodID getMethod(JNIEnv* env, const char* cname, const char* mname, const char* sig, jclass* jc = NULL);
/* get a time stamp via the System.currentTimeMillis method */
jlong getTimestamp(JNIEnv* env);


}	/* end namespace jmx */
}	/* end namespace j9 */
}	/* end namespace plugins */
}	/* end namespace monitoring */
}	/* end namespace ibmras */


#endif /* ibmras_monitoring_plugins_j9_jmx_jmxutility_h */
