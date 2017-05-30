 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_j9_trace_tracereceiver_h
#define ibmras_monitoring_plugins_j9_trace_tracereceiver_h

#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"

#include "ibmras/common/port/Lock.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace trace {

class TraceReceiver: public ibmras::monitoring::connector::Receiver, public ibmras::monitoring::Plugin {
public:
	TraceReceiver();
	virtual ~TraceReceiver();
	int startReceiver();
	int stopReceiver();
	void receiveMessage(const std::string &id, uint32 size, void *data);
private:
	ibmras::common::port::Lock receiverLock;
};

}
}
}
}
}/* namespace trace */
#endif /* ibmras_monitoring_plugins_j9_trace_tracereceiver_h */
