#include "CInjector.h"
#include <stdbool.h>
#include <stdlib.h>
#include <optional>
#include <argparse/argparse.hpp>

/**
* This is a custom project configuration structure where you can store the
* parsed information.
*/

struct ArgsCtx {
    bool autoInject;
    bool autoInjectWindow;
    bool stealHandleJob;
    blackbone::eLoadFlags flags;
    std::optional<int> pid;
    std::optional<std::string> processPath;
    MAPPING_TYPE mapping;
    std::string dllPath;

    std::optional<std::string_view> getProcessName() {
        if (processPath.has_value()) {
			return getFileNameFromPath(processPath.value());
		}
        else {
			return std::nullopt;
		}
    }
};

ArgsCtx parseArgs(int argc, char* argv[]) {
    argparse::ArgumentParser cli("InjectorCLI");

    cli.add_argument("dll_path").help("The dll file to inject");
    cli.add_argument("-m", "--mapping").default_value("STANDART").required().help("Set's the type of mapping, this field can take the following values: STANDART,MANUAL");
    cli.add_argument("-x", "--delete_headers").default_value(0).implicit_value((int)blackbone::eLoadFlags::WipeHeader).help("Delete PE headers after injection (Only for manual mapping)");
    cli.add_argument("-i", "--manual_import_headers").default_value(0).implicit_value((int)blackbone::eLoadFlags::ManualImports).help("Manually map import libraries (Only for manual mapping)");
    cli.add_argument("-t", "--no_threads").default_value(0).implicit_value((int)blackbone::eLoadFlags::NoThreads).help("Use thread hijack and do not create new threads (Only for manual mapping)");
    cli.add_argument("--pid").default_value(0).scan<'i', int>().help("The process id where the dll should be injected");
    cli.add_argument("--process_path").help("The injector will also execute the process and inject on it");
    cli.add_argument("--steal_handle_job").default_value(false).implicit_value(true).help("Whether or not to steal the process handle, it uses a special technique to retrive the process handle even before the own process knows it's PID, it can only be used in conjunction with --process_path and should be combined with manualmap.");
    cli.add_argument("--auto_inject").default_value(false).implicit_value(true).help("Use auto inject, will keep running the injector in the background and inject into processes with the process name specified");
    cli.add_argument("--auto_inject_window").default_value(false).implicit_value(true).help("Use auto inject and only injects in new windows, will keep running the injector in the background and inject into new windows with the specified name");

    ArgsCtx args;
    try {
		cli.parse_args(argc, argv);
		args.dllPath = cli.get<std::string>("dll_path");
		args.mapping = cli.get<std::string>("mapping") == "STANDART" ? MAPPING_TYPE::LOADLIBRARY : MAPPING_TYPE::MANUAL_MAP;
		args.processPath = cli.present("--process_path");
		args.stealHandleJob = cli.get<bool>("--steal_handle_job");
		args.autoInject = cli.get<bool>("--auto_inject");
		args.autoInjectWindow = cli.get<bool>("--auto_inject_window");
        args.flags = (blackbone::eLoadFlags)(cli.get<int>("--delete_headers") | cli.get<int>("--manual_import_headers") | cli.get<int>("--no_threads"));

        printf("Flags: %d\n", args.flags);
        if (cli.get<int>("--pid") != 0) {
            args.pid = cli.get<int>("--pid");
        }
        else {
            args.pid = std::nullopt;
        }
	}
    catch (const std::exception& err) {
		std::cout << err.what() << std::endl;
		std::cout << cli;
		exit(0);
	}
    return args;

}


int main(int argc, char* argv[])
{
    ArgsCtx args = parseArgs(argc, argv);
    std::optional<std::string_view> processName = args.getProcessName();


    DEBUG_LOG("Injecting %s", args.dllPath.c_str());

    CInjector injector;
    std::unique_ptr<CDllMap> map;


    //Set mapping type
    if (args.mapping == MAPPING_TYPE::MANUAL_MAP) {
        map = std::make_unique<CManualMap>(args.flags);
    }
    else if (args.mapping == MAPPING_TYPE::LOADLIBRARY) {
        map = std::make_unique<CLoadLibrary>(false);
    }
    else {
        ERROR_LOG("Uknown mapping type\n");
        return EXIT_SUCCESS;
    }

    if (args.autoInjectWindow && processName) {
        if (!injector.AutoInjectFromWindow(*map,args.dllPath,std::string(processName.value()))){
            printf("Error Window on injection\n");
        }
        return 0;
    }

    if (!args.autoInject) {
        //Single normal injection
        if (processName) {
            DEBUG_LOG("Starting injection");
            if (!injector.StartProcessAndInject(*map, args.stealHandleJob, args.processPath.value(), args.dllPath)) {
                printf("Error on injection\n");
            }
        }
        else {
            if (!args.pid) {
                printf("Either a PID or a file path needs to be provided in order to use injection\n");
                return EXIT_SUCCESS;
            }
            injector.InjectFromPID(*map, args.pid.value(), args.dllPath);
        }
    }
    else if(processName) {
        injector.AutoInjectProcess(*map, args.stealHandleJob, args.dllPath, std::string(processName.value()));
    }
    else {
        ERROR_LOG("Either a PID or a file path needs to be provided in order to use injection");
    }
    DEBUG_LOG("Exiting");

    return EXIT_SUCCESS;
}