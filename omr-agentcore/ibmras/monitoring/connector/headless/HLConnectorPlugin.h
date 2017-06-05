 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_connector_headless_hlconnectorplugin_h
#define ibmras_monitoring_connector_headless_hlconnectorplugin_h

#include <ibmras/monitoring/Plugin.h>
#include "ibmras/monitoring/connector/headless/HLConnector.h"
#include "jni.h"

namespace ibmras {
namespace monitoring {
namespace connector {
namespace headless {

const char* getVersionHL();

class HLConnectorPlugin: public ibmras::monitoring::Plugin {
public:
	static HLConnectorPlugin* getInstance(JavaVM* theVM);
	virtual ~HLConnectorPlugin();

protected:
	HLConnectorPlugin(JavaVM* theVM);

private:

	static void* getConnector(const char* properties);

	HLConnector* getHLConnector();
	JavaVM* vm;

};


} /*end namespace headless*/
} /*end namespace connector*/
} /*end namespace monitoring*/
} /*end namespace ibmras*/



#endif /* ibmras_monitoring_connector_headless_hlconnectorplugin_h */
