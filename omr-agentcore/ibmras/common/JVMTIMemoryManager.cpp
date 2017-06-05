/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#include "JVMTIMemoryManager.h"
#include "ibmras/common/logging.h"
#include <string.h>

namespace ibmras {
namespace vm {
namespace java {

IBMRAS_DEFINE_LOGGER("jvmtimemory")
;

JVMTIMemoryManager::JVMTIMemoryManager(jvmtiEnv* env) :
		jvmti(env) {
}

JVMTIMemoryManager::~JVMTIMemoryManager() {
}

unsigned char* JVMTIMemoryManager::allocate(uint32 size) {

	unsigned char* memory = NULL;

	jvmtiError rc = jvmti->Allocate(size, (unsigned char**) &memory);
	if (rc != JVMTI_ERROR_NONE) {
		IBMRAS_LOG_1(warning, "Failed to allocate memory of size %d", size);
		return NULL;
	}

	memset(memory, 0, size);

	IBMRAS_DEBUG_2(debug, "Allocated %d at %p", size, (void*)memory);

	return memory;
}

void JVMTIMemoryManager::deallocate(unsigned char** memoryPtr) {

	IBMRAS_DEBUG_1(debug, "Deallocate called for %p", (void*)memoryPtr);

	if (memoryPtr != NULL && *memoryPtr != NULL) {
		IBMRAS_DEBUG_1(debug, "Deallocating memory at %p", (void*)*memoryPtr);
		jvmtiError rc = jvmti->Deallocate(*memoryPtr);
		if (rc != JVMTI_ERROR_NONE) {
			IBMRAS_LOG_1(warning, "Failed to deAllocate memory at %p",(void*)*memoryPtr);
		}

		*memoryPtr = NULL;
	}
}

}
} /* namespace vm */
} /* namespace ibmras */
