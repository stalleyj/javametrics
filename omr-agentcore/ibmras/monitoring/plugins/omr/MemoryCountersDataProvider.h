 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_omr_memorycountersdataprovider_h
#define ibmras_monitoring_plugins_omr_memorycountersdataprovider_h

#include "ibmras/monitoring/Plugin.h"
#include "omr.h"
#include "omragent.h"
#include "ibmras/vm/omr/healthcenter.h"




namespace plugins {
	namespace omr {
		namespace memorycounters {



			class MemoryCountersDataProvider : ibmras::monitoring::Plugin {
			public:
				static MemoryCountersDataProvider* getInstance(omrRunTimeProviderParameters oRTPP);
				static MemoryCountersDataProvider* getInstance();

				MemoryCountersDataProvider(omrRunTimeProviderParameters oRTPP);
				virtual ~MemoryCountersDataProvider() {};  //destructor
				static int memstart();
				static int memstop();
				static pullsource* registerPullSource(agentCoreFunctions aCF,uint32 provID);


			private:
				static monitordata* generateData(uint32 srcid);
				static void dealloc_report_lines(char *lines[], int count);
				static void hc_dealloc(char* buffer);
				static char* getMemoryCounters();
				static char * join_strings(char *strings[], int count);
				static monitordata* pullCallback();
				static void pullComplete(monitordata* data);

			};

		} /* end namespace memorycounters */
	} /* end namespace omr */
} /* end namespace plugins */

#endif /* ibmras_monitoring_plugins_omr_memorycountersdataprovider_h */
