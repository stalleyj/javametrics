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

/**
 * Object set as attribute on ServletRequest
 *
 */
public class HttpRequestTracker {
<<<<<<< HEAD
	private long requestTime;
	private int requestDepth;

	/**
	 * Initialize request time and depth
	 */
	public HttpRequestTracker() {
		requestTime = System.currentTimeMillis();
		requestDepth = 0;
	}

	public long getRequestTime() {
		return requestTime;
	}

	public int getRequestDepth() {
		return requestDepth;
	}

	/**
	 * Increment nesting depth
	 */
	public void increment() {
		requestDepth += 1;
	}

	/**
	 * @return true if still nested
	 */
	public boolean decrement() {
		return (--requestDepth > 0);
	}
=======
    private long requestTime;
    private int requestDepth;

    /**
     * Initialize request time and depth
     */
    public HttpRequestTracker() {
        requestTime = System.currentTimeMillis();
        requestDepth = 0;
    }

    public long getRequestTime() {
        return requestTime;
    }

    public int getRequestDepth() {
        return requestDepth;
    }

    /**
     * Increment nesting depth
     */
    public void increment() {
        requestDepth += 1;
    }

    /**
     * @return true if still nested
     */
    public boolean decrement() {
        return (--requestDepth > 0);
    }
>>>>>>> 788e9f57cffe1a428784683542f66e5918ad303b
}