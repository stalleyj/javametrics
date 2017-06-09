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

import java.io.PrintStream;

import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Type;
import org.objectweb.asm.commons.AdviceAdapter;
import org.objectweb.asm.commons.Method;

public class BaseAdviceAdapter extends AdviceAdapter {

	protected String className;
	protected String methodName;
	protected int methodEntertime;

	protected BaseAdviceAdapter(String className, MethodVisitor mv, int access, String name, String desc) {
		super(ASM5, mv, access, name, desc);
		this.className = className;
		this.methodName = name;
	}

	protected void insertMethodTimer() {
		methodEntertime = newLocal(Type.LONG_TYPE);
		invokeStatic(Type.getType(System.class), Method.getMethod("long currentTimeMillis()"));
		storeLocal(methodEntertime);
		if (Agent.debug) {
			getStatic(Type.getType(System.class), "err", Type.getType(PrintStream.class));
			push(">> Calling method: " + className + "." + methodName);
			invokeVirtual(Type.getType(PrintStream.class), Method.getMethod("void println(java.lang.String)"));
		}
	}

}
