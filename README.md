# CPUZ-DSEFix
Exploiting CPU-Z Driver To Load Unsigned Drivers.
Supports Only x64 OS, Windows 7, Windows 8, Windows 8.1, Windows 10.
On Windows 10 PatchGuard Will Be Triggered And Cause BSOD. Driver May Be Loaded For a Limited Time Before BSOD.

This program patches a global variable in the system files. The global variable is pattern searched for and then the offset from ImageBase is calculated. It is later patched from ring0. On Windows 7, the global variable is located in ntoskrnl.exe (g_CiEnable). g_CiEnable is a boolean, if it is equal to 1 then DSE is ON. If it is equal to 0 then DSE if OFF.

On Windows 8 to Windows 10, the global variable is located in CI.dll (g_CiOptions). PG will notice if this variable is patched. If g_CiOptions == 6 then DSE is ON. If g_CiOptions == 0 then DSE if OFF. If g_CiOptions == 8 then DSE is on TEST_SIGN.

# Usage
Enable "Run As Administrator". Drag And Drop Driver Onto It. CPUZ-DSEFix Will Output Necessary Instructions.

![ezgif-2-cfaea52718](https://user-images.githubusercontent.com/25548756/29171024-48daeac8-7ddb-11e7-9e4f-bcc62a820d19.gif)

# Credits
MarkHC, https://github.com/MarkHC/HandleMaster
