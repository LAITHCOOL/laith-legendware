#pragma once
class weapon_info_t;

class IWeaponSystem
{
public:
    virtual ~IWeaponSystem() = 0;
    virtual void pad04() = 0;
    virtual weapon_info_t* GetWpnData(int weaponId) = 0;
};