#pragma once
//fixed by shonax
#include "..\includes.hpp"
#include "interfaces\IClientEntity.hpp"
#include "misc\EHandle.hpp"
#include "misc\UtlVector.hpp"
#include "math\QAngle.hpp"
#include "..\utils\netmanager.hpp"
#include "misc\CBoneAccessor.hpp"
#include "..\cheats\misc\fakelag.h"
#include "..\sdk\misc\Recv.hpp"

inline uint32_t FindInDataMap(datamap_t* pDataMap, const std::string& szPropName)
{
    while (pDataMap)
    {
        for (int i = 0; i < pDataMap->dataNumFields; i++)
        {
            if (!pDataMap->dataDesc[i].fieldName)
                continue;

            if (pDataMap->dataDesc[i].fieldName == szPropName)
                return pDataMap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL];

            if (pDataMap->dataDesc[i].fieldType != FIELD_EMBEDDED)
                continue;

            if (!pDataMap->dataDesc[i].td)
                continue;

            uint32_t iOffset = FindInDataMap(pDataMap->dataDesc[i].td, szPropName);
            if (!iOffset)
                continue;

            return iOffset;
        }

        pDataMap = pDataMap->baseMap;
    }

    return 0;
}
#define DATAMAP( type, name ) \
__forceinline type& name( ) \
{\
	static uint32_t g_Datamap_##name = 0;\
    if ( !g_Datamap_##name )\
        g_Datamap_##name = FindInDataMap( this->GetPredDescMap( ), #name );\
    return *( type* )( ( uintptr_t )( this ) + g_Datamap_##name );\
}\

#define get_netvar(type, name, table, netvar, offset)                           \
    type& name() const {                                          \
        static int _##name = netvars::get_offset(table, netvar) + offset;     \
        return *(type*)((DWORD)this + _##name);                 \
    }

#define get_p_netvar(type, name, table, netvar, offset)                           \
    type* name() const {                                          \
        static int _##name = netvars::get_offset(table, netvar) + offset;     \
        return (type*)((DWORD)this + _##name);                 \
    }\

#define CUSTOM_OFFSET( name, type, prop, offset ) \
__forceinline type& name( ) \
{\
    return *( type* )( ( uintptr_t )( this ) + offset ); \
}\



#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define TIME_TO_TICKS(t) ((int)(0.5f + (float)(t) / m_globals()->m_intervalpertick))
#define TICKS_TO_TIME(t) (m_globals()->m_intervalpertick * (t))

#define NETVAR(type, name, table, netvar)                           \
    type& name##() const {                                          \
        static int _##name = netvars::get().get_offset(table, netvar);     \
        return *(type*)((uintptr_t)this + _##name);                 \
        }

#define PNETVAR(type, name, table, netvar)                           \
    type* name##() const {                                          \
        static int _##name = netvars::get().get_offset(table, netvar);     \
        return (type*)((uintptr_t)this + _##name);                 \
        }

#define OFFSET(type, name, offset)\
type &name##() const\
{\
        return *(type*)(uintptr_t(this) + offset);\
}

class player_t;
struct datamap_t;

enum CSWeaponType
{
    WEAPONTYPE_KNIFE = 0,
    WEAPONTYPE_PISTOL,
    WEAPONTYPE_SUBMACHINEGUN,
    WEAPONTYPE_RIFLE,
    WEAPONTYPE_SHOTGUN,
    WEAPONTYPE_SNIPER_RIFLE,
    WEAPONTYPE_MACHINEGUN,
    WEAPONTYPE_C4,
    WEAPONTYPE_PLACEHOLDER,
    WEAPONTYPE_GRENADE,
    WEAPONTYPE_UNKNOWN
};

class VarMapEntry_t
{
public:
    unsigned short type;
    unsigned short m_bNeedsToInterpolate;

    void* data;
    void* watcher;
};

struct VarMapping_t
{
    VarMapping_t()
    {
        m_Entries = nullptr;
        m_nInterpolatedEntries = 0;
        m_lastInterpolationTime = 0.0f;
    }

    VarMapEntry_t* m_Entries;
    int m_nInterpolatedEntries;
    float m_lastInterpolationTime;
};

struct client_hit_verify_t
{
    Vector position;
    float time;
    float expires;
};

class AnimationLayer;
class c_baseplayeranimationstate;
class C_CSGOPlayerAnimationState;
class entity_t;
class clientanimating_t;

class weapon_info_t
{
public:
    char pad_0000[4];
    char* ConsoleName;
    char pad_0008[12];
    int iMaxClip1;
    char pad_0018[12];
    int iMaxClip2;
    char pad_0028[4];
    char* szWorldModel;
    char* szViewModel;
    char* szDropedModel;
    char pad_0038[4];
    char* N00000984;
    char pad_0040[56];
    char* szEmptySound;
    char pad_007C[4];
    char* szBulletType;
    char pad_0084[4];
    char* szHudName;
    char* szWeaponName;
    char pad_0090[60];
    int WeaponType;
    int iWeaponPrice;
    int iKillAward;
    char* szAnimationPrefex;
    float flCycleTime;
    float flCycleTimeAlt;
    float flTimeToIdle;
    float flIdleInterval;
    bool bFullAuto;
    char pad_00ED[3];
    int iDamage;
    float headshotmultyplrier;
    float flArmorRatio;
    int iBullets;
    float flPenetration;
    float flFlinchVelocityModifierLarge;
    float flFlinchVelocityModifierSmall;
    float flRange;
    float flRangeModifier;
    char pad_0110[28];
    int iCrosshairMinDistance;
    float flMaxPlayerSpeed;
    float flMaxPlayerSpeedAlt;
    char pad_0138[4];
    float flSpread;
    float flSpreadAlt;
    float flInaccuracyCrouch;
    float flInaccuracyCrouchAlt;
    float flInaccuracyStand;
    float flInaccuracyStandAlt;
    float flInaccuracyJumpIntial;
    float flInaccaurcyJumpApex;
    float flInaccuracyJump;
    float flInaccuracyJumpAlt;
    float flInaccuracyLand;
    float flInaccuracyLandAlt;
    float flInaccuracyLadder;
    float flInaccuracyLadderAlt;
    float flInaccuracyFire;
    float flInaccuracyFireAlt;
    float flInaccuracyMove;
    float flInaccuracyMoveAlt;
    float flInaccuracyReload;
    int iRecoilSeed;
    float flRecoilAngle;
    float flRecoilAngleAlt;
    float flRecoilVariance;
    float flRecoilAngleVarianceAlt;
    float flRecoilMagnitude;
    float flRecoilMagnitudeAlt;
    float flRecoilMagnatiudeVeriance;
    float flRecoilMagnatiudeVerianceAlt;
    float flRecoveryTimeCrouch;
    float flRecoveryTimeStand;
    float flRecoveryTimeCrouchFinal;
    float flRecoveryTimeStandFinal;
    int iRecoveryTransititionStartBullet;
    int iRecoveryTransititionEndBullet;
    bool bUnzoomAfterShot;
    char pad_01C1[31];
    char* szWeaponClass;
    char pad_01E4[56];
    float flInaccuracyPitchShift;
    float flInaccuracySoundThreshold;
    float flBotAudibleRange;
    char pad_0228[12];
    bool bHasBurstMode;
};

struct NoticeText_t
{
    wchar_t text[512];
    int unk0; // 0x400
    float unk1; // 0x404
    float unk2; // 0x408
    int unk3;   // 0x40C
    float time; // 0x410
    int unk4;       // 0x414
    float fade; // 0x418
    int unk5;   // 0x41C
};

struct KillFeed_t
{
    char pad[0x7C];
    CUtlVector <NoticeText_t> notices;
};

class CBoneCache {
public:
    matrix3x4_t* m_pCachedBones;
    char pad[0x8];
    int        m_CachedBoneCount;
};

class entity_t : public IClientEntity
{
public:
    NETVAR(int, body, crypt_str("CBaseAnimating"), crypt_str("m_nBody"));
    NETVAR(int, m_nModelIndex, crypt_str("CBaseEntity"), crypt_str("m_nModelIndex"));
    NETVAR(int, m_iTeamNum, crypt_str("CBaseEntity"), crypt_str("m_iTeamNum"));
    NETVAR(Vector, m_vecOrigin, crypt_str("CBaseEntity"), crypt_str("m_vecOrigin"));
    NETVAR(CHandle <player_t>, m_hOwnerEntity, crypt_str("CBaseEntity"), crypt_str("m_hOwnerEntity"));
    NETVAR(int, m_CollisionGroup, crypt_str("CBaseEntity"), crypt_str("m_CollisionGroup"));
    NETVAR(int, m_nSequence, crypt_str("CBaseAnimating"), crypt_str("m_nSequence"));

    void set_m_bUseCustomBloomScale(byte value)
    {
        *reinterpret_cast<byte*>(uintptr_t(this) + (int)netvars::get().get_offset(crypt_str("CEnvTonemapController"), crypt_str("m_bUseCustomBloomScale"))) = value;
    }

    void set_m_flCustomBloomScale(float value)
    {
        *reinterpret_cast<float*>(uintptr_t(this) + (int)netvars::get().get_offset(crypt_str("CEnvTonemapController"), crypt_str("m_flCustomBloomScale"))) = value;
    }

    void set_m_bUseCustomAutoExposureMin(byte value)
    {
        *reinterpret_cast<byte*>(uintptr_t(this) + (int)netvars::get().get_offset(crypt_str("CEnvTonemapController"), crypt_str("m_bUseCustomAutoExposureMin"))) = value;
    }

    void set_m_flCustomAutoExposureMin(float value)
    {
        *reinterpret_cast<float*>(uintptr_t(this) + (int)netvars::get().get_offset(crypt_str("CEnvTonemapController"), crypt_str("m_flCustomAutoExposureMin"))) = value;
    }

    void set_m_bUseCustomAutoExposureMax(byte value)
    {
        *reinterpret_cast<byte*>(uintptr_t(this) + (int)netvars::get().get_offset(crypt_str("CEnvTonemapController"), crypt_str("m_bUseCustomAutoExposureMax"))) = value;
    }

    void set_m_flCustomAutoExposureMax(float value)
    {
        *reinterpret_cast<float*>(uintptr_t(this) + (int)netvars::get().get_offset(crypt_str("CEnvTonemapController"), crypt_str("m_flCustomAutoExposureMax"))) = value;
    }

    const matrix3x4_t& m_rgflCoordinateFrame()
    {
        static auto _m_rgflCoordinateFrame = netvars::get().get_offset(crypt_str("CBaseEntity"), crypt_str("m_CollisionGroup")) - 0x30;
        return *(matrix3x4_t*)((uintptr_t)this + _m_rgflCoordinateFrame);
    }
    int GetPropInt(std::string& table, std::string& var)
    {
        static auto offset = netvars::get().get_offset(table.c_str(), var.c_str());
        int val = *(int*)(uintptr_t(this) + (int)offset);
        return val;
    }
    float GetPropFloat(std::string& table, std::string& var)
    {
        static auto offset = netvars::get().get_offset(table.c_str(), var.c_str());
        float val = *(float*)(uintptr_t(this) + (int)offset);
        return val;
    }
    bool GetPropBool(std::string& table, std::string& var)
    {
        static auto offset = netvars::get().get_offset(table.c_str(), var.c_str());
        bool val = *(bool*)(uintptr_t(this) + (int)offset);
        return val;
    }
    std::string GetPropString(std::string& table, std::string& var)
    {
        static auto offset = netvars::get().get_offset(table.c_str(), var.c_str());
        char* val = (char*)(uintptr_t(this) + (int)offset);
        return std::string(val);
    }

    void SetPropInt(std::string& table, std::string& var, int val)
    {
        *reinterpret_cast<int*>(uintptr_t(this) + (int)netvars::get().get_offset(table.c_str(), var.c_str())) = val;
    }
    void SetPropFloat(std::string& table, std::string& var, float val)
    {
        *reinterpret_cast<float*>(uintptr_t(this) + (int)netvars::get().get_offset(table.c_str(), var.c_str())) = val;
    }
    void SetPropBool(std::string& table, std::string& var, bool val)
    {
        *reinterpret_cast<float*>(uintptr_t(this) + (int)netvars::get().get_offset(table.c_str(), var.c_str())) = val;
    }
    datamap_t* GetPredDescMap();
    std::array <float, 24>& m_flPoseParameter();
    bool is_player();
    void set_model_index(int index);
    void set_abs_angles(const Vector& angle);
    void set_abs_origin(const Vector& origin);
};

class projectile_t : public entity_t
{
public:
    NETVAR(Vector, m_vInitialVelocity, crypt_str("CBaseCSGrenadeProjectile"), crypt_str("m_vInitialVelocity"));
    NETVAR(int, m_flAnimTime, crypt_str("CBaseCSGrenadeProjectile"), crypt_str("m_flAnimTime"));
    NETVAR(int, m_nExplodeEffectTickBegin, crypt_str("CBaseCSGrenadeProjectile"), crypt_str("m_nExplodeEffectTickBegin"));
    NETVAR(int, m_nBody, crypt_str("CBaseCSGrenadeProjectile"), crypt_str("m_nBody"));
    NETVAR(int, m_nForceBone, crypt_str("CBaseCSGrenadeProjectile"), crypt_str("m_nForceBone"));
    NETVAR(Vector, m_vecVelocity, crypt_str("CBaseGrenade"), crypt_str("m_vecVelocity"));
    NETVAR(CHandle<player_t>, m_hThrower, "CBaseGrenade", "m_hThrower");
    NETVAR(Vector, m_vecOrigin, crypt_str("CBaseCSGrenadeProjectile"), crypt_str("m_vecOrigin"));
    OFFSET(float, m_flSpawnTime, netvars::get().get_offset(crypt_str("CBaseCSGrenadeProjectile"), crypt_str("m_vecExplodeEffectOrigin")) + 0xC);
};

class attributableitem_t : public entity_t
{
public:

    NETVAR(int, m_iItemDefinitionIndex, crypt_str("CBaseAttributableItem"), crypt_str("m_iItemDefinitionIndex"));
    NETVAR(int, m_nFallbackStatTrak, crypt_str("CBaseAttributableItem"), crypt_str("m_nFallbackStatTrak"));
    NETVAR(int, m_nFallbackPaintKit, crypt_str("CBaseAttributableItem"), crypt_str("m_nFallbackPaintKit"));
    NETVAR(int, m_nFallbackSeed, crypt_str("CBaseAttributableItem"), crypt_str("m_nFallbackSeed"));
    NETVAR(float, m_flFallbackWear, crypt_str("CBaseAttributableItem"), crypt_str("m_flFallbackWear"));
    NETVAR(int, m_iAccountID, crypt_str("CBaseAttributableItem"), crypt_str("m_iAccountID"));
    NETVAR(int, m_iItemIDHigh, crypt_str("CBaseAttributableItem"), crypt_str("m_iItemIDHigh"));
    PNETVAR(char, m_szCustomName, crypt_str("CBaseAttributableItem"), crypt_str("m_szCustomName"));
    NETVAR(int, m_OriginalOwnerXuidLow, crypt_str("CBaseAttributableItem"), crypt_str("m_OriginalOwnerXuidLow"));
    NETVAR(int, m_OriginalOwnerXuidHigh, crypt_str("CBaseAttributableItem"), crypt_str("m_OriginalOwnerXuidHigh"));
    NETVAR(int, m_iEntityQuality, crypt_str("CBaseAttributableItem"), crypt_str("m_iEntityQuality"));
};

class weapon_t : public attributableitem_t
{
public:
    NETVAR(float, m_flNextPrimaryAttack, crypt_str("CBaseCombatWeapon"), crypt_str("m_flNextPrimaryAttack"));
    NETVAR(float, m_flNextSecondaryAttack, crypt_str("CBaseCombatWeapon"), crypt_str("m_flNextSecondaryAttack"));
    NETVAR(bool, initialized, crypt_str("CBaseAttributableItem"), crypt_str("m_bInitialized"));
    NETVAR(int, weapon, crypt_str("CBaseViewModel"), crypt_str("m_hWeapon"));
    NETVAR(short, m_iItemDefinitionIndex, crypt_str("CBaseCombatWeapon"), crypt_str("m_iItemDefinitionIndex"));
    NETVAR(int, m_iClip1, crypt_str("CBaseCombatWeapon"), crypt_str("m_iClip1"));
    NETVAR(int, m_iViewModelIndex, crypt_str("CBaseCombatWeapon"), crypt_str("m_iViewModelIndex"));
    NETVAR(int, m_iWorldModelIndex, crypt_str("CBaseCombatWeapon"), crypt_str("m_iWorldModelIndex"));
    NETVAR(float, m_fAccuracyPenalty, crypt_str("CWeaponCSBase"), crypt_str("m_fAccuracyPenalty"));
    NETVAR(int, m_zoomLevel, crypt_str("CWeaponCSBaseGun"), crypt_str("m_zoomLevel"));
    NETVAR(bool, m_bPinPulled, crypt_str("CBaseCSGrenade"), crypt_str("m_bPinPulled"));
    NETVAR(float, m_fThrowTime, crypt_str("CBaseCSGrenade"), crypt_str("m_fThrowTime"));
    NETVAR(float, m_flPostponeFireReadyTime, crypt_str("CWeaponCSBase"), crypt_str("m_flPostponeFireReadyTime"));
    NETVAR(float, m_fLastShotTime, crypt_str("CWeaponCSBase"), crypt_str("m_fLastShotTime"));
    NETVAR(float, m_flRecoilSeed, crypt_str("CWeaponCSBase"), crypt_str("m_flRecoilIndex"));
    NETVAR(int, m_weaponMode, crypt_str("CWeaponCSBase"), crypt_str("m_weaponMode"));
    NETVAR(CHandle <weapon_t>, m_hWeaponWorldModel, crypt_str("CBaseCombatWeapon"), crypt_str("m_hWeaponWorldModel"));

    __forceinline bool IsZoomable(bool extra_check = true) {
        return m_iItemDefinitionIndex() == WEAPON_SSG08
            || m_iItemDefinitionIndex() == WEAPON_SCAR20
            || m_iItemDefinitionIndex() == WEAPON_AWP
            || m_iItemDefinitionIndex() == WEAPON_G3SG1
            || (extra_check && (m_iItemDefinitionIndex() == WEAPON_SG556 || m_iItemDefinitionIndex() == WEAPON_AUG));
    }

    weapon_info_t* get_csweapon_info();
    bool is_empty();
    bool can_fire(bool check_revolver);
    int get_weapon_group(bool rage);
    bool IsGun();
    bool is_rifle();
    bool is_smg();
    bool is_shotgun();
    bool is_pistol();
    bool is_sniper();
    bool is_grenade();
    bool is_knife();
    bool is_non_aim();
    bool can_double_tap();
    int get_max_tickbase_shift();
    float get_inaccuracy();
    float get_spread();
    void update_accuracy_penality();
    char* get_icon();
    std::string get_name();
};

class viewmodel_t;

class C_BaseViewModel : public entity_t
{
public:
    CBaseHandle get_viewmodel_weapon()
    {
        static auto offset = netvars::get().get_offset(crypt_str("CBaseViewModel"), crypt_str("m_hWeapon"));
        return *(CBaseHandle*)(uintptr_t(this) + offset);
    }

    int m_nViewModelIndex()
    {
        static auto offset = netvars::get().get_offset(crypt_str("CBaseViewModel"), crypt_str("m_nViewModelIndex"));
        return *(int*)(uintptr_t(this) + offset);
    }

    float& m_flCycle()
    {
        //static auto m_flCycle = utils::find_in_data_map(GetPredDescMap(), "m_flCycle");
        static auto offset = netvars::get().get_offset(crypt_str("CCSPlayer"), crypt_str("m_flCycle"));
        return *(float*)(uintptr_t(this) + offset);
    }

    float& m_flModelAnimTime()
    {
        //static auto offset = utils::find_in_data_map(GetPredDescMap(), "m_flAnimTime");
        static auto offset = netvars::get().get_offset(crypt_str("CCSPlayer"), crypt_str("m_flAnimTime"));
        return *(float*)(uintptr_t(this) + offset);
    }

    int& m_nAnimationParity()
    {
        static auto offset = netvars::get().get_offset(crypt_str("CBaseViewModel"), crypt_str("m_nAnimationParity"));
        return *(int*)(uintptr_t(this) + offset);
    }

    CBaseHandle& m_hNetworkMoveParent()
    {
        static auto offset = netvars::get().get_offset(crypt_str("CBasePlayer"), crypt_str("m_hOwnerEntity"));
        return *(CBaseHandle*)(uintptr_t(this) + offset - sizeof(CBaseHandle));
    }

    int& m_nSequence()
    {
        //static auto m_nSequence = utils::find_in_data_map(GetPredDescMap(), "m_nSequence");
        static auto m_nSequence = netvars::get().get_offset(crypt_str("CPredictedViewModel"), crypt_str("m_nSequence"));
        return *(int*)(uintptr_t(this) + m_nSequence);
    }

    void send_matching_seq(int seq)
    {
        return call_virtual<void(__thiscall*)(C_BaseViewModel*, int)>(this, 245)(this, seq);
    }
};

class player_t : public entity_t
{
public:
    NETVAR(int, m_flMaxSpeed, crypt_str("CBasePlayer"), crypt_str("m_flMaxSpeed"));
    NETVAR(bool, m_bClientSideAnimation, crypt_str("CBaseAnimating"), crypt_str("m_bClientSideAnimation"));
    NETVAR(bool, m_bHasDefuser, crypt_str("CCSPlayer"), crypt_str("m_bHasDefuser"));
    NETVAR(bool, m_bGunGameImmunity, crypt_str("CCSPlayer"), crypt_str("m_bGunGameImmunity"));
    NETVAR(int, m_iShotsFired, crypt_str("CCSPlayer"), crypt_str("m_iShotsFired"));
    NETVAR(Vector, m_angEyeAngles, crypt_str("CCSPlayer"), crypt_str("m_angEyeAngles[0]"));
    NETVAR(QAngle, m_angEyeAngles1, crypt_str("CCSPlayer"), crypt_str("m_angEyeAngles[0]"));

    NETVAR(Vector, m_angRotation, crypt_str("CBaseEntity"), crypt_str("m_angRotation"));
    NETVAR(int, m_ArmorValue, crypt_str("CCSPlayer"), crypt_str("m_ArmorValue"));
    NETVAR(int, m_iAccount, crypt_str("CCSPlayer"), crypt_str("m_iAccount"));
    NETVAR(bool, m_bHasHelmet, crypt_str("CCSPlayer"), crypt_str("m_bHasHelmet"));
    NETVAR(bool, m_bHasHeavyArmor, crypt_str("CCSPlayer"), crypt_str("m_bHasHeavyArmor"));
    NETVAR(bool, m_bIsScoped, crypt_str("CCSPlayer"), crypt_str("m_bIsScoped"));
    NETVAR(float, m_flLowerBodyYawTarget, crypt_str("CCSPlayer"), crypt_str("m_flLowerBodyYawTarget"));
    NETVAR(float, m_flFlashDuration, crypt_str("CCSPlayer"), crypt_str("m_flFlashDuration"));
    NETVAR(CBaseHandle, m_hVehicle, crypt_str("CBasePlayer"), crypt_str("m_hVehicle"));
    NETVAR(int, m_iHealth, crypt_str("CBasePlayer"), crypt_str("m_iHealth"));
    NETVAR(int, m_lifeState, crypt_str("CBasePlayer"), crypt_str("m_lifeState"));
    NETVAR(int, m_fFlags, crypt_str("CBasePlayer"), crypt_str("m_fFlags"));
    NETVAR(int, m_nHitboxSet, crypt_str("CBasePlayer"), crypt_str("m_nHitboxSet"));
    NETVAR(int, m_nTickBase, crypt_str("CBasePlayer"), crypt_str("m_nTickBase"));
    NETVAR(Vector, m_vecViewOffset, crypt_str("CBasePlayer"), crypt_str("m_vecViewOffset[0]"));
    NETVAR(Vector, m_viewPunchAngle, crypt_str("CBasePlayer"), crypt_str("m_viewPunchAngle"));
    NETVAR(QAngle, m_viewPunchAngle1, crypt_str("CBasePlayer"), crypt_str("m_viewPunchAngle"));//
    NETVAR(Vector, m_aimPunchAngle, crypt_str("CBasePlayer"), crypt_str("m_aimPunchAngle"));
    NETVAR(QAngle, m_aimPunchAngle1, crypt_str("CBasePlayer"), crypt_str("m_aimPunchAngle"));//
    NETVAR(Vector, m_aimPunchAngleVel, crypt_str("CBasePlayer"), crypt_str("m_aimPunchAngleVel"));
    NETVAR(CHandle <viewmodel_t>, m_hViewModel, crypt_str("CBasePlayer"), crypt_str("m_hViewModel[0]"));
    NETVAR(Vector, m_vecVelocity, crypt_str("CBasePlayer"), crypt_str("m_vecVelocity[0]"));
    NETVAR(Vector, m_vecMins, crypt_str("CBaseEntity"), crypt_str("m_vecMins"));
    NETVAR(Vector, m_vecMaxs, crypt_str("CBaseEntity"), crypt_str("m_vecMaxs"));
    NETVAR(float, m_flVelocityModifier, crypt_str("CCSPlayer"), crypt_str("m_flVelocityModifier"));
    NETVAR(float, m_flSimulationTime, crypt_str("CBaseEntity"), crypt_str("m_flSimulationTime"));
    OFFSET(float, m_flOldSimulationTime, netvars::get().get_offset(crypt_str("CBaseEntity"), crypt_str("m_flSimulationTime")) + 0x4);
    NETVAR(float, m_flDuckSpeed, crypt_str("CCSPlayer"), crypt_str("m_flDuckSpeed"));
    NETVAR(float, m_flDuckAmount, crypt_str("CCSPlayer"), crypt_str("m_flDuckAmount"));
    NETVAR(bool, m_bDucked, crypt_str("CCSPlayer"), crypt_str("m_bDucked"));
    NETVAR(bool, m_bDucking, crypt_str("CCSPlayer"), crypt_str("m_bDucking"));
    NETVAR(float, m_flHealthShotBoostExpirationTime, crypt_str("CCSPlayer"), crypt_str("m_flHealthShotBoostExpirationTime"));
    NETVAR(int, m_bInBombZone, crypt_str("CCSPlayer"), crypt_str("m_bInBombZone"));
    NETVAR(float, m_flFallVelocity, crypt_str("CBasePlayer"), crypt_str("m_flFallVelocity"));
    NETVAR(float, m_flStepSize, crypt_str("CBaseEntity"), crypt_str("m_flStepSize"));
    NETVAR(float, m_flNextAttack, crypt_str("CBaseCombatCharacter"), crypt_str("m_flNextAttack"));
    PNETVAR(CBaseHandle, m_hMyWearables, crypt_str("CBaseCombatCharacter"), crypt_str("m_hMyWearables"));
    NETVAR(int, m_iObserverMode, crypt_str("CBasePlayer"), crypt_str("m_iObserverMode"));
    NETVAR(CHandle <player_t>, m_hObserverTarget, crypt_str("CBasePlayer"), crypt_str("m_hObserverTarget"));
    NETVAR(CHandle <weapon_t>, m_hActiveWeapon, crypt_str("CBaseCombatCharacter"), crypt_str("m_hActiveWeapon"));
    NETVAR(CHandle <attributableitem_t>, m_hWeaponWorldModel, crypt_str("CBaseCombatWeapon"), crypt_str("m_hWeaponWorldModel"));
    NETVAR(CHandle <entity_t>, m_hGroundEntity, crypt_str("CBasePlayer"), crypt_str("m_hGroundEntity"));
    NETVAR(bool, m_bSpotted, crypt_str("CBaseEntity"), crypt_str("m_bSpotted"));
    NETVAR(int, m_vphysicsCollisionState, crypt_str("CBasePlayer"), crypt_str("m_vphysicsCollisionState"));
    NETVAR(bool, m_bIsWalking, crypt_str("CCSPlayer"), crypt_str("m_bIsWalking"));
    NETVAR(bool, m_bIsDefusing, crypt_str("CCSPlayer"), crypt_str("m_bIsDefusing"));

    NETVAR(float, m_flCycle, crypt_str("CBaseAnimating"), crypt_str("m_flCycle"));
    NETVAR(float, get_fall_velocity, crypt_str("CBasePlayer"), crypt_str("m_flFallVelocity"))
        NETVAR(int, get_sequence, crypt_str("CBaseAnimating"), crypt_str("m_nSequence"))


        inline bool IsArmored(const int iHitGroup)
    {
        // @ida isarmored: server.dll @ 55 8B EC 32 D2

        bool bIsArmored = false;

        if (this->m_ArmorValue() > 0)
        {
            switch (iHitGroup)
            {
            case HITGROUP_GENERIC:
            case HITGROUP_CHEST:
            case HITGROUP_STOMACH:
            case HITGROUP_LEFTARM:
            case HITGROUP_RIGHTARM:
            case HITGROUP_NECK:
                bIsArmored = true;
                break;
            case HITGROUP_HEAD:
                if (this->m_bHasHelmet())
                    bIsArmored = true;
                [[fallthrough]];
            case HITGROUP_LEFTLEG:
            case HITGROUP_RIGHTLEG:
                if (this->m_bHasHeavyArmor())
                    bIsArmored = true;
                break;
            default:
                break;
            }
        }

        return bIsArmored;
    }


     //

    //CUSTOM_OFFSET(m_CachedBoneData, CUtlVector < matrix3x4_t >, FNV32("CachedBoneData"), 0x2914);
    CUSTOM_OFFSET(GetBoneAccessor, CBoneAccessor, FNV32("BoneAccessor"), 0x26A8);
    CUSTOM_OFFSET(m_angVisualAngles, QAngle, FNV32("VisualAngles"), 0x31E8);
    //CUSTOM_OFFSET(m_flSpawnTime, float_t, FNV32("SpawnTime"), 0x103C0);
    CUSTOM_OFFSET(GetMoveType, int32_t, FNV32("MoveType"), 0x25C);
    CUSTOM_OFFSET(m_nClientEffects, int32_t, FNV32("ClientEffects"), 0x68);
    CUSTOM_OFFSET(m_nLastSkipFramecount, int32_t, FNV32("LastSkipFramecount"), 0xA68);
    CUSTOM_OFFSET(m_nOcclusionFrame, int32_t, FNV32("OcclusionFrame"), 2608);
    CUSTOM_OFFSET(m_nOcclusionMask, int32_t, FNV32("OcclusionMask"), 2600);
    CUSTOM_OFFSET(m_pInverseKinematics, LPVOID, FNV32("InverseKinematics"), 9840);
    CUSTOM_OFFSET(m_bJiggleBones, bool, FNV32("JiggleBones"), 0x2930);
    CUSTOM_OFFSET(m_bMaintainSequenceTransition, bool, FNV32("MaintainSequenceTransition"), 0x9F0);

    inline std::array < float, 24 >& GetBaseAnimating()
    {
        return *(std::array < float, 24 >*)((DWORD)(this) + 0x2778);
    }


    DATAMAP(Vector, m_vecBaseVelocity);

    //
        NETVAR(int, get_next_think_tick, crypt_str("CBasePlayer"), crypt_str("m_nNextThinkTick"));
    NETVAR(float, m_flThirdpersonRecoil, crypt_str("CCSPlayer"), crypt_str("m_flThirdpersonRecoil"));

    VIRTUAL(think(void), 138, void(__thiscall*)(void*));
    VIRTUAL(pre_think(void), 317, void(__thiscall*)(void*));

    Vector GetOrigin()
    {
        if (!this)
            return Vector(0, 0, 0);
        return *(Vector*)((DWORD)this + 0x138);
    }

    // helper methods.
    template< typename t >
    inline t& get(size_t offset) { return *reinterpret_cast<t*>(reinterpret_cast<std::size_t>(this) + offset); }
    enum indices : size_t {
        GETREFEHANDLE = 2,
        GETABSORIGIN = 10,
        GETABSANGLES = 11,
        TESTHITBOXES = 52,
        SET_MODEL_INDEX = 75,
        GETATTACHMENT = 84,
        THINK = 139, // from that place and more +1 to idx for last upd
        EYEANGLESPOSITION = 169,
        ISFOLLOWINGENTITY = 178,
        BUILDTRANSFORMATIONS = 190,
        UPDATE_IK_LOCKS = 192,
        CALCULATE_IK_LOCKS = 193,
        DOEXTRABONEPROCESSING = 198,
        STANDARDBLENDINGRULES = 206,
        ACCUMULATELAYERS = 209,
        SETSEQUENCY = 219,
        STUDIOFRAMEADVANCE = 220,
        GETLAYERSEQUENCYCICLERATE = 223,
        UPDATECLIENTSIDEANIMATION = 224, // 55 8B EC 51 56 8B F1 80 BE ? ? ? ? ? 74 36
        UPDATE_DISPATCH_LAYER = 247,
        GETACTIVEWEAPON = 268,
        WEAPONSHOOTPOSITION = 285,
        CALCVIEW = 277,
        PRETHINK = 318,
        POSTTHINK = 319,
        RUNPOSTTHINK = 320,
        GETFOV = 327,
        SELECT_ITEM = 330,
        UPDATECOLLISIONBOUNDS = 340, // 56 57 8B F9 8B 0D ? ? ? ? F6 87 ? ? ? ? ?
        GETAIMPUNCHANG = 346,
        GETEYEPOSITION = 348,
        GETAIMPUNCH = 409
    };


    __forceinline void RunPostThink() {
        return call_virtual< void(__thiscall*)(decltype(this)) >(this, RUNPOSTTHINK)(this);
    }
    __forceinline void PreThink() {
        return call_virtual< void(__thiscall*)(decltype(this)) >(this, PRETHINK)(this);
    }

    __forceinline void Think() {
        return call_virtual< void(__thiscall*)(decltype(this)) >(this, THINK)(this);
    }


    int& m_nNextThinkTick() {
        static auto next_think = netvars::get().get_offset(crypt_str("CCSPlayer"), crypt_str("m_nNextThinkTick"));
        return get <int>(next_think);
    }

    int& m_nFinalPredictedTick() {
        static int fpt = (netvars::get().get_offset(crypt_str("CCSPlayer"), crypt_str("m_nTickBase")) + 0x4);
        return get< int >(fpt);
    }
    CUserCmd*& m_PlayerCommand() {
        return *(CUserCmd**)(std::uintptr_t(this) + 0x3288);
    }

    VIRTUAL(set_local_view_angles(Vector& angle), 372, void(__thiscall*)(void*, Vector&), angle);


    //void SetLocalVAngles(QAngle& angle);

    void SetLocalVAngles(Vector& angle);

    bool player_t::IsDead()
    {
        if (this == nullptr || *(void**)this == nullptr)
            return true;

        return (m_lifeState());
    }

    uint32_t& get_buttons_disabled()
    {
        return *(uint32_t*)((uintptr_t)this + 0x3330);
    }
    uint32_t& get_buttons_forced()
    {
        return *(uint32_t*)((uintptr_t)this + 0x3334);
    }

    CBaseHandle* m_hMyWeapons()
    {
        return (CBaseHandle*)((uintptr_t)this + 0x2DF8);
    }

    __forceinline Vector GetBonePos(matrix3x4_t* mat, int i)
    {
        if (!mat)
            return Vector(0, 0, 0);

        return Vector(mat[i][0][3], mat[i][1][3], mat[i][2][3]);
    }

    CBoneCache* m_BoneCache() // (если фикс костей на этом индексе у вас)
    {
        return (CBoneCache*)((uintptr_t)this + 0x2914);
    }

    float& m_flSpawnTime()
    {
        return *(float*)((uintptr_t)this + 0xA360);
    }

    CUserCmd& m_lastcmd()
    {
        return *(CUserCmd*)((uintptr_t)this + 0x3288);
    }

    uint32_t& m_iMostRecentModelBoneCounter();
    float& m_flLastBoneSetupTime();

    void select_item(const char* string, int sub_type);
    bool using_standard_weapons_in_vechile();
    CUserCmd*& current_command();
    bool physics_run_think(int index);

    VarMapping_t* var_mapping();
    c_baseplayeranimationstate* get_animation_state();
    C_CSGOPlayerAnimationState* get_animation_state1();
    CStudioHdr* m_pStudioHdr();
    CStudioHdr* GetStudioHdr();
    void SetupBones_AttachmentHelper();
    bool setup_bones_rebuilt(matrix3x4_t* matrix, int mask);
    Vector GetEyePosition();
    bool CanSeePlayer(player_t* player, const Vector& pos);
    CUserCmd*& m_pCurrentCommand();
    void RunPreThink();
    void RunThink();
    Vector get_shoot_position();
    void modify_eye_position(Vector& eye_position);
    uint32_t& m_fEffects();
    uint32_t& m_iEFlags();
    float& m_surfaceFriction();
    Vector& m_vecAbsVelocity();
    int get_hitbox_bone_id(int hitbox_id);
    Vector hitbox_position(int hitbox_id);
    bool is_on_ground();
    Vector hitbox_position_matrix(int hitbox_id, matrix3x4_t matrix[MAXSTUDIOBONES]);
    AnimationLayer* get_animlayers();
    CUtlVector <matrix3x4_t>& m_CachedBoneData();
    CBoneAccessor& m_BoneAccessor();
    void invalidate_bone_cache();
    void set_abs_velocity(const Vector& velocity);
    Vector get_render_angles();
    void set_render_angles(const Vector& angles);
    Vector m_aimPunchAngleScaled();
    void update_clientside_animation();
    bool is_alive();
    bool valid(bool check_team, bool check_dormant = true);
    int get_move_type();
    int animlayer_count();
    int sequence_activity(int sequence);
    float get_max_desync_delta();
    void invalidate_physics_recursive(int change_flags);
    float GetMaxPlayerSpeed();
};

class viewmodel_t : public entity_t
{
public:
    NETVAR(int, m_nModelIndex, crypt_str("CBaseViewModel"), crypt_str("m_nModelIndex"));
    NETVAR(int, m_nViewModelIndex, crypt_str("CBaseViewModel"), crypt_str("m_nViewModelIndex"));
    NETVAR(CHandle <weapon_t>, m_hWeapon, crypt_str("CBaseViewModel"), crypt_str("m_hWeapon"));
    NETVAR(CHandle <player_t>, m_hOwner, crypt_str("CBaseViewModel"), crypt_str("m_hOwner"));
    NETVAR(int, m_nAnimationParity, crypt_str("CBaseViewModel"), crypt_str("m_nAnimationParity"));

    float& m_flCycle();
    float& m_flAnimTime();
    void SendViewModelMatchingSequence(int sequence);
};

class CCSBomb : public entity_t
{
public:
    NETVAR(float, m_flDefuseCountDown, crypt_str("CPlantedC4"), crypt_str("m_flDefuseCountDown"));
    NETVAR(int, m_hBombDefuser, crypt_str("CPlantedC4"), crypt_str("m_hBombDefuser"));
    NETVAR(float, m_flC4Blow, crypt_str("CPlantedC4"), crypt_str("m_flC4Blow"));
    NETVAR(bool, m_bBombDefused, crypt_str("CPlantedC4"), crypt_str("m_bBombDefused"));
};

class ragdoll_t : public entity_t
{
public:
    NETVAR(Vector, m_vecForce, crypt_str("CCSRagdoll"), crypt_str("m_vecForce"));
    NETVAR(Vector, m_vecRagdollVelocity, crypt_str("CCSRagdoll"), crypt_str("m_vecRagdollVelocity"));
};

struct inferno_t : public entity_t
{
    OFFSET(float, get_spawn_time, 0x20);

    static float get_expiry_time()
    {
        return 7.03125f;
    }
};

struct smoke_t : public entity_t
{
    NETVAR(int, m_nSmokeEffectTickBegin, crypt_str("CSmokeGrenadeProjectile"), crypt_str("m_nSmokeEffectTickBegin"));
    NETVAR(bool, m_bDidSmokeEffect, crypt_str("CSmokeGrenadeProjectile"), crypt_str("m_bDidSmokeEffect"));

    static float get_expiry_time()
    {
        return 19.0f;
    }
};

class CHudChat
{
public:
    char pad_0x0000[0x4C];
    int m_timesOpened;
    char pad_0x0050[0x8];
    bool m_isOpen;
    char pad_0x0059[0x427];

    void chat_print(const char* fmt, ...);
};

class AnimationLayer
{
public:
    bool m_bClientBlend;		 //0x0000
    float m_flBlendIn;			 //0x0004
    void* m_pStudioHdr;			 //0x0008
    int m_nDispatchSequence;     //0x000C
    int m_nDispatchSequence_2;   //0x0010
    uint32_t m_nOrder;           //0x0014
    uint32_t m_nSequence;        //0x0018
    float_t m_flPrevCycle;       //0x001C
    float_t m_flWeight;          //0x0020
    float_t m_flWeightDeltaRate; //0x0024
    float_t m_flPlaybackRate;    //0x0028
    float_t m_flCycle;           //0x002C
    void* m_pOwner;              //0x0030
    char pad_0038[4];            //0x0034
};
#pragma pack(push, 1)
struct C_AimLayer
{
    float m_flUnknown0;
    float m_flTotalTime;
    float m_flUnknown1;
    float m_flUnknown2;
    float m_flWeight;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct C_AimLayers
{
    C_AimLayer layers[3];
};
#pragma pack(pop)

struct AnimstatePose_t
{
    bool		m_bInitialized;
    int			m_nIndex;
    const char* m_szName;

    AnimstatePose_t()
    {
        m_bInitialized = false;
        m_nIndex = -1;
        m_szName = "";
    }
};

struct procedural_foot_t
{
    Vector m_vecPosAnim;
    Vector m_vecPosAnimLast;
    Vector m_vecPosPlant;
    Vector m_vecPlantVel;
    float m_flLockAmount;
    float m_flLastPlantTime;
};
class C_CSGOPlayerAnimationState
{
public:
    void* m_pThis;
    bool					m_bIsReset;
    bool					m_bUnknownClientBoolean;
    char					m_aSomePad[2];
    int						m_nTick;
    float					m_flFlashedStartTime;
    float					m_flFlashedEndTime;
    C_AimLayers				m_AimLayers;
    int						m_iModelIndex;
    int						m_iUnknownArray[3];
    player_t* m_pBasePlayer;
    LPVOID					m_pWeapon;
    LPVOID					m_pWeaponLast;
    float					m_flLastUpdateTime;
    int						m_nLastUpdateFrame;
    float					m_flLastUpdateIncrement;
    float					m_flEyeYaw;
    float					m_flEyePitch;
    float					m_flFootYaw;
    float					m_flFootYawLast;
    float					m_flMoveYaw;
    float					m_flMoveYawIdeal;
    float					m_flMoveYawCurrentToIdeal;
    float					m_flTimeToAlignLowerBody;
    float					m_flPrimaryCycle;
    float					m_flMoveWeight;
    float					m_flMoveWeightSmoothed;
    float					m_flAnimDuckAmount;
    float					m_flDuckAdditional;
    float					m_flRecrouchWeight;
    Vector					m_vecPositionCurrent;
    Vector					m_vecPositionLast;
    Vector					m_vecVelocity;
    Vector					m_vecVelocityNormalized;
    Vector					m_vecVelocityNormalizedNonZero;
    float					m_flVelocityLengthXY;
    float					m_flVelocityLengthZ;
    float					m_flSpeedAsPortionOfRunTopSpeed;
    float					m_flSpeedAsPortionOfWalkTopSpeed;
    float					m_flSpeedAsPortionOfCrouchTopSpeed;
    float					m_flDurationMoving;
    float					m_flDurationStill;
    bool					m_bOnGround;
    bool					m_bLanding;
    char					m_pad[2];
    float					m_flJumpToFall;
    float					m_flDurationInAir;
    float					m_flLeftGroundHeight;
    float					m_flLandAnimMultiplier;
    float					m_flWalkToRunTransition;
    bool					m_bLandedOnGroundThisFrame;
    bool					m_bLeftTheGroundThisFrame;
    float					m_flInAirSmoothValue;
    bool					m_bOnLadder;
    float					m_flLadderWeight;
    float					m_flLadderSpeed;
    bool					m_bWalkToRunTransitionState;
    bool					m_bDefuseStarted;
    bool					m_bPlantAnimStarted;
    bool					m_bTwitchAnimStarted;
    bool					m_bAdjustStarted;
    char					m_ActivityModifiers[20];
    float					m_flNextTwitchTime;
    float					m_flTimeOfLastKnownInjury;
    float					m_flLastVelocityTestTime;
    Vector					m_vecVelocityLast;
    Vector					m_vecTargetAcceleration;
    Vector					m_vecAcceleration;
    float					m_flAccelerationWeight;
    float					m_flAimMatrixTransition;
    float					m_flAimMatrixTransitionDelay;
    bool					m_bFlashed;
    float					m_flStrafeChangeWeight;
    float					m_flStrafeChangeTargetWeight;
    float					m_flStrafeChangeCycle;
    int						m_nStrafeSequence;
    bool					m_bStrafeChanging;
    float					m_flDurationStrafing;
    float					m_flFootLerp;
    bool					m_bFeetCrossed;
    bool					m_bPlayerIsAccelerating;
    AnimstatePose_t			m_tPoseParamMappings[20];
    float					m_flDurationMoveWeightIsTooHigh;
    float					m_flStaticApproachSpeed;
    int						m_nPreviousMoveState;
    float					m_flStutterStep;
    float					m_flActionWeightBiasRemainder;
    procedural_foot_t		m_footLeft;
    procedural_foot_t		m_footRight;
    float					m_flCameraSmoothHeight;
    bool					m_bSmoothHeightValid;
    float					m_flLastTimeVelocityOverTen;
    float					m_flFuckValve;
    float					m_flAimYawMin;
    float					m_flAimYawMax;
    float					m_flAimPitchMin;
    float					m_flAimPitchMax;
    int						m_nAnimstateModelVersion;


    float& time_since_in_air()
    {
        return *(float*)((uintptr_t)this + 0x110);
    }

    float& yaw_desync_adjustment()
    {
        return *(float*)((uintptr_t)this + 0x334 + 0x4);
    }
};


class c_baseplayeranimationstate
{
public:
    char pad[3];
    char bUnknown;
    char pad2[87];
    weapon_t* m_pLastBoneSetupWeapon;
    player_t* m_pBaseEntity;
    weapon_t* m_pActiveWeapon;
    weapon_t* m_pLastActiveWeapon;
    float m_flLastClientSideAnimationUpdateTime;
    int m_iLastClientSideAnimationUpdateFramecount;
    float m_flUpdateTimeDelta;
    float m_flEyeYaw;
    float m_flPitch;
    float m_flGoalFeetYaw;
    float m_flCurrentFeetYaw;
    float m_flCurrentTorsoYaw;
    float m_flUnknownVelocityLean;
    float m_flLeanAmount;
    char pad4[4];
    float m_flFeetCycle;
    float m_flFeetYawRate;
    float m_fUnknown2;
    float m_fDuckAmount;
    float m_fLandingDuckAdditiveSomething;
    float m_fUnknown3;
    Vector m_vOrigin;
    Vector m_vLastOrigin;
    float m_vVelocityX;
    float m_vVelocityY;
    char pad5[4];
    float m_flUnknownFloat1;
    char pad6[8];
    float m_flUnknownFloat2;
    float m_flUnknownFloat3;
    float m_unknown;
    float m_velocity;
    float flUpVelocity;
    float m_flSpeedNormalized;
    float m_flFeetSpeedForwardsOrSideWays;
    float m_flFeetSpeedUnknownForwardOrSideways;
    float m_flTimeSinceStartedMoving;
    float m_flTimeSinceStoppedMoving;
    bool m_bOnGround;
    bool m_bInHitGroundAnimation;
    char pad7[10];
    float m_flLastOriginZ;
    float m_flHeadHeightOrOffsetFromHittingGroundAnimation;
    float m_flStopToFullRunningFraction;
    char pad8[4];
    float m_flMovingFraction;
    char pad9[4];
    float m_flUnknown3;
    char pad10[528];


    float& time_since_in_air()
    {
        return *(float*)((uintptr_t)this + 0x110);
    }

    float& yaw_desync_adjustment()
    {
        return *(float*)((uintptr_t)this + 0x338);
    }
};






enum Activity
{
    ACT_RESET,
    ACT_IDLE,
    ACT_TRANSITION,
    ACT_COVER,
    ACT_COVER_MED,
    ACT_COVER_LOW,
    ACT_WALK,
    ACT_WALK_AIM,
    ACT_WALK_CROUCH,
    ACT_WALK_CROUCH_AIM,
    ACT_RUN,
    ACT_RUN_AIM,
    ACT_RUN_CROUCH,
    ACT_RUN_CROUCH_AIM,
    ACT_RUN_PROTECTED,
    ACT_SCRIPT_CUSTOM_MOVE,
    ACT_RANGE_ATTACK1,
    ACT_RANGE_ATTACK2,
    ACT_RANGE_ATTACK1_LOW,
    ACT_RANGE_ATTACK2_LOW,
    ACT_DIESIMPLE,
    ACT_DIEBACKWARD,
    ACT_DIEFORWARD,
    ACT_DIEVIOLENT,
    ACT_DIERAGDOLL,
    ACT_FLY,
    ACT_HOVER,
    ACT_GLIDE,
    ACT_SWIM,
    ACT_JUMP,
    ACT_HOP,
    ACT_LEAP,
    ACT_LAND,
    ACT_CLIMB_UP,
    ACT_CLIMB_DOWN,
    ACT_CLIMB_DISMOUNT,
    ACT_SHIPLADDER_UP,
    ACT_SHIPLADDER_DOWN,
    ACT_STRAFE_LEFT,
    ACT_STRAFE_RIGHT,
    ACT_ROLL_LEFT,
    ACT_ROLL_RIGHT,
    ACT_TURN_LEFT,
    ACT_TURN_RIGHT,
    ACT_CROUCH,
    ACT_CROUCHIDLE,
    ACT_STAND,
    ACT_USE,
    ACT_ALIEN_BURROW_IDLE,
    ACT_ALIEN_BURROW_OUT,
    ACT_SIGNAL1,
    ACT_SIGNAL2,
    ACT_SIGNAL3,
    ACT_SIGNAL_ADVANCE,
    ACT_SIGNAL_FORWARD,
    ACT_SIGNAL_GROUP,
    ACT_SIGNAL_HALT,
    ACT_SIGNAL_LEFT,
    ACT_SIGNAL_RIGHT,
    ACT_SIGNAL_TAKECOVER,
    ACT_LOOKBACK_RIGHT,
    ACT_LOOKBACK_LEFT,
    ACT_COWER,
    ACT_SMALL_FLINCH,
    ACT_BIG_FLINCH,
    ACT_MELEE_ATTACK1,
    ACT_MELEE_ATTACK2,
    ACT_RELOAD,
    ACT_RELOAD_START,
    ACT_RELOAD_FINISH,
    ACT_RELOAD_LOW,
    ACT_ARM,
    ACT_DISARM,
    ACT_DROP_WEAPON,
    ACT_DROP_WEAPON_SHOTGUN,
    ACT_PICKUP_GROUND,
    ACT_PICKUP_RACK,
    ACT_IDLE_ANGRY,
    ACT_IDLE_RELAXED,
    ACT_IDLE_STIMULATED,
    ACT_IDLE_AGITATED,
    ACT_IDLE_STEALTH,
    ACT_IDLE_HURT,
    ACT_WALK_RELAXED,
    ACT_WALK_STIMULATED,
    ACT_WALK_AGITATED,
    ACT_WALK_STEALTH,
    ACT_RUN_RELAXED,
    ACT_RUN_STIMULATED,
    ACT_RUN_AGITATED,
    ACT_RUN_STEALTH,
    ACT_IDLE_AIM_RELAXED,
    ACT_IDLE_AIM_STIMULATED,
    ACT_IDLE_AIM_AGITATED,
    ACT_IDLE_AIM_STEALTH,
    ACT_WALK_AIM_RELAXED,
    ACT_WALK_AIM_STIMULATED,
    ACT_WALK_AIM_AGITATED,
    ACT_WALK_AIM_STEALTH,
    ACT_RUN_AIM_RELAXED,
    ACT_RUN_AIM_STIMULATED,
    ACT_RUN_AIM_AGITATED,
    ACT_RUN_AIM_STEALTH,
    ACT_CROUCHIDLE_STIMULATED,
    ACT_CROUCHIDLE_AIM_STIMULATED,
    ACT_CROUCHIDLE_AGITATED,
    ACT_WALK_HURT,
    ACT_RUN_HURT,
    ACT_SPECIAL_ATTACK1,
    ACT_SPECIAL_ATTACK2,
    ACT_COMBAT_IDLE,
    ACT_WALK_SCARED,
    ACT_RUN_SCARED,
    ACT_VICTORY_DANCE,
    ACT_DIE_HEADSHOT,
    ACT_DIE_CHESTSHOT,
    ACT_DIE_GUTSHOT,
    ACT_DIE_BACKSHOT,
    ACT_FLINCH_HEAD,
    ACT_FLINCH_CHEST,
    ACT_FLINCH_STOMACH,
    ACT_FLINCH_LEFTARM,
    ACT_FLINCH_RIGHTARM,
    ACT_FLINCH_LEFTLEG,
    ACT_FLINCH_RIGHTLEG,
    ACT_FLINCH_PHYSICS,
    ACT_FLINCH_HEAD_BACK,
    ACT_FLINCH_HEAD_LEFT,
    ACT_FLINCH_HEAD_RIGHT,
    ACT_FLINCH_CHEST_BACK,
    ACT_FLINCH_STOMACH_BACK,
    ACT_FLINCH_CROUCH_FRONT,
    ACT_FLINCH_CROUCH_BACK,
    ACT_FLINCH_CROUCH_LEFT,
    ACT_FLINCH_CROUCH_RIGHT,
    ACT_IDLE_ON_FIRE,
    ACT_WALK_ON_FIRE,
    ACT_RUN_ON_FIRE,
    ACT_RAPPEL_LOOP,
    ACT_180_LEFT,
    ACT_180_RIGHT,
    ACT_90_LEFT,
    ACT_90_RIGHT,
    ACT_STEP_LEFT,
    ACT_STEP_RIGHT,
    ACT_STEP_BACK,
    ACT_STEP_FORE,
    ACT_GESTURE_RANGE_ATTACK1,
    ACT_GESTURE_RANGE_ATTACK2,
    ACT_GESTURE_MELEE_ATTACK1,
    ACT_GESTURE_MELEE_ATTACK2,
    ACT_GESTURE_RANGE_ATTACK1_LOW,
    ACT_GESTURE_RANGE_ATTACK2_LOW,
    ACT_MELEE_ATTACK_SWING_GESTURE,
    ACT_GESTURE_SMALL_FLINCH,
    ACT_GESTURE_BIG_FLINCH,
    ACT_GESTURE_FLINCH_BLAST,
    ACT_GESTURE_FLINCH_BLAST_SHOTGUN,
    ACT_GESTURE_FLINCH_BLAST_DAMAGED,
    ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN,
    ACT_GESTURE_FLINCH_HEAD,
    ACT_GESTURE_FLINCH_CHEST,
    ACT_GESTURE_FLINCH_STOMACH,
    ACT_GESTURE_FLINCH_LEFTARM,
    ACT_GESTURE_FLINCH_RIGHTARM,
    ACT_GESTURE_FLINCH_LEFTLEG,
    ACT_GESTURE_FLINCH_RIGHTLEG,
    ACT_GESTURE_TURN_LEFT,
    ACT_GESTURE_TURN_RIGHT,
    ACT_GESTURE_TURN_LEFT45,
    ACT_GESTURE_TURN_RIGHT45,
    ACT_GESTURE_TURN_LEFT90,
    ACT_GESTURE_TURN_RIGHT90,
    ACT_GESTURE_TURN_LEFT45_FLAT,
    ACT_GESTURE_TURN_RIGHT45_FLAT,
    ACT_GESTURE_TURN_LEFT90_FLAT,
    ACT_GESTURE_TURN_RIGHT90_FLAT,
    ACT_BARNACLE_HIT,
    ACT_BARNACLE_PULL,
    ACT_BARNACLE_CHOMP,
    ACT_BARNACLE_CHEW,
    ACT_DO_NOT_DISTURB,
    ACT_SPECIFIC_SEQUENCE,
    ACT_VM_DRAW,
    ACT_VM_HOLSTER,
    ACT_VM_IDLE,
    ACT_VM_FIDGET,
    ACT_VM_PULLBACK,
    ACT_VM_PULLBACK_HIGH,
    ACT_VM_PULLBACK_LOW,
    ACT_VM_THROW,
    ACT_VM_PULLPIN,
    ACT_VM_PRIMARYATTACK,
    ACT_VM_SECONDARYATTACK,
    ACT_VM_RELOAD,
    ACT_VM_DRYFIRE,
    ACT_VM_HITLEFT,
    ACT_VM_HITLEFT2,
    ACT_VM_HITRIGHT,
    ACT_VM_HITRIGHT2,
    ACT_VM_HITCENTER,
    ACT_VM_HITCENTER2,
    ACT_VM_MISSLEFT,
    ACT_VM_MISSLEFT2,
    ACT_VM_MISSRIGHT,
    ACT_VM_MISSRIGHT2,
    ACT_VM_MISSCENTER,
    ACT_VM_MISSCENTER2,
    ACT_VM_HAULBACK,
    ACT_VM_SWINGHARD,
    ACT_VM_SWINGMISS,
    ACT_VM_SWINGHIT,
    ACT_VM_IDLE_TO_LOWERED,
    ACT_VM_IDLE_LOWERED,
    ACT_VM_LOWERED_TO_IDLE,
    ACT_VM_RECOIL1,
    ACT_VM_RECOIL2,
    ACT_VM_RECOIL3,
    ACT_VM_PICKUP,
    ACT_VM_RELEASE,
    ACT_VM_ATTACH_SILENCER,
    ACT_VM_DETACH_SILENCER,
    ACT_VM_EMPTY_FIRE,
    ACT_VM_EMPTY_RELOAD,
    ACT_VM_EMPTY_DRAW,
    ACT_VM_EMPTY_IDLE,
    ACT_SLAM_STICKWALL_IDLE,
    ACT_SLAM_STICKWALL_ND_IDLE,
    ACT_SLAM_STICKWALL_ATTACH,
    ACT_SLAM_STICKWALL_ATTACH2,
    ACT_SLAM_STICKWALL_ND_ATTACH,
    ACT_SLAM_STICKWALL_ND_ATTACH2,
    ACT_SLAM_STICKWALL_DETONATE,
    ACT_SLAM_STICKWALL_DETONATOR_HOLSTER,
    ACT_SLAM_STICKWALL_DRAW,
    ACT_SLAM_STICKWALL_ND_DRAW,
    ACT_SLAM_STICKWALL_TO_THROW,
    ACT_SLAM_STICKWALL_TO_THROW_ND,
    ACT_SLAM_STICKWALL_TO_TRIPMINE_ND,
    ACT_SLAM_THROW_IDLE,
    ACT_SLAM_THROW_ND_IDLE,
    ACT_SLAM_THROW_THROW,
    ACT_SLAM_THROW_THROW2,
    ACT_SLAM_THROW_THROW_ND,
    ACT_SLAM_THROW_THROW_ND2,
    ACT_SLAM_THROW_DRAW,
    ACT_SLAM_THROW_ND_DRAW,
    ACT_SLAM_THROW_TO_STICKWALL,
    ACT_SLAM_THROW_TO_STICKWALL_ND,
    ACT_SLAM_THROW_DETONATE,
    ACT_SLAM_THROW_DETONATOR_HOLSTER,
    ACT_SLAM_THROW_TO_TRIPMINE_ND,
    ACT_SLAM_TRIPMINE_IDLE,
    ACT_SLAM_TRIPMINE_DRAW,
    ACT_SLAM_TRIPMINE_ATTACH,
    ACT_SLAM_TRIPMINE_ATTACH2,
    ACT_SLAM_TRIPMINE_TO_STICKWALL_ND,
    ACT_SLAM_TRIPMINE_TO_THROW_ND,
    ACT_SLAM_DETONATOR_IDLE,
    ACT_SLAM_DETONATOR_DRAW,
    ACT_SLAM_DETONATOR_DETONATE,
    ACT_SLAM_DETONATOR_HOLSTER,
    ACT_SLAM_DETONATOR_STICKWALL_DRAW,
    ACT_SLAM_DETONATOR_THROW_DRAW,
    ACT_SHOTGUN_RELOAD_START,
    ACT_SHOTGUN_RELOAD_FINISH,
    ACT_SHOTGUN_PUMP,
    ACT_SMG2_IDLE2,
    ACT_SMG2_FIRE2,
    ACT_SMG2_DRAW2,
    ACT_SMG2_RELOAD2,
    ACT_SMG2_DRYFIRE2,
    ACT_SMG2_TOAUTO,
    ACT_SMG2_TOBURST,
    ACT_PHYSCANNON_UPGRADE,
    ACT_RANGE_ATTACK_AR1,
    ACT_RANGE_ATTACK_AR2,
    ACT_RANGE_ATTACK_AR2_LOW,
    ACT_RANGE_ATTACK_AR2_GRENADE,
    ACT_RANGE_ATTACK_HMG1,
    ACT_RANGE_ATTACK_ML,
    ACT_RANGE_ATTACK_SMG1,
    ACT_RANGE_ATTACK_SMG1_LOW,
    ACT_RANGE_ATTACK_SMG2,
    ACT_RANGE_ATTACK_SHOTGUN,
    ACT_RANGE_ATTACK_SHOTGUN_LOW,
    ACT_RANGE_ATTACK_PISTOL,
    ACT_RANGE_ATTACK_PISTOL_LOW,
    ACT_RANGE_ATTACK_SLAM,
    ACT_RANGE_ATTACK_TRIPWIRE,
    ACT_RANGE_ATTACK_THROW,
    ACT_RANGE_ATTACK_SNIPER_RIFLE,
    ACT_RANGE_ATTACK_RPG,
    ACT_MELEE_ATTACK_SWING,
    ACT_RANGE_AIM_LOW,
    ACT_RANGE_AIM_SMG1_LOW,
    ACT_RANGE_AIM_PISTOL_LOW,
    ACT_RANGE_AIM_AR2_LOW,
    ACT_COVER_PISTOL_LOW,
    ACT_COVER_SMG1_LOW,
    ACT_GESTURE_RANGE_ATTACK_AR1,
    ACT_GESTURE_RANGE_ATTACK_AR2,
    ACT_GESTURE_RANGE_ATTACK_AR2_GRENADE,
    ACT_GESTURE_RANGE_ATTACK_HMG1,
    ACT_GESTURE_RANGE_ATTACK_ML,
    ACT_GESTURE_RANGE_ATTACK_SMG1,
    ACT_GESTURE_RANGE_ATTACK_SMG1_LOW,
    ACT_GESTURE_RANGE_ATTACK_SMG2,
    ACT_GESTURE_RANGE_ATTACK_SHOTGUN,
    ACT_GESTURE_RANGE_ATTACK_PISTOL,
    ACT_GESTURE_RANGE_ATTACK_PISTOL_LOW,
    ACT_GESTURE_RANGE_ATTACK_SLAM,
    ACT_GESTURE_RANGE_ATTACK_TRIPWIRE,
    ACT_GESTURE_RANGE_ATTACK_THROW,
    ACT_GESTURE_RANGE_ATTACK_SNIPER_RIFLE,
    ACT_GESTURE_MELEE_ATTACK_SWING,
    ACT_IDLE_RIFLE,
    ACT_IDLE_SMG1,
    ACT_IDLE_ANGRY_SMG1,
    ACT_IDLE_PISTOL,
    ACT_IDLE_ANGRY_PISTOL,
    ACT_IDLE_ANGRY_SHOTGUN,
    ACT_IDLE_STEALTH_PISTOL,
    ACT_IDLE_PACKAGE,
    ACT_WALK_PACKAGE,
    ACT_IDLE_SUITCASE,
    ACT_WALK_SUITCASE,
    ACT_IDLE_SMG1_RELAXED,
    ACT_IDLE_SMG1_STIMULATED,
    ACT_WALK_RIFLE_RELAXED,
    ACT_RUN_RIFLE_RELAXED,
    ACT_WALK_RIFLE_STIMULATED,
    ACT_RUN_RIFLE_STIMULATED,
    ACT_IDLE_AIM_RIFLE_STIMULATED,
    ACT_WALK_AIM_RIFLE_STIMULATED,
    ACT_RUN_AIM_RIFLE_STIMULATED,
    ACT_IDLE_SHOTGUN_RELAXED,
    ACT_IDLE_SHOTGUN_STIMULATED,
    ACT_IDLE_SHOTGUN_AGITATED,
    ACT_WALK_ANGRY,
    ACT_POLICE_HARASS1,
    ACT_POLICE_HARASS2,
    ACT_IDLE_MANNEDGUN,
    ACT_IDLE_MELEE,
    ACT_IDLE_ANGRY_MELEE,
    ACT_IDLE_RPG_RELAXED,
    ACT_IDLE_RPG,
    ACT_IDLE_ANGRY_RPG,
    ACT_COVER_LOW_RPG,
    ACT_WALK_RPG,
    ACT_RUN_RPG,
    ACT_WALK_CROUCH_RPG,
    ACT_RUN_CROUCH_RPG,
    ACT_WALK_RPG_RELAXED,
    ACT_RUN_RPG_RELAXED,
    ACT_WALK_RIFLE,
    ACT_WALK_AIM_RIFLE,
    ACT_WALK_CROUCH_RIFLE,
    ACT_WALK_CROUCH_AIM_RIFLE,
    ACT_RUN_RIFLE,
    ACT_RUN_AIM_RIFLE,
    ACT_RUN_CROUCH_RIFLE,
    ACT_RUN_CROUCH_AIM_RIFLE,
    ACT_RUN_STEALTH_PISTOL,
    ACT_WALK_AIM_SHOTGUN,
    ACT_RUN_AIM_SHOTGUN,
    ACT_WALK_PISTOL,
    ACT_RUN_PISTOL,
    ACT_WALK_AIM_PISTOL,
    ACT_RUN_AIM_PISTOL,
    ACT_WALK_STEALTH_PISTOL,
    ACT_WALK_AIM_STEALTH_PISTOL,
    ACT_RUN_AIM_STEALTH_PISTOL,
    ACT_RELOAD_PISTOL,
    ACT_RELOAD_PISTOL_LOW,
    ACT_RELOAD_SMG1,
    ACT_RELOAD_SMG1_LOW,
    ACT_RELOAD_SHOTGUN,
    ACT_RELOAD_SHOTGUN_LOW,
    ACT_GESTURE_RELOAD,
    ACT_GESTURE_RELOAD_PISTOL,
    ACT_GESTURE_RELOAD_SMG1,
    ACT_GESTURE_RELOAD_SHOTGUN,
    ACT_BUSY_LEAN_LEFT,
    ACT_BUSY_LEAN_LEFT_ENTRY,
    ACT_BUSY_LEAN_LEFT_EXIT,
    ACT_BUSY_LEAN_BACK,
    ACT_BUSY_LEAN_BACK_ENTRY,
    ACT_BUSY_LEAN_BACK_EXIT,
    ACT_BUSY_SIT_GROUND,
    ACT_BUSY_SIT_GROUND_ENTRY,
    ACT_BUSY_SIT_GROUND_EXIT,
    ACT_BUSY_SIT_CHAIR,
    ACT_BUSY_SIT_CHAIR_ENTRY,
    ACT_BUSY_SIT_CHAIR_EXIT,
    ACT_BUSY_STAND,
    ACT_BUSY_QUEUE,
    ACT_DUCK_DODGE,
    ACT_DIE_BARNACLE_SWALLOW,
    ACT_GESTURE_BARNACLE_STRANGLE,
    ACT_PHYSCANNON_DETACH,
    ACT_PHYSCANNON_ANIMATE,
    ACT_PHYSCANNON_ANIMATE_PRE,
    ACT_PHYSCANNON_ANIMATE_POST,
    ACT_DIE_FRONTSIDE,
    ACT_DIE_RIGHTSIDE,
    ACT_DIE_BACKSIDE,
    ACT_DIE_LEFTSIDE,
    ACT_DIE_CROUCH_FRONTSIDE,
    ACT_DIE_CROUCH_RIGHTSIDE,
    ACT_DIE_CROUCH_BACKSIDE,
    ACT_DIE_CROUCH_LEFTSIDE,
    ACT_OPEN_DOOR,
    ACT_DI_ALYX_ZOMBIE_MELEE,
    ACT_DI_ALYX_ZOMBIE_TORSO_MELEE,
    ACT_DI_ALYX_HEADCRAB_MELEE,
    ACT_DI_ALYX_ANTLION,
    ACT_DI_ALYX_ZOMBIE_SHOTGUN64,
    ACT_DI_ALYX_ZOMBIE_SHOTGUN26,
    ACT_READINESS_RELAXED_TO_STIMULATED,
    ACT_READINESS_RELAXED_TO_STIMULATED_WALK,
    ACT_READINESS_AGITATED_TO_STIMULATED,
    ACT_READINESS_STIMULATED_TO_RELAXED,
    ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED,
    ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK,
    ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED,
    ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED,
    ACT_IDLE_CARRY,
    ACT_WALK_CARRY,
    ACT_STARTDYING,
    ACT_DYINGLOOP,
    ACT_DYINGTODEAD,
    ACT_RIDE_MANNED_GUN,
    ACT_VM_SPRINT_ENTER,
    ACT_VM_SPRINT_IDLE,
    ACT_VM_SPRINT_LEAVE,
    ACT_FIRE_START,
    ACT_FIRE_LOOP,
    ACT_FIRE_END,
    ACT_CROUCHING_GRENADEIDLE,
    ACT_CROUCHING_GRENADEREADY,
    ACT_CROUCHING_PRIMARYATTACK,
    ACT_OVERLAY_GRENADEIDLE,
    ACT_OVERLAY_GRENADEREADY,
    ACT_OVERLAY_PRIMARYATTACK,
    ACT_OVERLAY_SHIELD_UP,
    ACT_OVERLAY_SHIELD_DOWN,
    ACT_OVERLAY_SHIELD_UP_IDLE,
    ACT_OVERLAY_SHIELD_ATTACK,
    ACT_OVERLAY_SHIELD_KNOCKBACK,
    ACT_SHIELD_UP,
    ACT_SHIELD_DOWN,
    ACT_SHIELD_UP_IDLE,
    ACT_SHIELD_ATTACK,
    ACT_SHIELD_KNOCKBACK,
    ACT_CROUCHING_SHIELD_UP,
    ACT_CROUCHING_SHIELD_DOWN,
    ACT_CROUCHING_SHIELD_UP_IDLE,
    ACT_CROUCHING_SHIELD_ATTACK,
    ACT_CROUCHING_SHIELD_KNOCKBACK,
    ACT_TURNRIGHT45,
    ACT_TURNLEFT45,
    ACT_TURN,
    ACT_OBJ_ASSEMBLING,
    ACT_OBJ_DISMANTLING,
    ACT_OBJ_STARTUP,
    ACT_OBJ_RUNNING,
    ACT_OBJ_IDLE,
    ACT_OBJ_PLACING,
    ACT_OBJ_DETERIORATING,
    ACT_OBJ_UPGRADING,
    ACT_DEPLOY,
    ACT_DEPLOY_IDLE,
    ACT_UNDEPLOY,
    ACT_CROSSBOW_DRAW_UNLOADED,
    ACT_GAUSS_SPINUP,
    ACT_GAUSS_SPINCYCLE,
    ACT_VM_PRIMARYATTACK_SILENCED,
    ACT_VM_RELOAD_SILENCED,
    ACT_VM_DRYFIRE_SILENCED,
    ACT_VM_IDLE_SILENCED,
    ACT_VM_DRAW_SILENCED,
    ACT_VM_IDLE_EMPTY_LEFT,
    ACT_VM_DRYFIRE_LEFT,
    ACT_VM_IS_DRAW,
    ACT_VM_IS_HOLSTER,
    ACT_VM_IS_IDLE,
    ACT_VM_IS_PRIMARYATTACK,
    ACT_PLAYER_IDLE_FIRE,
    ACT_PLAYER_CROUCH_FIRE,
    ACT_PLAYER_CROUCH_WALK_FIRE,
    ACT_PLAYER_WALK_FIRE,
    ACT_PLAYER_RUN_FIRE,
    ACT_IDLETORUN,
    ACT_RUNTOIDLE,
    ACT_VM_DRAW_DEPLOYED,
    ACT_HL2MP_IDLE_MELEE,
    ACT_HL2MP_RUN_MELEE,
    ACT_HL2MP_IDLE_CROUCH_MELEE,
    ACT_HL2MP_WALK_CROUCH_MELEE,
    ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,
    ACT_HL2MP_GESTURE_RELOAD_MELEE,
    ACT_HL2MP_JUMP_MELEE,
    ACT_VM_FIZZLE,
    ACT_MP_STAND_IDLE,
    ACT_MP_CROUCH_IDLE,
    ACT_MP_CROUCH_DEPLOYED_IDLE,
    ACT_MP_CROUCH_DEPLOYED,
    ACT_MP_DEPLOYED_IDLE,
    ACT_MP_RUN,
    ACT_MP_WALK,
    ACT_MP_AIRWALK,
    ACT_MP_CROUCHWALK,
    ACT_MP_SPRINT,
    ACT_MP_JUMP,
    ACT_MP_JUMP_START,
    ACT_MP_JUMP_FLOAT,
    ACT_MP_JUMP_LAND,
    ACT_MP_JUMP_IMPACT_N,
    ACT_MP_JUMP_IMPACT_E,
    ACT_MP_JUMP_IMPACT_W,
    ACT_MP_JUMP_IMPACT_S,
    ACT_MP_JUMP_IMPACT_TOP,
    ACT_MP_DOUBLEJUMP,
    ACT_MP_SWIM,
    ACT_MP_DEPLOYED,
    ACT_MP_SWIM_DEPLOYED,
    ACT_MP_VCD,
    ACT_MP_ATTACK_STAND_PRIMARYFIRE,
    ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED,
    ACT_MP_ATTACK_STAND_SECONDARYFIRE,
    ACT_MP_ATTACK_STAND_GRENADE,
    ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,
    ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED,
    ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,
    ACT_MP_ATTACK_CROUCH_GRENADE,
    ACT_MP_ATTACK_SWIM_PRIMARYFIRE,
    ACT_MP_ATTACK_SWIM_SECONDARYFIRE,
    ACT_MP_ATTACK_SWIM_GRENADE,
    ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,
    ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,
    ACT_MP_ATTACK_AIRWALK_GRENADE,
    ACT_MP_RELOAD_STAND,
    ACT_MP_RELOAD_STAND_LOOP,
    ACT_MP_RELOAD_STAND_END,
    ACT_MP_RELOAD_CROUCH,
    ACT_MP_RELOAD_CROUCH_LOOP,
    ACT_MP_RELOAD_CROUCH_END,
    ACT_MP_RELOAD_SWIM,
    ACT_MP_RELOAD_SWIM_LOOP,
    ACT_MP_RELOAD_SWIM_END,
    ACT_MP_RELOAD_AIRWALK,
    ACT_MP_RELOAD_AIRWALK_LOOP,
    ACT_MP_RELOAD_AIRWALK_END,
    ACT_MP_ATTACK_STAND_PREFIRE,
    ACT_MP_ATTACK_STAND_POSTFIRE,
    ACT_MP_ATTACK_STAND_STARTFIRE,
    ACT_MP_ATTACK_CROUCH_PREFIRE,
    ACT_MP_ATTACK_CROUCH_POSTFIRE,
    ACT_MP_ATTACK_SWIM_PREFIRE,
    ACT_MP_ATTACK_SWIM_POSTFIRE,
    ACT_MP_STAND_PRIMARY,
    ACT_MP_CROUCH_PRIMARY,
    ACT_MP_RUN_PRIMARY,
    ACT_MP_WALK_PRIMARY,
    ACT_MP_AIRWALK_PRIMARY,
    ACT_MP_CROUCHWALK_PRIMARY,
    ACT_MP_JUMP_PRIMARY,
    ACT_MP_JUMP_START_PRIMARY,
    ACT_MP_JUMP_FLOAT_PRIMARY,
    ACT_MP_JUMP_LAND_PRIMARY,
    ACT_MP_SWIM_PRIMARY,
    ACT_MP_DEPLOYED_PRIMARY,
    ACT_MP_SWIM_DEPLOYED_PRIMARY,
    ACT_MP_ATTACK_STAND_PRIMARY,
    ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED,
    ACT_MP_ATTACK_CROUCH_PRIMARY,
    ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED,
    ACT_MP_ATTACK_SWIM_PRIMARY,
    ACT_MP_ATTACK_AIRWALK_PRIMARY,
    ACT_MP_RELOAD_STAND_PRIMARY,
    ACT_MP_RELOAD_STAND_PRIMARY_LOOP,
    ACT_MP_RELOAD_STAND_PRIMARY_END,
    ACT_MP_RELOAD_CROUCH_PRIMARY,
    ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP,
    ACT_MP_RELOAD_CROUCH_PRIMARY_END,
    ACT_MP_RELOAD_SWIM_PRIMARY,
    ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,
    ACT_MP_RELOAD_SWIM_PRIMARY_END,
    ACT_MP_RELOAD_AIRWALK_PRIMARY,
    ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP,
    ACT_MP_RELOAD_AIRWALK_PRIMARY_END,
    ACT_MP_ATTACK_STAND_GRENADE_PRIMARY,
    ACT_MP_ATTACK_CROUCH_GRENADE_PRIMARY,
    ACT_MP_ATTACK_SWIM_GRENADE_PRIMARY,
    ACT_MP_ATTACK_AIRWALK_GRENADE_PRIMARY,
    ACT_MP_STAND_SECONDARY,
    ACT_MP_CROUCH_SECONDARY,
    ACT_MP_RUN_SECONDARY,
    ACT_MP_WALK_SECONDARY,
    ACT_MP_AIRWALK_SECONDARY,
    ACT_MP_CROUCHWALK_SECONDARY,
    ACT_MP_JUMP_SECONDARY,
    ACT_MP_JUMP_START_SECONDARY,
    ACT_MP_JUMP_FLOAT_SECONDARY,
    ACT_MP_JUMP_LAND_SECONDARY,
    ACT_MP_SWIM_SECONDARY,
    ACT_MP_ATTACK_STAND_SECONDARY,
    ACT_MP_ATTACK_CROUCH_SECONDARY,
    ACT_MP_ATTACK_SWIM_SECONDARY,
    ACT_MP_ATTACK_AIRWALK_SECONDARY,
    ACT_MP_RELOAD_STAND_SECONDARY,
    ACT_MP_RELOAD_STAND_SECONDARY_LOOP,
    ACT_MP_RELOAD_STAND_SECONDARY_END,
    ACT_MP_RELOAD_CROUCH_SECONDARY,
    ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP,
    ACT_MP_RELOAD_CROUCH_SECONDARY_END,
    ACT_MP_RELOAD_SWIM_SECONDARY,
    ACT_MP_RELOAD_SWIM_SECONDARY_LOOP,
    ACT_MP_RELOAD_SWIM_SECONDARY_END,
    ACT_MP_RELOAD_AIRWALK_SECONDARY,
    ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP,
    ACT_MP_RELOAD_AIRWALK_SECONDARY_END,
    ACT_MP_ATTACK_STAND_GRENADE_SECONDARY,
    ACT_MP_ATTACK_CROUCH_GRENADE_SECONDARY,
    ACT_MP_ATTACK_SWIM_GRENADE_SECONDARY,
    ACT_MP_ATTACK_AIRWALK_GRENADE_SECONDARY,
    ACT_MP_STAND_MELEE,
    ACT_MP_CROUCH_MELEE,
    ACT_MP_RUN_MELEE,
    ACT_MP_WALK_MELEE,
    ACT_MP_AIRWALK_MELEE,
    ACT_MP_CROUCHWALK_MELEE,
    ACT_MP_JUMP_MELEE,
    ACT_MP_JUMP_START_MELEE,
    ACT_MP_JUMP_FLOAT_MELEE,
    ACT_MP_JUMP_LAND_MELEE,
    ACT_MP_SWIM_MELEE,
    ACT_MP_ATTACK_STAND_MELEE,
    ACT_MP_ATTACK_STAND_MELEE_SECONDARY,
    ACT_MP_ATTACK_CROUCH_MELEE,
    ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,
    ACT_MP_ATTACK_SWIM_MELEE,
    ACT_MP_ATTACK_AIRWALK_MELEE,
    ACT_MP_ATTACK_STAND_GRENADE_MELEE,
    ACT_MP_ATTACK_CROUCH_GRENADE_MELEE,
    ACT_MP_ATTACK_SWIM_GRENADE_MELEE,
    ACT_MP_ATTACK_AIRWALK_GRENADE_MELEE,
    ACT_MP_STAND_ITEM1,
    ACT_MP_CROUCH_ITEM1,
    ACT_MP_RUN_ITEM1,
    ACT_MP_WALK_ITEM1,
    ACT_MP_AIRWALK_ITEM1,
    ACT_MP_CROUCHWALK_ITEM1,
    ACT_MP_JUMP_ITEM1,
    ACT_MP_JUMP_START_ITEM1,
    ACT_MP_JUMP_FLOAT_ITEM1,
    ACT_MP_JUMP_LAND_ITEM1,
    ACT_MP_SWIM_ITEM1,
    ACT_MP_ATTACK_STAND_ITEM1,
    ACT_MP_ATTACK_STAND_ITEM1_SECONDARY,
    ACT_MP_ATTACK_CROUCH_ITEM1,
    ACT_MP_ATTACK_CROUCH_ITEM1_SECONDARY,
    ACT_MP_ATTACK_SWIM_ITEM1,
    ACT_MP_ATTACK_AIRWALK_ITEM1,
    ACT_MP_STAND_ITEM2,
    ACT_MP_CROUCH_ITEM2,
    ACT_MP_RUN_ITEM2,
    ACT_MP_WALK_ITEM2,
    ACT_MP_AIRWALK_ITEM2,
    ACT_MP_CROUCHWALK_ITEM2,
    ACT_MP_JUMP_ITEM2,
    ACT_MP_JUMP_START_ITEM2,
    ACT_MP_JUMP_FLOAT_ITEM2,
    ACT_MP_JUMP_LAND_ITEM2,
    ACT_MP_SWIM_ITEM2,
    ACT_MP_ATTACK_STAND_ITEM2,
    ACT_MP_ATTACK_STAND_ITEM2_SECONDARY,
    ACT_MP_ATTACK_CROUCH_ITEM2,
    ACT_MP_ATTACK_CROUCH_ITEM2_SECONDARY,
    ACT_MP_ATTACK_SWIM_ITEM2,
    ACT_MP_ATTACK_AIRWALK_ITEM2,
    ACT_MP_GESTURE_FLINCH,
    ACT_MP_GESTURE_FLINCH_PRIMARY,
    ACT_MP_GESTURE_FLINCH_SECONDARY,
    ACT_MP_GESTURE_FLINCH_MELEE,
    ACT_MP_GESTURE_FLINCH_ITEM1,
    ACT_MP_GESTURE_FLINCH_ITEM2,
    ACT_MP_GESTURE_FLINCH_HEAD,
    ACT_MP_GESTURE_FLINCH_CHEST,
    ACT_MP_GESTURE_FLINCH_STOMACH,
    ACT_MP_GESTURE_FLINCH_LEFTARM,
    ACT_MP_GESTURE_FLINCH_RIGHTARM,
    ACT_MP_GESTURE_FLINCH_LEFTLEG,
    ACT_MP_GESTURE_FLINCH_RIGHTLEG,
    ACT_MP_GRENADE1_DRAW,
    ACT_MP_GRENADE1_IDLE,
    ACT_MP_GRENADE1_ATTACK,
    ACT_MP_GRENADE2_DRAW,
    ACT_MP_GRENADE2_IDLE,
    ACT_MP_GRENADE2_ATTACK,
    ACT_MP_PRIMARY_GRENADE1_DRAW,
    ACT_MP_PRIMARY_GRENADE1_IDLE,
    ACT_MP_PRIMARY_GRENADE1_ATTACK,
    ACT_MP_PRIMARY_GRENADE2_DRAW,
    ACT_MP_PRIMARY_GRENADE2_IDLE,
    ACT_MP_PRIMARY_GRENADE2_ATTACK,
    ACT_MP_SECONDARY_GRENADE1_DRAW,
    ACT_MP_SECONDARY_GRENADE1_IDLE,
    ACT_MP_SECONDARY_GRENADE1_ATTACK,
    ACT_MP_SECONDARY_GRENADE2_DRAW,
    ACT_MP_SECONDARY_GRENADE2_IDLE,
    ACT_MP_SECONDARY_GRENADE2_ATTACK,
    ACT_MP_MELEE_GRENADE1_DRAW,
    ACT_MP_MELEE_GRENADE1_IDLE,
    ACT_MP_MELEE_GRENADE1_ATTACK,
    ACT_MP_MELEE_GRENADE2_DRAW,
    ACT_MP_MELEE_GRENADE2_IDLE,
    ACT_MP_MELEE_GRENADE2_ATTACK,
    ACT_MP_ITEM1_GRENADE1_DRAW,
    ACT_MP_ITEM1_GRENADE1_IDLE,
    ACT_MP_ITEM1_GRENADE1_ATTACK,
    ACT_MP_ITEM1_GRENADE2_DRAW,
    ACT_MP_ITEM1_GRENADE2_IDLE,
    ACT_MP_ITEM1_GRENADE2_ATTACK,
    ACT_MP_ITEM2_GRENADE1_DRAW,
    ACT_MP_ITEM2_GRENADE1_IDLE,
    ACT_MP_ITEM2_GRENADE1_ATTACK,
    ACT_MP_ITEM2_GRENADE2_DRAW,
    ACT_MP_ITEM2_GRENADE2_IDLE,
    ACT_MP_ITEM2_GRENADE2_ATTACK,
    ACT_MP_STAND_BUILDING,
    ACT_MP_CROUCH_BUILDING,
    ACT_MP_RUN_BUILDING,
    ACT_MP_WALK_BUILDING,
    ACT_MP_AIRWALK_BUILDING,
    ACT_MP_CROUCHWALK_BUILDING,
    ACT_MP_JUMP_BUILDING,
    ACT_MP_JUMP_START_BUILDING,
    ACT_MP_JUMP_FLOAT_BUILDING,
    ACT_MP_JUMP_LAND_BUILDING,
    ACT_MP_SWIM_BUILDING,
    ACT_MP_ATTACK_STAND_BUILDING,
    ACT_MP_ATTACK_CROUCH_BUILDING,
    ACT_MP_ATTACK_SWIM_BUILDING,
    ACT_MP_ATTACK_AIRWALK_BUILDING,
    ACT_MP_ATTACK_STAND_GRENADE_BUILDING,
    ACT_MP_ATTACK_CROUCH_GRENADE_BUILDING,
    ACT_MP_ATTACK_SWIM_GRENADE_BUILDING,
    ACT_MP_ATTACK_AIRWALK_GRENADE_BUILDING,
    ACT_MP_STAND_PDA,
    ACT_MP_CROUCH_PDA,
    ACT_MP_RUN_PDA,
    ACT_MP_WALK_PDA,
    ACT_MP_AIRWALK_PDA,
    ACT_MP_CROUCHWALK_PDA,
    ACT_MP_JUMP_PDA,
    ACT_MP_JUMP_START_PDA,
    ACT_MP_JUMP_FLOAT_PDA,
    ACT_MP_JUMP_LAND_PDA,
    ACT_MP_SWIM_PDA,
    ACT_MP_ATTACK_STAND_PDA,
    ACT_MP_ATTACK_SWIM_PDA,
    ACT_MP_GESTURE_VC_HANDMOUTH,
    ACT_MP_GESTURE_VC_FINGERPOINT,
    ACT_MP_GESTURE_VC_FISTPUMP,
    ACT_MP_GESTURE_VC_THUMBSUP,
    ACT_MP_GESTURE_VC_NODYES,
    ACT_MP_GESTURE_VC_NODNO,
    ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,
    ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,
    ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,
    ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,
    ACT_MP_GESTURE_VC_NODYES_PRIMARY,
    ACT_MP_GESTURE_VC_NODNO_PRIMARY,
    ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY,
    ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY,
    ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY,
    ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY,
    ACT_MP_GESTURE_VC_NODYES_SECONDARY,
    ACT_MP_GESTURE_VC_NODNO_SECONDARY,
    ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,
    ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,
    ACT_MP_GESTURE_VC_FISTPUMP_MELEE,
    ACT_MP_GESTURE_VC_THUMBSUP_MELEE,
    ACT_MP_GESTURE_VC_NODYES_MELEE,
    ACT_MP_GESTURE_VC_NODNO_MELEE,
    ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1,
    ACT_MP_GESTURE_VC_FINGERPOINT_ITEM1,
    ACT_MP_GESTURE_VC_FISTPUMP_ITEM1,
    ACT_MP_GESTURE_VC_THUMBSUP_ITEM1,
    ACT_MP_GESTURE_VC_NODYES_ITEM1,
    ACT_MP_GESTURE_VC_NODNO_ITEM1,
    ACT_MP_GESTURE_VC_HANDMOUTH_ITEM2,
    ACT_MP_GESTURE_VC_FINGERPOINT_ITEM2,
    ACT_MP_GESTURE_VC_FISTPUMP_ITEM2,
    ACT_MP_GESTURE_VC_THUMBSUP_ITEM2,
    ACT_MP_GESTURE_VC_NODYES_ITEM2,
    ACT_MP_GESTURE_VC_NODNO_ITEM2,
    ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,
    ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,
    ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,
    ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,
    ACT_MP_GESTURE_VC_NODYES_BUILDING,
    ACT_MP_GESTURE_VC_NODNO_BUILDING,
    ACT_MP_GESTURE_VC_HANDMOUTH_PDA,
    ACT_MP_GESTURE_VC_FINGERPOINT_PDA,
    ACT_MP_GESTURE_VC_FISTPUMP_PDA,
    ACT_MP_GESTURE_VC_THUMBSUP_PDA,
    ACT_MP_GESTURE_VC_NODYES_PDA,
    ACT_MP_GESTURE_VC_NODNO_PDA,
    ACT_VM_UNUSABLE,
    ACT_VM_UNUSABLE_TO_USABLE,
    ACT_VM_USABLE_TO_UNUSABLE,
    ACT_PRIMARY_VM_DRAW,
    ACT_PRIMARY_VM_HOLSTER,
    ACT_PRIMARY_VM_IDLE,
    ACT_PRIMARY_VM_PULLBACK,
    ACT_PRIMARY_VM_PRIMARYATTACK,
    ACT_PRIMARY_VM_SECONDARYATTACK,
    ACT_PRIMARY_VM_RELOAD,
    ACT_PRIMARY_VM_DRYFIRE,
    ACT_PRIMARY_VM_IDLE_TO_LOWERED,
    ACT_PRIMARY_VM_IDLE_LOWERED,
    ACT_PRIMARY_VM_LOWERED_TO_IDLE,
    ACT_SECONDARY_VM_DRAW,
    ACT_SECONDARY_VM_HOLSTER,
    ACT_SECONDARY_VM_IDLE,
    ACT_SECONDARY_VM_PULLBACK,
    ACT_SECONDARY_VM_PRIMARYATTACK,
    ACT_SECONDARY_VM_SECONDARYATTACK,
    ACT_SECONDARY_VM_RELOAD,
    ACT_SECONDARY_VM_DRYFIRE,
    ACT_SECONDARY_VM_IDLE_TO_LOWERED,
    ACT_SECONDARY_VM_IDLE_LOWERED,
    ACT_SECONDARY_VM_LOWERED_TO_IDLE,
    ACT_MELEE_VM_DRAW,
    ACT_MELEE_VM_HOLSTER,
    ACT_MELEE_VM_IDLE,
    ACT_MELEE_VM_PULLBACK,
    ACT_MELEE_VM_PRIMARYATTACK,
    ACT_MELEE_VM_SECONDARYATTACK,
    ACT_MELEE_VM_RELOAD,
    ACT_MELEE_VM_DRYFIRE,
    ACT_MELEE_VM_IDLE_TO_LOWERED,
    ACT_MELEE_VM_IDLE_LOWERED,
    ACT_MELEE_VM_LOWERED_TO_IDLE,
    ACT_PDA_VM_DRAW,
    ACT_PDA_VM_HOLSTER,
    ACT_PDA_VM_IDLE,
    ACT_PDA_VM_PULLBACK,
    ACT_PDA_VM_PRIMARYATTACK,
    ACT_PDA_VM_SECONDARYATTACK,
    ACT_PDA_VM_RELOAD,
    ACT_PDA_VM_DRYFIRE,
    ACT_PDA_VM_IDLE_TO_LOWERED,
    ACT_PDA_VM_IDLE_LOWERED,
    ACT_PDA_VM_LOWERED_TO_IDLE,
    ACT_ITEM1_VM_DRAW,
    ACT_ITEM1_VM_HOLSTER,
    ACT_ITEM1_VM_IDLE,
    ACT_ITEM1_VM_PULLBACK,
    ACT_ITEM1_VM_PRIMARYATTACK,
    ACT_ITEM1_VM_SECONDARYATTACK,
    ACT_ITEM1_VM_RELOAD,
    ACT_ITEM1_VM_DRYFIRE,
    ACT_ITEM1_VM_IDLE_TO_LOWERED,
    ACT_ITEM1_VM_IDLE_LOWERED,
    ACT_ITEM1_VM_LOWERED_TO_IDLE,
    ACT_ITEM2_VM_DRAW,
    ACT_ITEM2_VM_HOLSTER,
    ACT_ITEM2_VM_IDLE,
    ACT_ITEM2_VM_PULLBACK,
    ACT_ITEM2_VM_PRIMARYATTACK,
    ACT_ITEM2_VM_SECONDARYATTACK,
    ACT_ITEM2_VM_RELOAD,
    ACT_ITEM2_VM_DRYFIRE,
    ACT_ITEM2_VM_IDLE_TO_LOWERED,
    ACT_ITEM2_VM_IDLE_LOWERED,
    ACT_ITEM2_VM_LOWERED_TO_IDLE,
    ACT_RELOAD_SUCCEED,
    ACT_RELOAD_FAIL,
    ACT_WALK_AIM_AUTOGUN,
    ACT_RUN_AIM_AUTOGUN,
    ACT_IDLE_AUTOGUN,
    ACT_IDLE_AIM_AUTOGUN,
    ACT_RELOAD_AUTOGUN,
    ACT_CROUCH_IDLE_AUTOGUN,
    ACT_RANGE_ATTACK_AUTOGUN,
    ACT_JUMP_AUTOGUN,
    ACT_IDLE_AIM_PISTOL,
    ACT_WALK_AIM_DUAL,
    ACT_RUN_AIM_DUAL,
    ACT_IDLE_DUAL,
    ACT_IDLE_AIM_DUAL,
    ACT_RELOAD_DUAL,
    ACT_CROUCH_IDLE_DUAL,
    ACT_RANGE_ATTACK_DUAL,
    ACT_JUMP_DUAL,
    ACT_IDLE_SHOTGUN,
    ACT_IDLE_AIM_SHOTGUN,
    ACT_CROUCH_IDLE_SHOTGUN,
    ACT_JUMP_SHOTGUN,
    ACT_IDLE_AIM_RIFLE,
    ACT_RELOAD_RIFLE,
    ACT_CROUCH_IDLE_RIFLE,
    ACT_RANGE_ATTACK_RIFLE,
    ACT_JUMP_RIFLE,
    ACT_SLEEP,
    ACT_WAKE,
    ACT_FLICK_LEFT,
    ACT_FLICK_LEFT_MIDDLE,
    ACT_FLICK_RIGHT_MIDDLE,
    ACT_FLICK_RIGHT,
    ACT_SPINAROUND,
    ACT_PREP_TO_FIRE,
    ACT_FIRE,
    ACT_FIRE_RECOVER,
    ACT_SPRAY,
    ACT_PREP_EXPLODE,
    ACT_EXPLODE,
    ACT_DOTA_IDLE,
    ACT_DOTA_RUN,
    ACT_DOTA_ATTACK,
    ACT_DOTA_ATTACK_EVENT,
    ACT_DOTA_DIE,
    ACT_DOTA_FLINCH,
    ACT_DOTA_DISABLED,
    ACT_DOTA_CAST_ABILITY_1,
    ACT_DOTA_CAST_ABILITY_2,
    ACT_DOTA_CAST_ABILITY_3,
    ACT_DOTA_CAST_ABILITY_4,
    ACT_DOTA_OVERRIDE_ABILITY_1,
    ACT_DOTA_OVERRIDE_ABILITY_2,
    ACT_DOTA_OVERRIDE_ABILITY_3,
    ACT_DOTA_OVERRIDE_ABILITY_4,
    ACT_DOTA_CHANNEL_ABILITY_1,
    ACT_DOTA_CHANNEL_ABILITY_2,
    ACT_DOTA_CHANNEL_ABILITY_3,
    ACT_DOTA_CHANNEL_ABILITY_4,
    ACT_DOTA_CHANNEL_END_ABILITY_1,
    ACT_DOTA_CHANNEL_END_ABILITY_2,
    ACT_DOTA_CHANNEL_END_ABILITY_3,
    ACT_DOTA_CHANNEL_END_ABILITY_4,
    ACT_MP_RUN_SPEEDPAINT,
    ACT_MP_LONG_FALL,
    ACT_MP_TRACTORBEAM_FLOAT,
    ACT_MP_DEATH_CRUSH,
    ACT_MP_RUN_SPEEDPAINT_PRIMARY,
    ACT_MP_DROWNING_PRIMARY,
    ACT_MP_LONG_FALL_PRIMARY,
    ACT_MP_TRACTORBEAM_FLOAT_PRIMARY,
    ACT_MP_DEATH_CRUSH_PRIMARY,
    ACT_DIE_STAND,
    ACT_DIE_STAND_HEADSHOT,
    ACT_DIE_CROUCH,
    ACT_DIE_CROUCH_HEADSHOT,
    ACT_CSGO_NULL,
    ACT_CSGO_DEFUSE,
    ACT_CSGO_DEFUSE_WITH_KIT,
    ACT_CSGO_FLASHBANG_REACTION,
    ACT_CSGO_FIRE_PRIMARY,
    ACT_CSGO_FIRE_PRIMARY_OPT_1,
    ACT_CSGO_FIRE_PRIMARY_OPT_2,
    ACT_CSGO_FIRE_SECONDARY,
    ACT_CSGO_FIRE_SECONDARY_OPT_1,
    ACT_CSGO_FIRE_SECONDARY_OPT_2,
    ACT_CSGO_RELOAD,
    ACT_CSGO_RELOAD_START,
    ACT_CSGO_RELOAD_LOOP,
    ACT_CSGO_RELOAD_END,
    ACT_CSGO_OPERATE,
    ACT_CSGO_DEPLOY,
    ACT_CSGO_CATCH,
    ACT_CSGO_SILENCER_DETACH,
    ACT_CSGO_SILENCER_ATTACH,
    ACT_CSGO_TWITCH,
    ACT_CSGO_TWITCH_BUYZONE,
    ACT_CSGO_PLANT_BOMB,
    ACT_CSGO_IDLE_TURN_BALANCEADJUST,
    ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING,
    ACT_CSGO_ALIVE_LOOP,
    ACT_CSGO_FLINCH,
    ACT_CSGO_FLINCH_HEAD,
    ACT_CSGO_FLINCH_MOLOTOV,
    ACT_CSGO_JUMP,
    ACT_CSGO_FALL,
    ACT_CSGO_CLIMB_LADDER,
    ACT_CSGO_LAND_LIGHT,
    ACT_CSGO_LAND_HEAVY,
    ACT_CSGO_EXIT_LADDER_TOP,
    ACT_CSGO_EXIT_LADDER_BOTTOM,
};