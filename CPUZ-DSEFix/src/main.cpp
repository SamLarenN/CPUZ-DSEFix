#include "Global.h"



int main(int argc, char* argv[])
{
	try
	{
		OSVERSIONINFO osver;
		osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		std::string ServiceRegKey;
		auto& utils = Utils::instance();

		if (!utils.EnablePrivilege("SeLoadDriverPrivilege"))		// Enable 'SeLoadDriverPrivilege' For This Process To Load Drivers
			throw std::runtime_error("Could Not Set 'SeLoadDriverPrivilege'.");

		cpuz_driver* CPUZ = new cpuz_driver();		// Initiate cpuz141.sys Driver For Reading And Writing System Memory
		
		if (!utils.RegisterService(argv[1], &ServiceRegKey))		// Register Desired Driver
			throw std::runtime_error("Could Not Register Service.");

						
		GetVersionExA(&osver);			// Get BuildNumber For The OS
		
		sys* System = new sys(osver.dwBuildNumber);			// Initiate The System Global Var

		if (!System->LoadDriverRoutine(ServiceRegKey))			// Call The LoadDriverRoutine To Load The Desired Driver
			throw std::runtime_error("Driver Load and Unload Routine Failed.\n");
		
		System->~sys();		// EXIT
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl << "Error Code: 0x" << std::hex << GetLastError() << std::endl;
	}
	
	std::cout << "[+] Press 'ENTER' To Exit.\n";
	std::cin.get();
	return 0;
}