 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ibmras/common/logging.h"
#include "ibmras/monitoring/plugins/omr/TraceDataProvider.h"
#include "omragent.h"
#include "ibmras/common/port/ThreadData.h"


namespace plugins {
namespace omr {
namespace trace {

I_64 htonjl(I_64 l);
int Tracestart();
int Tracestop();
bool startTraceSubscriber(long maxCircularBufferSize, int traceBufferSize);

void *traceMeta = NULL;
I_32 traceMetaLength = 0;
char *traceMetadata = NULL;
int headerSize = 0;
int initialisedTraceBuffers = 0;
int firstConnectionMade = 0;
int countDroppedBuffers = 0;
bool running = false;


uint32 provID;
PUSH_CALLBACK sendDataToAgent;

IBMRAS_DEFINE_LOGGER("TraceDataProvider");

static const char* traceSet = "{omrmm.0,omrmm.1,omrmm.10,omrmm.11,omrmm.12,omrmm.13,omrmm.14,omrmm.15,omrti.8,omrti.9}";

omrRunTimeProviderParameters vmData;
UtSubscription *subscriptionID;
int DEFAULT_MAX_CIRCULAR_BUFFER_SIZE = 4000000;
ibmras::common::port::Lock* traceLock = new ibmras::common::port::Lock;
/**
 * the agent calls registerPushSource to find out which data sources we have
 * and will provide to us a provID which we use for the callbacks.
 *
 * ProvID value needs to be store
 *
 * also given a callback pointer which is a pointer to a function that we
 * need to push the data to.  All data will be pushed into the same function regardless
 * of which data provider it comes from.  for us though in this provider, its invisible
 * that this happens as it may not scale.  but we just use the callback as given anyway
 */
pushsource* registerPushSource(agentCoreFunctions aCF,
		uint32 provID) {
	pushsource *src = new pushsource();
	src->header.name = "trace";
	src->header.description = "Data provided by omr trace engine";
	/*
	 * the sourceID is for the plugin to manage. If we have more than 1 source
	 * then we need to give each source (in this plugin) a different number
	 */
	src->header.sourceID = 0;
	src->next = NULL;
	src->header.capacity = 1048576; /* 1MB bucket capacity */
	plugins::omr::trace::provID = provID;
	plugins::omr::trace::sendDataToAgent = aCF.agentPushData;


	return src;
}

monitordata* generateData(uint32 sourceID, char *dataToSend, int size) {
	monitordata* data = new monitordata;
	data->persistent = false;
	data->provID = provID;
	data->data = dataToSend;
	data->size = size;
	data->sourceID = sourceID;
	return data;
}

monitordata* generateTraceHeader() {
	return generateData(0, traceMetadata, headerSize);
}

void sendTraceHeader(bool persistent) {
	monitordata* mdata = generateTraceHeader();
	mdata->persistent = persistent;
	sendDataToAgent(mdata);
	delete mdata;
}

/**
 * The start() method starts the plugin and is the method called from the setup ibmras::monitoring::Plugin* getPlugin()
 * function above
 */
int Tracestart() {

	IBMRAS_DEBUG(debug,  "Tracestart enter");
	char* tempMeta;
	I_32 tempHeaderSize = 0;
	int bufferSize = 0;
	OMR_VMThread *vmThread = NULL;
	int rc = 0;

	long maxCircularBufferSize = DEFAULT_MAX_CIRCULAR_BUFFER_SIZE;
	/* this is the eye catcher that tells the health center client trace parser
	 * that this is a header record */
	char METADATA_EYE_CATCHER[] = { 'H', 'C', 'T', 'H' };

	const char *opts[] = { "maximal", traceSet, NULL };

	if (OMR_ERROR_NONE == rc) {
		rc = vmData.omrti->BindCurrentThread(vmData.theVm, "HC Tracestart", &vmThread);
	}

	/* just enable Health Center tracepoints */
	if (OMR_ERROR_NONE == rc) {
		vmData.omrti->SetTraceOptions(vmThread, opts);
	}

	/* get the trace header data from the vm */
	if (OMR_ERROR_NONE == rc) {
		omr_error_t apiRc = vmData.omrti->GetTraceMetadata(vmThread,
				(void**) &tempMeta, &tempHeaderSize);
		if (OMR_ERROR_NONE != apiRc) {
			rc = OMR_ERROR_INTERNAL;
		} else {
			IBMRAS_DEBUG_1(debug,  "Tracestart traceMetaLength=%d\n", tempHeaderSize);
		}
	}

	if (tempMeta == NULL || rc != OMR_ERROR_NONE) {
		IBMRAS_DEBUG(warning,  "failed to get trace header");
		return -1;
	}

	I_64 traceHeaderLength = (I_64) tempHeaderSize;
	traceHeaderLength = htonjl(traceHeaderLength);

	// the size of the trace buffer is at position 17 in the array
	bufferSize = *((int*) &tempMeta[16]);

	headerSize = sizeof(METADATA_EYE_CATCHER) + sizeof(traceHeaderLength)
			+ tempHeaderSize;

	traceMetadata = (char*) malloc(headerSize);
	/* set everything to 0 */
	memset(traceMetadata, 0, (headerSize));

	/* now build up the traceMetaData structure starting with the eyecatcher */
	memcpy(traceMetadata, METADATA_EYE_CATCHER, sizeof(METADATA_EYE_CATCHER));

	/* now add the record of the length of the trace header */
	memcpy(traceMetadata + sizeof(METADATA_EYE_CATCHER), &traceHeaderLength,
			sizeof(traceHeaderLength));

	/* now add the trace header data itself */
	memcpy(
			traceMetadata + sizeof(METADATA_EYE_CATCHER)
					+ sizeof(traceHeaderLength), tempMeta, tempHeaderSize);

	sendTraceHeader(true);


	if (OMR_ERROR_NONE == rc) {
		rc = vmData.omrti->UnbindCurrentThread(vmThread);
	}

	running = startTraceSubscriber(maxCircularBufferSize, bufferSize);

	IBMRAS_DEBUG(debug,  "Tracestart exit");
	return 0;
}




int Tracestop() {
	IBMRAS_DEBUG(debug,  "Tracestop enter");
	if (running) {
		running = false;

		OMR_VMThread *vmThread = NULL;
		int rc;

		rc = vmData.omrti->BindCurrentThread(vmData.theVm,
				"HC startTraceSubscriber", &vmThread);
		if (OMR_ERROR_NONE == rc) {
			IBMRAS_DEBUG(debug,  "DeregisterRecordSubscriber");
			omr_error_t apiRc = vmData.omrti->DeregisterRecordSubscriber(
					vmThread, subscriptionID);
			if (OMR_ERROR_NONE != apiRc) {
				IBMRAS_DEBUG(warning, "DeregisterRecordSubscriber failed");
			}
			vmData.omrti->UnbindCurrentThread(vmThread);
		}
	}
	IBMRAS_DEBUG(debug,  "Tracestop exit");
	return 0;

}


TraceDataProvider* instance = NULL;
TraceDataProvider* TraceDataProvider::getInstance(omrRunTimeProviderParameters oRTPP) {
	if (!instance) {
		instance = new TraceDataProvider(oRTPP);
	}
	return instance;
}

TraceDataProvider* TraceDataProvider::getInstance() {
	if (!instance) {
		return NULL;
	}
	return instance;
}


TraceDataProvider::TraceDataProvider(
		omrRunTimeProviderParameters oRTPP) {
	vmData = oRTPP;
	name = "trace";
	pull = NULL;
	push = registerPushSource;
	start = Tracestart;
	stop = Tracestop;
	type = ibmras::monitoring::plugin::data;
	confactory = NULL;
	recvfactory = NULL;
}

omr_error_t traceSubscriber(UtSubscription *subscriptionID) {

	IBMRAS_DEBUG(debug, "entering trace subscriber callback");
	if (!running) {
		return OMR_ERROR_NONE;
	}
	if (subscriptionID->data == NULL || subscriptionID->dataLength == 0) {
		IBMRAS_DEBUG(debug, "exiting trace subscriber callback: no buffer");
		return OMR_ERROR_NONE;
	}

	unsigned char* buffer = new unsigned char[subscriptionID->dataLength + 4 + sizeof(I_64)];
	/* Write eye catcher */
	strcpy((char*) buffer, "HCTB");

	I_64 payLoadLength = subscriptionID->dataLength;
	/* Convert payload length to network byte order */
	payLoadLength = htonjl(payLoadLength);

	/* Write length of trace buffer */
	memcpy(buffer + 4, (char*) &payLoadLength, sizeof(I_64));

	/* Copy the trace buffer */
	memcpy(buffer + 4 + sizeof(I_64), subscriptionID->data,
			subscriptionID->dataLength);

	monitordata* mdata = generateData(0, (char*) buffer,
			subscriptionID->dataLength + 4 + sizeof(I_64));
	sendDataToAgent(mdata);
	delete[] buffer;
	delete mdata;

	IBMRAS_DEBUG(debug, "exiting trace subscriber callback");

	return OMR_ERROR_NONE;
}



bool startTraceSubscriber(long maxCircularBufferSize, int traceBufferSize) {
	IBMRAS_DEBUG(debug,  "startTraceSubscriber entry");


	OMR_VMThread *vmThread = NULL;
	int rc;

	rc = vmData.omrti->BindCurrentThread(vmData.theVm, "HC startTraceSubscriber", &vmThread);

	if (OMR_ERROR_NONE != rc) {
		IBMRAS_DEBUG(warning,  "startTraceSubscriber exit as unable to bindCurrentThread");
		return false;
	}

	omr_error_t apiRc = vmData.omrti->RegisterRecordSubscriber(vmThread,
			"startTraceSubscriber", traceSubscriber, NULL, NULL,
			&subscriptionID);
	if (OMR_ERROR_NONE != apiRc) {
		IBMRAS_DEBUG(warning,  "startTraceSubscriber exit as unable to register to jvmtiRegisterTraceSubscriber");

		return false;

	} else {
		IBMRAS_DEBUG(debug,  "startTraceSubscriber exit");
		return true;
	}

}


I_64 htonjl(I_64 l) {
	I_32 byteOrderTest = 0x01020304;
	if (byteOrderTest == htonl(byteOrderTest)) {
		/* big endian */
		return l;
	} else {
		I_32 hi = (I_32)(l >> 32);
		I_32 lo = (I_32)(l & 0xffffffff);
		I_64 convhi = htonl(hi);
		I_64 convlo = htonl(lo);
		/* little endian */
		return (I_64)((convlo << 32) | (convhi & 0xffffffff));
	}
}


}
}
} /* end of namespace*/

