 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_connector_headless_hlconnector_h
#define ibmras_monitoring_connector_headless_hlconnector_h

#include <string>
#include <vector>
#include <map>
#include <ctime>

#include "jni.h"

#include "ibmras/monitoring/connector/Connector.h"
#include "ibmras/monitoring/agent/BucketList.h"
#include "ibmras/monitoring/connector/Receiver.h"
#include "ibmras/common/port/ThreadData.h"

namespace ibmras {
namespace monitoring {
namespace connector {
namespace headless {

class HLConnector: public ibmras::monitoring::connector::Connector {
public:

	static HLConnector* getInstance(JavaVM* theVM);
	static HLConnector* getInstance();
	virtual ~HLConnector();

	virtual std::string getID() {return "HLConnector"; }
	int sendMessage(const std::string &sourceID, uint32 size, void* data);

	int start();
	int stop();

private:
	static void* thread(ibmras::common::port::ThreadData* tData);
	void processLoop();
	void sleep(uint32 seconds);
	int packFiles();
	bool jniPackFiles();
	void lockAndPackFiles();

	bool enabled;
	bool running;
	bool filesInitialized;

	JavaVM* vm;
	int32 seqNumber;
	time_t lastPacked;
	uint32 upper_limit;
	int32 files_to_keep;
	std::map<std::string, std::fstream*> createdFiles;
	std::map<std::string, std::string> expandedIDs;
	ibmras::common::port::Lock* lock;
	int32 run_duration;
	int32 run_pause;
	int32 number_runs;
	std::string userDefinedPath;
	std::string tmpPath;
	std::string userDefinedPrefix;
	int32 times_run;
	std::time_t startTime;
	char startDate[100];
	std::string hcdName;
	int startDelay;

	HLConnector(JavaVM* theVM);
	void createFile(const std::string &fileName);
	bool createDirectory(std::string& path);
};

void* runCounterThread(ibmras::common::port::ThreadData* tData);


} /*end namespace headless*/
} /*end namespace connector*/
} /*end namespace monitoring*/
} /*end namespace ibmras*/

#endif /*ibmras_monitoring_connector_headless_hlconnector_h*/
