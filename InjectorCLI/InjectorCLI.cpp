#include "CInjector.h"

// InjectorCLI.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#include <string.h> 

#include "cargs.h"
#include <stdbool.h>
#include <stdlib.h>

/**
* This is the main configuration of all options available.
*/
static struct cag_option options[] = {
 {'m',
  "mM",
  "mapping",
  "STANDART",
  "Set's the type of mapping, this field can take the following values: STANDART,MANUAL"},

 {'i',
  "iI",
  "pid",
  NULL ,
  "The process id where the dll should be injected"},

 {'p',
  "pP",
  "process_path",
  NULL,
  "The injector will also execute the process and inject on it"},

 {'s',
  "",
  "steal_handle_job",
  NULL,
  "Whether or not to steal the process handle, it uses a special technique to retrive the process handle even before the own process knows it's PID, it can only be used in conjunction with --process_path and should be combined with manualmap.\n"},

 {'a',
  "a",
  "auto_inject",
  "",
  "Use auto inject, will keep running the injector in the background and inject into processes with the process name specified"},

  {'w',
  "aW",
  "auto_inject_window",
  "",
  "Use auto inject and only injects in new windows, will keep running the injector in the background and inject into new windows with the specified name\n"},

  {'h',
  "h",
  "help",
  NULL,
  "For auto injection the user must provide -auto_inject with a process path\nFor normal injection either the option pid or process_path must be provided\nIf pid is provied will inject into the respective pid else it will execute the process\nsteal_handle_job option uses a diferent technique to inject."},
};

/**
* This is a custom project configuration structure where you can store the
* parsed information.
*/


int main(int argc, char* argv[])
{


    char identifier;
    const char* value;
    cag_option_context context;


    bool executeFile = false;
    bool stealHandle = false;
    bool autoInject = false;
    bool autoInjectWindow = false;
    const char* process_path ="";
    const char* process_name = "";
    int pid = 0;
    const char* dllPath = "";
    int mappingType = LOADLIBRARY_A;

    cag_option_prepare(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&context)) {
        identifier = cag_option_get(&context);
        switch (identifier) {
        case 'm':
            value = cag_option_get_value(&context);
            if (strcmp(value, "MANUAL") == 0) {
                mappingType = MANUAL_MAP;
            }
            else {
                mappingType = LOADLIBRARY_A;
            }
            break;
        case 'i':
            value = cag_option_get_value(&context);
            pid = atoi(value);
            break;
        case 'p':
            executeFile = true;
            process_path = cag_option_get_value(&context);
            break;
        case 's':
            stealHandle = true;
        case 'a':
            process_name = cag_option_get_value(&context);
            if (strlen(process_name) == 0) {
                printf("A valid process name must be specified to use auto inject\n");
                return 0;
            }
            autoInject= true;
            break;
        case 'w':
            process_name = cag_option_get_value(&context);
            if (strlen(process_name) == 0) {
                printf("A valid process name must be specified to use auto inject\n");
                return 0;
            }
            autoInjectWindow = true;
            break;
        case 'h':
            printf("Usage: InjectorCLI [OPTIONS] <PATH_TO_DLL>...\n");
            cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
            return 0;
            break;
        }
    }
    int index = cag_option_get_index(&context);
    if (index > argc) {
        printf("Missing path of dll file\n");
        return EXIT_SUCCESS;
    }
    else {
        dllPath = argv[index];
    }

#ifndef _DEBUG
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif // DEBUG

    CInjector injector;
    CDllMap* map;


    //Set mapping type
    if (mappingType == MANUAL_MAP) {
        map = new CManualMap(false);
    }
    else if (mappingType == LOADLIBRARY_A) {
        map = new CLoadLibrary(false);
    }
    else {
        printf("Uknown mapping type\n");
        return EXIT_SUCCESS;
    }

    if (autoInjectWindow) {
        if (!injector.AutoInjectFromWindow(*map,dllPath,process_name)){
            printf("Error Window on injection\n");
        }
        return 0;
    }

    if (!autoInject) {
        //Single normal injection
        if (strlen(process_path) > 0) {
            if (!injector.StartProcessAndInject(*map, stealHandle, process_path, dllPath)) {
                printf("Error on injection\n");
            }
        }
        else {
            if (pid == 0) {
                printf("Either a PID or a file path needs to be provided in order to use injection");
            }
            injector.InjectFromPID(*map, pid, dllPath);
        }
    }
    else {
        injector.AutoInjectProcess(*map, stealHandle, dllPath, process_name);
    }

    delete map;
    return EXIT_SUCCESS;
}