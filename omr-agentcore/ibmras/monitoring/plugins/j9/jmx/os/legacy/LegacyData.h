/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_j9_jmx_os_legacy_legacydata_h
#define ibmras_monitoring_plugins_j9_jmx_os_legacy_legacydata_h

#include "ibmras/common/port/ThreadData.h"
#include "ibmras/common/common.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {
namespace os {
namespace legacy {

/*
 * Superclass for all legacy data values. Forces specific subclasses to implement
 * the toString() method which is invoked when sending data to the client.
 */
class LegacyDataValue {
public:
	virtual std::string toString() = 0;		/* string representation of the data to be sent back to the client */
	virtual ~LegacyDataValue();
};

/*
 * Template class to handle the different types of numeric data that can be returned
 * int, long, double etc. etc.
 */
template<class V>
class LegacyDataNumeric : public LegacyDataValue {
public:
	LegacyDataNumeric(V value) { this->value = value; }
	std::string toString() {
		return ibmras::common::itoa(value);
	}
private:
	V value;
};

/*
 * Legacy data is that defined as being used by HC versions earlier than 3.0.
 *
 * The format expected is <EYECATCHER><DELIM><TIMESTAMP><DELIM> followed by delimeted data
 */

class LegacyData {
public:
	LegacyData(const char* eyecatcher, uint64 timestamp);
	void add(LegacyDataValue* value);
	char* getData();
	virtual ~LegacyData();
private:
	uint64 timestamp;
	std::string* eyecatcher;
	std::vector<LegacyDataValue*>* data;
};

} /* namespace legacy */
} /* namespace os */
} /* namespace jmx */
} /* namespace j9 */
} /* namespace plugins */
} /* namespace monitoring */
} /* namespace ibmras */
#endif /* ibmras_monitoring_plugins_j9_jmx_os_legacy_legacydata_h */
