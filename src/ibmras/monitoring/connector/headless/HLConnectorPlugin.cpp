 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/monitoring/connector/headless/HLConnectorPlugin.h"
//#include <cstdio>
#include <cstdlib>

namespace ibmras {
namespace monitoring {
namespace connector {
namespace headless {

HLConnectorPlugin* instance = NULL;
static HLConnector* hlConnector = NULL;

const char* hlVersion = "1.0";

const char* getVersionHL() {
	return hlVersion;
}

HLConnectorPlugin::HLConnectorPlugin(JavaVM* theVM) : vm(theVM) {
	name = "Headless Connector";
	type = ibmras::monitoring::plugin::connector;
	start = NULL;
	stop = NULL;
	getVersion = ibmras::monitoring::connector::headless::getVersionHL;
	confactory = getConnector;
	pull = NULL;
	push = NULL;
	recvfactory = NULL;
}

HLConnectorPlugin::~HLConnectorPlugin(){}

HLConnectorPlugin* HLConnectorPlugin::getInstance(JavaVM* theVM) {
	if(!instance) {
		instance = new HLConnectorPlugin(theVM);
	}
	return instance;
}


void* HLConnectorPlugin::getConnector(const char* properties) {
	if (instance == NULL) {
		return NULL;
	}
	return instance->getHLConnector();
}

HLConnector* HLConnectorPlugin::getHLConnector() {
	if(!hlConnector) {
		hlConnector = HLConnector::getInstance(vm);
	}

	return hlConnector;
}




} /*end namespace headless*/
} /*end namespace connector*/
} /*end namespace monitoring*/
} /*end namespace ibmras*/


