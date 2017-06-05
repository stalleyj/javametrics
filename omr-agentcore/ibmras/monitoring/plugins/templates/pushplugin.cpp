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
#include <cstring>
#include "ibmras/common/logging.h"


#define DEFAULT_CAPACITY 1024	//Capacity of the bucket that will host the pushsource

namespace pushplugin {

IBMRAS_DEFINE_LOGGER("pushtemplate");

bool running = false;
void (*callback)(monitordata*);
uint32 provid = 0;

class pushtemplate{
public:
	pushsource* registerPushSource(void (*callback)(monitordata*), uint32 provID);
	int start();
	int stop();
};

uint32 srcid = 0;

pushtemplate* pt;


extern "C"{

pushsource* createPushSource(uint32 srcid, const char* name) {
	/**
	 * 	This method serves to specify the fields of the pushsource and its header, structures which are defined as
	 * 	follows:
	 *
	 *	typedef struct pushsource {
	 *	srcheader header;			* common source header
	 *	pushsource *next;			* next source or null if this is the last one in the list
	 *} pushsource;
	 *
	 *	typedef struct srcheader {
	 *	uint32 sourceID;			* ID assigned by the provider - unique by provider
	 *	uint32 capacity;			* the amount of space in bytes that should be allocated for this source (bucket size)
	 *	const char *description;	* null terminated C string
	 *} srcheader;
	 *
	 *
	 */
	pushsource *src = new pushsource();
	src->header.name = name;
	std::string desc("Generic pushsource template ");
	desc.append(name);
	src->header.description = desc.c_str();
	src->header.sourceID = srcid;
	src->header.capacity = DEFAULT_CAPACITY;
	src->next = NULL;
	return src;
}



pushsource* pushtemplate::registerPushSource(void (*callback)(monitordata*), uint32 provID) {
	/**
	 *	This method is called by the agent to discover the push sources the plugin has.
	 *	In the call, the agent provides us with the callback function that the plugin will
	 *	use to push data into the agent as well as with a data provider ID, provID, which will
	 *	differentiate this data provider from the others.
	 *
	 *	It returns a pointer to the first element of a singly linked list formed by all the push
	 *	sources that the plugin has. (Just one element/source in this case)
	 */
	IBMRAS_DEBUG(info, "Registering push sources");
	pushsource *head = createPushSource(srcid, "PushSource");
	head->next = NULL;
	pushplugin::callback = callback;
	pushplugin::provid = provID;
	return head;
}


void* pushData(ibmras::common::port::ThreadData* param) {
	/**
	 * This is the function which is effectively used to push data into the agent employing
	 * the callback function provided and that was stored when registering the sources.
	 *
	 * What will be pushed to the agent is a monitordata*, the monitordata
	 * structure is defined as follows:
	 *
	 * typedef struct monitordata {
	 *	uint32 provID;				* provider ID, supplied by agent during the source registration
	 *	uint32 sourceID;			* source ID, previously supplied by the source during registration
	 *	uint32 size;				* amount of data being provided
	 *	char *data;					* char array of the data to push
	 *} monitordata;
	 *
	 *
	 *
	 * The push loop in this template is a simple while loop controlled by pushplugin::running,
	 * which changes accordingly to the agent cycle (whether it is on init or shutdown)
	 */
	monitordata* data = new monitordata();
	data->persistent = false;
	data->provID = pushplugin::provid;
	//uint32 id = 0;
	data->data = "This is where the monitoring information will be!! ";
	data->size = std::strlen(data->data);
	while(pushplugin::running) {
		ibmras::common::port::sleep(3);
		pushplugin::callback(data);
	}
	ibmras::common::port::exitThread(NULL);
	return NULL;
}



int pushtemplate::start() {
	/**
	 * This method is exposed and will be called by the agent when starting all the plugins, anything
	 * required to start the plugin has to be added here. In this case, we set pushplugin::running=true and
	 * declare pushData to be the entry point for the thread that will get created.
	 */
	int result = 0;
	IBMRAS_DEBUG(info, "Starting");
	pushplugin::running = true;
	ibmras::common::port::ThreadData* data = new ibmras::common::port::ThreadData(pushData);
	result = ibmras::common::port::createThread(data);
	if(result) {
		IBMRAS_DEBUG(info, "Warning : unable to create call back thread");
	}
	return 0;
}



int pushtemplate::stop() {
	/**
	 * The stop method will be called by the agent on shutdown, here is where any cleanup has to be done, in this case
	 * it suffices to set pushplugin::running=false, so the loop that pushes the data into the agent stops and the
	 * thread where it was runnning finishes.
	 */
	IBMRAS_DEBUG(info, "Stopping");
	pushplugin::running = false;
	return 0;

}

}

} /* end pushplugin namespace */

extern "C" {

	pushsource* ibmras_monitoring_registerPushSource(void (*callback)(monitordata*), uint32 provID) {
	  return pushplugin::pt->registerPushSource(callback, provID);
	}

	int ibmras_monitoring_plugin_start() {
	  return pushplugin::pt->start();
	}

	int ibmras_monitoring_plugin_stop() {
	  return pushplugin::pt->stop();
	}
}
