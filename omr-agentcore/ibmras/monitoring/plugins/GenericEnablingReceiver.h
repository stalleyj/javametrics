 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef GENERICENABLINGRECEIVER_H_
#define GENERICENABLINGRECEIVER_H_

#include "ibmras/export.h"
#include "ibmras/monitoring/connector/Receiver.h"

typedef void set_enabled_callback(bool);

namespace ibmras {
namespace monitoring {
namespace plugins {

class DECL GenericEnablingReceiver : public ibmras::monitoring::connector::Receiver {
public:
	GenericEnablingReceiver(std::string name, set_enabled_callback *setEnabledCallback);
	virtual ~GenericEnablingReceiver();
	void receiveMessage(const std::string &id, uint32 size, void *data);
	
private:
	std::string name;
	set_enabled_callback *setEnabledCallback;
};
	
}
}
}

#endif /* GENERICENABLINGRECEIVER_H_ */