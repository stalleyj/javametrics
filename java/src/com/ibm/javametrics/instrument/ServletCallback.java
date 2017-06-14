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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Collection;
import java.util.Enumeration;

import com.ibm.javametrics.Javametrics;

public class ServletCallback {

	@SuppressWarnings("unchecked")
	public static void doGetCallback(long requestTime, Object request, Object response) {

		HttpData data = new HttpData();
		data.setRequestTime(requestTime);

		Class<?> reqClass = request.getClass();
		Class<?> respClass = response.getClass();
		try {
			Method getRequestURL = reqClass.getMethod("getRequestURL");
			data.setUrl(((StringBuffer) getRequestURL.invoke(request)).toString());

			Method getMethod = reqClass.getMethod("getMethod");
			data.setMethod((String) getMethod.invoke(request));

			Method getContentType = respClass.getMethod("getContentType");
			data.setContentType((String) getContentType.invoke(response));

			Method getHeaders = respClass.getMethod("getHeaderNames");
			Method getHeader = respClass.getMethod("getHeader", String.class);
			Collection<String> headers = (Collection<String>) getHeaders.invoke(response);
			if (headers != null) {
				for (String headerName : headers) {
					String header = (String) getHeader.invoke(response, headerName);
					if (header != null) {
						data.addHeader(headerName, header);
					}
				}
			}

			Method getReqHeaders = reqClass.getMethod("getHeaderNames");
			Method getReqHeader = reqClass.getMethod("getHeader", String.class);
			Enumeration<String> reqHeaders = (Enumeration<String>) getReqHeaders.invoke(request);
			if (reqHeaders != null) {
				while (reqHeaders.hasMoreElements()) {
					String headerName = reqHeaders.nextElement();
					String header = (String) getReqHeader.invoke(request, headerName);
					if (header != null) {
						data.addRequestHeader(headerName, header);
					}
				}
			}

		} catch (NoSuchMethodException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (SecurityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IllegalAccessException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InvocationTargetException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		data.setDuration(System.currentTimeMillis() - requestTime);
		
		if (Agent.debug) {
			System.err.println("{\"http\" : " + data.toJsonString() + "}");
		}

		Javametrics.sendJSON("http", data.toJsonString());

	}

}
