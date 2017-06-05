 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#include <iostream>
#include <sstream>
#include "ibmras/monitoring/plugins/j9/trace/TraceReceiver.h"
#include "ibmras/monitoring/plugins/j9/trace/TraceDataProvider.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/monitoring/Plugin.h"
#include "ibmras/common/util/strUtils.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace trace {

int startReceiver() {
	return 0;
}

int stopReceiver() {
	return 0;
}

TraceReceiver::TraceReceiver() {
}

TraceReceiver::~TraceReceiver() {
}

void TraceReceiver::receiveMessage(const std::string &id, uint32 size,
		void *data) {

	std::string message((const char*) data, size);

	if ((id == "trace") || (id == "TRACESubscriberSourceConfiguration")) {
		if (message == "header") {
			ibmras::monitoring::plugins::j9::trace::sendTraceHeader(false);
		} else {
			try {
				receiverLock.acquire();
				std::size_t found = message.find(',');
				if (found != std::string::npos) {
					std::string command = message.substr(0, found);
					std::string rest = message.substr(found + 1);
					std::vector<std::string> parameters =
							ibmras::common::util::split(rest, ',');
					ibmras::monitoring::plugins::j9::trace::handleCommand(
							command, parameters);
				}
			} catch (...) {

			}
			receiverLock.release();
		}
	}
}

}
}
}
}
} /* end namespace agent */
