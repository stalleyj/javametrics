package com.ibm.java.appmetrics;

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

import com.ibm.java.appmetrics.metrics.CPUTime;

/**
 * Websocket Endpoint implementation class EchoProtocol */

@ServerEndpoint(value="/", subprotocols="appmetrics4j-dash")

public class AppmetricsWebSocket {
    
	ScheduledExecutorService exec;
	
	private Set<Session> openSessions = new HashSet<>();
	
    public AppmetricsWebSocket() {
        super();
        exec = Executors.newSingleThreadScheduledExecutor();
        exec.scheduleAtFixedRate( this::emitMemoryUsage, 2, 2, TimeUnit.SECONDS);
    }

    @OnOpen
    public void open(Session session) {
    	System.err.println("open called");
    	System.err.println("Sub protocol is: " + session.getNegotiatedSubprotocol());
    	try {
    		session.getBasicRemote().sendText("{\"topic\": \"title\", \"payload\": {\"title\":\"Application Metrics for Java\", \"docs\": \"http://github.com/RuntimeTools/AppMetrics4j\"}}");
		} catch (IOException e) {
			// TODO Auto-generated catch block
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
    	openSessions.forEach((session) -> {
    		try {
				session.getBasicRemote().sendText("{\"topic\": \"memory\", \"payload\": "
						+ "{\"time\":\"" + timeStamp + "\""
						+ ", \"physical\": \"" + memTotal + "\""
						+ ", \"physical_used\": \"" + memUsed + "\""
						+ "}}");
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
    	});
    	
    }
    
}
