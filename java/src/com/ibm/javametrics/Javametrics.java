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

import java.util.HashMap;

/**
 * Javametrics public API class.  Used to create Topics which can send data to Javametrics
 * Use Javametrics.getJavametrics() to get the singleton instance of this class
 */
public class Javametrics
{
	
	private static Javametrics instance;
	
	static {
		instance = new Javametrics();
	}
	
	private Javametrics() {}
	
	/**
	 * Get a Javametrics instance
	 * @return
	 */
	public static Javametrics getJavametrics() {
		return instance;
	}

	private HashMap<String, Topic> topics = new HashMap<String, Topic>();
	private JavametricsAgentConnector javametricsAgentConnector;
	
	/**
	 * Get a Topic to send data on.  If a topic with the given name already exists then that will be returned to you
	 * @param topicName
	 * @return
	 */
	public Topic getTopic(String topicName) {
		if(topics.containsKey(topicName)) {
			return topics.get(topicName);
		} else {
			Topic topic = new UserTopic(topicName, this);
			topics.put(topicName, topic);
			return topic;
		}
	}

	protected void registerJavametricsAgentConnector(JavametricsAgentConnector javametricsAgentConnector)
	{
		this.javametricsAgentConnector = javametricsAgentConnector;
		
	}
	
	protected void sendData(String data) {
		if(javametricsAgentConnector != null) {
			javametricsAgentConnector.sendDataToAgent(data);
		}
	}
	
	/**
	 * Send data to Javametrics
	 * @param topicName the name of the topic to send data on
	 * @param payload the JSON formatted String to send
	 */
	public void sendJSON(String topicName, String payload) {
		getTopic(topicName).sendJSON(payload);		
	}
	
	/**
	 * Returns true if the given topic is enabled
	 * @param topicName
	 * @return
	 */
	public boolean isEnabled(String topicName) {
		return getTopic(topicName).isEnabled();
	}

	
}
