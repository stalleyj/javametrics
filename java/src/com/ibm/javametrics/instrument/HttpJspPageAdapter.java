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

public class HttpJspPageAdapter extends ServletCallBackAdapter {

	protected HttpJspPageAdapter(String className, MethodVisitor mv, int access, String name, String desc) {
		super(className, mv, access, name, desc);
	}

	@Override
	protected void onMethodEnter() {
		if (methodName.equals("_jspService")) {
			injectMethodTimer();
		}
	}

	@Override
	protected void onMethodExit(int opcode) {
		if (methodName.equals("_jspService")) {
			injectServletCallback();
		}
	}

}
