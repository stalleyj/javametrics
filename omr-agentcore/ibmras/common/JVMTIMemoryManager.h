/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_vm_java_jvmtimemorymanager_h
#define ibmras_vm_java_jvmtimemorymanager_h

/*
 * Memory management functionality
 */

#include "ibmras/common/MemoryManager.h"
#include "jvmti.h"

namespace ibmras {
namespace vm {
namespace java {

class JVMTIMemoryManager : public ibmras::common::MemoryManager{

public:
	JVMTIMemoryManager(jvmtiEnv *env);
	virtual unsigned char* allocate(uint32 size);
	virtual void deallocate(unsigned char**);
protected:

private:
	virtual ~JVMTIMemoryManager();

	jvmtiEnv *jvmti;

};

}
}
}

#endif /* ibmras_vm_java_jvmtimemorymanager_h */
