 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_omr_nativememorydataprovider_h
#define ibmras_monitoring_plugins_omr_nativememorydataprovider_h

#include "ibmras/monitoring/Plugin.h"
#include "omr.h"
#include "omragent.h"
#include "ibmras/vm/omr/healthcenter.h"

namespace plugins {
	namespace omr {
		namespace nativememory {

			class NativeMemoryDataProvider : ibmras::monitoring::Plugin {
			public:
				static NativeMemoryDataProvider* getInstance(omrRunTimeProviderParameters oRTPP);
				static NativeMemoryDataProvider* getInstance();

				NativeMemoryDataProvider(omrRunTimeProviderParameters oRTPP);
				virtual ~NativeMemoryDataProvider() {};  //destructor
				static int memstart();
				static int memstop();
				static pullsource* registerPullSource(agentCoreFunctions aCF, uint32 provID);


			private:
				static monitordata* generateData(uint32 srcid);
				static char* getNativeMemoryData();
				static monitordata* pullCallback();
				static void pullComplete(monitordata* data);

			};

		} /* end namespace nativememory */
	} /* end namespace omr */
} /* end namespace plugins */

#endif /* ibmras_monitoring_plugins_omr_nativememorydataprovider_h */
