 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/common/port/ThreadData.h"
#include "ibmras/common/port/Lock.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include <string>
#include <iostream>
#include "ibmras/common/logging.h"

#define DEFAULT_CAPACITY 10240	/* default bucket capacity = 1MB */

namespace plugin {

bool running = false;
void (*callback)(monitordata*);
uint32 provid = 0;
ibmras::common::port::Lock* lock = new ibmras::common::port::Lock;

} /* end plugin namespace */


IBMRAS_DEFINE_LOGGER("TestPlugin");

uint32 srcid = 0;
uint32 pushSrcIDStart = 0;

pushsource* createPushSource(uint32 srcid, const char* name) {
	pushsource *src = new pushsource();
	src->header.name = name;
	std::string desc("Description for ");
	desc.append(name);
	src->header.description = desc.c_str();
	src->header.sourceID = srcid;
	src->next = NULL;
	src->header.capacity = (DEFAULT_CAPACITY / (srcid+1));
	return src;
}

/*
 * Test push source which returns three dummy sources
 */

pushsource* ibmras_monitoring_registerPushSource(void (*callback)(monitordata*), uint32 provID) {
	IBMRAS_DEBUG(info, "Registering push sources");
	pushsource *head = createPushSource(srcid++, "PushSource1");
	pushsource *src = createPushSource(srcid++, "PushSource2");
	pushsource *src2 = createPushSource(srcid++, "PushSource3");
	head->next = src;
	src->next = src2;
	plugin::callback = callback;
	plugin::provid = provID;
	return head;
}

monitordata* generateData(uint32 srcOffset) {
	static uint32 id = 0;
	const int size = 1024;
	monitordata* data = new monitordata;
	data->persistent = false;
	std::string str;
	char* buffer = new char[size];
	str.append(size, 'x');
	str.copy(buffer, size, 0);
	data->provID = plugin::provid;
	data->data = buffer;
	data->size = size;
	id = ((id + 1) % 3) + srcOffset;
	data->sourceID = id;
	return data;
}

monitordata* pullCallback() {
	plugin::lock->acquire();
	IBMRAS_DEBUG(fine, "Generating data for pull from agent");
	monitordata* data = generateData(pushSrcIDStart);
	plugin::lock->release();
	return data;
}

void pullComplete(monitordata* data) {
	if (data != NULL) {
		if (data->data != NULL) {
			delete[] data->data;
		}
		delete data;
	}
}

pullsource* createPullSource(uint32 srcid, const char* name) {
	pullsource *src = new pullsource();
	src->header.name = name;
	std::string desc("Description for ");
	desc.append(name);
	src->header.description = desc.c_str();
	src->header.sourceID = srcid;
	src->header.capacity = (DEFAULT_CAPACITY / (srcid+1));
	src->next = NULL;
	src->callback = pullCallback;
	src->complete = pullComplete;
	src->pullInterval = srcid;	/* space pull intervals apart for successive calls */
	return src;
}

/*
 * Test push source which returns three dummy sources
 */

pullsource* ibmras_monitoring_registerPullSource(uint32 provID) {
	IBMRAS_DEBUG(info, "Registering pull sources");
	pushSrcIDStart = srcid;
	pullsource *head = createPullSource(srcid++, "PullSource1");
	pullsource *src = createPullSource(srcid++, "PullSource2");
	pullsource *src2 = createPullSource(srcid++, "PullSource3");
	head->next = src;
	src->next = src2;
	return head;
}


void* callback(ibmras::common::port::ThreadData* param) {
	const int size = 1024;
	monitordata data;
	std::string str;
	char buffer[size];
	str.append(size, 'x');
	str.copy(buffer, size, 0);
	data.persistent = false;
	data.provID = plugin::provid;
	uint32 id = 0;
	data.data = buffer;
	data.size = size;
	while(plugin::running) {
		ibmras::common::port::sleep(1);
		id = (id + 1) % 3;
		data.sourceID = id;
		plugin::callback(&data);
	}
	ibmras::common::port::exitThread(NULL);
	return NULL;
}

int ibmras_monitoring_plugin_start() {
	int result = 0;
	IBMRAS_DEBUG(info, "Starting");
	plugin::running = true;
	ibmras::common::port::ThreadData* data = new ibmras::common::port::ThreadData(callback);
	result = ibmras::common::port::createThread(data);
	if(result) {
		IBMRAS_DEBUG(warning, "unable to create call back thread");
	}
	return 0;
}

int ibmras_monitoring_plugin_stop() {
	IBMRAS_DEBUG(info, "Stopping");
	plugin::running = false;
	return 0;

}
