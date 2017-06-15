package com.ibm.javametrics;

import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import javax.json.Json;
import javax.json.JsonObject;
import javax.json.JsonReader;
import javax.websocket.OnClose;
import javax.websocket.OnError;
import javax.websocket.OnMessage;
import javax.websocket.OnOpen;
import javax.websocket.Session;
import javax.websocket.server.ServerEndpoint;

/**
 * Websocket Endpoint implementation for JavametricsWebSocket
 */

@ServerEndpoint(value = "/", subprotocols = "javametrics-dash")
public class JavametricsWebApp implements JavametricsListener {

	private Set<Session> openSessions = new HashSet<>();
	
	private JavametricsAgentConnector connector;

	private HttpDataAggregator aggregateHttpData;

	public JavametricsWebApp() {
		super();
		System.out.println("starting websocket");
		this.connector = new JavametricsAgentConnector(this);
		this.aggregateHttpData = new HttpDataAggregator();
	}

	@OnOpen
	public void open(Session session) {
		System.err.println("open called");
		System.err.println("Sub protocol is: " + session.getNegotiatedSubprotocol());
		try {
			session.getBasicRemote().sendText(
					"{\"topic\": \"title\", \"payload\": {\"title\":\"Application Metrics for Java\", \"docs\": \"http://github.com/RuntimeTools/javametrics\"}}");
		} catch (IOException e) {
			e.printStackTrace();
		}
		openSessions.add(session);
	}

	@OnClose
	public void close(Session session) {
		System.err.println("close called");
		openSessions.remove(session);
	}

	@OnError
	public void onError(Throwable error) {
		System.err.println("error called");
	}

	@OnMessage
	public void handleMessage(String message, Session session) {
		System.err.println("handleMessage called with: " + message);
	}

	public void emit(String message) {
		openSessions.forEach((session) -> {
			try {
				if (session.isOpen()) {
					session.getBasicRemote().sendText(message);
//					System.err.println("sending " + message);
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		});
	}

	private void emitHttp() {
		HttpDataAggregator httpData;
		String httpUrlData;
		synchronized (aggregateHttpData) {
			if (aggregateHttpData.total == 0) {
				return;
			}
			httpData = aggregateHttpData.getCurrent();
			httpUrlData = aggregateHttpData.urlDatatoJsonString();
			
			aggregateHttpData.clear();	
		}
		emit(httpData.toJsonString());
		emit(httpUrlData);
	}
	
	private void emitEnv(String data) {
		//split the data into lines containing key/value pairs
		String[] envPairs = data.split("\n");
		//put the pairs into a map
		HashMap<String, String> envMap = new HashMap<String, String>();
		for (int i = 0; i < envPairs.length; i++) {
			if (envPairs[i].contains("=")) {
				//only split on the first equals
				String[] keyValue = envPairs[i].split("=", 2);
				if (1 == keyValue.length) {
					//don't have a value for this key - use the empty string
					envMap.put(keyValue[0], "");
				} else {
					envMap.put(keyValue[0], keyValue[1]);
				}
			}
		}
		//currently we display 4 items from the environment in the dash
		String commandLineValue = envMap.get("command.line");
		String environmentHOSTNAMEValue = envMap.get("environment.HOSTNAME");
		String osArchValue = envMap.get("os.arch");
		String numberOfProcessorsValue = envMap.get("number.of.processors");
		
		//check that we don't have null values
		if (null != commandLineValue && null != environmentHOSTNAMEValue && null != osArchValue && null != numberOfProcessorsValue) {
			//construct the expected payload
			StringBuffer messageBuf = new StringBuffer("{\"topic\":\"env\",\"payload\":[");
			messageBuf.append("{\"Parameter\":\"Command Line\",\"Value\":\"");
			// remove quote marks (") in command.line value so JSON parse doesn't get confused later, also escape backslashes
			// this looks stupid, but \ needs to be escaped once for Java strings, then escaped again for regex
			messageBuf.append(commandLineValue.replaceAll("\"", "").replaceAll("\\\\", "\\\\\\\\"));
			messageBuf.append("\"},");
			messageBuf.append("{\"Parameter\":\"Hostname\",\"Value\":\"");
			messageBuf.append(environmentHOSTNAMEValue);
			messageBuf.append("\"},");
			messageBuf.append("{\"Parameter\":\"OS Architecture\",\"Value\":\"");
			messageBuf.append(osArchValue);
			messageBuf.append("\"},");
			messageBuf.append("{\"Parameter\":\"Number of Processors\",\"Value\":\"");
			messageBuf.append(numberOfProcessorsValue);
			messageBuf.append("\"}]}");
		
			emit(messageBuf.toString());
		}
	}

	@Override
	public void receive(String topic, String data) {
		if (topic.equals("api")) {
			List<String> split = splitIntoJSONObjects(data);
			//System.out.println("data = " + data.toString());
			for (Iterator<String> iterator = split.iterator(); iterator.hasNext();) {
				String jsonStr = iterator.next();
				JsonReader jsonReader = Json.createReader(new StringReader(jsonStr));
				JsonObject jsonObject = jsonReader.readObject();
				String topicName = jsonObject.getString("topic", null);
				if (topicName != null) {
					if (topicName.equals("http")) {
						synchronized (aggregateHttpData) {
							aggregateHttpData.aggregate(jsonObject.getJsonObject("payload"));
						}
					} else {
						emit(jsonObject.toString());
					}
				}
			}
			emitHttp();
		} else if (topic.contains("common_env")) {
			emitEnv(data);
		}
	}

	/**
	 * Split a string of JSON objects into multiple strings
	 * @param data
	 * @return
	 */
	private List<String> splitIntoJSONObjects(String data) {
		List<String> strings = new ArrayList<String>();
		int index = 0;
		int closingBracket = index + 1;
		int bracketCounter = 1;
		while(index < data.length() - 1 && closingBracket < data.length()) {
			// Find the matching bracket for the bracket at location 'index'
			boolean found = false;
			if(data.charAt(closingBracket) == '{') {
				bracketCounter++;
			} else if(data.charAt(closingBracket) == '}') {
				bracketCounter--;
				if(bracketCounter == 0) {
					// found matching bracket
					found = true;
				}
			}
			if (found) {
				strings.add(data.substring(index, closingBracket + 1));
				index = closingBracket + 1;
				closingBracket = index + 1;
				found = false;
				bracketCounter = 1;
			} else {
				closingBracket++;
			}
		}
		return strings;
	}
}
