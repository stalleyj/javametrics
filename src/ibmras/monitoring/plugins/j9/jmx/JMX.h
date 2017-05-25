 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_j9_jmx_jmx_h
#define ibmras_monitoring_plugins_j9_jmx_jmx_h

/*
 * These declarations are in a separate file to prevent circular inclusion
 * by the JMXSourceManager and specific JMX data provider plugins.
 */

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {

DECL JavaVM* getJVM();
DECL void setJVM(JavaVM* vm);

DECL ibmras::monitoring::Plugin* getPlugin();

}
}
}
}
}

#endif /* ibmras_monitoring_plugins_j9_jmx_jmx_h */
