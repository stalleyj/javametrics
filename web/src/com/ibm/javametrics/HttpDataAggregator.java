package com.ibm.javametrics;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;

import javax.json.JsonObject;

public class HttpDataAggregator {
	int total;
	long average;
	long longest;
	long time;
	String url;
	
	private HashMap<String, HttpUrlData> responseTimes = new HashMap<String, HttpUrlData>();
	
	public HttpDataAggregator() {
		clear();
	}

	private HttpDataAggregator(int total, long average, long longest, long time, String url) {
		this.total = total;
		this.average = average;
		this.longest = longest;
		this.time = time;
		this.url = url;
	}

	HttpDataAggregator getCurrent() {
		return new HttpDataAggregator(total, average, longest, time, url);
	}
	
	void clear() {
		total = 0;
		average = 0;
		longest = 0;
		time = 0;
		url = "";
	}

	public void aggregate(JsonObject jsonObject) {
		
		if (total == 0) {
			time = jsonObject.getJsonNumber("time").longValue();
		}
		
		total += 1;

		long requestDuration = jsonObject.getJsonNumber("duration").longValue();
		String requestUrl = jsonObject.getString("url", "");
		if (requestDuration > longest) {
			longest = requestDuration;
			url = requestUrl;
		}	
		average = ((average * (total-1)) + requestDuration)/total;	
		
		HttpUrlData urlData = responseTimes.get(requestUrl);
		if (urlData == null) {
			urlData = new HttpUrlData();
		}
		urlData.hits += 1;
		urlData.averageResponseTime = (urlData.averageResponseTime * (urlData.hits-1))/urlData.hits;
		responseTimes.put(requestUrl, urlData);
	}
	
	String toJsonString() {
		StringBuilder sb = new StringBuilder("{\"topic\" : \"http\", \"payload\" : {\"time\" : ");
		sb.append(time);
		sb.append(", \"total\" : ");
		sb.append(total);
		sb.append(", \"longest\" : ");
		sb.append(longest);
		sb.append(", \"average\" : ");
		sb.append(average);
		sb.append(", \"url\" : \"");
		sb.append(url);
		sb.append("\"}}");
		return sb.toString();
	}
	
	private class HttpUrlData {
		int hits;
		long averageResponseTime;
		
		public HttpUrlData() {
			hits = 0;
			averageResponseTime = 0;
		}
	}

	public String urlDatatoJsonString() {
		StringBuilder sb = new StringBuilder("{\"topic\" : \"httpURLs\", \"payload\" : [");
		
	    Iterator<Entry<String, HttpUrlData>> it = responseTimes.entrySet().iterator();
        boolean first = true;
	    while (it.hasNext()) {
	        Entry<String, HttpUrlData> pair = it.next();
			if (!first) {
	        	sb.append(',');
	        }
			first = false;
	        sb.append("{ \"url\" : \"");
	        sb.append(pair.getKey());
	        sb.append("\", \"averageResponseTime\" : ");
	        sb.append(pair.getValue().averageResponseTime);
	        sb.append("}");
	    }
	    
		sb.append(" ] }");
		return sb.toString();
	}
}
