# SendCtrlC

This is an extremely simple Windows console application for sending
a CTRL+C signal (SIGINT) to another process. This is something that is
extremely tricky to do directly with the Win32 API, but is quite simple
in a standalone helper process.

If the target process is cmd.exe, it guesses that it is a batch file
and attempts to dismiss the "Terminate batch job?" prompt. It does
this by sending 'y' to standard input, and is far from robust. In
particular, it will likely fail on non-English versions of windows.

## Usage

SendCtrlC &lt;procesid&gt;

## Error Codes

Because the app works by connecting to the external process's console,
it cannot print errors (or anything) to its own console. Errors are
therefore communicated purely through error codes. Possible values are
the follows:

&emsp;0: Success  
&emsp;1: No process id specified  
&emsp;2: Invalid process id (zero or non-numeric)  
&emsp;3: Specified process id does not exist  
&emsp;4: Specified process is not a console app  
&emsp;5: Unknown AttachConsole error  
&emsp;6: Failed to send CTRL+C signal to process  
