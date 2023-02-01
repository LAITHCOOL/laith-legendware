// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\cheats\lagcompensation\local_animations.h"
#include "..\..\cheats\misc\prediction_system.h"

_declspec(noinline)bool hooks::setupbones_detour(void* ecx, matrix3x4_t* bone_world_out, int max_bones, int bone_mask, float current_time)
{
    auto result = true;

    static auto r_jiggle_bones = m_cvar()->FindVar(crypt_str("r_jiggle_bones"));
    auto r_jiggle_bones_backup = r_jiggle_bones->GetInt();

    r_jiggle_bones->SetValue(0);

    if (!ecx)
        result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
    else if (!g_cfg.ragebot.enable)
        result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
    else
    {
        auto player = (player_t*)((uintptr_t)ecx - 0x4);

        if (!player->valid(false, false))
            result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
        else
        {
            auto animstate = player->get_animation_state();
            auto previous_weapon = animstate ? animstate->m_pLastBoneSetupWeapon : nullptr;

            if (previous_weapon)
                animstate->m_pLastBoneSetupWeapon = animstate->m_pActiveWeapon; //-V1004

            if (g_ctx.globals.setuping_bones)
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (g_cfg.legitbot.enabled && player != g_ctx.local())
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (!g_ctx.local()->is_alive())
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (player == g_ctx.local())
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (!player->m_CachedBoneData().Count()) //-V807
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (bone_world_out && max_bones != -1)
                memcpy(bone_world_out, player->m_CachedBoneData().Base(), player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

            if (previous_weapon)
                animstate->m_pLastBoneSetupWeapon = previous_weapon;
        }
    }

    r_jiggle_bones->SetValue(r_jiggle_bones_backup);
    return result;
}

bool __fastcall hooks::hooked_setupbones(void* ecx, void* edx, matrix3x4_t* bone_world_out, int max_bones, int bone_mask, float current_time)
{
    return setupbones_detour(ecx, bone_world_out, max_bones, bone_mask, current_time);
}

_declspec(noinline)void hooks::standardblendingrules_detour(player_t* player, int i, CStudioHdr* hdr, Vector* pos, Quaternion* q, float curtime, int boneMask)
{
    if (!player || !player->is_player() || player->IsDormant()) {
        return ((StandardBlendingRulesFn)original_standardblendingrules)(player, hdr, pos, q, curtime, boneMask);
    }

    // disable interpolation on bones.
    *(uint32_t*)((uintptr_t)player + 0xf0) |= 8;

    ((StandardBlendingRulesFn)original_standardblendingrules)(player, hdr, pos, q, curtime, boneMask);

    *(uint32_t*)((uintptr_t)player + 0xf0) &= ~8;

    // disable only on hitbox.
    boneMask |= BONE_USED_BY_HITBOX;

    //================================================================

    auto backup_effects = player->m_fEffects();

    if (player == g_ctx.local())
        player->m_fEffects() |= 8;

    ((StandardBlendingRulesFn)original_standardblendingrules)(player, hdr, pos, q, curtime, boneMask);

    if (player == g_ctx.local())
        player->m_fEffects() = backup_effects;
        
}

void __fastcall hooks::hooked_standardblendingrules(player_t* player, int i, CStudioHdr* hdr, Vector* pos, Quaternion* q, float curtime, int boneMask)
{
    return standardblendingrules_detour(player, i, hdr, pos, q, curtime, boneMask);
}

_declspec(noinline)void hooks::doextrabonesprocessing_detour(player_t* player, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_list, void* context)
{
    // get animstate ptr.
    auto animstate = player->get_animation_state();

    // backup pointer.
    player_t* backup = nullptr;

    // zero out animstate player pointer so CCSGOPlayerAnimState::DoProceduralFootPlant will not do anything.
    if (animstate) {
        // backup player ptr.
        backup = animstate->m_pBaseEntity;

        // null player ptr, GUWOP gang.
        animstate->m_pBaseEntity = nullptr;
    }

    ((DoExtraBonesProcessingFn)original_doextrabonesprocessing)(player, hdr, pos, q, matrix, bone_list, context);

    // restore ptr.
    if (animstate && backup)
        animstate->m_pBaseEntity = backup;
}

void __fastcall hooks::hooked_doextrabonesprocessing(player_t* player, void* edx, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_list, void* context)
{
    return doextrabonesprocessing_detour(player, hdr, pos, q, matrix, bone_list, context);
}

_declspec(noinline)void hooks::updateclientsideanimation_detour(player_t* player)
{
    if (g_ctx.local()->is_alive() && player != nullptr)
    {
        if (g_ctx.globals.updating_animation)
            return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

        if (player == g_ctx.local())
            return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

        if (!g_cfg.ragebot.enable)
            return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

        if (!player->valid(false, false))
            return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);
    }
   
}

void __fastcall hooks::hooked_updateclientsideanimation(player_t* player, uint32_t i)
{
    return updateclientsideanimation_detour(player);
}
_declspec(noinline)void hooks::physicssimulate_detour(player_t* player)
{
    auto m_nSimulationTick = *(int*)((uintptr_t)player + 0x2AC); //get simulation tick
    auto cctx = reinterpret_cast<C_CommandContext*>(uintptr_t(player) + 0x34FC); //GetCommandContext
    auto cctx_srv = reinterpret_cast<CUtlVector<C_CommandContext>*>(uintptr_t(player) + 0x34FC); //GetCommandContext

    if (player != g_ctx.local() || !g_ctx.local()->is_alive() || m_globals()->m_tickcount == m_nSimulationTick || !cctx->needsprocessing || m_engine()->IsPlayingDemo() || m_engine()->IsHLTV() || player->m_fFlags() & 0x40)
    {
        ((PhysicsSimulateFn)original_physicssimulate)(player);
        return;
    }


    //if (!cctx || !cctx->needsprocessing)
    //    return;

    // if invalid tickcount store data
    if (cctx->cmd.m_tickcount == 0x7FFFFFFF)
    {
        // get current record and store data.
        StoredData_t* ndata = &engineprediction::get().m_data[cctx->cmd.m_command_number % MULTIPLAYER_BACKUP];
        engineprediction::get().store(ndata, cctx, cctx->cmd.m_tickcount);
        m_nSimulationTick = m_globals()->m_tickcount;
        cctx->needsprocessing = false;
        return;
    }
    else
    {
        int tickbase = g_ctx.local()->m_nTickBase();

        if (cctx->cmd.m_command_number == g_ctx.globals.shifting_command_number)
            tickbase +=1;
        else  if (cctx->cmd.m_command_number == g_ctx.globals.shifting_command_number + 1)
            tickbase += 1;

        g_ctx.local()->m_nTickBase() = tickbase;


        // get record data and apply
        StoredData_t* data = &engineprediction::get().m_data[(cctx->cmd.m_command_number - 1) % MULTIPLAYER_BACKUP];
        if (data && data->m_command_number == (cctx->cmd.m_command_number - 1) && std::abs(data->m_command_number - cctx->cmd.m_command_number) <= MULTIPLAYER_BACKUP)
            engineprediction::get().apply(cctx->cmd.m_command_number - 1);

        // fix velocity modifier to update
        if (cctx->cmd.m_command_number == (m_clientstate()->iCommandAck + 1) && engine_prediction::m_misc.m_bOverrideModifier)
            player->m_flVelocityModifier() = engine_prediction::m_stored_variables.m_flVelocityModifier;

        // detect prediction error and repredict
        if (data && data->m_command_number == (cctx->cmd.m_command_number - 1) && std::abs(data->m_command_number - cctx->cmd.m_command_number) <= MULTIPLAYER_BACKUP)
            engineprediction::get().detect_prediction_error(data, cctx->cmd.m_command_number - 1);

        // call original
        ((PhysicsSimulateFn)original_physicssimulate)(player);

        // get current record and store data.
        StoredData_t* ndata = &engineprediction::get().m_data[cctx->cmd.m_command_number % MULTIPLAYER_BACKUP];
        engineprediction::get().store(ndata, cctx, m_globals()->m_tickcount);
    }
}


void __fastcall hooks::hooked_physicssimulate(player_t* player)
{
    return physicssimulate_detour(player);
}

_declspec(noinline)void hooks::modifyeyeposition_detour(c_baseplayeranimationstate* state, Vector& position)
{
    if (state && g_ctx.globals.in_createmove)
        return ((ModifyEyePositionFn)original_modifyeyeposition)(state, position);
}

void __fastcall hooks::hooked_modifyeyeposition(c_baseplayeranimationstate* state, void* edx, Vector& position)
{
    return modifyeyeposition_detour(state, position);
}

_declspec(noinline)void hooks::calcviewmodelbob_detour(player_t* player, Vector& position)
{
    if (!g_cfg.esp.removals[REMOVALS_LANDING_BOB] || player != g_ctx.local() || !g_ctx.local()->is_alive())
        return ((CalcViewmodelBobFn)original_calcviewmodelbob)(player, position);
}

void __fastcall hooks::hooked_calcviewmodelbob(player_t* player, void* edx, Vector& position)
{
    return calcviewmodelbob_detour(player, position);
}

bool __fastcall hooks::hooked_shouldskipanimframe()
{
    return false;
}

int hooks::processinterpolatedlist()
{
    static auto allow_extrapolation = *(bool**)(util::FindSignature(crypt_str("client.dll"), crypt_str("A2 ? ? ? ? 8B 45 E8")) + 0x1);

    if (allow_extrapolation)
        *allow_extrapolation = false;

    return ((ProcessInterpolatedListFn)original_processinterpolatedlist)();
}

using ClampBonesInBBox_t = void(__thiscall*) (void*, matrix3x4_t*, int);
void __fastcall hooks::ClampBonesInBBox(player_t* player, void* edx, matrix3x4_t* matrix, int mask)
{
    if (g_ctx.globals.setuping_bones)
        ((ClampBonesInBBox_t)original_oClampBonesInBBox)(player, matrix, mask);
    
}