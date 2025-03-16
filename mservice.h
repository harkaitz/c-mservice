/* Copyright (c) 2025, Harkaitz Agirre, All rights reserved, ISC License */
/* Simple C library for writting services for Windows and POSIX. */
/* mservice.h is documented in https://github.com/harkaitz/c-mservice */
#ifndef PROGRAM_NAME
#  error Please define PROGRAM_NAME.
#endif

#include <stdio.h>
#include <stdbool.h>
#ifdef _WIN32
#  include <ws2tcpip.h>
#  include <Windows.h>
#  define PROGRAM_LOGFILE "C:/log/" PROGRAM_NAME ".log"
#  define ROOT_DIR "C:/"
#else
#  include <signal.h>
#  define PROGRAM_LOGFILE "/var/lib/log/" PROGRAM_NAME ".log"
#  define ROOT_DIR "/"
#endif

FILE       *mservice_log = NULL;
bool        mservice_shall_quit(void);
extern void mservice_main (void);

#define mservice_debug(TXT, ...) do { fprintf(mservice_log, PROGRAM_NAME ": debug: " TXT "\n", ##__VA_ARGS__); fflush(mservice_log); } while(0)
#define mservice_error(TXT, ...) do { fprintf(mservice_log, PROGRAM_NAME ": error: " TXT "\n", ##__VA_ARGS__); fflush(mservice_log); } while(0)

#ifdef MSERVICE_DISABLE

bool
mservice_shall_quit(void)
{
	return false;
}

int
main()
{
	mservice_log = stderr;
	mservice_debug("main: Launching ...");
	mservice_main();
	mservice_debug("main: Quitting ...");
}

#endif


#ifndef MSERVICE_DISABLE

#ifdef __unix__
static bool quit_flag = false;
static void quit_sig (int _sig);

int
main()
{
	mservice_log = fopen(PROGRAM_LOGFILE, "wb");
	if (!mservice_log) {
		mservice_log = stderr;
	}
	mservice_debug("main: Launching ...");
	signal(SIGINT, quit_sig);
	mservice_main();
	mservice_debug("main: Quitting ...");
}

static void
quit_sig(int _sig)
{
	quit_flag = true;
}

bool
mservice_shall_quit(void)
{
	return quit_flag;
}

#endif

#ifdef _WIN32
SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

static VOID  WINAPI ServiceMain (); /*DWORD argc, LPTSTR *argv*/
static VOID  WINAPI ServiceCtrlHandler (DWORD);
static DWORD WINAPI ServiceWorkerThread (); /* LPVOID lpParam */

bool
mservice_shall_quit(void)
{
	return (WaitForSingleObject(g_ServiceStopEvent, 0) == WAIT_OBJECT_0);
}

int
main() /* int _argc, char *_argv[] */
{
	mservice_log = fopen(PROGRAM_LOGFILE, "wb");
	if (!mservice_log) {
		mservice_log = stderr;
	}
	SERVICE_TABLE_ENTRY ServiceTable[] = {
		{PROGRAM_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
		{NULL, NULL}
	};
	mservice_debug("main: Launching ...");
	if (StartServiceCtrlDispatcher (ServiceTable) == FALSE) {
		mservice_error("Failed loading the service table.");
		return GetLastError ();
	}
	mservice_debug("main: Quitting ...");
	return 0;
}

static VOID WINAPI
ServiceMain() /* DWORD _argc, LPTSTR *_argv */
{
	int      e;
	mservice_debug("Entered the service main ...");
	g_StatusHandle = RegisterServiceCtrlHandler (PROGRAM_NAME, ServiceCtrlHandler);
	if (g_StatusHandle == NULL) {
		mservice_error("Failed to register the service control handler");
		goto EXIT;
	}

	/* Tell the service controller we are starting */
	ZeroMemory (&g_ServiceStatus, sizeof (g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	e = SetServiceStatus (g_StatusHandle, &g_ServiceStatus);
	if (e == FALSE/*err*/) { mservice_error("SetServiceStatus returned error."); }

	/* Perform tasks neccesary to start the service here */
	mservice_debug("Performing Service Start Operations");

	/* Create stop event to wait on later. */
	g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL) {
		mservice_error("CreateEvent(g_ServiceStopEvent) returned error");
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;
		e = SetServiceStatus (g_StatusHandle, &g_ServiceStatus);
		if (e == FALSE/*err*/) { mservice_error("SetServiceStatus returned error"); }
		goto EXIT; 
	}
	
	/* Tell the service controller we are started. */
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	e = SetServiceStatus (g_StatusHandle, &g_ServiceStatus);
	if (e == FALSE/*err*/) { mservice_error("SetServiceStatus returned error"); }
	
	/* Start the thread that will perform the main task of the service. */
	HANDLE hThread = CreateThread (NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	mservice_debug("Waiting for Worker Thread to complete");
	WaitForSingleObject (hThread, INFINITE);
	mservice_debug("Worker Thread Stop Event signaled");
    
	/* Perform any cleanup tasks. */
	mservice_debug("Performing Cleanup Operations");
	CloseHandle (g_ServiceStopEvent);
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;
	e = SetServiceStatus (g_StatusHandle, &g_ServiceStatus);
	if (e == FALSE/*err*/) { mservice_error("SetServiceStatus returned error"); }

	EXIT:
	mservice_debug("My Sample Service: ServiceMain: Exit");
	return;
}

static VOID WINAPI
ServiceCtrlHandler(DWORD CtrlCode)
{
	int e;
	mservice_debug("ServiceCtrlHandler: Entry");
	switch (CtrlCode) {
	case SERVICE_CONTROL_STOP :
		mservice_debug("ServiceCtrlHandler: SERVICE_CONTROL_STOP Request");
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) 
			break;
		/* Perform tasks neccesary to stop the service here */
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;
		e = SetServiceStatus (g_StatusHandle, &g_ServiceStatus);
		if (e == FALSE/*err*/) { mservice_error("ServiceCtrlHandler: SetServiceStatus returned error"); }
		/* This will signal the worker thread to start shutting down. */
		SetEvent (g_ServiceStopEvent);
		break;
	default:
		break;
	}
	mservice_debug("ServiceCtrlHandler: Exit");
}

static DWORD WINAPI
ServiceWorkerThread() /* LPVOID lpParam */
{
	if (!mservice_shall_quit()) {
		mservice_main();
	}
	return ERROR_SUCCESS;
}

#endif


#endif
