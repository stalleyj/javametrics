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
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include <string>
#include <iostream>
#include "ibmras/common/logging.h"

#define DEFAULT_CAPACITY 1024	//Capacity of the bucket that will host the pushsource

namespace pullplugin{

IBMRAS_DEFINE_LOGGER("Pullsource");

ibmras::common::port::Lock* lock = new ibmras::common::port::Lock;

uint32 provid = 0;
class pulltemplate{
public:
	pullsource* registerPullSource(uint32 provID);
	static monitordata* pullCallback();
	static void pullComplete(monitordata*);
	int start();
	int stop();
};

uint32 srcid = 0;
uint32 pushSrcIDStart = 0;

pulltemplate* pt;

monitordata* generateData(uint32 srcOffset) {
	/**
	 * This method here only generates the mock data that is provided by
	 * any of the pullsources that were defined in the code
	 */
	static uint32 id = 0;
	const int size = 1024;
	monitordata* data = new monitordata;
	data->persistent = false;
	std::string str;
	char* buffer = new char[size];
	str.append(size, 'x');
	str.copy(buffer, size, 0);
	data->provID = pullplugin::provid;
	data->data = buffer;
	data->size = size;
	id = ((id + 1) % 3) + srcOffset;
	data->sourceID = id;
	return data;
}

monitordata* pulltemplate::pullCallback() {
	/**
	 * This method is called by the agent to get the information from the pullsource,
	 * in this case the mock data is produced from the "generateData" method.
	 */
	pullplugin::lock->acquire();
	IBMRAS_DEBUG(fine, "Generating data for pull from agent");
	monitordata* data = generateData(pushSrcIDStart);
	pullplugin::lock->release();
	return data;
}



void pulltemplate::pullComplete(monitordata* data) {
	if (data != NULL) {
		if (data->data != NULL) {
			delete[] data->data;
		}
		delete data;
	}
}

pullsource* createPullSource(uint32 srcid, const char* name) {
	/**
	 * 	This method serves to specify the fields of the pullsource and its header, structures which are defined as
	 * 	follows:
	 *
	 *	typedef struct pullsource{
	 *	srcheader header;			* common source header
	 *	pullsource *next;			* the next source or null if this is the last one in the list
	 *	uint32 pullInterval;		* time in seconds at which data should be pulled from this source
	 *	PULL_CALLBACK callback;
	 *	PULL_CALLBACK_COMPLETE complete;
	 *} pullsource;
	 *
	 *	typedef struct srcheader {
	 *	uint32 sourceID;			* ID assigned by the provider - unique by provider
	 *	uint32 capacity;			* the amount of space in bytes that should be allocated for this source (bucket size)
	 *	const char *description;	* null terminated C string
	 *} srcheader;
	 *
	 *
	 */
	pullsource *src = new pullsource();
	src->header.name = name;
	std::string desc("Description for ");
	desc.append(name);
	src->header.description = desc.c_str();
	src->header.sourceID = srcid;
	src->header.capacity = (DEFAULT_CAPACITY / (srcid+1));
	src->next = NULL;
	src->callback = pulltemplate::pullCallback;
	src->complete = pulltemplate::pullComplete;
	src->pullInterval = srcid;	/* space pull intervals apart for successive calls */
	return src;
}



pullsource* pulltemplate::registerPullSource(uint32 provID) {
	/**
	 *	This method is called by the agent to discover the pull sources the plugin has.
	 *	In the call, the agent provides us with a data provider ID, provID, which will
	 *	differentiate this data provider from the others.
	 *
	 *	It returns a pointer to the first element of a singly linked list formed by all the pull
	 *	sources that the plugin has.
	 */
	IBMRAS_DEBUG(fine, "Registering pull sources");
	pushSrcIDStart = srcid;
	pullsource *head = createPullSource(srcid++, "PullSource1");
	pullsource *src = createPullSource(srcid++, "PullSource2");
	pullsource *src2 = createPullSource(srcid++, "PullSource3");
	head->next = src;
	src->next = src2;
	return head;
}

int pulltemplate::start() {

	/**
	 * This method is exposed and will be called by the agent when starting all the plugins, anything
	 * required to start the plugin has to be added here. In this case, there is just a pullsource, it
	 * does not require any kind of initialization.
	 */
	IBMRAS_DEBUG(info, "Starting");

	return 0;
}

int pulltemplate::stop() {
	/**
	 * The stop method will be called by the agent on shutdown, here is where any cleanup has to be done
	 */
	IBMRAS_DEBUG(info, "Stopping");
	return 0;

}
}/*end of namespace pullplugin*/

extern "C" {

	pullsource* ibmras_monitoring_registerPullSource(uint32 provID) {
	  return pullplugin::pt->registerPullSource(provID);
	}

	int ibmras_monitoring_plugin_start() {
	  return pullplugin::pt->start();
	}

	int ibmras_monitoring_plugin_stop() {
	  return pullplugin::pt->stop();
	}

	monitordata* pullCallback(){
		return pullplugin::pulltemplate::pullCallback();
	}

	void pullComplete(monitordata* md){
		pullplugin::pulltemplate::pullComplete(md);
	}

}
