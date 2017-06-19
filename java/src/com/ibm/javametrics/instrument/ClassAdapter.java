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

/**
 * An instance of this ClassVisitor is created for each class being loaded.
 *
 */
public class ClassAdapter extends ClassVisitor implements Opcodes {

	String className;

	private boolean instrumentHttpServlet = false;
	private boolean instrumentJsp = false;

	/**
	 * @param cv
	 */
	public ClassAdapter(ClassVisitor cv) {
		super(ASM5, cv);
	}

	/*
	 * First visitor method called during class parsing
	 * 
	 * Here we determine which classes/methods to instrument. This is currently
	 * hard-coded here but could be implemented via configuration files
	 */
	@Override
	public void visit(int version, int access, String name, String signature, String superName, String[] interfaces) {

		className = name;

		/*
		 * HTTP request instrumentation
		 * 
		 * Servlets: instrument any class that is a subclass of HttpServlet
		 * 
		 * JSP pages: instrument any class that implements the HttpJspPage
		 * interface
		 */
		if (superName != null)
			if (superName.equals("javax/servlet/http/HttpServlet")) {
				instrumentHttpServlet = true;
			} else if (superName.equals("com/ibm/ws/jsp/runtime/HttpJspBase")) {
				/*
				 * For Liberty the HttpJspBase class implements HttpJspPage but
				 * is later subclassed so we need to instrument the subclasses.
				 */
				// TODO: work out how to inspect the class hierarchy to find
				// classes we need to instrument
				instrumentJsp = true;
			}

		if (interfaces != null) {
			for (String iface : interfaces) {
				if (iface.equals("javax/servlet/jsp/HttpJspPage")) {
					instrumentJsp = true;
				}
			}
		}
		super.visit(version, access, name, signature, superName, interfaces);
	}

	/*
	 * Called for each method in the class
	 * 
	 * We return a new instance of a class specific MethodVisitor for each
	 * method we need to instrument
	 */
	@Override
	public MethodVisitor visitMethod(int access, String name, String desc, String signature, String[] exceptions) {

		MethodVisitor mv = super.visitMethod(access, name, desc, signature, exceptions);

		/*
		 * The order here is important as JSP may also extend HttpServlet
		 */
		if (instrumentJsp) {
			mv = new HttpJspPageAdapter(className, mv, access, name, desc);
		} else if (instrumentHttpServlet) {
			mv = new HttpServletAdapter(className, mv, access, name, desc);
		}

		return mv;
	}

}
