 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_connector_jmx_jmxconnectorplugin_h
#define ibmras_monitoring_connector_jmx_jmxconnectorplugin_h

#include "ibmras/monitoring/Plugin.h"
#include "ibmras/monitoring/connector/jmx/JMXConnector.h"

namespace ibmras {
namespace monitoring {
namespace connector {
namespace jmx {

const char* getVersionJMX();

class JMXConnectorPlugin: public ibmras::monitoring::Plugin {
public:
	static JMXConnectorPlugin* getInstance(JavaVM *theVM);
	virtual ~JMXConnectorPlugin();

	static int startPlugin();
	static int stopPlugin();
	static void* getConnector(const char* properties);

protected:
	JMXConnectorPlugin(JavaVM *theVM);

private:
	JMXConnector *jmxConnector;
	JMXConnector* getJMXConnector();

	JavaVM *vm;
};

} /* namespace jmx */
} /* namespace connector */
} /* namespace monitoring */
} /* namespace ibmras */

#endif /* ibmras_monitoring_connector_jmx_jmxconnectorplugin_h */
