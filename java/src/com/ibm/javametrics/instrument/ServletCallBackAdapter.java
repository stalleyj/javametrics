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

import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Type;
import org.objectweb.asm.commons.Method;

/**
 * MethodVisitor providing common servlet instrumentation
 *
 */
public class ServletCallBackAdapter extends BaseAdviceAdapter {

	private static final String SERVLET_CALLBACK_TYPE = "com/ibm/javametrics/instrument/ServletCallback";
	private static final String SERVLET_CALLBACK_METHOD = "void after(long, java.lang.Object, java.lang.Object)";

	protected ServletCallBackAdapter(String className, MethodVisitor mv, int access, String name, String desc) {
		super(className, mv, access, name, desc);
	}

	@Override
	protected void onMethodEnter() {
		injectMethodTimer();
	}

	@Override
	protected void onMethodExit(int opcode) {
		injectServletCallback();
	}

	/**
	 * Inject a callback to our servlet handler. To be called from
	 * onMethodExit(...). Assumes BaseAdviceAdapter.injectMethodTimer() was
	 * called during onMethodEnter(...)
	 */
	private void injectServletCallback() {
		loadLocal(methodEntertime);
		loadArgs();
		invokeStatic(Type.getType(SERVLET_CALLBACK_TYPE), Method.getMethod(SERVLET_CALLBACK_METHOD));
	}

}
