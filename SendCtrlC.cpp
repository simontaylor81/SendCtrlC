// SendCtrlC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// Helper class for making sure a HANDLE is closed properly.
class ScopedHandle
{
public:
	// Construct from a HANDLE.
	ScopedHandle(HANDLE inHandle)
		: handle(inHandle)
	{}

	// Destructor: close the handle when going out of scope, if valid.
	~ScopedHandle()
	{
		if (IsValid())
		{
			CloseHandle(handle);
		}
	}

	// Do we wrap a valid handle.
	bool IsValid() const
	{
		return handle != NULL;
	}

	// Implicit conversion to HANDLE to allow passing direct to Win32 functions.
	operator HANDLE()
	{
		return handle;
	}

private:
	HANDLE handle;

	// Non-copyable.
	ScopedHandle(const ScopedHandle&);
	ScopedHandle& operator=(const ScopedHandle&);
};

bool IsCmdExe(DWORD procId);
void TerminateBatchFile();

// Return codes:
//   0: Success
//   1: No process id specified
//   2: Invalid process id (zero or non-numeric)
//   3: Specified process id does not exist
//   4: Specified process is not a console app
//   5: Unknown AttachConsole error
//   6: Failed to send CTRL+C signal to process
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc <= 1)
	{
		// Need process id.
		return 1;
	}

	DWORD procId = _ttol(argv[1]);
	if (procId == 0)
	{
		// Invalid id
		return 2;
	}

	// Ignore the CTRL+C event we send to ourselves.
	SetConsoleCtrlHandler(NULL, TRUE);

	// Detach from current console (can only be attached to one at a time).
	// Ignore error -- it just means we weren't already attached.
	FreeConsole();

	// Attach to console of given proc id
	if (!AttachConsole(procId))
	{
		auto error = GetLastError();
		if (error == ERROR_GEN_FAILURE)
		{
			// Process does not exist
			return 3;
		}
		else if (error == ERROR_INVALID_HANDLE)
		{
			// Process does not have a console.
			return 4;
		}
		return 5;
	}

	// Send CTRL+C to target process (and outselves).
	if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0))
	{
		return 6;
	}

	// If the process is cmd.exe (and is therefore probably a batch file)
	// attempt to dismiss the "Terminate batch job?" prompt.
	if (IsCmdExe(procId))
	{
		TerminateBatchFile();
	}

	return 0;
}

// Attempt to dismiss a "Terminate batch job?" prompt.
void TerminateBatchFile()
{
	INPUT_RECORD input[2];
	ZeroMemory(input, sizeof(input));

	// Y key.
	input[0].EventType = KEY_EVENT;
	input[0].Event.KeyEvent.bKeyDown = TRUE;
	input[0].Event.KeyEvent.uChar.UnicodeChar = L'y';
	input[0].Event.KeyEvent.wRepeatCount = 1;

	// Enter
	input[1].EventType = KEY_EVENT;
	input[1].Event.KeyEvent.bKeyDown = TRUE;
	input[1].Event.KeyEvent.uChar.UnicodeChar = VK_RETURN;
	input[1].Event.KeyEvent.wRepeatCount = 1;

	// Slight hack: wait a bit for the prompt to appear.
	Sleep(1);

	DWORD numWritten;
	auto stdInHandle = GetStdHandle(STD_INPUT_HANDLE);
	WriteConsoleInput(stdInHandle, input, 2, &numWritten);
}

// Is the process with the given ID an instance of cmd.exe (and is therefore probably a batch file)?
bool IsCmdExe(DWORD procId)
{
	// Open the process.
	ScopedHandle process(OpenProcess(
		PROCESS_QUERY_INFORMATION,
		FALSE,	// bInheritHandle
		procId));
	if (!process.IsValid())
	{
		return false;
	}

	TCHAR filename[MAX_PATH];
	DWORD size = MAX_PATH;
	if (!QueryFullProcessImageName(process, 0, filename, &size))
	{
		return false;
	}

	// Convert to lowercase.
	_tcslwr_s(filename);

	// Does the filename include "cmd.exe"?
	return _tcsstr(filename, TEXT("cmd.exe")) != NULL;
}
