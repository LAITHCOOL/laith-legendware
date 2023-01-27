#pragma once
#include "..\..\includes.hpp"

class weapon_info_t;
class weapon_t;

struct FireBulletData_t
{
    Vector            vecPosition = { };
    Vector            vecDirection = { };
    trace_t            enterTrace = { };
    float            flCurrentDamage = 0.0f;
    int                iPenetrateCount = 0;
    bool visible;
    int hitbox;
    int damage;
};

// @credits: outlassn
//@credits : flengo for converting it to lw
class CAutoWall
{
public:
    // Get
    /* returns damage at point and simulated bullet data (if given) */
    static float GetDamage(const Vector& eye_pos, player_t*, const Vector& vecPoint, FireBulletData_t* pDataOut = nullptr);
    /* calculates damage factor */
    static void ScaleDamage(const int iHitGroup, player_t* pEntity, const float flWeaponArmorRatio, const float flWeaponHeadShotMultiplier, float& flDamage);
    /* simulates fire bullet to penetrate up to 4 walls, return true when hitting player */
    static bool SimulateFireBullet(player_t* pLocal, weapon_t* pWeapon, FireBulletData_t& data);
    static bool IsBreakableEntity(IClientEntity* e);
    static bool HandleBulletPenetration(player_t* pLocal, weapon_info_t* pWeaponData, const surfacedata_t* pEnterSurfaceData, FireBulletData_t& data);
    static bool handle_bullet_penetration_lw(weapon_info_t* weaponData, CGameTrace& enterTrace, Vector& eyePosition, const Vector& direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, bool draw_impact = false);

private:
    // Main
    static void ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, const unsigned int fMask, ITraceFilter* pFilter, trace_t* pTrace, const float flMinRange = 0.0f);
    static bool TraceToExit(trace_t& enterTrace, trace_t& exitTrace, const Vector& vecPosition, const Vector& vecDirection, player_t* pClipPlayer);
};