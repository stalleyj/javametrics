 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_vm_java_healthcenter_h
#define ibmras_vm_java_healthcenter_h

#include "jvmti.h"

struct jvmFunctions {
// 	jvmtiExtensionFunction setTraceOption;
// 	jvmtiExtensionFunction jvmtiRegisterTraceSubscriber;
// 	jvmtiExtensionFunction jvmtiDeregisterTraceSubscriber;
// 	jvmtiExtensionFunction jvmtiGetTraceMetadata;
// 	jvmtiExtensionFunction jvmtiGetMemoryCategories;
// 	jvmtiExtensionFunction jvmtiGetMethodAndClassNames;
// 	jvmtiExtensionFunction jvmtiFlushTraceData;
// 	jvmtiExtensionFunction getJ9method;
// 	jvmtiExtensionFunction jvmtiSetVmDump;
// 	jvmtiExtensionFunction jvmtiQueryVmDump;
// 	jvmtiExtensionFunction jvmtiResetVmDump;
// 	jvmtiExtensionFunction dumpVMLockMonitor;
// 	jvmtiExtensionFunction setVMLockMonitor;
// 	jvmtiExtensionFunction verboseGCsubscribe;
// 	jvmtiExtensionFunction verboseGCunsubscribe;
// 	jvmtiExtensionFunction jvmtiTriggerVmDump;
	
	jvmtiEnv *pti;
	JavaVM *theVM;
};

#endif /* ibmras_vm_java_healthcenter_h */
