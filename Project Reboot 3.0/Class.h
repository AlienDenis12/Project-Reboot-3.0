#pragma once

#include "Object.h"

#include "addresses.h"
#include "UnrealString.h"
#include "Map.h"

struct UField : UObject
{
	UField* Next;
	// void* pad; void* pad2;
};

struct UFieldPadding : UObject
{
	UField* Next;
	void* pad; void* pad2;
};

template <typename PropertyType = void>
static inline PropertyType* GetNext(void* Field)
{
	return *(PropertyType**)(__int64(Field) + Offsets::Next);
}

static inline FName* GetFNameOfProp(void* Property)
{
	FName* NamePrivate = (FName*)(__int64(Property) + Offsets::PropName);
	return NamePrivate;
}

class UStruct : public UField
{
public:
	int GetPropertiesSize();

	UStruct* GetSuperStruct() const { return *(UStruct**)(__int64(this) + Offsets::SuperStruct); } // idk if this is in UStruct

	TArray<uint8_t> GetScript() { return *(TArray<uint8_t>*)(__int64(this) + Offsets::Script); }
};

class UClass : public UStruct
{
public:
	UObject* CreateDefaultObject();
};

class UFunction : public UStruct
{
public:
	void*& GetFunc() { return *(void**)(__int64(this) + Offsets::Func); }
};

class UEnum : public UField
{
public:
	int64 GetValue(const std::string& EnumMemberName)
	{
		auto Names = (TArray<TPair<FName, int64>>*)(__int64(this) + sizeof(UField) + sizeof(FString));

		for (int i = 0; i < Names->Num(); ++i)
		{
			auto& Pair = Names->At(i);
			auto& Name = Pair.Key();
			auto Value = Pair.Value();

			if (Name.ComparisonIndex.Value)
			{
				auto nameStr = Name.ToString();

				if (nameStr.contains(EnumMemberName))
					return Value;
			}
		}

		return -1;
	}
};