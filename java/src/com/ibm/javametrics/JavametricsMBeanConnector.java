/*******************************************************************************
 * Copyright 2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 ******************************************************************************/
package com.ibm.javametrics;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import com.ibm.javametrics.dataproviders.CPUDataProvider;
import com.ibm.javametrics.dataproviders.MemoryPoolDataProvider;

/**
 * Uses MBean data providers to send data to the Javametrics agent at regular intervals.
 */
public class JavametricsMBeanConnector {

	
	private ScheduledExecutorService exec;
	private JavametricsAgentConnector javametricsAgentConnector;

	/**
	 * Create a JavametricsMBeanConnector
	 * @param agentConnector JavametricsAgentConnector - required so data can be sent to the agent
	 */
	public JavametricsMBeanConnector(JavametricsAgentConnector agentConnector) {
		this.javametricsAgentConnector = agentConnector;
		exec = Executors.newSingleThreadScheduledExecutor();
		exec.scheduleAtFixedRate(this::emitMemoryUsage, 2, 2, TimeUnit.SECONDS);
		exec.scheduleAtFixedRate(this::emitCPUUsage, 2, 2, TimeUnit.SECONDS);
		exec.scheduleAtFixedRate(this::emitMemoryPoolUsage, 2, 2, TimeUnit.SECONDS);
	}

	private void emitMemoryUsage() {
		long timeStamp = System.currentTimeMillis();
		long memTotal = Runtime.getRuntime().totalMemory();
		long memFree = Runtime.getRuntime().freeMemory();
		long memUsed = memTotal - memFree;
		String message = "{\"topic\": \"memory\", \"payload\": " + "{\"time\":\"" + timeStamp + "\""
				+ ", \"physical\": \"" + memTotal + "\"" + ", \"physical_used\": \"" + memUsed + "\"" + "}}";
		javametricsAgentConnector.sendDataToAgent(message);
	}

	private void emitCPUUsage() {
		long timeStamp = System.currentTimeMillis();
		double process = CPUDataProvider.getProcessCpuLoad();
		double system = CPUDataProvider.getSystemCpuLoad();
		if (system >= 0 && process >= 0) {
			String message = "{\"topic\": \"cpu\", \"payload\": " + "{\"time\":\"" + timeStamp + "\""
					+ ", \"system\": \"" + system + "\"" + ", \"process\": \"" + process + "\"" + "}}";
			javametricsAgentConnector.sendDataToAgent(message);
		}
	}

	private void emitMemoryPoolUsage() {
		long timeStamp = System.currentTimeMillis();
		long usedHeapAfterGC = MemoryPoolDataProvider.getUsedHeapAfterGC();
		long usedNative = MemoryPoolDataProvider.getNativeMemory();
		long usedHeap = MemoryPoolDataProvider.getHeapMemory();
		if (usedHeapAfterGC >= 0) { // check that some data is available
			String message = "{\"topic\": \"memoryPools\", \"payload\": " + "{\"time\":\"" + timeStamp + "\""
					+ ", \"usedHeapAfterGC\": \"" + usedHeapAfterGC + "\"" + ", \"usedHeap\": \"" + usedHeap + "\""
					+ ", \"usedNative\": \"" + usedNative + "\"" + "}}";
			javametricsAgentConnector.sendDataToAgent(message);
		}
	}
}
