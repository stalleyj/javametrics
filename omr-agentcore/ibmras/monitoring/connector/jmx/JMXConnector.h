 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_agent_connector_jmxconnector_h
#define ibmras_monitoring_agent_connector_jmxconnector_h

#include "ibmras/monitoring/connector/Connector.h"

#include "jni.h"

namespace ibmras {
namespace monitoring {
namespace connector {
namespace jmx {

class JMXConnector: public ibmras::monitoring::connector::Connector {
public:
	virtual std::string getID() { return "JMX Connector"; }

	JMXConnector(JavaVM *theVM);
	virtual ~JMXConnector();

	virtual int start();
	virtual int stop();

protected:

private:
	int launchMBean();

	JavaVM *vm;
};

} /* namespace jmx */
} /* namespace connector */
} /* namespace monitoring */
} /* namespace ibmras */

#endif /* ibmras_monitoring_agent_connector_jmxconnector_h */
