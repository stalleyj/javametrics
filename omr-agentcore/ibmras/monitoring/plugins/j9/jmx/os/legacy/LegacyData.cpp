/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#include "ibmras/monitoring/plugins/j9/jmx/os/legacy/LegacyData.h"
#include <stdlib.h>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {
namespace os {
namespace legacy {

static const char* DELIMETER = "@#";
static const char* EOL = "\n";

LegacyDataValue::~LegacyDataValue() {
	/* default no-op destructor */
}

LegacyData::LegacyData(const char* eyecatcher, uint64 timestamp) {
	this->eyecatcher = new std::string(eyecatcher);
	this->data = new std::vector<LegacyDataValue*>;
	this->timestamp = timestamp;
}

LegacyData::~LegacyData() {
	data->clear();
	delete data;
	delete eyecatcher;
}

void LegacyData::add(LegacyDataValue* value) {
	data->push_back(value);
}

char* LegacyData::getData() {
	std::vector<LegacyDataValue*>::iterator i = data->begin();
	std::string sdata;
	sdata.append(eyecatcher->c_str());
	sdata.append(DELIMETER);
	sdata.append(ibmras::common::itoa(timestamp));
	for ( ; i != data->end(); ++i) {
		sdata.append(DELIMETER);
		sdata.append((*i)->toString());
	}
	sdata.append(EOL);
	size_t len = sdata.size() + 1;	/* add space for null terminator */
	char* p = new char[len];
	if(p) {
		sdata.copy((char*)p, len - 1, 0);
		*(p + len - 1) = '\0';
		return p;			/* the memory will be free'd when the monitor data is cleaned up */
	}
	return NULL;
}

} /* namespace legacy */
} /* namespace os */
} /* namespace jmx */
} /* namespace j9 */
} /* namespace plugins */
} /* namespace monitoring */
} /* namespace ibmras */
