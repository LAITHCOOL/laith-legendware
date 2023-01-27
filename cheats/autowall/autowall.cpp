#include "autowall.h"
#include "..\misc\logs.h"

//qo0 Autowall for LW

bool CAutoWall::IsBreakableEntity(IClientEntity* e)
{
    if (!e || !e->EntIndex())
        return false;

    static auto is_breakable = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 56 8B F1 85 F6 74 68"));

    auto take_damage = *(uintptr_t*)((uintptr_t)is_breakable + 0x26);
    auto backup = *(uint8_t*)((uintptr_t)e + take_damage);

    auto client_class = e->GetClientClass();
    auto network_name = client_class->m_pNetworkName;

    if (!strcmp(network_name, "CBreakableSurface"))
        *(uint8_t*)((uintptr_t)e + take_damage) = DAMAGE_YES;
    else if (!strcmp(network_name, "CBaseDoor") || !strcmp(network_name, "CDynamicProp"))
        *(uint8_t*)((uintptr_t)e + take_damage) = DAMAGE_NO;

    using Fn = bool(__thiscall*)(IClientEntity*);
    auto result = ((Fn)is_breakable)(e);


    *(uint8_t*)((uintptr_t)e + take_damage) = backup;
    return result;
}

float CAutoWall::GetDamage(const Vector& eye_pos, player_t* pLocal, const Vector& vecPoint, FireBulletData_t* pDataOut)
{
    g_ctx.globals.autowalling = true;
    Vector vecPosition = g_ctx.globals.eye_pos;

    auto tmp = vecPoint - eye_pos;

    auto angles = ZERO;
    math::vector_angles(tmp, angles);

    auto direction = ZERO;
    math::angle_vectors(angles, direction);

    direction.NormalizeInPlace();

    FireBulletData_t data = { };
    data.visible = true;
    data.vecPosition = vecPosition;
    data.vecDirection = direction;

    auto pWeapon = g_ctx.local()->m_hActiveWeapon().Get();

    if (pWeapon == nullptr || !SimulateFireBullet(pLocal, pWeapon, data)) {
        g_ctx.globals.autowalling = false;
        return -1.0f;
    }


    if (pDataOut != nullptr)
        *pDataOut = data;

    g_ctx.globals.autowalling = false;

    return data.flCurrentDamage;
}

void CAutoWall::ScaleDamage(const int iHitGroup, player_t* pEntity, const float flWeaponArmorRatio, const float flWeaponHeadShotMultiplier, float& flDamage)
{
    // [USER=409548]@ida[/USER] traceattack: server.dll @ 55 8B EC 83 E4 F8 81 EC C0 00 00 00 56 8B 75 08 57 8B F9 C6 44 24 13 01

    const bool bHeavyArmor = pEntity->m_bHasHeavyArmor();

    static auto mp_damage_scale_ct_head = m_cvar()->FindVar(crypt_str("mp_damage_scale_ct_head")); //-V807
    static auto mp_damage_scale_t_head = m_cvar()->FindVar(crypt_str("mp_damage_scale_t_head"));

    static auto mp_damage_scale_ct_body = m_cvar()->FindVar(crypt_str("mp_damage_scale_ct_body"));
    static auto mp_damage_scale_t_body = m_cvar()->FindVar(crypt_str("mp_damage_scale_t_body"));

    float flHeadDamageScale = pEntity->m_iTeamNum() == 3 ? mp_damage_scale_ct_head->GetFloat() : pEntity->m_iTeamNum() == 2 ? mp_damage_scale_t_head->GetFloat() : 1.0f;
    const float flBodyDamageScale = pEntity->m_iTeamNum() == 3 ? mp_damage_scale_ct_body->GetFloat() : pEntity->m_iTeamNum() == 2 ? mp_damage_scale_t_body->GetFloat() : 1.0f;

    if (bHeavyArmor)
        flHeadDamageScale *= 0.5f;

    switch (iHitGroup)
    {
    case HITGROUP_HEAD:
        flDamage *= flWeaponHeadShotMultiplier * flHeadDamageScale;
        break;
    case HITGROUP_CHEST:
    case HITGROUP_LEFTARM:
    case HITGROUP_RIGHTARM:
    case HITGROUP_NECK:
        flDamage *= flBodyDamageScale;
        break;
    case HITGROUP_STOMACH:
        flDamage *= 1.25f * flBodyDamageScale;
        break;
    case HITGROUP_LEFTLEG:
    case HITGROUP_RIGHTLEG:
        flDamage *= 0.75f * flBodyDamageScale;
        break;
    default:
        break;
    }

    if (pEntity->IsArmored(iHitGroup))
    {
        // [USER=409548]@ida[/USER] ontakedamage: server.dll @ 80 BF ? ? ? ? ? F3 0F 10 5C 24 ? F3 0F 10 35

        const int iArmor = pEntity->m_ArmorValue();
        float flHeavyArmorBonus = 1.0f, flArmorBonus = 0.5f, flArmorRatio = flWeaponArmorRatio * 0.5f;

        if (bHeavyArmor)
        {
            flHeavyArmorBonus = 0.25f;
            flArmorBonus = 0.33f;
            flArmorRatio *= 0.20f;
        }

        float flDamageToHealth = flDamage * flArmorRatio;
        if (const float flDamageToArmor = (flDamage - flDamageToHealth) * (flHeavyArmorBonus * flArmorBonus); flDamageToArmor > static_cast<float>(iArmor))
            flDamageToHealth = flDamage - static_cast<float>(iArmor) / flArmorBonus;

        flDamage = flDamageToHealth;
    }
}



// @credits: https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/util_shared.cpp#L757
void CAutoWall::ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, const unsigned int fMask, ITraceFilter* pFilter, trace_t* pTrace, const float flMinRange)
{
    // [USER=409548]@ida[/USER] util_cliptracetoplayers: client.dll @ E8 ? ? ? ? 0F 28 84 24 68 02 00 00

    trace_t trace = { };
    float flSmallestFraction = pTrace->fraction;

    const Ray_t ray(vecAbsStart, vecAbsEnd);

    for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
    {
        auto pEntity = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

        if (pEntity == nullptr || !pEntity->is_alive() || pEntity->IsDormant())
            continue;

        if (pFilter != nullptr && !pFilter->ShouldHitEntity(pEntity, fMask))
            continue;

        const ICollideable* pCollideable = pEntity->GetCollideable();

        if (pCollideable == nullptr)
            continue;

        // get bounding box
        const Vector vecMin = pCollideable->OBBMins();
        const Vector vecMax = pCollideable->OBBMaxs();

        // calculate world space center
        const Vector vecCenter = (vecMax + vecMin) * 0.5f;
        const Vector vecPosition = vecCenter + pEntity->m_vecOrigin();

        const Vector vecTo = vecPosition - vecAbsStart;
        Vector vecDirection = vecAbsEnd - vecAbsStart;
        const float flLength = vecDirection.NormalizeInPlace1();

        const float flRangeAlong = vecDirection.Dot(vecTo);
        float flRange = 0.0f;

        // calculate distance to ray
        if (flRangeAlong < 0.0f)
            // off start point
            flRange = -vecTo.Length();
        else if (flRangeAlong > flLength)
            // off end point
            flRange = -(vecPosition - vecAbsEnd).Length();
        else
            // within ray bounds
            flRange = (vecPosition - (vecDirection * flRangeAlong + vecAbsStart)).Length();

        constexpr float flMaxRange = 60.f;
        if (flRange < flMinRange || flRange > flMaxRange)
            continue;

        m_trace()->ClipRayToEntity(ray, fMask | CONTENTS_HITBOX, pEntity, &trace);


        if (trace.fraction < flSmallestFraction)
        {
            // we shortened the ray - save off the trace
            *pTrace = trace;
            flSmallestFraction = trace.fraction;
        }
    }
}

bool CAutoWall::TraceToExit(trace_t& enterTrace, trace_t& exitTrace, const Vector& vecPosition, const Vector& vecDirection, player_t* pClipPlayer)
{
    // [USER=409548]@ida[/USER] tracetoexit: client.dll @ 55 8B EC 83 EC 4C F3
    // server.dll @ 55 8B EC 83 EC 4C F3 0F 10 75

    float flDistance = 0.0f;
    int iStartContents = 0;

    while (flDistance <= 90.0f)
    {
        // add extra distance to our ray
        flDistance += 4.0f;

        // multiply the direction vector to the distance so we go outwards, add our position to it
        Vector vecStart = vecPosition + vecDirection * flDistance;


        if (!iStartContents)
            iStartContents = m_trace()->GetPointContents(vecStart, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr);

        const int iCurrentContents = m_trace()->GetPointContents(vecStart, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr);

        if (!(iCurrentContents & MASK_SHOT_HULL) || (iCurrentContents & CONTENTS_HITBOX && iCurrentContents != iStartContents))
        {
            // setup our end position by deducting the direction by the extra added distance
            const Vector vecEnd = vecStart - (vecDirection * 4.0f);

            // trace ray to world
            Ray_t rayWorld(vecStart, vecEnd);
            m_trace()->TraceRay(rayWorld, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr, &exitTrace);

            if (static auto sv_clip_penetration_traces_to_players = m_cvar()->FindVar(crypt_str("sv_clip_penetration_traces_to_players")); sv_clip_penetration_traces_to_players != nullptr && sv_clip_penetration_traces_to_players->GetBool())
            {
                CTraceFilter filter;

                filter.pSkip = pClipPlayer;

                ClipTraceToPlayers(vecEnd, vecStart, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &exitTrace, -60.f);
            }

            // check if a hitbox is in-front of our enemy and if they are behind of a solid wall
            if (exitTrace.startsolid && exitTrace.surface.flags & SURF_HITBOX)
            {
                // trace ray to entity
                Ray_t ray(vecStart, vecPosition);
                CTraceFilter filter;
                auto ent = static_cast<player_t*>(m_entitylist()->GetClientEntity(exitTrace.hit_entity->EntIndex()));

                m_trace()->TraceRay(ray, MASK_SHOT_HULL, &filter, &exitTrace);

                if (exitTrace.DidHit() && !exitTrace.startsolid)
                {
                    vecStart = exitTrace.endpos;
                    return true;
                }

                continue;
            }
            if (exitTrace.DidHit() && !exitTrace.startsolid)
            {

                if (IsBreakableEntity(exitTrace.hit_entity) && IsBreakableEntity(enterTrace.hit_entity))
                    return true;

                if (enterTrace.surface.flags & SURF_NODRAW || (!(exitTrace.surface.flags & SURF_NODRAW) && exitTrace.plane.normal.Dot(vecDirection) <= 1.0f))
                {
                    const float flMultiplier = exitTrace.fraction * 4.0f;
                    vecStart -= vecDirection * flMultiplier;
                    return true;
                }

                continue;
            }

            if (!exitTrace.DidHit() || exitTrace.startsolid)
            {
                if (enterTrace.hit_entity != nullptr && enterTrace.hit_entity->EntIndex() != 0 && IsBreakableEntity(enterTrace.hit_entity))
                {
                    // did hit breakable non world entity
                    exitTrace = enterTrace;
                    exitTrace.endpos = vecStart + vecDirection;
                    return true;
                }

                continue;
            }
        }
    }

    return false;
}

bool CAutoWall::HandleBulletPenetration(player_t* pLocal, weapon_info_t* pWeaponData, const surfacedata_t* pEnterSurfaceData, FireBulletData_t& data)
{
    // [USER=409548]@ida[/USER] handlebulletpenetration: client.dll @ E8 ? ? ? ? 83 C4 40 84 C0

    static auto ff_damage_reduction_bullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
    static auto ff_damage_bullet_penetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

    const float flReductionDamage = ff_damage_reduction_bullets->GetFloat();
    const float flPenetrateDamage = ff_damage_bullet_penetration->GetFloat();

    const MaterialHandle_t hEnterMaterial = pEnterSurfaceData->game.material;

    if (data.iPenetrateCount == 0 && hEnterMaterial != CHAR_TEX_GRATE && hEnterMaterial != CHAR_TEX_GLASS && !(data.enterTrace.surface.flags & SURF_NODRAW))
        return false;

    if (pWeaponData->flPenetration <= 0.0f || data.iPenetrateCount <= 0)
        return false;

    trace_t exitTrace = { };
    if (!TraceToExit(data.enterTrace, exitTrace, data.enterTrace.endpos, data.vecDirection, pLocal) && !(m_trace()->GetPointContents(data.enterTrace.endpos, MASK_SHOT_HULL, nullptr) & MASK_SHOT_HULL))
        return false;

    const surfacedata_t* pExitSurfaceData = m_physsurface()->GetSurfaceData(exitTrace.surface.surfaceProps);
    const MaterialHandle_t hExitMaterial = pExitSurfaceData->game.material;

    const float flEnterPenetrationModifier = pEnterSurfaceData->game.flPenetrationModifier;
    const float flExitPenetrationModifier = pExitSurfaceData->game.flPenetrationModifier;

    float flDamageLostModifier = 0.16f;
    float flPenetrationModifier = 0.0f;

    auto player123 = static_cast<player_t*>(m_entitylist()->GetClientEntity(data.enterTrace.hit_entity->EntIndex()));

    if (hEnterMaterial == CHAR_TEX_GRATE || hEnterMaterial == CHAR_TEX_GLASS)
    {
        flDamageLostModifier = 0.05f;
        flPenetrationModifier = 3.0f;
    }
    else if (((data.enterTrace.contents >> 3) & CONTENTS_SOLID) || ((data.enterTrace.surface.flags >> 7) & SURF_LIGHT))
    {
        flDamageLostModifier = 0.16f;
        flPenetrationModifier = 1.0f;
    }
    else if (hEnterMaterial == CHAR_TEX_FLESH && flReductionDamage == 0.0f && data.enterTrace.hit_entity != nullptr && player123->is_player() && (pLocal->m_iTeamNum() == player123->m_iTeamNum()))
    {
        if (flPenetrateDamage == 0.0f)
            return false;

        // shoot through teammates
        flDamageLostModifier = flPenetrateDamage;
        flPenetrationModifier = flPenetrateDamage;
    }
    else
    {
        flDamageLostModifier = 0.16f;
        flPenetrationModifier = (flEnterPenetrationModifier + flExitPenetrationModifier) * 0.5f;
    }

    if (hEnterMaterial == hExitMaterial)
    {
        if (hExitMaterial == CHAR_TEX_CARDBOARD || hExitMaterial == CHAR_TEX_WOOD)
            flPenetrationModifier = 3.0f;
        else if (hExitMaterial == CHAR_TEX_PLASTIC)
            flPenetrationModifier = 2.0f;
    }

    const float flTraceDistance = (exitTrace.endpos - data.enterTrace.endpos).LengthSqr();

    // penetration modifier
    const float flModifier = (flPenetrationModifier > 0.0f ? 1.0f / flPenetrationModifier : 0.0f);

    // this calculates how much damage we've lost depending on thickness of the wall, our penetration, damage, and the modifiers set earlier
    const float flLostDamage = (data.flCurrentDamage * flDamageLostModifier + (pWeaponData->flPenetration > 0.0f ? 3.75f / pWeaponData->flPenetration : 0.0f) * (flModifier * 3.0f)) + ((flModifier * flTraceDistance) / 24.0f);

    // did we loose too much damage?
    if (flLostDamage > data.flCurrentDamage)
        return false;

    // we can't use any of the damage that we've lost
    if (flLostDamage > 0.0f)
        data.flCurrentDamage -= flLostDamage;

    // do we still have enough damage to deal?
    if (data.flCurrentDamage < 1.0f)
        return false;

    data.vecPosition = exitTrace.endpos;
    --data.iPenetrateCount;
    return true;
}

bool CAutoWall::SimulateFireBullet(player_t* pLocal, weapon_t* pWeapon, FireBulletData_t& data)
{
    // [USER=409548]@ida[/USER] firebullet: client.dll @ 55 8B EC 83 E4 F0 81 EC ? ? ? ? F3 0F 7E

    auto pWeaponData = pWeapon->get_csweapon_info();

    if (pWeaponData == nullptr)
        return false;

    float flMaxRange = pWeaponData->flRange;

    // the total number of surfaces any bullet can penetrate in a single flight is capped at 4
    data.iPenetrateCount = 4;
    // set our current damage to what our gun's initial damage reports it will do
    data.flCurrentDamage = static_cast<float>(pWeaponData->iDamage);

    float flTraceLenght = 0.0f;
    CTraceFilter filter;

    filter.pSkip = g_ctx.local();

    while (data.iPenetrateCount > 0 && data.flCurrentDamage >= 1.0f)
    {
        // max bullet range
        flMaxRange -= flTraceLenght;

        // end position of bullet
        const Vector vecEnd = data.vecPosition + data.vecDirection * flMaxRange;

        Ray_t ray(data.vecPosition, vecEnd);
        m_trace()->TraceRay(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &data.enterTrace);

        // check for player hitboxes extending outside their collision bounds
        ClipTraceToPlayers(data.vecPosition, vecEnd + data.vecDirection * 40.0f, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &data.enterTrace);

        const surfacedata_t* pEnterSurfaceData = m_physsurface()->GetSurfaceData(data.enterTrace.surface.surfaceProps);
        const float flEnterPenetrationModifier = pEnterSurfaceData->game.flPenetrationModifier;

        // we didn't hit anything, stop tracing shoot
        if (data.enterTrace.fraction == 1.0f)
            break;

        // calculate the damage based on the distance the bullet traveled
        flTraceLenght += data.enterTrace.fraction * flMaxRange;
        data.flCurrentDamage *= std::powf(pWeaponData->flRangeModifier, flTraceLenght / 500.f);

        // check is actually can shoot through
        if (flTraceLenght > 3000.f || flEnterPenetrationModifier < 0.1f)
            break;

        // check is can do damage

        auto hitentity_index = data.enterTrace.hit_entity->EntIndex();
        auto entity_by_index = static_cast<player_t*>(m_entitylist()->GetClientEntity(hitentity_index));
        auto is_enemy = entity_by_index->m_iTeamNum() != g_ctx.local()->m_iTeamNum();
        if (data.enterTrace.hitgroup != HITGROUP_GENERIC && data.enterTrace.hitgroup != HITGROUP_GEAR && is_enemy)
        {
            // we got target - scale damage

            ScaleDamage(data.enterTrace.hitgroup, entity_by_index, pWeaponData->flArmorRatio, pWeaponData->headshotmultyplrier, data.flCurrentDamage);
            data.hitbox = data.enterTrace.hitbox;
            return true;
        }

        // calling handlebulletpenetration here reduces our penetration ñounter, and if it returns true, we can't shoot through it
        if (!HandleBulletPenetration(pLocal, pWeaponData, pEnterSurfaceData, data)) {
            break;
        }


        data.visible = false;


    }



    return false;
}

//def lw handle bullet penetration is used only for other esp(i know its so laughy but idc)
bool CAutoWall::handle_bullet_penetration_lw(weapon_info_t* weaponData, CGameTrace& enterTrace, Vector& eyePosition, const Vector& direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, bool draw_impact)
{
    if (weaponData->flPenetration <= 0.0f)
        return false;

    if (possibleHitsRemaining <= 0)
        return false;

    auto contents_grate = enterTrace.contents & CONTENTS_GRATE;
    auto surf_nodraw = enterTrace.surface.flags & SURF_NODRAW;

    auto enterSurfaceData = m_physsurface()->GetSurfaceData(enterTrace.surface.surfaceProps);
    auto enter_material = enterSurfaceData->game.material;

    auto is_solid_surf = enterTrace.contents >> 3 & CONTENTS_SOLID;
    auto is_light_surf = enterTrace.surface.flags >> 7 & SURF_LIGHT;

    trace_t exit_trace;

    if (!TraceToExit(enterTrace, exit_trace, enterTrace.endpos, direction, g_ctx.local()) && !(m_trace()->GetPointContents(enterTrace.endpos, MASK_SHOT_HULL) & MASK_SHOT_HULL))
        return false;

    auto enter_penetration_modifier = enterSurfaceData->game.flPenetrationModifier;
    auto exit_surface_data = m_physsurface()->GetSurfaceData(exit_trace.surface.surfaceProps);

    if (!exit_surface_data)
        return false;

    auto exit_material = exit_surface_data->game.material;
    auto exit_penetration_modifier = exit_surface_data->game.flPenetrationModifier;

    auto combined_damage_modifier = 0.16f;
    auto combined_penetration_modifier = (enter_penetration_modifier + exit_penetration_modifier) * 0.5f;

    if (enter_material == CHAR_TEX_GLASS || enter_material == CHAR_TEX_GRATE)
    {
        combined_penetration_modifier = 3.0f;
        combined_damage_modifier = 0.05f;
    }
    else if (contents_grate || surf_nodraw)
        combined_penetration_modifier = 1.0f;
    else if (enter_material == CHAR_TEX_FLESH && ((player_t*)enterTrace.hit_entity)->m_iTeamNum() == g_ctx.local()->m_iTeamNum() && !ff_damage_reduction_bullets)
    {
        if (!ff_damage_bullet_penetration) //-V550
            return false;

        combined_penetration_modifier = ff_damage_bullet_penetration;
        combined_damage_modifier = 0.16f;
    }

    if (enter_material == exit_material)
    {
        if (exit_material == CHAR_TEX_WOOD || exit_material == CHAR_TEX_CARDBOARD)
            combined_penetration_modifier = 3.0f;
        else if (exit_material == CHAR_TEX_PLASTIC)
            combined_penetration_modifier = 2.0f;
    }

    auto penetration_modifier = std::fmaxf(0.0f, 1.0f / combined_penetration_modifier);
    auto penetration_distance = (exit_trace.endpos - enterTrace.endpos).Length();

    penetration_distance = penetration_distance * penetration_distance * penetration_modifier * 0.041666668f;

    auto damage_modifier = max(0.0f, 3.0f / weaponData->flPenetration * 1.25f) * penetration_modifier * 3.0f + currentDamage * combined_damage_modifier + penetration_distance;
    auto damage_lost = max(0.0f, damage_modifier);

    if (damage_lost > currentDamage)
        return false;

    currentDamage -= damage_lost;

    if (currentDamage < 1.0f)
        return false;

    eyePosition = exit_trace.endpos;
    --possibleHitsRemaining;

    return true;
}