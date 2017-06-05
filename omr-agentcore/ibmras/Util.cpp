 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "Util.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/util/strUtils.h"
#include "jni.h"
#include "ibmras/common/MemoryManager.h"

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {

IBMRAS_DEFINE_LOGGER("J9Utils");

int Util::getJavaLevel() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty("capability.JavaLevel");
	return atoi(level.c_str());
}

int Util::getServiceRefreshNumber() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty(
			"capability.ServiceRefreshNumber");
	return atoi(level.c_str());
}

int Util::getFixpackNumber() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty(
			"capability.FixpackNumber");
	return atoi(level.c_str());
}

bool Util::is26VMOrLater() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty("capability.is26VMOrLater");
	return (level == "true");
}

bool Util::is27VMOrLater() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty("capability.is27VMOrLater");
	return (level == "true");
}

bool Util::vmHasLOATracePoints() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty(
			"capability.vmHasLOATracePoints");
	return (level == "true");
}

bool Util::j9DmpTrcAvailable() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty("capability.j9DmpTrcAvailable");
	return (level == "true");
}

bool Util::isRealTimeVM() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string level = agent->getAgentProperty("capability.isRealTimeVM");
	return (level == "true");
}

std::string Util::getLowAllocationThreshold() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	return agent->getAgentProperty("allocation.threshold.low");
}

std::string Util::getHighAllocationThreshold() {
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	return agent->getAgentProperty("allocation.threshold.high");
}

/**
 Returns VM Dump options
 @param jvmtiQueryVmDump Function pointer to the jvmti function naming the parameter
 @param pti Pointer to jvmti environment.
 @return A char* which contains the options OR an empty string if there is an error.
 */
std::string Util::queryVmDump(jvmtiExtensionFunction jvmtiQueryVmDump,
		jvmtiEnv* pti) {
	IBMRAS_DEBUG(debug, "> queryVmDump");
	jvmtiError rc;
	char *buffer = NULL;
	jint buffer_size;
	std::string options;

	if (jvmtiQueryVmDump == 0) {
		IBMRAS_DEBUG(debug, "No jvmtiQueryVmDump extension function");
		return "";
	}

	buffer_size = 1024;
	rc = pti->Allocate(buffer_size, (unsigned char**) &buffer);
	if (buffer == NULL) {
		IBMRAS_DEBUG(debug, "< queryVmDump failed to allocate buffer");
		return "";
	}

	/* call jvmti function */
	rc = (jvmtiQueryVmDump)(pti, buffer_size, buffer, &buffer_size);
	if (rc == JVMTI_ERROR_ILLEGAL_ARGUMENT) {
		IBMRAS_DEBUG(debug, "Buffer smaller than expected");
		/* allocate buffer of correct size */
		pti->Deallocate((unsigned char*) buffer);
		pti->Allocate(buffer_size + 100, (unsigned char**) &buffer);
		if (buffer == NULL) {
			IBMRAS_DEBUG(debug, "< queryVmDump failed to reallocate buffer");
			return "";
		}
		rc = (jvmtiQueryVmDump)(pti, buffer_size, buffer, &buffer_size);
	}

	if (rc == JVMTI_ERROR_NONE) {
		ibmras::common::util::force2Native(buffer);
		options = buffer;

	} else {
		IBMRAS_DEBUG(debug, "RC_ERROR_2");
	}

	pti->Deallocate((unsigned char*) buffer);

	IBMRAS_DEBUG(debug, "< queryVmDump");
	return options;
}

/***
 * Returns the string that will be retrieved from the Java side using JNI
 * @param env The JNI environment
 * @param cname The name of the class containing the method we want to call
 * @param mname The name of the method to be called.
 * @param signature The signature of cname's mname.
 */

std::string getString(JNIEnv* env, const char* cname, const char* mname,
		const char* signature) {

	IBMRAS_DEBUG(debug, ">>>..j9::getString");

	IBMRAS_DEBUG(debug, "Retrieving class");
	jclass clazz = env->FindClass(cname);
	if (!clazz) {
		IBMRAS_DEBUG(warning, "<<<..j9::getString[Failed to find class]");
		return "";
	}

	jmethodID method = env->GetStaticMethodID(clazz, mname, signature);
	if (!method) {
		IBMRAS_DEBUG(warning, "<<<..j9::getString[Failed to get method ID]");
		return "";
	}

	jstring jobj = (jstring) env->CallStaticObjectMethod(clazz, method, NULL);
	if (env->ExceptionOccurred()) {
		env->ExceptionDescribe();
	}
	if (NULL == jobj){
			IBMRAS_DEBUG(warning, "jobj was NULL");
			return "";
	}

	const char* value = env->GetStringUTFChars(jobj, NULL);
	if (env->ExceptionOccurred()) {
		env->ExceptionDescribe();
	}
	if (value == NULL){
			IBMRAS_DEBUG(warning, "Value was NULL");
			return "";
	}

	std::string sval(value);

	env->ReleaseStringUTFChars(jobj, value);
	env->DeleteLocalRef(jobj);

	IBMRAS_DEBUG(debug, "<<<..j9::getString[OK]");
	return sval;
}

unsigned char* hc_alloc(int size) {
	return ibmras::common::memory::allocate(size);
}

void hc_dealloc(unsigned char** buffer) {
	ibmras::common::memory::deallocate(buffer);
}

/***
 * Returns 0 [OK] or -1 [ERR]
 * @param env The JNI environment to be set up
 * @param name The name of the plugin calling this method.
 * @param jvm The virtual machine that we'll attach to.
 * @param asDaemon Whether this should be a daemon thread or not.
 */

int setEnv(JNIEnv** env, std::string name, JavaVM* jvm, bool asDaemon) {
if (!*env) {
		JavaVMAttachArgs threadArgs;

		memset(&threadArgs, 0, sizeof(threadArgs));
		threadArgs.version = JNI_VERSION_1_4;

#if defined(_ZOS)
		/**
		 * To IFA enable a thread we have to call AttachCurrentThread with the JNI version OR'd
		 * with 0x79000000. This is only necessary on z/OS and is only supported on newer
		 * JVMs.
		 * Older JVMs will return an error and set the env to null as they will consider
		 * this an unsupported JNI_VERSION so we use GetEnv to determine if IFA is supported.
		 * Newer JVMs will return JNI_OK immediately if called with a version set to
		 * 0x79000000.
		 */
		JNIEnv* ifaEnv = NULL;
		int ifaEnabled = jvm->GetEnv((void **) &ifaEnv, 0x79000000);
		if( JNI_OK == ifaEnabled ) {
			IBMRAS_DEBUG_1(debug, "Thread %s IFA enabled.", name.c_str());
			threadArgs.version |= 0x79000000;
		} else {
			IBMRAS_DEBUG_1(debug, "Thread %s IFA enablement failed", name.c_str());
		}
#endif

		threadArgs.name = ibmras::common::util::createAsciiString(name.c_str());
		threadArgs.group = NULL;
		IBMRAS_DEBUG_1(debug, "Attaching thread %s", name.c_str());
		jint errcode = 0;
		if( asDaemon ) {
			jvm->AttachCurrentThreadAsDaemon((void **) env, &threadArgs);
		} else {
			jvm->AttachCurrentThread((void **) env, &threadArgs);
		}
		ibmras::common::memory::deallocate((unsigned char**)&threadArgs.name);
		if (errcode != JNI_OK) {
			if( asDaemon ) {
				IBMRAS_DEBUG_2(debug, "AttachCurrentThreadAsDaemon failed %d for %s", errcode, name.c_str());
			} else {
				IBMRAS_DEBUG_2(debug, "AttachCurrentThread failed %d for %s", errcode, name.c_str());
			}
			return -1;
		}
		IBMRAS_DEBUG_1(debug, "Attached thread %s", name.c_str());
	}
	return 0;
}

/*------------------------------------------------------------------------
 * JMXUtils
 *------------------------------------------------------------------------*/

jmethodID getMethod(JNIEnv* env, const char* cname, const char* mname,
		const char* sig, jclass* jc) {
	jclass clazz = env->FindClass(cname);
	if (!clazz) {
		IBMRAS_DEBUG_1(warning, "!Failed to find class %s", cname);
		env->ExceptionClear();
		return NULL;
	}
	jmethodID method =
			jc ? env->GetStaticMethodID(clazz, mname, sig) : env->GetMethodID(
							clazz, mname, sig);
	if (!method) {
		IBMRAS_DEBUG_2(warning, "!Failed to find method %s/%s", cname, mname);
		env->ExceptionClear();
		return NULL;
	}
	if (jc) {
		*jc = clazz; /* pass back the class reference */
	}
	return method;
}

jdouble getDouble(JNIEnv* env, jobject* obj, const char* cname,
		const char* mname, jclass* jc) {

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jmethodID method = getMethod(env, cname, mname, "()D", jc);
#if defined(_ZOS)
#pragma convert(pop)
#endif
	if (method) {
		IBMRAS_DEBUG(debug, "got Method");
		return jc ?
				env->CallStaticDoubleMethod(*jc, method, NULL) :
				env->CallDoubleMethod(*obj, method, NULL);
	}
	return -1.0;
}

jlong getLong(JNIEnv* env, jobject* obj, const char* cname, const char* mname,
		jclass* jc) {
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jmethodID method = getMethod(env, cname, mname, "()J", jc);
#if defined(_ZOS)
#pragma convert(pop)
#endif
	if (method) {
		return jc ?
				env->CallStaticLongMethod(*jc, method, NULL) :
				env->CallLongMethod(*obj, method, NULL);
	}
	return -1;
}

jlong getTimestamp(JNIEnv* env) {
	jclass clazz = NULL;
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jmethodID method = getMethod(env, "java/lang/System", "currentTimeMillis",
			"()J", &clazz);
#if defined(_ZOS)
#pragma convert(pop)
#endif
	if (method) {
		return env->CallStaticLongMethod(clazz, method, NULL);
	}
	return -1;
}

jobject getMXBean(JNIEnv* env, jclass* mgtBean, const char* name) {
	std::string get, signature;
	get.append("get");
	get.append(name);
	get.append("MXBean");
	signature.append("()Ljava/lang/management/");
	signature.append(name);
	signature.append("MXBean;");
#if defined(_ZOS)
	char* mxb = ibmras::common::util::createAsciiString(get.c_str());
	char* sig = ibmras::common::util::createAsciiString(signature.c_str());
#else
	const char* mxb = get.c_str();
	const char* sig = signature.c_str();
#endif

	jmethodID method = env->GetStaticMethodID(*mgtBean, mxb,
			sig);
#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&mxb);
	ibmras::common::memory::deallocate((unsigned char**)&sig);
#endif

	if (!method) {
		IBMRAS_DEBUG_1(warning, "!Failed to find MXBean %s", name);
		env->ExceptionClear();
		return NULL;
	}
	jobject mgt = env->CallStaticObjectMethod(*mgtBean, method, NULL);
	if (!mgt) {
		IBMRAS_DEBUG_1(warning, "!Failed to get MXBean %s", name);
		env->ExceptionClear();
		return NULL;
	}
	return mgt;
}



} /* namespace j9 */
} /* namespace plugins */
} /* namespace monitoring */
} /* namespace ibmras */
