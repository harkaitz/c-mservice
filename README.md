# C-MSERVICE

Simple C library for writting services for Windows and POSIX.

## How to

Instead of defining `main()` define `mservice_main` in your main.c or
main.cpp file.

    #include PROGRAM_NAME "my_service"
    #include "mservice.h"
    
    void mservice_main(void) {
        fprintf(mservice_log, "The service started.\n");
        while (!mservice_shall_quit()) {
            ...
        }
    }
    
Service installation (Windows):

    $ sc create "My Service" binPath= C:\opt\bin\my_service.exe
    $ sc start "My service"
    $ sc stop "My service"
    $ sc delete "My service"

Service installation (POSIX):

    Write the systemd/runit/... file yourself.

## Important notes.

1. The log goes to "C:/log/PROGRAM_NAME.log" in windows if "C:/log" exists.
2. The log goes to "/var/lib/log/PROGRAM_NAME.log" in POSIX systems.
3. You can use `mservice_debug(FMT,...)` and `mservice_error(FMT,...)` wrappers for
   logging or write to `FILE *mservice_log`.
4. You can disable all service parafernalia and make mservice_log stderr by
   defining `MSERVICE_DISABLE`, do this when debugging.

## Style guide

This project follows the OpenBSD kernel source file style guide (KNF).

Read the guide [here](https://man.openbsd.org/style).

## Collaborating

For making bug reports, feature requests, support or consulting visit
one of the following links:

1. [gemini://harkadev.com/oss/](gemini://harkadev.com/oss/)
2. [https://harkadev.com/oss/](https://harkadev.com/oss/)
