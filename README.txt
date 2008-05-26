Openwide is written in C using the Win32 API, so no Microsoft class
libraries.  I use C because there are some good free C compiler/IDEs and I
haven't found the C++ ones as easy to use.

To compile it, you'll probably want to use the PellesC free IDE.
Some fiddling with the project files (.PPJ) may be required.  These are
essentially Makefiles, in any case.

I used the Win32 API SetWindowsHookEx to set up a WH_CBT hook which watches for
opening windows.  When a window opens I check the title and window styles in
order to determine whether it is an Open/Save dialog box.  If it is (or rather,
if my program THINKS it is), then I subclass the window procedure.

In my subclassed window proc. I set a timer for a short duration, then pass
control back to the original dialog proc. This allows the dialog to initialize
itself, as I discovered that setting things immediately did not work.  I then
send the dialog appropriate messages to set up things the way we want them.

This is the briefest possible introduction to the workings of the program!
