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

import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

public class ClassAdapter extends ClassVisitor implements Opcodes {

	String className;

	private boolean instrumentHttpServlet = false;
	private boolean instrumentJsp = false;

	public ClassAdapter(ClassVisitor cv, String className) {
		super(ASM5, cv);
		this.className = className;
	}

	@Override
	public void visit(int version, int access, String name, String signature, String superName, String[] interfaces) {

		if (superName != null)
			if (superName.equals("javax/servlet/http/HttpServlet")) {
				instrumentHttpServlet = true;
			} else if (superName.equals("com/ibm/ws/jsp/runtime/HttpJspBase")) {
				instrumentJsp = true;
			}

		if (interfaces != null) {
			for (

			String iface : interfaces) {
				if (iface.equals("javax/servlet/jsp/HttpJspPage")) {
					instrumentJsp = true;
				}
			}
		}
		super.visit(version, access, name, signature, superName, interfaces);
	}

	@Override
	public MethodVisitor visitMethod(int access, String name, String desc, String signature, String[] exceptions) {

		MethodVisitor mv = super.visitMethod(access, name, desc, signature, exceptions);

		if (instrumentJsp) {
			mv = new HttpJspPageAdapter(className, mv, access, name, desc);
		} else if (instrumentHttpServlet) {
			mv = new HttpServletAdapter(className, mv, access, name, desc);
		}

		return mv;
	}

}
