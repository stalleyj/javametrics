 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#include "ibmras/monitoring/connector/jmx/JMXConnector.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/MemoryManager.h"
#include "ibmras/monitoring/agent/Agent.h"
#include "jvmti.h"
#include "jni.h"
#include "ibmjvmti.h"
#include "jniport.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/port/Process.h"
#include <string.h>

#ifdef _ZOS
#include <unistd.h>
#endif

/* ensure common reporting of JNI version required */
#define JNI_VERSION JNI_VERSION_1_4

static jclass javaHCLaunchMBean = NULL;
static jmethodID mainMethod = NULL;
static jobjectArray applicationArgs = NULL;
static jstring applicationArg0 = NULL;
static jstring applicationArg1 = NULL;
static int processID = 0;
static char args0[20];

#ifdef __cplusplus
extern "C" {
#endif
#undef com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_OK
#define com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_OK 0L
#undef com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_ERR
#define com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_ERR -1L
#undef com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_ALREADY_LOADED
#define com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_ALREADY_LOADED -2L
#undef com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_JVMTI_ERR
#define com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_JVMTI_ERR -3L
#undef com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR
#define com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR -4L
#undef com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_EXC
#define com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_EXC -5L
#ifdef __cplusplus
}
#endif

namespace ibmras {
namespace monitoring {
namespace connector {
namespace jmx {

IBMRAS_DEFINE_LOGGER("JMXConnector");

JMXConnector::JMXConnector(JavaVM *theVM) :
		vm(theVM) {
}

JMXConnector::~JMXConnector() {
}

int JMXConnector::start() {
	int rc = 0;
	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string enabled = agent->getAgentProperty("jmx");
	if (ibmras::common::util::equalsIgnoreCase(enabled, "on")) {
		rc = launchMBean();
	}
	return rc;
}

int JMXConnector::stop() {
	int rc = 0;

	return rc;
}

/*****************************
 * Check & clear JNI Exception
 * Return 1 (true) if there was an Exception
 * return 0 (false) otherwise.
 */
int ExceptionCheck(JNIEnv *env) {
	if (env->ExceptionCheck()) {
		IBMRAS_DEBUG(debug, "JNI exception:");
		env->ExceptionDescribe();
		env->ExceptionClear();
		return 1;
	} else {
		return 0;
	}
}

/******************************/
int JMXConnector::launchMBean() {
	IBMRAS_DEBUG(debug, "> launchMBean");

	JNIEnv *env = NULL;
	javaHCLaunchMBean = NULL;
	mainMethod = NULL;
	IBMRAS_DEBUG_1(debug, "VM structure: %p", vm);
	jint rc = vm->GetEnv((void **) &env, JNI_VERSION);
	if (rc < 0 || NULL == env) {
		IBMRAS_DEBUG(warning, "Agent_OnAttach: GetEnv failed");
		return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_JVMTI_ERR;
	}

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif

	if (NULL == javaHCLaunchMBean) {
		javaHCLaunchMBean =
				env->FindClass(
						"com/ibm/java/diagnostics/healthcenter/agent/mbean/HCLaunchMBean");
#if defined(_ZOS)
#pragma convert(pop)
#endif

		if (ExceptionCheck(env) || NULL == javaHCLaunchMBean) {
			IBMRAS_LOG(warning,
					"launchMBean couldn't find com.ibm.java.diagnostics.healthcenter/agent/mbean/HCLaunchMBean class. Agent not started.");
			return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR;
		}
	}
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	if (NULL == mainMethod) {
		mainMethod = env->GetStaticMethodID(javaHCLaunchMBean, "main",
				"([Ljava/lang/String;)V");
#if defined(_ZOS)
#pragma convert(pop)
#endif

		if (ExceptionCheck(env) || NULL == mainMethod) {
			IBMRAS_LOG(warning,
					"launchMBean couldn't find main method in HCLaunchMBean class. Agent not started.");
			return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR;
		}
	}
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	applicationArgs = env->NewObjectArray(2, env->FindClass("java/lang/String"),
	NULL);
#if defined(_ZOS)
#pragma convert(pop)
#endif


	/* should throw OOM or come back null */
	if (ExceptionCheck(env) || NULL == applicationArgs) {
		IBMRAS_LOG(warning,
				"launchMBean couldn't create object array. Agent not started.");
		return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR;
	}

	processID = ibmras::common::port::getProcessId();
	sprintf(args0, "%d", processID);

#ifdef _ZOS
	__etoa(args0);
#endif

	applicationArg0 = env->NewStringUTF(args0);
	/* should throw OOM or come back null */
	if (ExceptionCheck(env) || NULL == applicationArg0) {
		IBMRAS_LOG(warning,
				"launchMBean couldn't create jstring for main args. Agent not started.");
		return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR;
	}

	env->SetObjectArrayElement(applicationArgs, 0, applicationArg0);
	if (ExceptionCheck(env)) {
		IBMRAS_LOG(warning,
				"launchMBean couldn't set object array element for main args. Agent not started.");
		return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR;
	}

	ibmras::monitoring::agent::Agent* agent =
			ibmras::monitoring::agent::Agent::getInstance();
	std::string agentOptions = agent->getAgentProperty("launch.options");

	applicationArg1 = env->NewStringUTF(agentOptions.c_str());
	/* should throw OOM or come back null */
	if (ExceptionCheck(env) || NULL == applicationArg1) {
		IBMRAS_LOG(warning,
				"launchMBean couldn't create jstring for main args. Agent not started.");
		return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR;
	}

	/* Also pass our options to the launcher. */
	env->SetObjectArrayElement(applicationArgs, 1, applicationArg1);
	if (ExceptionCheck(env)) {
		IBMRAS_LOG(warning,
				"launchMBean couldn't set object array element for main args. Agent not started.");
		return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_ERR;
	}

	env->CallStaticVoidMethod(javaHCLaunchMBean, mainMethod, applicationArgs);
	/* could throw an exception (doesn't say it'll return null) */
	if (ExceptionCheck(env)) {
		IBMRAS_LOG(warning,
				"launchMBean couldn't run main on com/ibm/java/diagnostics/healthcenter/agent/mbean/HCLaunchMBean class. Agent not started.");
		return com_ibm_java_diagnostics_healthcenter_agent_lateattach_AttachAgent_attachAgent_MBEAN_EXC;
	}

	/* Everything OK, return OK */
	IBMRAS_LOG(debug, "< launchMBean");

	return JNI_OK;
}

} /* namespace jmx */
} /* namespace connector */
} /* namespace monitoring */
} /* namespace ibmras */

using namespace ibmras::monitoring::connector::jmx;

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_ibm_java_diagnostics_healthcenter_agent_mbean_HealthCenter_getProviders(
		JNIEnv * jni_env, jobject obj) {
	ibmras::monitoring::agent::Agent* agent =
	ibmras::monitoring::agent::Agent::getInstance();
	ibmras::monitoring::agent::BucketList* buckets = agent->getBucketList();
	std::vector<std::string> ids = buckets->getIDs();

#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jclass stringClass = jni_env->FindClass("java/lang/String");
#if defined(_ZOS)
#pragma convert(pop)
#endif
	jobjectArray stringArray = jni_env->NewObjectArray(ids.size(), stringClass,
			0);
	for (uint32 i = 0; i < ids.size(); ++i) {
#if defined(_ZOS)
	char* bucket = ibmras::common::util::createAsciiString(ids[i].c_str());
#else
	const char* bucket = ids[i].c_str();
#endif
		jstring javaString = jni_env->NewStringUTF(bucket);
		jni_env->SetObjectArrayElement(stringArray, i, javaString);
#if defined(_ZOS)
		ibmras::common::memory::deallocate((unsigned char**)&bucket);
#endif
	}


	return stringArray;

}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_ibm_java_diagnostics_healthcenter_agent_dataproviders_MonitoringDataProvider_getData(
		JNIEnv * jni_env, jobject obj, jstring name, jint requestedSize,
		jintArray requestedId) {
	const char* bucketName = jni_env->GetStringUTFChars(name, NULL);
#if defined(_ZOS)
	char* nativeBucketName = ibmras::common::util::createNativeString(bucketName);
#else
	const char* nativeBucketName = bucketName;
#endif
	if (nativeBucketName == NULL) {
		return NULL;
	}
	IBMRAS_DEBUG_1(debug, "getData for bucket %s", nativeBucketName);

	ibmras::monitoring::agent::Agent* agent =
	ibmras::monitoring::agent::Agent::getInstance();
	ibmras::monitoring::agent::BucketList* buckets = agent->getBucketList();
	ibmras::monitoring::agent::Bucket* bucket = buckets->findBucket(nativeBucketName);

	jni_env->ReleaseStringUTFChars(name, bucketName);
#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&nativeBucketName);
#endif

	if (bucket == NULL) {
		IBMRAS_DEBUG(debug, "getData for non-existent bucket");
		return NULL;
	}

	signed char* data = NULL;
	int32 size = requestedSize;

	jint * retID = jni_env->GetIntArrayElements(requestedId, NULL);
	uint32 id = retID[0];

	IBMRAS_DEBUG_2(debug, "Getting data for %s, id %d", bucket->getUniqueID().c_str(), id);
	uint32 droppedCount = 0;
	id = bucket->getNextData(id, size, (void**) &data, droppedCount);
	retID[0] = id;
	jni_env->ReleaseIntArrayElements(requestedId, retID, 0);

	if (size == 0) {
		IBMRAS_DEBUG_1(debug, "No data returned for %s", bucket->getUniqueID().c_str());
		return NULL;
	}

	IBMRAS_DEBUG_2(debug, "%d bytes of data returned for %s", size, bucket->getUniqueID().c_str());

	if (droppedCount > 0) {
		IBMRAS_DEBUG_2(warning, "Missed %d data buffers for %s", droppedCount, bucket->getUniqueID().c_str());
	}

	jbyteArray buffersByteArray = jni_env->NewByteArray(size);
	jni_env->SetByteArrayRegion(buffersByteArray, 0, (int) size, data);

	ibmras::common::memory::deallocate((unsigned char**)&data);

	return buffersByteArray;

}

extern "C" JNIEXPORT void JNICALL
Java_com_ibm_java_diagnostics_healthcenter_agent_dataproviders_MonitoringDataProvider_sendMessage(
		JNIEnv * jni_env, jobject obj, jstring topic, jstring message) {
	const char* subject = jni_env->GetStringUTFChars(topic, NULL);
	const char* msg = jni_env->GetStringUTFChars(message, NULL);
#if defined(_ZOS)
	char* nativeSubject = ibmras::common::util::createNativeString(subject);
	char* nativeMsg = ibmras::common::util::createNativeString(msg);
#else
	const char* nativeSubject = subject;
	const char* nativeMsg = msg;
#endif
	if (nativeSubject == NULL || nativeMsg == NULL) {
		return;
	}
	ibmras::monitoring::agent::Agent* agent =
	ibmras::monitoring::agent::Agent::getInstance();
	ibmras::monitoring::connector::ConnectorManager *conMan =
	agent->getConnectionManager();
	conMan->processMessage(nativeSubject, strlen(nativeMsg), (void*) nativeMsg);

	jni_env->ReleaseStringUTFChars(topic, subject);
	jni_env->ReleaseStringUTFChars(message, msg);

#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&nativeSubject);
	ibmras::common::memory::deallocate((unsigned char**)&nativeMsg);
#endif
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_ibm_java_diagnostics_healthcenter_agent_dataproviders_MonitoringDataProvider_getSourceDetails(
		JNIEnv * jni_env, jobject obj, jstring name) {

	const char* bucketName = jni_env->GetStringUTFChars(name, NULL);
	ibmras::monitoring::agent::Agent* agent =
	ibmras::monitoring::agent::Agent::getInstance();

#if defined(_ZOS)
	char* nativeBucketName = ibmras::common::util::createNativeString(bucketName);
#else
	const char* nativeBucketName = bucketName;
#endif
	if (nativeBucketName == NULL) {
		return NULL;
	}

	std::string config = agent->getConfig(nativeBucketName);
	jni_env->ReleaseStringUTFChars(name, bucketName);
#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&nativeBucketName);

	char* asciiConfig = ibmras::common::util::createAsciiString(config.c_str());
#else
	const char* asciiConfig = config.c_str();
#endif
	jstring javaString = jni_env->NewStringUTF(asciiConfig);
#if defined(_ZOS)
	ibmras::common::memory::deallocate((unsigned char**)&asciiConfig);
#endif

	return javaString;
}

