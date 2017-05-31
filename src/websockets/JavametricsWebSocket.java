package websockets;

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

import dataProviders.CPUDataProvider;
import dataProviders.MemoryPoolDataProvider;

/**
 * Websocket Endpoint implementation class EchoProtocol
 */

@ServerEndpoint(value = "/", subprotocols = "javametrics-dash")

public class JavametricsWebSocket {

	private static native void regListener(JavametricsWebSocket jm);

	private static native void deregListener();

	private static native void sendMessage(String message, byte[] id);

	private static final String CLIENT_ID = "localNative";//$NON-NLS-1$
	private static final String COMMA = ","; //$NON-NLS-1$
	private static final String DATASOURCE_TOPIC = "/datasource";//$NON-NLS-1$
	private static final String CONFIGURATION_TOPIC = "configuration/";//$NON-NLS-1$
	private static final String HISTORY_TOPIC = "/history/";//$NON-NLS-1$

	ScheduledExecutorService exec;

	private Set<Session> openSessions = new HashSet<>();

	public JavametricsWebSocket() {
		super();

		regListener(this);

		sendMessage("datasources", CLIENT_ID);//$NON-NLS-1$

		// request the agent to send us current history (flight recorder)
		sendMessage("history", CLIENT_ID);//$NON-NLS-1$

		// Need to request the method dictionary
		sendMessage("methoddictionary", "");//$NON-NLS-1$

		exec = Executors.newSingleThreadScheduledExecutor();
		exec.scheduleAtFixedRate(this::emitMemoryUsage, 2, 2, TimeUnit.SECONDS);
		exec.scheduleAtFixedRate(this::emitCPUUsage, 2, 2, TimeUnit.SECONDS);
		exec.scheduleAtFixedRate(this::emitMemoryPoolUsage, 2, 2, TimeUnit.SECONDS);

//		System.out.println("press enter key to exit");
//		try {
//			System.in.read();
//		} catch (Exception e) {
//		}

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

	public void sendMessage(String name, String command, String... params) {
		StringBuffer sb = new StringBuffer();
		sb.append(command);
		for (String parameter : params) {
			sb.append(COMMA).append(parameter);
		}
		sb.trimToSize();
		sendMessage(name, sb.toString().getBytes());
	}

	public void receiveData(String type, byte[] data) {
		String dataType;
		System.out.println("type is " + type);
		if (type.startsWith(CLIENT_ID)) {
			dataType = type.substring(CLIENT_ID.length());
		} else {
			dataType = type;
		}
		if (dataType.equals(DATASOURCE_TOPIC)) {
			System.out.println("dataType is " + dataType);
			String contents;
			contents = new String(data);
			System.out.println("contents is " + contents);
		}

		if (type.equalsIgnoreCase("memory")) {
			System.out.println("data is " + new String(data));

		}

		// System.out.println("data is " + new String(data));

	}

}
