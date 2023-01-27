// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\cheats\misc\fakelag.h"

IMaterial* CreateRagdollMaterial(bool lit, const std::string& material_data)
{
    static auto created = 0;
    std::string type = lit ? crypt_str("VertexLitGeneric") : crypt_str("UnlitGeneric");

    auto matname = crypt_str("n0n4m3_") + std::to_string(created);
    ++created;

    auto keyValues = new KeyValues(matname.c_str());
    static auto key_values_address = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 33 C0 C7 45"));

    using KeyValuesFn = void(__thiscall*)(void*, const char*);
    reinterpret_cast <KeyValuesFn> (key_values_address)(keyValues, type.c_str());

    static auto load_from_buffer_address = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89"));
    using LoadFromBufferFn = void(__thiscall*)(void*, const char*, const char*, void*, const char*, void*);

    reinterpret_cast <LoadFromBufferFn> (load_from_buffer_address)(keyValues, matname.c_str(), material_data.c_str(), nullptr, nullptr, nullptr);

    auto material = m_materialsystem()->CreateMaterial(matname.c_str(), keyValues);
    material->IncrementReferenceCount();

    return material;
}

using SceneEnd_t = void(__thiscall*)(void*);

void __fastcall hooks::hooked_sceneend(void* ecx, void* edx)
{
    static auto original_fn = renderview_hook->get_func_address <SceneEnd_t>(9);
    g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true); //-V807

    if (!g_ctx.local())
        return original_fn(ecx);

    if (!g_cfg.player.type[ENEMY].ragdoll_chams && !g_cfg.player.type[TEAM].ragdoll_chams)
        return original_fn(ecx);

    for (auto i = 1; i <= m_entitylist()->GetHighestEntityIndex(); i++)
    {
        auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

        if (!e)
            continue;

        if (((player_t*)e)->m_lifeState() == LIFE_ALIVE)
            continue;

        auto client_class = e->GetClientClass();

        if (!client_class)
            continue;

        if (client_class->m_ClassID != CCSRagdoll)
            continue;

        auto type = ENEMY;

        if (e->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
            type = TEAM;

        if (!g_cfg.player.type[type].ragdoll_chams)
            continue;

        static IMaterial* materials[] =
        {
            m_materialsystem()->FindMaterial(crypt_str("itzlaith_lw_chams"), nullptr), //-V807
            m_materialsystem()->FindMaterial(crypt_str("itzlaith_lw_metallic"), nullptr),
            m_materialsystem()->FindMaterial(crypt_str("itzlaith_lw_flat"), nullptr),
            m_materialsystem()->FindMaterial(crypt_str("models/inventory_items/dogtags/dogtags_outline"), nullptr),
            m_materialsystem()->FindMaterial(crypt_str("models/inventory_items/trophy_majors/crystal_clear"), nullptr),
            m_materialsystem()->FindMaterial(crypt_str("models/inventory_items/cologne_prediction/cologne_prediction_glass"), nullptr),
            m_materialsystem()->FindMaterial(crypt_str("dev/glow_armsrace.vmt"), nullptr),
            m_materialsystem()->FindMaterial(crypt_str("models/inventory_items/wildfire_gold/wildfire_gold_detail"), nullptr),
            m_materialsystem()->FindMaterial(crypt_str("itzlaith_lw_wifeframe"), nullptr)
        };

        auto material = materials[g_cfg.player.type[type].ragdoll_chams_material];

        if (material && !material->IsErrorMaterial())
        {
            auto alpha = (float)g_cfg.player.type[type].ragdoll_chams_color.a() / 255.0f;

            float ragdoll_color[3] =
            {
                g_cfg.player.type[type].ragdoll_chams_color[0] / 255.0f,
                g_cfg.player.type[type].ragdoll_chams_color[1] / 255.0f,
                g_cfg.player.type[type].ragdoll_chams_color[2] / 255.0f
            };

            m_renderview()->SetBlend(alpha);
            util::color_modulate(ragdoll_color, material);

            material->IncrementReferenceCount();
            material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

            m_modelrender()->ForcedMaterialOverride(material);
            e->DrawModel(0x1, 255);
            m_modelrender()->ForcedMaterialOverride(nullptr);
        }
    }

    return original_fn(ecx);
}