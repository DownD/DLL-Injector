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
    std::optional<std::string> autoInjectProcess;
    std::optional<std::string> autoInjectWindow;
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
    cli.add_argument("-m", "--mapping").default_value("STANDART").help("Set's the type of mapping, this field can take the following values: STANDART,MANUAL");
    cli.add_argument("-x", "--delete_headers").default_value(0).implicit_value((int)blackbone::eLoadFlags::WipeHeader).help("Delete PE headers after injection (Only for manual mapping)");
    cli.add_argument("-i", "--manual_import_headers").default_value(0).implicit_value((int)blackbone::eLoadFlags::ManualImports).help("Manually map import libraries (Only for manual mapping)");
    cli.add_argument("-t", "--no_threads").default_value(0).implicit_value((int)blackbone::eLoadFlags::NoThreads).help("Use thread hijack and do not create new threads (Only for manual mapping)");
    cli.add_argument("--pid").default_value(0).scan<'i', int>().help("The process id where the dll should be injected");
    cli.add_argument("--process_path").help("The injector will also execute the process and inject on it");
    cli.add_argument("--steal_handle_job").default_value(false).implicit_value(true).help("Whether or not to steal the process handle, it uses a special technique to retrive the process handle even before the own process knows it's PID, it can only be used in conjunction with --process_path and should be combined with manualmap.");
    cli.add_argument("--auto_inject_process").help("Use auto inject, will keep running the injector in the background and inject into processes with the process name specified");
    cli.add_argument("--auto_inject_window").help("Use auto inject and only injects in new windows, will keep running the injector in the background and inject into new windows with the specified name");

    ArgsCtx args;
    try {
		cli.parse_args(argc, argv);
		args.dllPath = cli.get<std::string>("dll_path");
		args.mapping = cli.get<std::string>("mapping") == "STANDART" ? MAPPING_TYPE::LOADLIBRARY : MAPPING_TYPE::MANUAL_MAP;
		args.stealHandleJob = cli.get<bool>("--steal_handle_job");
		args.processPath = cli.present("--process_path");
		args.autoInjectProcess = cli.present("--auto_inject_process");
		args.autoInjectWindow = cli.present("--auto_inject_window");
        args.flags = (blackbone::eLoadFlags)(cli.get<int>("--delete_headers") | cli.get<int>("--manual_import_headers") | cli.get<int>("--no_threads"));

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
    DEBUG_LOG("Parsing arguments..\n");
    ArgsCtx args = parseArgs(argc, argv);


    DEBUG_LOG("Injecting %s", args.dllPath.c_str());

    CInjector injector;
    std::unique_ptr<CDllMap> map;

    bool injectionResult = false;

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

    if (args.autoInjectWindow) {
        injectionResult = injector.AutoInjectFromWindow(*map, args.dllPath, args.autoInjectWindow.value());
    }else if (args.autoInjectProcess) {
        injectionResult = injector.AutoInjectProcess(*map, args.stealHandleJob, args.dllPath, args.autoInjectProcess.value());
	}else if (args.processPath) {
        injectionResult = injector.StartProcessAndInject(*map, args.stealHandleJob, args.processPath.value(), args.dllPath);
    }else if (args.pid) {
        injectionResult = injector.InjectFromPID(*map, args.pid.value(), args.dllPath);
    }
    else {
        ERROR_LOG("No injection method specified\n");
        return EXIT_FAILURE;
    }

    if (injectionResult) {
		printf("Injection successful\n");
		return EXIT_SUCCESS;
    }
    else {
        ERROR_LOG("Injection failed\n");
    }
    return EXIT_FAILURE;
}