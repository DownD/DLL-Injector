# DLL-Injector
ONLY FOR LEARNING PURPOSES<br>
This solution contains 2 different projects (InjectorGUI <Down'sInjector> and InjectorCLI) based on [BlackBone](https://github.com/DarthTon/Blackbone) API by DarthTon.<br>
Injector CLI implements more complex methods.

## GUI-Injector
![img](https://i.gyazo.com/568386b94fc2526e9446ca0a99371b12.png)

After pressing inject, it will wait until the user select another window, and will inject instantly.<br>
For now only manual-mapping is available for both x86 and x86_64.

## CLI-Injector
![img](https://i.gyazo.com/2f8ae4c0b83155f92095794326cdd48f.png)
This CLI binary provides multiple options for injecting, it also can execute in the background and inject the dll into multiple processes that can be selected by window name or process name.

This support multiple types of DLL injections:
- Manual-Mapping / LoadLibraryA Injection
- Auto-execution - launches the target process
- Auto-injection - keeps the injector in the background and injects the dll on every window or process that matches the option provided
- ASAP Handles - a technique mentioned [here](https://www.unknowncheats.me/forum/anti-cheat-bypass/236135-asap-handles-v2-getting-handle-process-getting-stripped.html) that allows for the injector to get an handle before every process one the system, bypassing some protections, this methos should only be used with manual mapping injection because the system might not have time to load Kernel32.dll that is required to load LoadLibraryA.
```
Usage: InjectorCLI [OPTIONS] <PATH_TO_DLL>...
  -m, -M, --mapping=STANDART Set's the type of mapping, this field can take the following values: STANDART,MANUAL
  -i, -I, --pid        The process id where the dll should be injected
  -p, -P, --process_path The injector will also execute the process and inject on it

  --steal_handle_job   Whether or not to steal the process handle, it uses a special technique to retrive the process handle even
  before the own process knows it's PID, it can only be used in conjunction with --process_path and should be combined with manualmap.

  -a, --auto_inject=   Use auto inject, will keep running the injector in the background and inject into processes with the process name specified

  -a, -W, --auto_inject_window= Use auto inject and only injects in new windows, will keep running the injector in the background and inject into new windows with the specified name

  -h, --help           For auto injection the user must provide -auto_inject with a process path
For normal injection either the option pid or process_path must be provided
If pid is provided, will inject into the respective pid, else it will execute the process
steal_handle_job option uses a different technique to inject.

```
### Examples

- This will inject dummy.dll into every process_name.exe that is executed after<br><br>
``` InjectorCLI.exe -m STANDART --auto_inject "process_name.exe" "C:\path_to_dll\dummy.dll" ```

- This will inject dummy.dll (Only once per process) into every window with the title as "process_name_window" that is executed after<br>
``` InjectorCLI.exe -m STANDART --auto_inject_window "process_name_window" "C:\path_to_dll\dummy.dll" ```

- This will launch the process process.exe and inject dummy.dll using manual map with the ASAP handle method<br>
``` InjectorCLI.exe -m MANUAL -s -process_path "C:\path_to_dll\process.exe" "C:\path_to_dll\dummy.dll" ```


## Compilation
Make sure to install all dependencies required by [BlackBone](https://github.com/DarthTon/Blackbone), and also these ones:
- Net Framework 4.7.1 Software Development Kit

Due to the github having a limit of 25mb per file you need to compile the [BlackBone](https://github.com/DarthTon/Blackbone) library and paste it into /External/lib/<architeture>/<configration>/.<br>
Example: Release 64 bits -> /External/lib/x64/Release/BlackBone.lib

## Notes
On future implementations the both projects could be merge in order to have a user interface for a more powerfull injector
