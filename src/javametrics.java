
class javametrics {
  private static native void regListener(javametrics jm);
  private static native void deregListener();
  private static native void sendMessage(String message, byte[] id);
  private static final String CLIENT_ID = "localNative";//$NON-NLS-1$
  private static final String COMMA = ","; //$NON-NLS-1$
  private static final String DATASOURCE_TOPIC = "/datasource";//$NON-NLS-1$
	private static final String CONFIGURATION_TOPIC = "configuration/";//$NON-NLS-1$
	private static final String HISTORY_TOPIC = "/history/";//$NON-NLS-1$



  static public void main(String[] args) {
	  new javametrics();
	}

	javametrics() {
	  regListener(this);

    sendMessage("datasources", CLIENT_ID);//$NON-NLS-1$

// request the agent to send us current history (flight recorder)
sendMessage("history", CLIENT_ID);//$NON-NLS-1$

// Need to request the method dictionary
sendMessage("methoddictionary", "");//$NON-NLS-1$
  System.out.println("press enter key to exit");
  try {
  System.in.read();
} catch (Exception e) {
		}
	}

  public void sendMessage(String name, String command, String... params)
	{
		StringBuffer sb = new StringBuffer();
		sb.append(command);
		for (String parameter : params) {
			sb.append(COMMA).append(parameter);
		}
		sb.trimToSize();
		sendMessage(name, sb.toString().getBytes());
	}

  public void receiveData(String type, byte[] data)
	{
		String dataType;
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

    System.out.println("data is " + new String(data));

	}


}
