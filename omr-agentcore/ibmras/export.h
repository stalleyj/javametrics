 /**
 * IBM Confidential
 * OCO Source Materials
 * IBM Monitoring and Diagnostic Tools - Health Center
 * (C) Copyright IBM Corp. 2007, 2016 All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
 */


#ifndef ibmras_export_h
#define ibmras_export_h


#if defined(_WINDOWS)
#if defined(EXPORT)
#define DECL __declspec(dllexport)	/* required for DLLs to export the plugin functions */
#else
#define DECL __declspec(dllimport)
#endif
#endif

/* provide a default definition of DECL of the platform does not define one */
#ifndef DECL
#define DECL
#endif


#endif /* ibmras_export_h */
