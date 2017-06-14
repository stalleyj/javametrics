 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_monitoring_plugins_j9_Util_h
#define ibmras_monitoring_plugins_j9_Util_h

#include <jvmti.h>
#include <string>
#include <cstring>

namespace ibmras {
namespace monitoring {
namespace plugins {
namespace j9 {

class Util {
public:
	static int getJavaLevel();
	static int getServiceRefreshNumber();
	static int getFixpackNumber();
	static bool is26VMOrLater();
	static bool is27VMOrLater();
	static bool vmHasLOATracePoints();
	static bool j9DmpTrcAvailable();
	static bool isRealTimeVM();
	static std::string getLowAllocationThreshold();
	static std::string getHighAllocationThreshold();

	static std::string queryVmDump(jvmtiExtensionFunction jvmtiQueryVmDump, jvmtiEnv* pti);
};

unsigned char* hc_alloc(int size);
void hc_dealloc(unsigned char** buffer);
int setEnv(JNIEnv** env, std::string name, JavaVM* jvm, bool asDaemon);
std::string getString(JNIEnv* env, const char* cname, const char* mname,
		const char* signature);
jmethodID getMethod(JNIEnv* env, const char* cname, const char* mname,
		const char* sig, jclass* jc = NULL);
jdouble getDouble(JNIEnv* env, jobject* obj, const char* cname,
		const char* mname, jclass* jc = NULL);
jlong getLong(JNIEnv* env, jobject* obj, const char* cname, const char* mname,
		jclass* jc = NULL);
jlong getTimestamp(JNIEnv* env);
jobject getMXBean(JNIEnv* env, jclass* mgtBean, const char* name);


} /* namespace j9 */
} /* namespace plugins */
} /* namespace monitoring */
} /* namespace ibmras */
#endif /* ibmras_monitoring_plugins_j9_Util_h */
