#pragma once



class cpuz
{
public:
	cpuz();
	~cpuz();

	BOOLEAN ReadSystemAddress(PVOID Address, PVOID Buffer, SIZE_T Length);
	template <typename T, typename U>
	T ReadSystemAddress(U Address)
	{
		T Buff{ 0 };
		if (!ReadSystemAddress((PVOID)Address, &Buff, sizeof(T)))
			throw std::runtime_error("Read Of Physical Address Failed.");
		return Buff;
	}

	BOOLEAN WriteSystemAddress(PVOID Address, PVOID Buffer, SIZE_T Length);
	template <typename T, typename U>
	BOOLEAN WriteSystemAddress(U Address, T Value)
	{
		return WriteSystemAddress((PVOID)Address, &Value, sizeof(T));
	}
	
private:
	BOOLEAN ReadPhysicalAddress(uint64_t Address, PVOID Buffer, SIZE_T Length);
	template <typename T, typename U>
	T ReadPhysicalAddress(U Address)
	{
		T Buff{ 0 };
		if (!ReadPhysicalAddress((uint64_t)Address, &Buff, sizeof(T)))
			throw std::runtime_error("Read Of Physical Address Failed.");
		return Buff;
	}

	BOOLEAN WritePhysicalAddress(uint64_t Address, PVOID Buffer, SIZE_T Length);

	uint64_t TranslateVirtualAddress(uint64_t directoryTableBase, PVOID virtualAddress);
	

private:
	BOOLEAN ReadCR3();

	HANDLE hDevice = INVALID_HANDLE_VALUE;
	uint64_t ControlReg3 = 0;
};

