 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/monitoring/connector/jmx/JMXConnectorPlugin.h"

namespace ibmras {
namespace monitoring {
namespace connector {
namespace jmx {

const char* JMXConnectorVersion = "1.0";

JMXConnectorPlugin* instance = NULL;
JMXConnectorPlugin* JMXConnectorPlugin::getInstance(JavaVM *theVM) {
	if (!instance) {
		if (theVM) {
			instance = new JMXConnectorPlugin(theVM);
		}
	}
	return instance;
}

int JMXConnectorPlugin::startPlugin() {
	return 0;
}

int JMXConnectorPlugin::stopPlugin() {
	return 0;
}

void* JMXConnectorPlugin::getConnector(const char* properties) {
	if (instance == NULL) {
		return NULL;
	}
	return instance->getJMXConnector();
}

JMXConnector* JMXConnectorPlugin::getJMXConnector() {
	if (!jmxConnector) {
		jmxConnector = new JMXConnector(vm);
	}
	return jmxConnector;
}

const char* getVersionJMX() {
	return JMXConnectorVersion;
}

JMXConnectorPlugin::JMXConnectorPlugin(JavaVM *theVM) : jmxConnector(NULL), vm(theVM) {
	name = "JMX Connector";
	type = ibmras::monitoring::plugin::connector;
	start = startPlugin;
	stop = stopPlugin;
	getVersion = getVersionJMX;
	confactory = getConnector;
	pull = NULL;
	push = NULL;
	recvfactory = NULL;
}


JMXConnectorPlugin::~JMXConnectorPlugin() {
	if (jmxConnector) {
		delete jmxConnector;
	}
}


} /* namespace jmx */
} /* namespace connector */
} /* namespace monitoring */
} /* namespace ibmras */
