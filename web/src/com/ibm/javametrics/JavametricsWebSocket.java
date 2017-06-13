package com.ibm.javametrics;

import java.io.IOException;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.websocket.OnClose;
import javax.websocket.OnError;
import javax.websocket.OnMessage;
import javax.websocket.OnOpen;
import javax.websocket.Session;
import javax.websocket.server.ServerEndpoint;

import com.ibm.javametrics.dataproviders.CPUDataProvider;
import com.ibm.javametrics.dataproviders.MemoryPoolDataProvider;

/**
 * Websocket Endpoint implementation for JavametricsWebSocket
 */
@ServerEndpoint(value = "/", subprotocols = "javametrics-dash")
public class JavametricsWebSocket implements JavametricsListener {

	ScheduledExecutorService exec;

	private Set<Session> openSessions = new HashSet<>();
	
	private JavametricsAgentConnector connector;

	public JavametricsWebSocket() {
		super();
		
		this.connector = new JavametricsAgentConnector(this);

		exec = Executors.newSingleThreadScheduledExecutor();
		exec.scheduleAtFixedRate(this::emitMemoryUsage, 2, 2, TimeUnit.SECONDS);
		exec.scheduleAtFixedRate(this::emitCPUUsage, 2, 2, TimeUnit.SECONDS);
		exec.scheduleAtFixedRate(this::emitMemoryPoolUsage, 2, 2, TimeUnit.SECONDS);

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

	private void emitMemoryUsage() {
		long timeStamp = System.currentTimeMillis();
		long memTotal = Runtime.getRuntime().totalMemory();
		long memFree = Runtime.getRuntime().freeMemory();
		long memUsed = memTotal - memFree;
		String message = "{\"topic\": \"memory\", \"payload\": " + "{\"time\":\"" + timeStamp + "\""
				+ ", \"physical\": \"" + memTotal + "\"" + ", \"physical_used\": \"" + memUsed + "\"" + "}}";
		openSessions.forEach((session) -> {
			try {
				session.getBasicRemote().sendText(message);
			} catch (IOException e) {
				e.printStackTrace();
			}
		});
	}

	private void emitCPUUsage() {
		long timeStamp = System.currentTimeMillis();
		double process = CPUDataProvider.getProcessCpuLoad();
		double system = CPUDataProvider.getSystemCpuLoad();
		if (system >= 0 && process >= 0) {
			String message = "{\"topic\": \"cpu\", \"payload\": " + "{\"time\":\"" + timeStamp + "\""
					+ ", \"system\": \"" + system + "\"" + ", \"process\": \"" + process + "\"" + "}}";
			openSessions.forEach((session) -> {
				try {
					session.getBasicRemote().sendText(message);
				} catch (IOException e) {
					e.printStackTrace();
				}

			});
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
			openSessions.forEach((session) -> {
				try {
					session.getBasicRemote().sendText(message);
				} catch (IOException e) {
					e.printStackTrace();
				}

			});
		}
	}
	
	public void emit(String message) {
		openSessions.forEach((session) -> {
			try {
				session.getBasicRemote().sendText(message);
			} catch (IOException e) {
				e.printStackTrace();
			}

		});
	}

}
