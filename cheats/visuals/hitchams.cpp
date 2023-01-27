#include "hitchams.h"

void hit_chams::add_matrix(player_t* player, matrix3x4_t* bones)
{
    auto& hit = m_Hitmatrix.emplace_back();

    std::memcpy(hit.pBoneToWorld, bones, player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
    hit.time = m_globals()->m_realtime + 2.5f;

    static int m_nSkin = util::find_in_datamap(player->GetPredDescMap(), crypt_str("m_nSkin"));
    static int m_nBody = util::find_in_datamap(player->GetPredDescMap(), crypt_str("m_nBody"));

    hit.info.origin = player->GetAbsOrigin();
    hit.info.angles = player->GetAbsAngles();

    auto renderable = player->GetClientRenderable();

    if (!renderable)
        return;

    auto model = player->GetModel();

    if (!model)
        return;

    auto hdr = *(studiohdr_t**)(player->m_pStudioHdr());

    if (!hdr)
        return;

    hit.state.m_pStudioHdr = hdr;
    hit.state.m_pStudioHWData = m_modelcache()->GetHardwareData(model->studio);
    hit.state.m_pRenderable = renderable;
    hit.state.m_drawFlags = 0;

    hit.info.pRenderable = renderable;
    hit.info.pModel = model;
    hit.info.pLightingOffset = nullptr;
    hit.info.pLightingOrigin = nullptr;
    hit.info.hitboxset = player->m_nHitboxSet();
    hit.info.skin = (int)(uintptr_t(player) + m_nSkin);
    hit.info.body = (int)(uintptr_t(player) + m_nBody);
    hit.info.entity_index = player->EntIndex();
    hit.info.instance = call_virtual<ModelInstanceHandle_t(__thiscall*)(void*) >(renderable, 30u)(renderable);
    hit.info.flags = 0x1;

    hit.info.pModelToWorld = &hit.model_to_world;
    hit.state.m_pModelToWorld = &hit.model_to_world;

    hit.model_to_world.AngleMatrix(hit.info.angles, hit.info.origin);
}