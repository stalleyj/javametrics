package com.ibm.javametrics;

/**
 * A listener to Javametrics events
 *
 */
public interface JavametricsListener {
	
	/**
	 * Receive data from the Javametrics agent
	 * @param pluginName - the plugin that sent the data
	 * @param data - the data as a String
	 */
	public void receive(String pluginName, String data);
}
