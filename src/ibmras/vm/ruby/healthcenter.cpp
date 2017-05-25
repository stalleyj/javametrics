#include "ruby.h"
#include "ibmras/monitoring/AgentExtensions.h"
#include "ibmras/monitoring/Typesdef.h"
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <dlfcn.h>

VALUE HealthCenter = Qnil;

static std::string fileJoin(const std::string& path, const std::string& filename) {
#if defined(_WINDOWS)
	static const std::string fileSeparator("\\");
#else
	static const std::string fileSeparator("/");
#endif
	return path + fileSeparator + filename;
}

static std::string installDir() {
   rb_eval_string("$version = RbConfig::CONFIG['ruby_version']");
   VALUE ruby_version = rb_gv_get("version");
   rb_eval_string("$libprefix = RbConfig::CONFIG['rubylibprefix']");
   VALUE ruby_lib_prefix = rb_gv_get("libprefix");
   return fileJoin(fileJoin(fileJoin(fileJoin(rb_string_value_cstr(&ruby_lib_prefix), "gems"), rb_string_value_cstr(&ruby_version)), "gems"), "healthcenter-1.0.0");
}


EXTERN "C" void Init_healthcenter()
{
    HealthCenter = rb_define_module("HealthCenter");

    std::string INSTALL_DIR = installDir();
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
        std::string* gemDir = new std::string(INSTALL_DIR);
        std::string propFilename(fileJoin(*gemDir, std::string("healthcenter.properties")));
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


}
