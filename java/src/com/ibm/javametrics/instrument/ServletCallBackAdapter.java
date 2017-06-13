package com.ibm.javametrics.instrument;

import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Type;
import org.objectweb.asm.commons.Method;

public class ServletCallBackAdapter extends BaseAdviceAdapter {

	protected ServletCallBackAdapter(String className, MethodVisitor mv, int access, String name, String desc) {
		super(className, mv, access, name, desc);
	}

	protected void insertServletCallback() {
		loadLocal(methodEntertime);
		loadArgs();
		invokeStatic(Type.getType("com/ibm/javametrics/instrument/ServletCallback"),
				Method.getMethod("void doGetCallback(long, java.lang.Object, java.lang.Object)"));
	}

}
