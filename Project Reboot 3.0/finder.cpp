#include "finder.h"

#include "reboot.h"
#include "FortPlayerControllerAthena.h"

uint64 FindStartAircraftPhase()
{
	if (Engine_Version < 427) // they scuf it
	{
		auto strRef = Memcury::Scanner::FindStringRef(L"STARTAIRCRAFT").Get();

		if (!strRef)
			return 0;

		int NumCalls = 0;

		for (int i = 0; i < 150; i++)
		{
			if (*(uint8_t*)(strRef + i) == 0xE8)
			{
				LOG_INFO(LogDev, "Found call 0x{:x}", __int64(strRef + i) - __int64(GetModuleHandleW(0)));
				NumCalls++;

				if (NumCalls == 2) // First is the str compare ig
				{
					return Memcury::Scanner(strRef + i).RelativeOffset(1).Get();
				}
			}
		}
	}
	else
	{
		auto StatAddress = Memcury::Scanner::FindStringRef(L"STAT_StartAircraftPhase").Get();

		for (int i = 0; i < 1000; i++)
		{
			if (*(uint8_t*)(uint8_t*)(StatAddress - i) == 0x48 && *(uint8_t*)(uint8_t*)(StatAddress - i + 1) == 0x8B && *(uint8_t*)(uint8_t*)(StatAddress - i + 2) == 0xC4)
			{
				return StatAddress - i;
			}
		}
	}

	return 0;
}

uint64 FindGIsClient()
{
	// if (Fortnite_Version >= 19) return 0;

	auto Addr = Memcury::Scanner::FindStringRef(L"AllowCommandletRendering", false);

	if (!Addr.Get()) // pretty sure only 22+ since the string is split (we could maybe try just searching without the A?)
	{
		// Looking for commandlet class

		if (Fortnite_Version == 22.3)
		{
			// return __int64(GetModuleHandleW(0)) + 0xDCE9DFA;
		}

		if (Fortnite_Version == 23.5)
		{
			// return __int64(GetModuleHandleW(0)) + 0xEBD8A4C;
		}

		LOG_ERROR(LogDev, "[FindGIsClient] Failed to find AllowCommandletRendering! Returning 0");
		return 0;
	}

	std::vector<std::vector<uint8_t>> BytesArray = {
		{0x88, 0x05}, // 20.40 21.00
		{0xC6, 0x05}, // mov cs X // Checked on 1.11, 12.41, 15.10
		{0x88, 0x1D}, // mov cs bl // Checked on 17.50, 19.10
		{0x44, 0x88} // 4.5
	};

	int Skip = 2;

	uint64 Addy = 0;
	int PickedByte = 0;

	for (int i = 0; i < 50; i++) // we should subtract from skip if go up
	{
		auto CurrentByte = *(Memcury::ASM::MNEMONIC*)(Addr.Get() - i);

		// if (bPrint)
			// std::cout << "CurrentByte: " << std::hex << (int)CurrentByte << '\n';

		bool ShouldBreak = false;

		// LOG_INFO(LogDev, "[{}] Byte: 0x{:x}", i, (int)CurrentByte);

		for (auto& Bytes : BytesArray)
		{
			if (CurrentByte == Bytes[0])
			{
				bool Found = true;
				for (int j = 1; j < Bytes.size(); j++)
				{
					if (*(Memcury::ASM::MNEMONIC*)(Addr.Get() - i + j) != Bytes[j])
					{
						Found = false;
						break;
					}
				}
				if (Found)
				{
					bool bIsScuffedByte = Bytes[0] == 0x44;
					int Relative = bIsScuffedByte ? 3 : 2;
					auto current = Memcury::Scanner(Addr.Get() - i);
					// LOG_INFO(LogDev, "[{}] No Rel 0x{:x} Rel: 0x{:x}", Skip, current.Get() - __int64(GetModuleHandleW(0)), Memcury::Scanner(Addr.Get() - i).RelativeOffset(Relative).Get() - __int64(GetModuleHandleW(0)));

					if (bIsScuffedByte)
					{
						if (*(Memcury::ASM::MNEMONIC*)(Addr.Get() - i + 2) == 0x74) // DIE 4.5 (todo check length of entire instruction)
						{
							LOG_INFO(LogDev, "Found broken byte, skipping!");
							continue;
						}
					}

					if (!PickedByte)
					{
						PickedByte = Bytes[0];
					}
					else if (PickedByte != Bytes[0])
						continue; // Its one of the bytes, but not the first one we found.

					if (Skip > 0)
					{
						Skip--;
						continue;
					}

					LOG_INFO(LogDev, "Found GIsClient with byte 0x{:x}", Bytes[0]);

					Addy = Bytes[0] == 0xC6 
						? current.RelativeOffset(Relative, 1).Get() // If mov cs then we add 1 because the last byte is the value and makes whole instructions 1 byte longer
						: current.RelativeOffset(Relative).Get();
					ShouldBreak = true;
					break;
				}
			}
		}

		if (ShouldBreak)
			break;

		// std::cout << std::format("CurrentByte: 0x{:x}\n", (uint8_t)CurrentByte);
	}

	// LOG_INFO(LogDev, "Addy: 0x{:x}", Addy - __int64(GetModuleHandleW(0)));

	return Addy; // 0; // Memcury::Scanner(Addy3).RelativeOffset(2).Get();

	/*
	auto Addr = Memcury::Scanner::FindStringRef(L"AllowCommandletRendering");
	int Skip = 1;
	auto Addy = FindBytes(Addr, { 0xC6, 0x05 }, 50, 0, true, Skip);
	Addy = Addy ? Addy : FindBytes(Addr, { 0x44, 0x88 }, 50, 0, true, Skip);
	Addy = Addy ? Addy : FindBytes(Addr, { 0x88, 0x1D }, 50, 0, true, Skip);

	return Memcury::Scanner(Addy).RelativeOffset(2).Get();
	*/
}

uint64 FindGetSessionInterface()
{
	auto strRef = Memcury::Scanner::FindStringRef(L"OnDestroyReservedSessionComplete %s bSuccess: %d", true, 0, Fortnite_Version >= 19).Get();

	LOG_INFO(LogDev, "strRef: 0x{:x}", strRef - __int64(GetModuleHandleW(0)));

	int NumCalls = 0;
	NumCalls -= Fortnite_Version >= 19;

	for (int i = 0; i < 2000; i++)
	{
		if (*(uint8_t*)(strRef + i) == 0xE8)
		{
			LOG_INFO(LogDev, "Found call 0x{:x}", __int64(strRef + i) - __int64(GetModuleHandleW(0)));
			NumCalls++;

			if (NumCalls == 2) // First is a FMemory::Free
			{
				return Memcury::Scanner(strRef + i).RelativeOffset(1).Get();
			}
		}
	}

	return 0;
}

uint64 FindGetPlayerViewpoint()
{
	// We find FailedToSpawnPawn and then go back on VFT by 1.

	uint64 FailedToSpawnPawnAddr = 0;

	auto FailedToSpawnPawnStrRefAddr = Memcury::Scanner::FindStringRef(L"%s failed to spawn a pawn", true, 0, Fortnite_Version >= 19 && Fortnite_Version < 24).Get();

	if (!FailedToSpawnPawnStrRefAddr)
	{
		LOG_ERROR(LogFinder, "Failed to find FailedToSpawnPawnStrRefAddr! Report to Milxnor immediately.");
		return 0;
	}

	for (int i = 0; i < 1000; i++)
	{
		if (*(uint8_t*)(uint8_t*)(FailedToSpawnPawnStrRefAddr - i) == 0x40 && *(uint8_t*)(uint8_t*)(FailedToSpawnPawnStrRefAddr - i + 1) == 0x53)
		{
			FailedToSpawnPawnAddr = FailedToSpawnPawnStrRefAddr - i;
			break;
		}

		if (*(uint8_t*)(uint8_t*)(FailedToSpawnPawnStrRefAddr - i) == 0x48 && *(uint8_t*)(uint8_t*)(FailedToSpawnPawnStrRefAddr - i + 1) == 0x89 && *(uint8_t*)(uint8_t*)(FailedToSpawnPawnStrRefAddr - i + 2) == 0x5C)
		{
			FailedToSpawnPawnAddr = FailedToSpawnPawnStrRefAddr - i;
			break;
		}
	}

	if (!FailedToSpawnPawnAddr)
	{
		LOG_ERROR(LogFinder, "Failed to find FailedToSpawnPawn! Report to Milxnor immediately.");
		return 0;
	}

	static auto FortPlayerControllerAthenaDefault = FindObject<AFortPlayerControllerAthena>(L"/Script/FortniteGame.Default__FortPlayerControllerAthena"); // FindObject<UClass>(L"/Game/Athena/Athena_PlayerController.Default__Athena_PlayerController_C");
	void** const PlayerControllerVFT = FortPlayerControllerAthenaDefault->VFTable;

	int FailedToSpawnPawnIdx = 0;

	for (int i = 0; i < 500; i++)
	{
		if (PlayerControllerVFT[i] == (void*)FailedToSpawnPawnAddr)
		{
			FailedToSpawnPawnIdx = i;
			break;
		}
	}

	if (FailedToSpawnPawnIdx == 0)
	{
		LOG_ERROR(LogFinder, "Failed to find FailedToSpawnPawn in virtual function table! Report to Milxnor immediately.");
		return 0;
	}

	return __int64(PlayerControllerVFT[FailedToSpawnPawnIdx - 1]);
}

uint64 ApplyGameSessionPatch()
{
	auto GamePhaseStepStringAddr = Memcury::Scanner::FindStringRef(L"Gamephase Step: %s", false).Get();

	uint64 BeginningOfGamePhaseStepFn = 0;
	uint8_t* ByteToPatch = 0;

	if (!GamePhaseStepStringAddr)
	{
		LOG_WARN(LogFinder, "Unable to find GamePhaseStepString!");
		// return 0;

		BeginningOfGamePhaseStepFn = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 E8 ? ? ? ? 48 8B D8 48 85 C0 0F 84 ? ? ? ? E8").Get(); // not actually the func but its fine

		if (!BeginningOfGamePhaseStepFn)
		{
			LOG_WARN(LogFinder, "Unable to find fallback sig for gamephase step! Report to Milxnor immediately.");
			return 0;
		}
	}

	if (!BeginningOfGamePhaseStepFn && !ByteToPatch)
	{
		for (int i = 0; i < 3000; i++)
		{
			if (*(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i) == 0x40 && *(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i + 1) == 0x55)
			{
				BeginningOfGamePhaseStepFn = GamePhaseStepStringAddr - i;
				break;
			}

			if (*(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i) == 0x48 && *(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i + 1) == 0x89 && *(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i + 2) == 0x5C)
			{
				BeginningOfGamePhaseStepFn = GamePhaseStepStringAddr - i;
				break;
			}

			if (*(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i) == 0x48 && *(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i + 1) == 0x8B && *(uint8_t*)(uint8_t*)(GamePhaseStepStringAddr - i + 2) == 0xC4)
			{
				BeginningOfGamePhaseStepFn = GamePhaseStepStringAddr - i;
				break;
			}
		}
	}
	 
	if (!BeginningOfGamePhaseStepFn && !ByteToPatch)
	{
		LOG_WARN(LogFinder, "Unable to find beginning of GamePhaseStep! Report to Milxnor immediately.");
		return 0;
	}

	if (!ByteToPatch)
	{
		for (int i = 0; i < 500; i++)
		{
			if (*(uint8_t*)(uint8_t*)(BeginningOfGamePhaseStepFn + i) == 0x0F && *(uint8_t*)(uint8_t*)(BeginningOfGamePhaseStepFn + i + 1) == 0x84)
			{
				ByteToPatch = (uint8_t*)(uint8_t*)(BeginningOfGamePhaseStepFn + i + 1);
				break;
			}
		}
	}

	if (!ByteToPatch)
	{
		LOG_WARN(LogFinder, "Unable to find byte to patch for GamePhaseStep!");
		return 0;
	}

	LOG_INFO(LogDev, "[ApplyGameSessionPatch] ByteToPatch: 0x{:x}", __int64(ByteToPatch) - __int64(GetModuleHandleW(0)));

	DWORD dwProtection;
	VirtualProtect((PVOID)ByteToPatch, 1, PAGE_EXECUTE_READWRITE, &dwProtection);

	*ByteToPatch = 0x85; // jz -> jnz

	DWORD dwTemp;
	VirtualProtect((PVOID)ByteToPatch, 1, dwProtection, &dwTemp);

	return 0;
}