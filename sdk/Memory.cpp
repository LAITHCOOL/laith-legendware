#include "Memory.h"
#include "..\utils\crypt_str.h"

template <typename T>
static constexpr auto relativeToAbsolute(int* address) noexcept
{
    return reinterpret_cast<T>(reinterpret_cast<char*>(address + 1) + *address);
}

#define FIND_PATTERN(type, ...) \
reinterpret_cast<type>(findPattern(__VA_ARGS__))

void Memory::initialize() noexcept
{
    auto temp = FIND_PATTERN(std::uintptr_t*, crypt_str("client"), crypt_str("\xB9????\xE8????\x8B\x5D\x08"), 1);
    hud = *temp;
    findHudElement = relativeToAbsolute<decltype(findHudElement)>(reinterpret_cast<int*>(reinterpret_cast<char*>(temp) + 5));
    clearHudWeapon = FIND_PATTERN(decltype(clearHudWeapon), crypt_str("client.dll"), crypt_str("55 8B EC 51 53 56 8B 75 08 8B D9 57 6B FE 34") + 1);
    itemSchema = relativeToAbsolute<decltype(itemSchema)>(FIND_PATTERN(int*, crypt_str("client"), crypt_str("\xE8????\x0F\xB7\x0F"), 1));
    equipWearable = FIND_PATTERN(decltype(equipWearable), crypt_str("client"), crypt_str("\x55\x8B\xEC\x83\xEC\x10\x53\x8B\x5D\x08\x57\x8B\xF9"));
}