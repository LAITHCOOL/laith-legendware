// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "misc.h"
#include "fakelag.h"
#include "..\ragebot\aim.h"
#include "..\visuals\world_esp.h"
#include "prediction_system.h"
#include "logs.h"
#include "..\visuals\hitchams.h"
#include "../menu_alpha.h"

#define ALPHA (ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar| ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)
#define NOALPHA (ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)

void misc::EnableHiddenCVars()
{
	auto p = **reinterpret_cast<ConCommandBase***>(reinterpret_cast<DWORD>(m_cvar()) + 0x34);

	for (auto c = p->m_pNext; c != nullptr; c = c->m_pNext) {
		c->m_nFlags &= ~FCVAR_DEVELOPMENTONLY; // FCVAR_DEVELOPMENTONLY
		c->m_nFlags &= ~FCVAR_HIDDEN; // FCVAR_HIDDEN
	}
}

bool draw_water_button(const char* label, const char* label_id, bool load, bool save, int curr_config, bool create = false)
{
	bool pressed = false;
	if (ImGui::PlusButton(label, 0, ImVec2(10, 10), label_id, ImColor(40 / 255.f, (40) / 255.f, (40) / 255.f), true));

	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_PopupBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_PopupRounding, 4);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(33 / 255.f, 33 / 255.f, 33 / 255.f, g_ctx.gui.pop_anim * 0.85f));

	if (ImGui::BeginPopup(label_id, ImGuiWindowFlags_NoMove))
	{
		ImGui::SetNextItemWidth(min(g_ctx.gui.pop_anim, 0.01f) * ImGui::GetFrameHeight() * 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_ctx.gui.pop_anim);
		ImGui::SetCursorPosX(8); ImGui::Text("Color"); ImGui::SameLine();
		ImGui::ColorEdit(("##color"), &g_cfg.menu.menu_theme, ALPHA);
		//draw_multicombo(crypt_str("Watermark additives"), g_cfg.menu.watermark_additives, watermark_adder, ARRAYSIZE(watermark_adder), c_menu::get().preview);
		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar(2);    ImGui::PopStyleColor(1);
	return pressed;
}

#define VERSION crypt_str("itzlaith_lw.uwu | ")

void misc::watermark()
{
	if (!g_cfg.menu.watermark)
		return;

	ImVec2 p, s;

	auto net_channel = m_engine()->GetNetChannelInfo();

	std::string watermark = VERSION;
	char buff[MAX_PATH];

	watermark += GetEnvironmentVariableA("USERNAME", buff, MAX_PATH);

	if (m_engine()->IsInGame())
	{
		auto nci = m_engine()->GetNetChannelInfo();

		if (nci)
		{
			auto server = nci->GetAddress();

			if (!strcmp(server, crypt_str("loopback")))
				server = crypt_str("local server");
			else if (m_gamerules()->m_bIsValveDS())
				server = crypt_str("valve server");

			auto tickrate = std::to_string((int)(1.0f / m_globals()->m_intervalpertick));

			watermark += crypt_str(" | ");
			watermark += server;

			watermark += crypt_str(" | ") + std::to_string(g_ctx.globals.ping) + crypt_str(" ms");
		}
	}
	else
	{
		watermark += crypt_str(" | no connection");
	}

	watermark += crypt_str(" | ") + std::to_string(g_ctx.globals.framerate) + crypt_str(" fps");

	ImGui::PushFont(c_menu::get().font);
	auto size_text = ImGui::CalcTextSize(watermark.c_str());
	ImGui::PopFont();
	ImGui::SetNextWindowSize(ImVec2(size_text.x + (hooks::menu_open ? 24 : 14), 20));

	static bool seted = true;

	if (seted)
		seted = false;

	if (seted)
		ImGui::SetNextWindowPos(ImVec2(200, 200));

	ImGui::Begin("wwwwwwww", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNav);
	{
		auto d = ImGui::GetWindowDrawList();
		p = ImGui::GetWindowPos();
		s = ImGui::GetWindowSize();
		ImGui::PushFont(c_menu::get().font);
		ImGui::SetWindowSize(ImVec2(s.x, 21 + 18));
		//
		d->AddRectFilled(p, p + ImVec2(s.x, 21), ImColor(39, 39, 39, int(50 * 1)));
		auto main_colf = ImColor(39, 39, 39, int(255 * 1));
		auto main_coll = ImColor(39, 39, 39, int(140 * 1));
		d->AddRectFilledMultiColor(p, p + ImVec2(s.x / 2, 20), main_coll, main_colf, main_colf, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(s.x / 2, 0), p + ImVec2(s.x, 20), main_colf, main_coll, main_coll, main_colf);
		//
		auto main_colf2 = ImColor(39, 39, 39, int(255 * min(1 * 2, 1.f)));
		d->AddRectFilledMultiColor(p, p + ImVec2(s.x / 2, 20), main_coll, main_colf2, main_colf2, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(s.x / 2, 0), p + ImVec2(s.x, 20), main_colf2, main_coll, main_coll, main_colf2);
		auto line_colf = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255);
		auto line_coll = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255);
		d->AddRectFilledMultiColor(p, p + ImVec2(s.x / 2, 2), line_coll, line_colf, line_colf, line_coll);
		d->AddRectFilledMultiColor(p + ImVec2(s.x / 2, 0), p + ImVec2(s.x, 2), line_colf, line_coll, line_coll, line_colf);
		d->AddText(p + ImVec2((hooks::menu_open ? s.x - 10 : s.x) / 2 - size_text.x / 2, (20) / 2 - size_text.y / 2), ImColor(250, 250, 250, int(230 * min(1 * 3, 1.f))), watermark.c_str());

		ImGui::SetCursorPos(ImVec2(s.x - 15, 5));
		if (hooks::menu_open)
			draw_water_button("+", "Swatermark_sett", false, false, NULL, false);

	}
	ImGui::End();
}

void misc::NoDuck(CUserCmd* cmd)
{
	if (!g_cfg.misc.noduck)
		return;

	if (m_gamerules()->m_bIsValveDS())
		return;

	cmd->m_buttons |= IN_BULLRUSH;
}

void misc::ChatSpammer()
{
	if (!g_cfg.misc.chat)
		return;

	static std::string chatspam[] =
	{
		crypt_str("itzlaith_lw - good cheat for the game."),
		crypt_str("Get good. Get itzlaith_lw."),
		crypt_str("itzlaith_lw - better than its competitors."),
		crypt_str("Go to another level with itzlaith_lw."),
	};

	static auto lastspammed = 0;

	if (GetTickCount() - lastspammed > 800)
	{
		lastspammed = GetTickCount();

		srand(m_globals()->m_tickcount);
		std::string msg = crypt_str("say ") + chatspam[rand() % 4];

		m_engine()->ExecuteClientCmd(msg.c_str());
	}
}

void misc::AutoCrouch(CUserCmd* cmd)
{
	if (fakelag::get().condition)
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (m_gamerules()->m_bIsValveDS())
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!key_binds::get().get_key_bind_state(20))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands != 7)
		return;

	if (m_clientstate()->iChokedCommands >= 7)
		cmd->m_buttons |= IN_DUCK;
	else
		cmd->m_buttons &= ~IN_DUCK;

	g_ctx.globals.fakeducking = true;
}

void misc::SlideWalk(CUserCmd* cmd)
{
	if (!g_ctx.local()->is_alive()) //-V807
		return;

	if (g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (antiaim::get().condition(cmd, true) && (g_cfg.misc.legs_movement == 2 && math::random_int(0, 2) == 0 || g_cfg.misc.legs_movement == 1))
	{
		if (cmd->m_forwardmove > 0.0f)
		{
			cmd->m_buttons |= IN_BACK;
			cmd->m_buttons &= ~IN_FORWARD;
		}
		else if (cmd->m_forwardmove < 0.0f)
		{
			cmd->m_buttons |= IN_FORWARD;
			cmd->m_buttons &= ~IN_BACK;
		}

		if (cmd->m_sidemove > 0.0f)
		{
			cmd->m_buttons |= IN_MOVELEFT;
			cmd->m_buttons &= ~IN_MOVERIGHT;
		}
		else if (cmd->m_sidemove < 0.0f)
		{
			cmd->m_buttons |= IN_MOVERIGHT;
			cmd->m_buttons &= ~IN_MOVELEFT;
		}
	}
	else
	{
		auto buttons = cmd->m_buttons & ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD);

		if (g_cfg.misc.legs_movement == 2 && math::random_int(0, 1) || g_cfg.misc.legs_movement == 1)
		{
			if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
				return;

			if (cmd->m_forwardmove <= 0.0f)
				buttons |= IN_BACK;
			else
				buttons |= IN_FORWARD;

			if (cmd->m_sidemove > 0.0f)
				goto LABEL_15;
			else if (cmd->m_sidemove >= 0.0f)
				goto LABEL_18;

			goto LABEL_17;
		}
		else
			goto LABEL_18;

		if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
			return;

		if (cmd->m_forwardmove <= 0.0f) //-V779
			buttons |= IN_FORWARD;
		else
			buttons |= IN_BACK;

		if (cmd->m_sidemove > 0.0f)
		{
		LABEL_17:
			buttons |= IN_MOVELEFT;
			goto LABEL_18;
		}

		if (cmd->m_sidemove < 0.0f)
			LABEL_15:

		buttons |= IN_MOVERIGHT;

	LABEL_18:
		cmd->m_buttons = buttons;
	}
}

void misc::automatic_peek(CUserCmd* cmd, float wish_yaw)
{
	if (!g_ctx.globals.weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
	{
		if (g_ctx.globals.start_position.IsZero())
		{
			g_ctx.globals.start_position = g_ctx.local()->GetAbsOrigin();

			if (!(engineprediction::get().backup_data.flags & FL_ONGROUND))
			{
				Ray_t ray;
				CTraceFilterWorldAndPropsOnly filter;
				CGameTrace trace;

				ray.Init(g_ctx.globals.start_position, g_ctx.globals.start_position - Vector(0.0f, 0.0f, 1000.0f));
				m_trace()->TraceRay(ray, MASK_SOLID, &filter, &trace);

				if (trace.fraction < 1.0f)
					g_ctx.globals.start_position = trace.endpos + Vector(0.0f, 0.0f, 2.0f);
			}
		}
		else
		{
			auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (cmd->m_buttons & IN_ATTACK || cmd->m_buttons & IN_ATTACK2);

			if (cmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
				g_ctx.globals.fired_shot = true;

			if (g_ctx.globals.fired_shot)
			{
				auto &current_position = g_ctx.local()->GetAbsOrigin();
				auto difference = current_position - g_ctx.globals.start_position;

				if (difference.Length2D() > 5.0f)
				{
					auto velocity = Vector(difference.x * cos(wish_yaw / 180.0f * M_PI) + difference.y * sin(wish_yaw / 180.0f * M_PI), difference.y * cos(wish_yaw / 180.0f * M_PI) - difference.x * sin(wish_yaw / 180.0f * M_PI), difference.z);

					cmd->m_forwardmove = -velocity.x * 20.0f;
					cmd->m_sidemove = velocity.y * 20.0f;
				}
				else
				{
					g_ctx.globals.fired_shot = false;
					g_ctx.globals.start_position.Zero();
				}
			}
		}
	}
	else
	{
		g_ctx.globals.fired_shot = false;
		g_ctx.globals.start_position.Zero();
	}
}

void misc::ViewModel()
{
	if (g_cfg.esp.viewmodel_fov)
	{
		auto viewFOV = (float)g_cfg.esp.viewmodel_fov + 68.0f;
		static auto viewFOVcvar = m_cvar()->FindVar(crypt_str("viewmodel_fov"));

		if (viewFOVcvar->GetFloat() != viewFOV) //-V550
		{
			*(float*)((DWORD)&viewFOVcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewFOVcvar->SetValue(viewFOV);
		}
	}

	if (g_cfg.esp.viewmodel_x)
	{
		auto viewX = (float)g_cfg.esp.viewmodel_x / 2.0f;
		static auto viewXcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_x")); //-V807

		if (viewXcvar->GetFloat() != viewX) //-V550
		{
			*(float*)((DWORD)&viewXcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewXcvar->SetValue(viewX);
		}
	}

	if (g_cfg.esp.viewmodel_y)
	{
		auto viewY = (float)g_cfg.esp.viewmodel_y / 2.0f;
		static auto viewYcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_y"));

		if (viewYcvar->GetFloat() != viewY) //-V550
		{
			*(float*)((DWORD)&viewYcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewYcvar->SetValue(viewY);
		}
	}

	if (g_cfg.esp.viewmodel_z)
	{
		auto viewZ = (float)g_cfg.esp.viewmodel_z / 2.0f;
		static auto viewZcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_z"));

		if (viewZcvar->GetFloat() != viewZ) //-V550
		{
			*(float*)((DWORD)&viewZcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewZcvar->SetValue(viewZ);
		}
	}
}

void misc::FullBright()
{
	if (!g_cfg.player.enable)
		return;

	static auto mat_fullbright = m_cvar()->FindVar(crypt_str("mat_fullbright"));

	if (mat_fullbright->GetBool() != g_cfg.esp.bright)
		mat_fullbright->SetValue(g_cfg.esp.bright);
}

void misc::DrawGray()
{
	if (!g_cfg.player.enable)
		return;

	static auto mat_drawgray = m_cvar()->FindVar(crypt_str("mat_drawgray"));

	if (mat_drawgray->GetBool() != g_cfg.esp.drawgray)
		mat_drawgray->SetValue(g_cfg.esp.drawgray);
}

void misc::PovArrows(player_t* e, Color color)
{
	auto isOnScreen = [](Vector origin, Vector& screen) -> bool
	{
		if (!math::world_to_screen(origin, screen))
			return false;

		static int iScreenWidth, iScreenHeight;
		m_engine()->GetScreenSize(iScreenWidth, iScreenHeight);

		auto xOk = iScreenWidth > screen.x;
		auto yOk = iScreenHeight > screen.y;

		return xOk && yOk;
	};

	Vector screenPos;

	if (isOnScreen(e->GetAbsOrigin(), screenPos))
		return;

	Vector viewAngles;
	m_engine()->GetViewAngles(viewAngles);

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto screenCenter = Vector2D(width * 0.5f, height * 0.5f);
	auto angleYawRad = DEG2RAD(viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);

	auto radius = g_cfg.player.distance;
	auto size = g_cfg.player.size;

	auto newPointX = screenCenter.x + ((((width - (size * 4)) * 0.5f) * (radius / 100.0f)) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
	auto newPointY = screenCenter.y + ((((height - (size * 4)) * 0.5f) * (radius / 100.0f)) * sin(angleYawRad));

	std::array <Vector2D, 3> points
	{
		Vector2D(newPointX - size, newPointY - size),
		Vector2D(newPointX + size, newPointY),
		Vector2D(newPointX - size, newPointY + size)
	};

	math::rotate_triangle(points, viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);
	render::get().triangle(points.at(0), points.at(1), points.at(2), color);
}

void misc::zeus_range()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.taser_range)
		return;

	//if (!m_input()->m_fCameraInThirdPerson)
	//	return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	if (weapon->m_iItemDefinitionIndex() != WEAPON_TASER)
		return;

	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	float circle_range = weapon_info->flRange / 3;

	auto draw_pos = g_ctx.local()->get_shoot_position();

	draw_pos.z -= 54;
	render::get().Draw3DCircle(draw_pos, circle_range, Color(g_cfg.esp.zeus_color.r(), g_cfg.esp.zeus_color.g(), g_cfg.esp.zeus_color.b(), 255));

	if (g_cfg.misc.zeusparty) 
	{
		if (!g_ctx.local()->is_alive() || !g_ctx.local()->is_player())
			return;
		static auto sv_party_mode = m_cvar()->FindVar(("sv_party_mode"));
		sv_party_mode->SetValue(1);
	}
	else if (!g_cfg.misc.zeusparty) 
	{
		if (!g_ctx.local()->is_alive() || !g_ctx.local()->is_player())
			return;
		static auto sv_party_mode = m_cvar()->FindVar(("sv_party_mode"));
		sv_party_mode->SetValue(0);
	}
}

void misc::NightmodeFix()
{
	static auto in_game = false;

	if (m_engine()->IsInGame() && !in_game)
	{
		in_game = true;

		g_ctx.globals.change_materials = true;
		worldesp::get().changed = true;

		static auto skybox = m_cvar()->FindVar(crypt_str("sv_skyname"));
		worldesp::get().backup_skybox = skybox->GetString();
		return;
	}
	else if (!m_engine()->IsInGame() && in_game)
		in_game = false;

	static auto player_enable = g_cfg.player.enable;

	if (player_enable != g_cfg.player.enable)
	{
		player_enable = g_cfg.player.enable;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting = g_cfg.esp.nightmode;

	if (setting != g_cfg.esp.nightmode)
	{
		setting = g_cfg.esp.nightmode;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting_world = g_cfg.esp.world_color;

	if (setting_world != g_cfg.esp.world_color)
	{
		setting_world = g_cfg.esp.world_color;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting_props = g_cfg.esp.props_color;

	if (setting_props != g_cfg.esp.props_color)
	{
		setting_props = g_cfg.esp.props_color;
		g_ctx.globals.change_materials = true;
	}
}

void misc::desync_arrows()
{
	if (!g_ctx.local()->is_alive())
		return;

	if (!g_cfg.ragebot.enable)
		return;

	if (!g_cfg.antiaim.enable)
		return;

	if ((g_cfg.antiaim.manual_back.key <= KEY_NONE || g_cfg.antiaim.manual_back.key >= KEY_MAX) && (g_cfg.antiaim.manual_left.key <= KEY_NONE || g_cfg.antiaim.manual_left.key >= KEY_MAX) && (g_cfg.antiaim.manual_right.key <= KEY_NONE || g_cfg.antiaim.manual_right.key >= KEY_MAX))
		antiaim::get().manual_side = SIDE_NONE;

	if (!g_cfg.antiaim.flip_indicator)
		return;

	bool on = g_cfg.antiaim.flip_indicator;

	if (g_ctx.globals.scoped && g_ctx.globals.weapon->is_sniper() != on == false)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto color = g_cfg.antiaim.flip_indicator_color;

	if (antiaim::get().manual_side == SIDE_LEFT)
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), color);
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), Color(55, 55, 55, 189));
	}
	else if (antiaim::get().manual_side == SIDE_RIGHT)
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), Color(55, 55, 55, 189));
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), color);
	}
	else
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), Color(55, 55, 55, 189));
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), Color(55, 55, 55, 189));
	}
}

void misc::aimbot_hitboxes()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.player.lag_hitbox)
		return;

	auto player = (player_t*)m_entitylist()->GetClientEntity(aim::get().last_target_index);

	if (!player)
		return;

	auto model = player->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto hitbox_set = studio_model->pHitboxSet(player->m_nHitboxSet());

	if (!hitbox_set)
		return;

	hit_chams::get().add_matrix(player, aim::get().last_target[aim::get().last_target_index].record.matrixes_data.main);
}

void misc::ragdolls()
{
	if (!g_cfg.misc.ragdolls)
		return;

	for (auto i = 1; i <= m_entitylist()->GetHighestEntityIndex(); ++i)
	{
		auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->IsDormant())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class)
			continue;

		if (client_class->m_ClassID != CCSRagdoll)
			continue;

		auto ragdoll = (ragdoll_t*)e;
		ragdoll->m_vecForce().z = 800000.0f;
	}
}

void misc::rank_reveal()
{
	if (!g_cfg.misc.rank_reveal)
		return;

	using RankReveal_t = bool(__cdecl*)(int*);
	static auto Fn = (RankReveal_t)(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 A1 ? ? ? ? 85 C0 75 37")));

	int array[3] =
	{
		0,
		0,
		0
	};

	Fn(array);
}

void misc::fast_stop(CUserCmd* m_pcmd)
{
	if (!g_cfg.misc.fast_stop)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	auto pressed_move_key = m_pcmd->m_buttons & IN_FORWARD || m_pcmd->m_buttons & IN_MOVELEFT || m_pcmd->m_buttons & IN_BACK || m_pcmd->m_buttons & IN_MOVERIGHT || m_pcmd->m_buttons & IN_JUMP;

	if (pressed_move_key)
		return;

	if (!((antiaim::get().type == ANTIAIM_LEGIT ? g_cfg.antiaim.desync : g_cfg.antiaim.type[antiaim::get().type].desync) && (antiaim::get().type == ANTIAIM_LEGIT ? !g_cfg.antiaim.legit_lby_type : !g_cfg.antiaim.lby_type) && (!g_ctx.globals.weapon->is_grenade() || g_cfg.esp.on_click && ~(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2))) || antiaim::get().condition(m_pcmd)) //-V648
	{
		auto &velocity = g_ctx.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;
		}
	}
	else
	{
		auto &velocity = g_ctx.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;
		}
		else
		{
			auto speed = 1.01f;

			if (m_pcmd->m_buttons & IN_DUCK || g_ctx.globals.fakeducking)
				speed *= 2.94117647f;

			static auto switch_move = false;

			if (switch_move)
				m_pcmd->m_sidemove += speed;
			else
				m_pcmd->m_sidemove -= speed;

			switch_move = !switch_move;
		}
	}
}

void misc::spectators_list()
{
	//LPDIRECT3DTEXTURE9 photo[32];
	int specs = 0;
	int id[32];
	int modes = 0;
	std::string spect = "";
	std::string mode = "";

	if (m_engine()->IsInGame() && m_engine()->IsConnected())
	{
		int localIndex = m_engine()->GetLocalPlayer();
		player_t* pLocalEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(localIndex));
		if (pLocalEntity)
		{
			for (int i = 0; i < m_engine()->GetMaxClients(); i++)
			{
				player_t* pBaseEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(i));
				if (!pBaseEntity)
					continue;
				if (pBaseEntity->m_iHealth() > 0)
					continue;
				if (pBaseEntity == pLocalEntity)
					continue;
				if (pBaseEntity->IsDormant())
					continue;
				if (pBaseEntity->m_hObserverTarget() != pLocalEntity)
					continue;

				player_info_t pInfo;
				m_engine()->GetPlayerInfo(pBaseEntity->EntIndex(), &pInfo);
				if (pInfo.ishltv)
					continue;
				spect += pInfo.szName;
				spect += "\n";
				specs++;
				id[i] = pInfo.steamID64;
				// photo[i] = c_menu::get().steam_image(CSteamID((uint64)pInfo.steamID64));
				switch (pBaseEntity->m_iObserverMode())
				{
				case OBS_MODE_IN_EYE:
					mode += "Perspective";
					break;
				case OBS_MODE_CHASE:
					mode += "3rd Person";
					break;
				case OBS_MODE_ROAMING:
					mode += "No Clip";
					break;
				case OBS_MODE_DEATHCAM:
					mode += "Deathcam";
					break;
				case OBS_MODE_FREEZECAM:
					mode += "Freezecam";
					break;
				case OBS_MODE_FIXED:
					mode += "Fixed";
					break;
				default:
					break;
				}

				mode += "\n";
				modes++;

			}
		}
	}

	static float main_alpha = 0.f;
	if (!g_cfg.misc.spectators_list)
		return;

	if (hooks::menu_open || specs > 0) {
		if (main_alpha < 1)
			main_alpha += 5 * ImGui::GetIO().DeltaTime;
	}
	else
		if (main_alpha > 0)
			main_alpha -= 5 * ImGui::GetIO().DeltaTime;

	if (main_alpha < 0.05f)
		return;

	ImVec2 p, s;

	ImGui::Begin("SPECTATORS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNav);
	{
		auto d = ImGui::GetWindowDrawList();
		int think_size = 200;
		int calced_size = think_size - 5;

		p = ImGui::GetWindowPos();
		s = ImGui::GetWindowSize();
		ImGui::SetWindowSize(ImVec2(200, 31 + 20 * specs));
		ImGui::PushFont(c_menu::get().font);

		//d->AddRectFilled(p, p + ImVec2(200, 21 + 20 * specs), ImColor(39, 39, 39, int(50 * main_alpha)));
		auto main_colf = ImColor(39, 39, 39, int(255 * main_alpha));
		auto main_coll = ImColor(39, 39, 39, int(140 * main_alpha));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf, main_colf, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf, main_coll, main_coll, main_colf);

		auto main_colf2 = ImColor(39, 39, 39, int(140 * min(main_alpha * 2, 1.f)));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf2, main_colf2, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf2, main_coll, main_coll, main_colf2);
		auto line_colf = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), int(200 * min(main_alpha * 2, 1.f)));
		auto line_coll = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), int(255 * min(main_alpha * 2, 1.f)));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 2), line_coll, line_colf, line_colf, line_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 2), line_colf, line_coll, line_coll, line_colf);
		d->AddText(p + ImVec2((200) / 2 - ImGui::CalcTextSize("spectators").x / 2, (20) / 2 - ImGui::CalcTextSize("spectators").y / 2), ImColor(250, 250, 250, int(230 * min(main_alpha * 3, 1.f))), "spectators");
		if (specs > 0) spect += "\n";
		if (modes > 0) mode += "\n";

		ImGui::SetCursorPosY(22);
		if (m_engine()->IsInGame() && m_engine()->IsConnected())
		{
			int localIndex = m_engine()->GetLocalPlayer();
			player_t* pLocalEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(localIndex));
			if (pLocalEntity)
			{
				for (int i = 0; i < m_engine()->GetMaxClients(); i++)
				{
					player_t* pBaseEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(i));
					if (!pBaseEntity)
						continue;
					if (pBaseEntity == pLocalEntity)
						continue;
					if (pBaseEntity->m_hObserverTarget() != pLocalEntity)
						continue;

					player_info_t pInfo;
					m_engine()->GetPlayerInfo(pBaseEntity->EntIndex(), &pInfo);
					if (pInfo.ishltv)
						continue;
					ImGui::SetCursorPosX(8);
					ImGui::Text(pInfo.szName);
					if (pInfo.fakeplayer)
					{
						//ImGui::SameLine(-1, s.x - 22);
						//ImGui::Image(getAvatarTexture(pBaseEntity->m_iTeamNum()), ImVec2(15, 15), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.f, 1.f, 1.f, min(main_alpha * 2, 1.f)));
					}
					else
					{
						//ImGui::SameLine(-1, s.x - 22);
						//ImGui::Image(c_menu::get().steam_image(CSteamID((uint64)pInfo.steamID64)), ImVec2(15, 15), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.f, 1.f, 1.f, min(main_alpha * 2, 1.f)));
					}

				}
			}
		}

		ImGui::PopFont();
	}
	ImGui::End();
}
enum key_bind_num
{
	_AUTOFIRE,
	_LEGITBOT,
	_DOUBLETAP,
	_SAFEPOINT,
	_MIN_DAMAGE,
	_ANTI_BACKSHOT = 12,
	_M_BACK,
	_M_LEFT,
	_M_RIGHT,
	_DESYNC_FLIP,
	_THIRDPERSON,
	_AUTO_PEEK,
	_EDGE_JUMP,
	_FAKEDUCK,
	_SLOWWALK,
	_BODY_AIM,
	_RAGEBOT,
	_TRIGGERBOT,
	_L_RESOLVER_OVERRIDE,
	_FAKE_PEEK,
};
void misc::key_binds()
{
	static float main_alpha = 0.f;

	bool rage = g_cfg.ragebot.enable, legit = g_cfg.legitbot.enabled, aa = g_cfg.antiaim.enable, vis = g_cfg.player.enable;

	int pressed_binds = 0;

	static float keys_alpha[15] = {};
	if (key_binds::get().get_key_bind_state(_RAGEBOT) && rage) pressed_binds++;
	if (g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key > KEY_NONE && g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key < KEY_MAX && key_binds::get().get_key_bind_state(_MIN_DAMAGE) && rage) pressed_binds++;
	if (key_binds::get().get_key_bind_state(_LEGITBOT) && legit)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_AUTOFIRE) && legit)pressed_binds++;
	if (g_cfg.ragebot.double_tap && misc::get().double_tap_key || key_binds::get().get_key_bind_state(_DOUBLETAP) && rage)pressed_binds++;
	if (g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key && rage)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_BODY_AIM) && rage)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_SAFEPOINT) && rage)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_THIRDPERSON) && vis)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_DESYNC_FLIP) && aa)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_FAKEDUCK) && aa)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_AUTO_PEEK))pressed_binds++;
	if (hooks::menu_open) pressed_binds++;

	key_bind gui;
	gui.mode = (key_bind_mode)(1);

	if (!g_cfg.misc.keybinds)
		return;

	if (hooks::menu_open || pressed_binds > 0) {
		if (main_alpha < 1)
			main_alpha += 5 * ImGui::GetIO().DeltaTime;
	}
	else
		if (main_alpha > 0)
			main_alpha -= 5 * ImGui::GetIO().DeltaTime;

	if (main_alpha < 0.05f)
		return;

	ImVec2 p, s;

	ImGui::Begin("KEYS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNav);
	{
		auto d = ImGui::GetWindowDrawList();
		p = ImGui::GetWindowPos();
		s = ImGui::GetWindowSize();
		ImGui::PushFont(c_menu::get().font);
		ImGui::SetWindowSize(ImVec2(200, 31 + 18 * (m_engine()->IsConnected() ? pressed_binds : hooks::menu_open ? 1 : 0)));

		//d->AddRectFilled(p, p + ImVec2(200, 21 + 18 * pressed_binds), ImColor(39, 39, 39, int(50 * main_alpha)));
		auto main_colf = ImColor(39, 39, 39, int(255 * main_alpha));
		auto main_coll = ImColor(39, 39, 39, int(140 * main_alpha));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf, main_colf, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf, main_coll, main_coll, main_colf);

		auto main_colf2 = ImColor(39, 39, 39, int(140 * min(main_alpha * 2, 1.f)));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf2, main_colf2, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf2, main_coll, main_coll, main_colf2);

		auto line_colf = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255);
		auto line_coll = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255);
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 2), line_coll, line_colf, line_colf, line_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 2), line_colf, line_coll, line_coll, line_colf);
		d->AddText(p + ImVec2((200) / 2 - ImGui::CalcTextSize("keybinds").x / 2, (20) / 2 - ImGui::CalcTextSize("keybinds").y / 2), ImColor(250, 250, 250, int(230 * min(main_alpha * 3, 1.f))), "keybinds");

		ImGui::SetCursorPosY(20 + 2);
		ImGui::BeginGroup();//keys 
		add_key("menu open", hooks::menu_open, gui, 200, keys_alpha[12], true);
		if (m_engine()->IsConnected())
		{
			add_key("minimum damage", key_binds::get().get_key_bind_state(_MIN_DAMAGE), g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key, 200, min(main_alpha * 2, 1.f), rage, true);
			add_key("legitbot key", key_binds::get().get_key_bind_state(_LEGITBOT), g_cfg.legitbot.key, 200, min(main_alpha * 2, 1.f), legit);
			add_key("trigger-bot", key_binds::get().get_key_bind_state(_AUTOFIRE), g_cfg.legitbot.autofire_key, 200, min(main_alpha * 2, 1.f), legit);
			add_key("double-tap", g_cfg.ragebot.double_tap && misc::get().double_tap_key || misc::get().double_tap_checkc || key_binds::get().get_key_bind_state(_DOUBLETAP), g_cfg.ragebot.double_tap_key, 200, min(main_alpha * 2, 1.f), rage, false);
			add_key("hide-shots", g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX&& misc::get().hide_shots_key, g_cfg.antiaim.hide_shots_key, 200, min(main_alpha * 2, 1.f), rage&& aa, false);
			add_key("force body-aim", key_binds::get().get_key_bind_state(_BODY_AIM), g_cfg.ragebot.body_aim_key, 200, min(main_alpha * 2, 1.f), rage);
			add_key("force safe-point", key_binds::get().get_key_bind_state(_SAFEPOINT), g_cfg.ragebot.safe_point_key, 200, min(main_alpha * 2, 1.f), rage);
			add_key("thirdperson", key_binds::get().get_key_bind_state(_THIRDPERSON), g_cfg.misc.thirdperson_toggle, 200, min(main_alpha * 2, 1.f), vis);
			add_key("desync flip", key_binds::get().get_key_bind_state(_DESYNC_FLIP), g_cfg.antiaim.flip_desync, 200, min(main_alpha * 2, 1.f), aa);
			add_key("fake-duck", key_binds::get().get_key_bind_state(_FAKEDUCK), g_cfg.misc.fakeduck_key, 200, min(main_alpha * 2, 1.f), rage && aa);
			add_key("automatic peek", key_binds::get().get_key_bind_state(_AUTO_PEEK), g_cfg.misc.automatic_peek, 200, min(main_alpha * 2, 1.f), true);
			add_key("edge jump", key_binds::get().get_key_bind_state(_EDGE_JUMP), g_cfg.misc.edge_jump, 200, min(main_alpha * 2, 1.f), true);
		}
		ImGui::EndGroup();
		ImGui::PopFont();
	}
	ImGui::End();
}

void misc::double_tap_deffensive(CUserCmd* cmd)
{
	bool did_peek = false;
	auto predicted_eye_pos = g_ctx.globals.eye_pos + engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * 6.f;
	bool SkipTick = false;
	if (antiaim::get().type != ANTIAIM_STAND)
	{
		for (auto i = 1; i < m_globals()->m_maxclients; i++)
		{
			auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));
			if (!e->valid(true))
				continue;

			if (!e || i == -1)
				continue;

			auto records = &player_records[i];
			if (records->empty())
				continue;

			auto record = &records->front();
			if (!record->valid())
				continue;

			scan_data predicted_data;
			aim::get().scan(record, predicted_data, predicted_eye_pos);

			if (!predicted_data.valid())
			{
				scan_data data;
				aim::get().scan(record, data, g_ctx.globals.eye_pos);

				if (!data.valid())
					continue;
				if (data.damage > 0)
					did_peek = true;
			}

			if (predicted_data.damage > 0)
				did_peek = true;
			else
				did_peek = false;

			if (did_peek)
			{
				g_ctx.send_packet = true;
				SkipTick = true;
				g_cfg.ragebot.defensive_doubletap = true;
				did_peek = false;
			}

			if (!g_cfg.ragebot.defensive_doubletap && !SkipTick)
			{
				if (m_clientstate()->pNetChannel->m_nChokedPackets < 15)
					g_ctx.send_packet = false;
				else
					g_cfg.ragebot.defensive_doubletap = false;
			}

		}
	}
		
	

	if (!g_cfg.ragebot.defensive_doubletap)
	{
		float shift_time = 15 * m_globals()->m_intervalpertick;
		if ((cmd->m_buttons && IN_ATTACK || IN_ATTACK2) && g_ctx.globals.weapon->m_flNextPrimaryAttack() <= m_globals()->m_curtime - shift_time)
		{
			g_ctx.globals.tickbase_shift = 15;
			g_ctx.globals.shifting_command_number = cmd->m_command_number;
			g_cfg.antiaim.triggers_fakelag_amount = true;
		}

	}

	for (auto i = 0; i < g_ctx.globals.tickbase_shift; i++)
	{
		auto command = m_input()->GetUserCmd(cmd[i].m_command_number);
		auto v8 = m_input()->GetVerifiedUserCmd(cmd[i].m_command_number);

		memcpy(command, &cmd[i], sizeof(CUserCmd));

		bool v6 = command->m_tickcount == 0x7F7FFFFF;
		command->m_predicted = v6;

		v8->m_cmd = *command;
		v8->m_crc = command->GetChecksum();

		++m_clientstate()->pNetChannel->m_nChokedPackets;
		++m_clientstate()->pNetChannel->m_nOutSequenceNr;
		++m_clientstate()->iChokedCommands;
	}
}





void misc::lagexploit(CUserCmd* m_pcmd)
{
	bool did_peek = false;
	//auto predicted_eye_pos = g_ctx.globals.eye_pos + engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * 6.f;
	Vector predicted_eye_pos = g_ctx.globals.eye_pos + (engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * 4.f);


	if (antiaim::get().type == ANTIAIM_STAND)
	{
		did_peek = false;
		return;
	}

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));
		if (!e->valid(true))
			continue;

		if (!e || i == -1)
			continue;

		auto records = &player_records[i];
		if (records->empty())
			continue;

		auto record = &records->front();
		if (!record->valid())
			continue;

		FireBulletData_t fire_data = { };

		fire_data.damage = CAutoWall::GetDamage(predicted_eye_pos, g_ctx.local(), e->hitbox_position_matrix(HITBOX_HEAD, record->matrixes_data.main), &fire_data);

		if ( fire_data.damage < 1)
			continue;

		did_peek = true;

	}

	if (did_peek)
	{
		g_ctx.globals.tickbase_shift = 13; // break lc
		g_ctx.globals.shifting_command_number = m_pcmd->m_command_number; // used for tickbase fix 
		did_peek = false;
	}
}



void misc::DoubleTap(CUserCmd* m_pcmd)
{
	double_tap_enabled = true;
	//vars
	auto shiftAmount = g_cfg.ragebot.shift_amount;
	float shiftTime = shiftAmount * m_globals()->m_intervalpertick;
	float recharge_time = TIME_TO_TICKS(g_cfg.ragebot.recharge_time);
	auto weapon = g_ctx.local()->m_hActiveWeapon();

	g_ctx.globals.tickbase_shift = shiftAmount;

	//Check if we can doubletap
	if (!CanDoubleTap(false))
		return;

	//Fix for doubletap hitchance
	if (g_ctx.globals.dt_shots == 1) {
		g_ctx.globals.dt_shots = 0;
	}

	//Recharge
	if (!aim::get().should_stop && !(m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife()) && !util::is_button_down(MOUSE_LEFT) && g_ctx.globals.tocharge < shiftAmount && (m_pcmd->m_command_number - lastdoubletaptime) > recharge_time) {
		/*INetChannelInfo* nci = m_engine()->GetNetChannelInfo();
		if (g_cfg.ragebot.exploit_modifiers.at(0) && nci->GetAvgLoss(FLOW_INCOMING) > 0)
			return;*/

		lastdoubletaptime = 0;

		g_ctx.globals.startcharge = true;
		g_ctx.globals.tochargeamount = shiftAmount;
		g_ctx.globals.dt_shots = 0;
	}
	else g_ctx.globals.startcharge = false;

	//Do the magic.
	bool restricted_weapon = (g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_TASER || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER);
	if ((m_pcmd->m_buttons & IN_ATTACK) && !restricted_weapon && g_ctx.globals.tocharge == shiftAmount && weapon->m_flNextPrimaryAttack() <= m_pcmd->m_command_number - shiftTime) {
		if (g_ctx.globals.aimbot_working)
		{
			g_ctx.globals.double_tap_aim = true;
			g_ctx.globals.double_tap_aim_check = true;
		}
		g_ctx.globals.shift_ticks = shiftAmount;
		g_ctx.globals.m_shifted_command = m_pcmd->m_command_number;
		lastdoubletaptime = m_pcmd->m_command_number;

	}
}

bool misc::CanDoubleTap(bool check_charge) {
	//check if DT key is enabled.
	if (!g_cfg.ragebot.double_tap ||g_cfg.ragebot.double_tap_key.key <= KEY_NONE || g_cfg.ragebot.double_tap_key.key >= KEY_MAX) {
		double_tap_enabled = false;
		double_tap_key = false;
		lastdoubletaptime = 0;
		g_ctx.globals.tocharge = 0;
		g_ctx.globals.tochargeamount = 0;
		g_ctx.globals.shift_ticks = 0;
		return false;
	}

	//if DT is on, disable hide shots.
	if (double_tap_key && g_cfg.ragebot.double_tap_key.key != g_cfg.antiaim.hide_shots_key.key)
		hide_shots_key = false;

	//disable DT if frozen, fakeducking, revolver etc.
	if (!double_tap_key || g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN || m_gamerules()->m_bIsValveDS() || g_ctx.globals.fakeducking) {
		double_tap_enabled = false;
		lastdoubletaptime = 0;
		g_ctx.globals.tocharge = 0;
		g_ctx.globals.tochargeamount = 0;
		g_ctx.globals.shift_ticks = 0;

		return false;
	}

	if (check_charge) {
		if (g_ctx.globals.tochargeamount > 0)
			return false;

		if (g_ctx.globals.startcharge)
			return false;
	}

	return true;
}


void misc::HideShots(CUserCmd* m_pcmd)
{
	if (double_tap_key)
		return;

	hide_shots_enabled = true;

	if (!g_cfg.ragebot.enable)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;

		return;
	}

	if (!g_cfg.antiaim.hide_shots)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;

		return;
	}

	if (g_cfg.antiaim.hide_shots_key.key <= KEY_NONE || g_cfg.antiaim.hide_shots_key.key >= KEY_MAX)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;

		return;
	}

	if (double_tap_key)
	{

		hide_shots_enabled = false;
		hide_shots_key = false;
		return;
	}

	if (!hide_shots_key)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return;
	}

	double_tap_key = false;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return;
	}

	if (g_ctx.globals.fakeducking)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return;
	}

	if (antiaim::get().freeze_check)
		return;

	g_ctx.globals.tickbase_shift = m_gamerules()->m_bIsValveDS() ? 6 : 9;

	auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);
	auto weapon_shoot = m_pcmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife() || revolver_shoot;

	if (g_ctx.send_packet && !g_ctx.globals.weapon->is_grenade() && weapon_shoot)
		g_ctx.globals.tickbase_shift = g_ctx.globals.tickbase_shift;
}

void misc::ax()
{
	static auto is_command_set = false;
	if (is_command_set != g_cfg.ragebot.anti_exploit) {
		int old_team = g_ctx.local()->m_iTeamNum();

		std::string command1 = "kill";
		std::string command2 = "jointeam 1";
		std::string command3 = g_cfg.ragebot.anti_exploit ? "cl_lagcompensation 0" : "cl_lagcompensation 1";
		std::string command4 = "jointeam " + std::to_string(old_team);


		m_engine()->ExecuteClientCmd(command1.data());
		m_engine()->ExecuteClientCmd(command2.data());
		m_engine()->ExecuteClientCmd(command3.data());
		m_engine()->ExecuteClientCmd(command4.data());
		is_command_set = g_cfg.ragebot.anti_exploit;
	}
}