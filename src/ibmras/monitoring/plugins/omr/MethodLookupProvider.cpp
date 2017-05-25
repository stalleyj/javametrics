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
#include <string>
#include <cstring>
#include <stdlib.h>

#include "ibmras/monitoring/Plugin.h"
#include "ibmras/vm/omr/healthcenter.h"
#include "ibmras/monitoring/plugins/omr/MethodLookupProvider.h"
#include "ibmras/common/util/memUtils.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/logging.h"
#include "omr.h"
#include "omragent.h"

#define JNI_VERSION JNI_VERSION_1_4

namespace plugins {
namespace omr {
namespace methods {

IBMRAS_DEFINE_LOGGER("MethodLookupProvider");

PUSH_CALLBACK sendMethodData;
uint32 MethodLookupProvider::providerID = 0;

int startReceiver() {
	MethodLookupProvider *mlp =
			(MethodLookupProvider*) MethodLookupProvider::getInstance();
	mlp->sendMethodDictionary(true);
	return 0;
}

int stopReceiver() {
	return 0;
}

pushsource* MethodLookupProvider::registerPushSource(agentCoreFunctions aCF, uint32 provID) {
	pushsource *src = new pushsource();
	src->header.name = "methoddictionary";
	src->header.description =
			"Method lookup data which maps hex value to method data";
	src->header.sourceID = 0;
	src->next = NULL;
	src->header.capacity = 1048576; /* 1MB bucket capacity */
	MethodLookupProvider::providerID = provID;
	plugins::omr::methods::sendMethodData = aCF.agentPushData;

	return src;
}

MethodLookupProvider::MethodLookupProvider(
		omrRunTimeProviderParameters functions) {
	vmData = functions;
	name = "Method Lookup";
	pull = NULL;
	push = registerPushSource;
	start = plugins::omr::methods::startReceiver;
	stop = plugins::omr::methods::stopReceiver;
	type = ibmras::monitoring::plugin::data
			| ibmras::monitoring::plugin::receiver;
	recvfactory = (RECEIVER_FACTORY) MethodLookupProvider::getInstance;
	confactory = NULL;
}

MethodLookupProvider::~MethodLookupProvider() {
}

MethodLookupProvider* instance = NULL;

MethodLookupProvider* MethodLookupProvider::getInstance(
		omrRunTimeProviderParameters oRTPP) {
	if (!instance) {
		instance = new MethodLookupProvider(oRTPP);
	}
	return instance;
}

void* MethodLookupProvider::getInstance() {
	if (!instance) {
		return NULL;
	}
	return instance;
}

void MethodLookupProvider::receiveMessage(const std::string &id, uint32 size,
		void *data) {
	// Send the initial empty dictionary
	if (id == "methoddictionary") {
		if (size == 0 || data == NULL) {
			IBMRAS_DEBUG_1(debug, "Received request %s", id.c_str());
			sendMethodDictionary(false);
		} else {
			std::string message((const char*) data, size);
			IBMRAS_DEBUG_2(debug, "Received request %s %s", id.c_str(), message.c_str());
			std::size_t found = message.find(',');
			if (found != std::string::npos) {
				std::string command = message.substr(0, found);
				std::string rest = message.substr(found + 1);
				std::vector < std::string > parameters =
						ibmras::common::util::split(rest, ',');

				if (parameters.size() > 0) {
					getMethodIDs (parameters);
				} else {
					sendMethodDictionary(false);
				}
			}
		}
	}
}

//static int called  = 0;
void MethodLookupProvider::getMethodIDs(std::vector<std::string> &jsMethodIds) {

//	if( called == 1 ) {
//		return;
//	}
//	called = 1;

//	1:03:59 PM: These are the properties for ruby:
//	static const char *methodPropertyNames[methodPropertyCount] = {
//		"methodName",
//		"className",
//		"fileName",
//		"lineNumber"
//	};

//	2:32:42 PM: Categories for python:
//	static const char *methodPropertyNames[methodPropertyCount] = {
//		"methodName",
//		"fileName",
//		"lineNumber"
//	};

	omr_error_t err;
	void **methodArray = NULL;
	OMR_SampledMethodDescription *descriptorBuffer = NULL;
	OMR_VMThread *vmThread = NULL;

	int numberOfMethods = jsMethodIds.size();

	err = vmData.omrti->BindCurrentThread(vmData.theVm,
			"MethodLookupProvider::getMethodIDs", &vmThread);
	if (OMR_ERROR_NONE != err) {
		IBMRAS_DEBUG(debug, "getMethodIDs exit as unable to bindCurrentThread");
		return;
	}

	size_t getPropertyCount = 0;
	const char *const *getPropertyNames = NULL;
	size_t getSizeof = 0;

	vmData.omrti->GetMethodProperties(vmThread, &getPropertyCount, &getPropertyNames, &getSizeof);
	int calc = sizeof(OMR_SampledMethodDescription)
			+ (getPropertyCount * sizeof(char*));

	int allocSize = (sizeof(OMR_SampledMethodDescription)
							+ (getPropertyCount * sizeof(char*))) * numberOfMethods;
	descriptorBuffer = (OMR_SampledMethodDescription*) malloc(allocSize);
	if (descriptorBuffer == NULL) {
		IBMRAS_DEBUG_1(warning, "failed to allocate descriptor buffer of size %d", allocSize);
	} else {

		/* Allocate memory for the method identifiers */
		allocSize = sizeof(void*) * numberOfMethods;
		methodArray = (void**) malloc(allocSize);
		if (methodArray == NULL) {
			IBMRAS_DEBUG_1(warning, "failed to allocate method buffer of size %d", allocSize);
		} else {

			size_t firstRetryMethod = 0;
			size_t nameBytesRemaining = 0;

			char * pEnd;
			int i = 0;
			for (std::vector<std::string>::iterator it = jsMethodIds.begin();
					it != jsMethodIds.end(); ++it) {
				methodArray[i] = (void*) strtol(((std::string) *it).c_str(),
						&pEnd, 16);
				i++;
			}

			/* NULL name buffer. All available methods should be marked RETRY. */
			/* the nameBytesRemaining field returns with the amount of data needed in the nameBuffer
			 * array.  My making an initial call with an initial size of 0, we can get told the exact
			 * size we need.  We can then call it a second time with the real sized buffer
			 */
			char nameBuffer[0];
			vmData.omrti->GetMethodDescriptions(vmThread, methodArray,
					numberOfMethods, descriptorBuffer, nameBuffer,
					sizeof(nameBuffer), &firstRetryMethod, &nameBytesRemaining);
			char *newNameBuffer = (char*) malloc(nameBytesRemaining);
			if (newNameBuffer == NULL) {
				IBMRAS_DEBUG_1(warning, "failed to allocate name buffer of size %d", nameBytesRemaining);
			} else {
				vmData.omrti->GetMethodDescriptions(vmThread, methodArray,
						numberOfMethods, descriptorBuffer, newNameBuffer,
						nameBytesRemaining, &firstRetryMethod,
						&nameBytesRemaining);

				std::stringstream ss;

				char* ptr = (char*) descriptorBuffer;
				for (std::vector<std::string>::iterator it =
						jsMethodIds.begin(); it != jsMethodIds.end(); ++it) {

					OMR_SampledMethodDescription* descriptorBufferPtr =
							(OMR_SampledMethodDescription*) ptr;

					/*
					 * return is going to be
					 * method name followed by list of extra info which is variable
					 * and differs between ruby and omr
					 */
					if ((*descriptorBufferPtr).reasonCode == OMR_ERROR_NONE) {
						ss << *it << "=" << "@omr@";
						for (int x = 0; x < getPropertyCount; x++) {
							if ((char*) (*descriptorBufferPtr).propertyValues[x]
									!= NULL) {
								ss << (char*) (*descriptorBufferPtr).propertyValues[x];
							}
							ss << "@@";
						}
						ss << "\n";
					}

					ptr += getSizeof;
				}

				if (newNameBuffer != NULL) {
					free(newNameBuffer);
				}

				std::string data = ss.str();
				monitordata *mdata = generateData(0, data.c_str(),
						data.length());
				sendMethodData(mdata);

				delete mdata;
			}
		}
	}
	err = vmData.omrti->UnbindCurrentThread(vmThread);
	if (methodArray != NULL) {
		free(methodArray);
	}
	if (descriptorBuffer != NULL) {
		free(descriptorBuffer);
	}

}

void MethodLookupProvider::sendMethodDictionary(bool persistent) {

	IBMRAS_DEBUG_1(debug, "sendMethodDictionary %d", persistent);
	const char header[] = "#MethodDictionarySource\n";
	monitordata *mdata = generateData(0, header, strlen(header));
	mdata->persistent = persistent;
	sendMethodData(mdata);
	delete mdata;
}

monitordata* MethodLookupProvider::generateData(uint32 sourceID,
		const char *dataToSend, int size) {
	monitordata* data = new monitordata;
	data->provID = MethodLookupProvider::providerID;
	if (dataToSend && size > 0) {
			char *buffer = new char[size];
			memcpy(buffer, dataToSend, size);
			ibmras::common::util::native2Ascii(buffer);
			data->data = buffer;
		} else {
			data->data = NULL;
		}
	data->size = size;
	data->sourceID = sourceID;
	data->persistent = false;
	return data;
}

}
}
} /* end namespace methods */

