#include <Python.h>
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <dlfcn.h>

static std::string fileJoin(const std::string& path, const std::string& filename) {
#if defined(_WINDOWS)
        static const std::string fileSeparator("\\");
#else
        static const std::string fileSeparator("/");
#endif
        return path + fileSeparator + filename;
}

static std::string sitePackageDir() {

    PyObject *pName, *pModule, *pFunc;
    PyObject *pValue, *pPath, *pString;

    std::string result = "";

    pName = PyUnicode_FromString("site");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "getsitepackages");

        if (pFunc && PyCallable_Check(pFunc)) {
            pValue = PyObject_CallObject(pFunc, NULL);
            if (pValue != NULL) {
                pPath = PyList_GetItem(pValue, 0);
                pString = PyUnicode_AsUTF8String(pPath);
                if(pString != NULL) {
                    result = PyBytes_AsString(pString);
                } else {
                    result = "";
                }
                Py_DECREF(pString);
                Py_DECREF(pPath);
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
    }

    return result;
}

static PyMethodDef healthcenter_funcs[] = {
    {NULL, NULL, 0, NULL}
};


#if PY_MAJOR_VERSION >= 3
  static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "healthcenter", 		/* m_name */
    NULL,      			/* m_doc */
    -1,                  	/* m_size */
    healthcenter_funcs,    	/* m_methods */
    NULL,                	/* m_reload */
    NULL,                	/* m_traverse */
    NULL,                	/* m_clear */
    NULL,                	/* m_free */
  };
#endif


PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
PyInit_healthcenter(void)
#else
inithealthcenter(void)
#endif
{
    std::string site_packages = sitePackageDir();
    std::string INSTALL_DIR = fileJoin(site_packages, "healthcenter");
    std::string PLUGINS_DIR = fileJoin(INSTALL_DIR, "plugins");

        void* handle;
    handle = dlopen("libagentcore.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Health Center Startup: failed to open libagentcore.so: " << dlerror();
        exit(-1);
    }
    typedef loaderCoreFunctions* (*entryFunc)(void);
    entryFunc entry = (entryFunc)dlsym(handle, "loader_entrypoint");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Health Center Startup: cannot find symbol 'loader_entrypoint' in libagentcore.so: " << dlsym_error <<
            '\n';
        dlclose(handle);
        exit(-1);
    }
    loaderCoreFunctions* functions = entry();

    bool propsLoaded = false;
    std::string propFilename("healthcenter.properties");
    propsLoaded = functions->loadPropertiesFile(propFilename.c_str());
    if (propsLoaded == false) {
        std::string* siteDir = new std::string(INSTALL_DIR);
        std::string propFilename(fileJoin(*siteDir, std::string("healthcenter.properties")));
        propsLoaded = functions->loadPropertiesFile(propFilename.c_str());
    }
    if (propsLoaded == false) {
        functions->logMessage(ibmras::common::logging::info,"Failed to load healthcenter.properties");
    } 

    const char* plugins_path = functions->getProperty("com.ibm.diagnostics.healthcenter.plugin.path");
    if (strcmp(plugins_path, "") == 0) {
    	functions->setProperty("com.ibm.diagnostics.healthcenter.plugin.path", PLUGINS_DIR.c_str());
    }

    functions->logMessage(ibmras::common::logging::info,"Initializing Health Center");
    functions->init();

    functions->setLogLevels();

    std::string agentVersion = "Health Center Agent ";
    agentVersion += functions->getAgentVersion();
    functions->logMessage(ibmras::common::logging::fine, agentVersion.c_str());

    functions->logMessage(ibmras::common::logging::info,"Starting Health Center");
    functions->start();

#if PY_MAJOR_VERSION >= 3
    PyObject *m;
    m = PyModule_Create(&moduledef);
    return m;
#else
    Py_InitModule3("healthcenter", healthcenter_funcs, "Health Center loaded"); 
#endif

}
