 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/monitoring/plugins/GenericEnablingReceiver.h"
#include "ibmras/monitoring/connector/Receiver.h"
//#include "ibmras/common/logging.h"
#include <string>

namespace ibmras {
namespace monitoring {
namespace plugins {

GenericEnablingReceiver::GenericEnablingReceiver(std::string name, set_enabled_callback *setEnabledCallback) {
	this->name = name;
	this->setEnabledCallback = setEnabledCallback;
}

GenericEnablingReceiver::~GenericEnablingReceiver() {
}

void GenericEnablingReceiver::receiveMessage(const std::string &id, uint32 size, void *data) {
	if (id == name) {
		//IBMRAS_LOG_2(debug, "Received message with id [%s], size [%d]", id.c_str(), size);
		
		std::string message((const char*) data, size);
		//if (size > 0) {
		//	IBMRAS_LOG_1(debug, "Message content [%s]", message.c_str());
		//}
		std::size_t found = message.find(',');
		std::string command = message.substr(0, found);
		std::string rest = message.substr(found + 1);
		
		if (rest == name + "_subsystem") {
			bool enabled = (command == "on");
			//IBMRAS_LOG_2(debug, "Setting [%s] to %s", rest.c_str(), (enabled ? "enabled" : "disabled"));
			setEnabledCallback(enabled);
		}
	} 
}
	
}
}
}
