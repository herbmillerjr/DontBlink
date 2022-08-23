#include <obs-module.h>
#include <obs-source.h>
#include <obs-frontend-api.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <optional>

OBS_DECLARE_MODULE()

HWINEVENTHOOK windowEvents;
HHOOK shellEvents;

const char *SUBSYSTEM_NAME="[Don't Blink]";

void Log(const std::optional<std::string> &message)
{
	if (message) blog(LOG_INFO,"%s%s%s",SUBSYSTEM_NAME," ",message->data());
}

void Warn(const std::string &message)
{
	blog(LOG_WARNING,"%s%s%s",SUBSYSTEM_NAME," ",message.data());
}

std::optional<std::string> GetWindowTitle(HWND window)
{
	int titleLength=GetWindowTextLength(window)+1;
	if (titleLength < 1) return std::nullopt;
	std::vector<char> title(titleLength);
	if (!GetWindowText(window,title.data(),titleLength)) // FIXME: don't assum LPSTR here
	{
		DWORD errorCode=GetLastError();
		if (errorCode != 127) Log("Failed to obtain window title text (" + std::to_string(GetLastError()) + ")");
		// for some reason, EnumWindows results in a lot of windows with 1 character long titles that
		// I can't call GetWindowText on, so just filter them out
		return std::nullopt;
	}

	return title.data();
}

VOID CALLBACK ForegroundWindowChanged(HWINEVENTHOOK windowEvents,DWORD event,HWND window,LONG objectID,LONG childID,DWORD eventThread,DWORD timestamp)
{
	Log(GetWindowTitle(window));
}

LRESULT CALLBACK ShellEvent(int code,WPARAM wParam,LPARAM lParam)
{
	if (code < 0) return CallNextHookEx(0,code,wParam,lParam);

	switch (code)
	{
	case HSHELL_WINDOWCREATED:
		Log("Window Created");
		break;
	case HSHELL_WINDOWDESTROYED:
		Log("Window Destroyed");
		break;
	default:
		break;
	}

	return 0;
}

BOOL CALLBACK WindowAvailable(HWND window,LPARAM lParam)
{
	if (GetWindowLong(window,GWL_STYLE) & WS_CHILD) return TRUE;
	Log(GetWindowTitle(window));
	return TRUE;
}

void UpdateAvailbleWindows()
{
	EnumWindows(WindowAvailable,NULL);
}

bool obs_module_load()
{
	//obs_frontend_add_event_callback(HandleEvent,nullptr);

	UpdateAvailbleWindows();

	windowEvents=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,ForegroundWindowChanged,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
	if (!windowEvents)
		Log("Failed to subscribe to system window events");
	else
		Log("Subscribed to system window events");

	shellEvents=SetWindowsHookEx(WH_SHELL,ShellEvent,0,0);
	if (!shellEvents)
		Log("Failed to subscribe to system shell events (" + std::to_string(GetLastError()) + ")");
	else
		Log("Subscribed to system shell events");

	return true;
}

void obs_module_unload()
{
	if (UnhookWinEvent(windowEvents))
		Log("Unsubscribed from system window events");
	else
		Log("Failed to unsubscribe from system window events");

	if (UnhookWindowsHookEx(shellEvents))
		Log("Unsubscribed from system shell events");
	else
		Log("Failed to unsubscribe from system window events");

	//obs_frontend_remove_event_callback(HandleEvent,nullptr);
}