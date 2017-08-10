#pragma once
class cpuz_driver
{
public:
	cpuz_driver();
	~cpuz_driver();

private:
	HANDLE hFile = INVALID_HANDLE_VALUE;
};

extern unsigned char CpuzShellcode[46400];