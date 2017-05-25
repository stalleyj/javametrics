/**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#if defined(_ZOS)
#define _XOPEN_SOURCE
#endif

#include "ibmras/monitoring/plugins/j9/jmx/JMXSourceManager.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXPullSource.h"
#include "ibmras/monitoring/plugins/j9/jmx/JMXUtility.h"
#include "ibmras/common/logging.h"
#include "ibmras/common/util/strUtils.h"
#include "ibmras/common/MemoryManager.h"
#include <cstring>
#include <stdlib.h>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {
namespace jmx {

const char* JMXdpVersion = "1.0";

JMXSourceManager* mgr = new JMXSourceManager;

#if defined(_ZOS)
#else
extern "C" {
#endif

DECL pullsource* registerPullSourceJMX(agentCoreFunctions aCF,uint32 provID) {

	return ibmras::monitoring::plugins::j9::jmx::mgr->registerPullSource(provID);
}

DECL int startJMX() {
	return ibmras::monitoring::plugins::j9::jmx::mgr->start();
}

DECL int stopJMX() {
	return ibmras::monitoring::plugins::j9::jmx::mgr->stop();
}

DECL const char* getVersionJMX() {
	return JMXdpVersion;
}

#if defined(_ZOS)
#else
}
#endif

IBMRAS_DEFINE_LOGGER("JMXSources")
;

/* default clean-up after monitor data has been processed */
void complete(monitordata* data) {
	if (data) {
		if (data->data) {
			delete data->data; /* free the internal buffer */
		}
		delete data; /* free the data structure */
	}
}

DECL void setJVM(JavaVM* vm) {
	ibmras::monitoring::plugins::j9::jmx::vm = vm;
}

DECL JavaVM* getJVM() {
	return vm;
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

char* getString(JNIEnv* env, jobject* obj, const char* cname, const char* mname,
		jclass* jc) {
#if defined(_ZOS)
#pragma convert("ISO8859-1")
#endif
	jmethodID method = getMethod(env, cname, mname, "()Ljava/lang/String;", jc);
#if defined(_ZOS)
#pragma convert(pop)
#endif
	jstring jobj =
			jc ? (jstring) env->CallStaticObjectMethod(*jc, method, NULL) : (jstring) env->CallObjectMethod(
							*obj, method, NULL);
	if (NULL == jobj) {
		return "";
	}
	const char* value = env->GetStringUTFChars(jobj, NULL);
	if (value == NULL) {
		return "";
	}
	jsize len = env->GetStringLength(jobj);
	char* sval = new char[len + 1];
	if (sval) {
		memccpy(sval, value, 0, len);
		*(sval + len) = '\0'; /* ensure string is null terminated */
	}
	env->ReleaseStringUTFChars(jobj, value); /* release the char array created by the JVM */
	return sval;
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



DECL ibmras::monitoring::Plugin* getPlugin() {
	ibmras::monitoring::Plugin* plugin = new ibmras::monitoring::Plugin;
	plugin->name = "java";
	plugin->pull = ibmras::monitoring::plugins::j9::jmx::registerPullSourceJMX;
	plugin->push = NULL;
	plugin->start = ibmras::monitoring::plugins::j9::jmx::startJMX;
	plugin->stop = ibmras::monitoring::plugins::j9::jmx::stopJMX;
	plugin->getVersion = ibmras::monitoring::plugins::j9::jmx::getVersionJMX;
	plugin->type = ibmras::monitoring::plugin::data;
	plugin->confactory = NULL;
	plugin->recvfactory = NULL;
	return plugin;
}

} /* end namespace jmx */
} /* end namespace j9 */
} /* end namespace plugins */
} /* end namespace monitoring */
} /* end namespace ibmras */

