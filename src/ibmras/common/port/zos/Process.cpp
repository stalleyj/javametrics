/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#define _XOPEN_SOURCE_EXTENDED 1
#include <unistd.h>

#include "ibmras/common/port/ThreadData.h"

namespace ibmras {
namespace common {
namespace port {

int getProcessId() {
	return getpid();
}

std::string getHostName() {
	char buffer[256];
	if (gethostname(buffer, 255) == 0) {
		return buffer;
	} else {
		return "unknown";
	}
}


}
}
} /* end namespace port */
