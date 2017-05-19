package com.ibm.java.appmetrics.metrics;
import java.lang.management.ManagementFactory;
import java.lang.management.OperatingSystemMXBean;

public class CPUTime {

	public static double getSystemLoad() {
		OperatingSystemMXBean bean = ManagementFactory.getOperatingSystemMXBean();
		return bean.getSystemLoadAverage();
	}
	
	public static long getProcessCpuTime() {
		com.sun.management.OperatingSystemMXBean bean = (com.sun.management.OperatingSystemMXBean)ManagementFactory.getOperatingSystemMXBean();;
		return bean.getProcessCpuTime();
	}
	
}
