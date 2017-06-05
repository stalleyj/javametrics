 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */

#ifndef ibmras_monitoring_plugins_omr_cpudataprovider_h
#define ibmras_monitoring_plugins_omr_cpudataprovider_h

#include "ibmras/monitoring/Plugin.h"
#include "omr.h"
#include "omragent.h"
#include "ibmras/vm/omr/healthcenter.h"

namespace plugins {
	namespace omr {
		namespace cpu {

			class CpuDataProvider : ibmras::monitoring::Plugin {
			public:
				static CpuDataProvider* getInstance(omrRunTimeProviderParameters oRTPP);
				static CpuDataProvider* getInstance();

				CpuDataProvider(omrRunTimeProviderParameters oRTPP);
				virtual ~CpuDataProvider() {};  //destructor
				static int cpustart();
				static int cpustop();
				static pullsource* registerPullSource(agentCoreFunctions aCF,uint32 provID);


			private:
				static monitordata* generateData(uint32 srcid);
				static char* getCpuData();
				static monitordata* pullCallback();
				static void pullComplete(monitordata* data);

			};

		} /* end namespace cpu */
	} /* end namespace omr */
} /* end namespace plugins */

#endif /* ibmras_monitoring_plugins_omr_cpudataprovider_h */
