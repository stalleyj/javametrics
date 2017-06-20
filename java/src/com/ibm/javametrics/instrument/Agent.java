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
package com.ibm.javametrics.instrument;

import java.lang.instrument.Instrumentation;

/**
 * Java instrumentation agent
 * 
 * Invoked by the -javaagent:jarpath=[options] command line parameter Entry
 * point is defined in the jar manifest as: Premain-Class:
 * com.ibm.javametrics.instrument.Agent
 *
 */
public class Agent {

	public static boolean debug = (System.getProperty("com.ibm.javamatrics.javaagent.debug", "false").equals("true"));

	/**
	 * Entry point for the agent via -javaagent command line parameter
	 * 
	 * @param agentArgs
	 * @param inst
	 */
	public static void premain(String agentArgs, Instrumentation inst) {
		// Register our class transformer
		inst.addTransformer(new ClassTransformer());
	}

	/**
	 * @param agentArgs
	 * @param inst
	 */
	public static void agentmain(String agentArgs, Instrumentation inst) {
		premain(agentArgs, inst);
	};
}
