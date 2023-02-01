#include "other_esp.h"
#include "..\autowall\autowall.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"
#include "..\misc\misc.h"
#include "..\lagcompensation\local_animations.h"
#include "../menu_alpha.h"
#include "../tickbase shift/tickbase_shift.h"

bool can_penetrate(weapon_t* weapon)
{
	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return false;

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	Vector direction;
	math::angle_vectors(view_angles, direction);

	CTraceFilter filter;
	filter.pSkip = g_ctx.local();

	trace_t trace;
	util::trace_line(g_ctx.globals.eye_pos, g_ctx.globals.eye_pos + direction * weapon_info->flRange, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

	if (trace.fraction == 1.0f) //-V550
		return false;

	auto eye_pos = g_ctx.globals.eye_pos;
	auto hits = 1;
	auto damage = (float)weapon_info->iDamage;
	auto penetration_power = weapon_info->flPenetration;

	static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
	static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

	return CAutoWall::handle_bullet_penetration_lw(weapon_info, trace, eye_pos, direction, hits, damage, penetration_power, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat());
}

void otheresp::penetration_reticle()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.penetration_reticle)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto color = Color::Red;

	if (!weapon->is_non_aim() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && can_penetrate(weapon))
		color = Color::Green;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	render::get().rect_filled(width / 2, height / 2 - 1, 1, 3, color);
	render::get().rect_filled(width / 2 - 1, height / 2, 3, 1, color);
}

void otheresp::velocity()
{
	if (!g_cfg.esp.Velocity_graph)
		return;

	if (!g_ctx.local())
		return;

	if (!m_engine()->IsInGame() || !g_ctx.local()->is_alive())
		return;

	static std::vector<float> velData(120, 0);

	Vector vecVelocity = g_ctx.local()->m_vecVelocity();
	float currentVelocity = sqrt(vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y);

	velData.erase(velData.begin());
	velData.push_back(currentVelocity);

	int vel = g_ctx.local()->m_vecVelocity().Length2D();

	static int width, height;
	m_engine()->GetScreenSize(width, height);
	render::get().text(fonts[VELOCITY], width / 2, height / 1.1, Color(0, 255, 100, 255), HFONT_CENTERED_X | HFONT_CENTERED_Y, "[%i]", vel);


	for (auto i = 0; i < velData.size() - 1; i++)
	{
		int cur = velData.at(i);
		int next = velData.at(i + 1);

		render::get().line(
			width / 2 + (velData.size() * 5 / 2) - (i - 1) * 5.f,
			height / 1.15 - (std::clamp(cur, 0, 450) * .2f),
			width / 2 + (velData.size() * 5 / 2) - i * 5.f,
			height / 1.15 - (std::clamp(next, 0, 450) * .2f), Color(255, 255, 255, 255)
		);
	}
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

void otheresp::holopanel(player_t* WhoUseThisBone, int hitbox_id, bool autodir)
{

	std::string ping = std::to_string(g_ctx.globals.ping);
	char const* pincc = ping.c_str();

	std::string fps = std::to_string(g_ctx.globals.framerate);
	char const* fpsc = fps.c_str();

	if (g_cfg.misc.holo_panel)
	{
		auto bone_pos = WhoUseThisBone->hitbox_position(hitbox_id);
		Vector angle;
		if (key_binds::get().get_key_bind_state(_THIRDPERSON))
		{
			if (math::world_to_screen(bone_pos, angle))
			{
				render::get().line(angle.x, angle.y, angle.x + 100, angle.y - 150, Color(255, 255, 255));

				render::get().rect_filled(angle.x + 100, angle.y - 155, 150, 5, Color(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255));
				if (g_ctx.globals.framerate < 60)
				{
					render::get().rect_filled(angle.x + 100, angle.y - 150, 150, 105 + render::get().text_heigth(fonts[NAME], "WARNING! LOW FPS") + 5, Color(0, 0, 0, 150));
					render::get().text(fonts[NAME], angle.x + 110, angle.y - 45, Color(219, 179, 15, 255), 0, "WARNING! LOW FPS");
				}
				else
				{
					render::get().rect_filled(angle.x + 100, angle.y - 150, 150, 105, Color(0, 0, 0, 150));
				}

				if (g_cfg.ragebot.double_tap && g_cfg.ragebot.double_tap_key.key > KEY_NONE && g_cfg.ragebot.double_tap_key.key < KEY_MAX && tickbase::get().double_tap_key)
				{
					render::get().text(fonts[NAME], angle.x + 110, angle.y - 145, Color(255, 255, 255, 255), 0, "Exploit : ");
					render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Exploit : "), angle.y - 145, Color(117, 219, 15, 255), 0, "Doubletap");
				}
				else if (g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && tickbase::get().hide_shots_key)
				{
					render::get().text(fonts[NAME], angle.x + 110, angle.y - 145, Color(255, 255, 255, 255), 0, "Exploit : ");
					render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Exploit : "), angle.y - 145, Color(117, 219, 15, 255), 0, "HideShots");
				}
				else
				{
					render::get().text(fonts[NAME], angle.x + 110, angle.y - 145, Color(255, 255, 255, 255), 0, "Exploit : ");
					render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Exploit : "), angle.y - 145, Color(219, 15, 15, 255), 0, "Not active");
				}

				render::get().text(fonts[NAME], angle.x + 110, angle.y - 125, Color(255, 255, 255, 255), 0, "DoubleTap Type : ");
				if (g_cfg.ragebot.instant_double_tap or g_cfg.ragebot.defensive_doubletap)
				{
					render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Doubletap Type : "), angle.y - 125, Color(117, 219, 15, 255), 0, "Instant");
				}
				else
				{
					render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Doubletap Type : "), angle.y - 125, Color(219, 15, 15, 255), 0, "Default");
				}

				render::get().text(fonts[NAME], angle.x + 110, angle.y - 105, Color(255, 255, 255, 255), 0, "Fps : ");
				if (g_ctx.globals.framerate >= 60)
				{
					render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Fps : "), angle.y - 105, Color(117, 219, 15, 255), 0, fpsc);
				}
				else
				{
					render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Fps : "), angle.y - 105, Color(219, 15, 15, 255), 0, fpsc);
				}
				

				render::get().text(fonts[NAME], angle.x + 110, angle.y - 85, Color(255, 255, 255, 255), 0, "Ping : ");
				render::get().text(fonts[NAME], angle.x + 110 + render::get().text_width(fonts[NAME], "Ping : "), angle.y - 85, Color(255,255,255, 255), 0, pincc);

				render::get().text(fonts[NAME], angle.x + 110, angle.y - 65, Color(255, 255, 255, 255), 0, "itzlaith_lw [Beta]");

			}
		}
	}
}

void otheresp::indicators()
{
	if (!g_ctx.local()->is_alive()) //-V807
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	static int height, width;
	m_engine()->GetScreenSize(width, height);

	g_ctx.globals.indicator_pos = height / 2;

	if (g_cfg.esp.indicators[INDICATOR_DT] && g_cfg.ragebot.double_tap && g_cfg.ragebot.double_tap_key.key > KEY_NONE && g_cfg.ragebot.double_tap_key.key < KEY_MAX && tickbase::get().double_tap_key)
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "DT") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "DT") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "DT") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "DT") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "DT") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "DT");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, !g_ctx.local()->m_bGunGameImmunity() && !(g_ctx.local()->m_fFlags() & FL_FROZEN) && !antiaim::get().freeze_check && tickbase::get().double_tap_enabled && !weapon->is_grenade() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && weapon->can_fire(false) ? Color(130, 20, 0, 255) : Color(255, 255, 255, 150), 0, "DT");
		g_ctx.globals.indicator_pos += 40;
	}

	if (g_cfg.esp.indicators[INDICATOR_HS] && g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && tickbase::get().hide_shots_key)
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "HS") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "HS") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "HS") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "HS") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "HS") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "HS");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(240, 240, 240, 255), 0, "HS");
		g_ctx.globals.indicator_pos += 40;
	}

   /*if (g_cfg.esp.indicators[INDICATOR_ANTI_EXPLOIT] && g_cfg.ragebot.anti_exploit)
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "AX") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "AX") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "AX") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "AX") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "AX") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "AX");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(167, 244, 24), 0, "AX");
		g_ctx.globals.indicator_pos += 40;
	}*/

	if (g_cfg.esp.indicators[INDICATOR_FAKE] && g_cfg.antiaim.desync)
	{
		auto colorfake = Color(130, 20 + (int)(((g_ctx.local()->get_max_desync_delta() - 29.f) / 29.f) * 200.0f), 0);
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "FAKE ") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "FAKE ") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "FAKE ") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "FAKE ") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "DT") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "FAKE");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, colorfake, 0, "FAKE");
		//render::get().draw_arc2(10 + render::get().text_width(fonts[INDICATORFONT2], "FAKE") + 12, g_ctx.globals.indicator_pos + 13, 7, NULL, 360, 4, Color(0, 0, 0, 200));
		//render::get().draw_arc2(10 + render::get().text_width(fonts[INDICATORFONT2], "FAKE") + 12, g_ctx.globals.indicator_pos + 13, 7, NULL, ((g_ctx.local()->get_max_desync_delta() - 29.f) / 29.f) * 360.f, 4, colorfake);
		g_ctx.globals.indicator_pos += 40;
	}

	if (g_cfg.esp.indicators[INDICATOR_CHOKE] && !fakelag::get().condition && !tickbase::get().double_tap_enabled && !tickbase::get().hide_shots_enabled)
	{
		auto colorfl = Color(130, 20 + (int)((m_clientstate()->iChokedCommands / 15.f) * 200.0f), 0);
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "FL ") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "FL ") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "FL ") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "FL ") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "DT") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "FL");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, colorfl, 0, "FL");
		render::get().draw_arc2(10 + render::get().text_width(fonts[INDICATORFONT2], "FL") + 12, g_ctx.globals.indicator_pos + 13, 7, NULL, 360, 4, Color(0, 0, 0, 200));
		render::get().draw_arc2(10 + render::get().text_width(fonts[INDICATORFONT2], "FL") + 12, g_ctx.globals.indicator_pos + 13, 7, NULL, (m_clientstate()->iChokedCommands / 15.f) * 360.f, 4, colorfl);
		g_ctx.globals.indicator_pos += 40;
	}

	if (g_cfg.esp.indicators[INDICATOR_DAMAGE] && g_ctx.globals.current_weapon != -1 && key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon) && !weapon->is_non_aim())
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "00") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "00") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "00") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "00") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "DT") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "%d", g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage);
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(255, 255, 255, 150), 0, "%d", g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage);
		g_ctx.globals.indicator_pos += 40;
	}

	if (g_cfg.esp.indicators[INDICATOR_SAFE_POINTS] && key_binds::get().get_key_bind_state(3) && !weapon->is_non_aim())
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "SAFE") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "SAFE") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "SAFE") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "SAFE") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "SAFE") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "SAFE");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(255, 255, 255, 150), 0, "SAFE");
		g_ctx.globals.indicator_pos += 40;
	}

	if (g_cfg.esp.indicators[INDICATOR_BODY_AIM] && key_binds::get().get_key_bind_state(22) && !weapon->is_non_aim())
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "BAIM") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "BAIM") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "BAIM") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "BAIM") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "BAIM") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "BAIM");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(255, 255, 255, 150), 0, "BAIM");
		g_ctx.globals.indicator_pos += 40;
	}

	if (g_cfg.esp.indicators[INDICATOR_LC] && g_cfg.antiaim.fakelag > 0)
	{
		if (g_ctx.canDrawLC || g_ctx.canBreakLC)
		{
			render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "LC") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "LC") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
			render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "LC") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "LC") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "LC") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
			render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "LC");
			render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(130, 20, 0, 255), 0, "LC");
			g_ctx.globals.indicator_pos += 40;
		}
	}

	if (key_binds::get().get_key_bind_state(18))
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "PEEK") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "PEEK") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "PEEK") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "PEEK") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "PEEK") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "PEEK");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(255, 255, 255, 150), 0, "PEEK");
		g_ctx.globals.indicator_pos += 40;
	}

	if (key_binds::get().get_key_bind_state(20))
	{
		render::get().gradient(10, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "DUCK") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "DUCK") + 8, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(10 + render::get().text_width(fonts[INDICATORFONT2], "DUCK") / 2, g_ctx.globals.indicator_pos - 4, render::get().text_width(fonts[INDICATORFONT2], "DUCK") / 2, render::get().text_heigth(fonts[INDICATORFONT2], "DUCK") + 8, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 11, g_ctx.globals.indicator_pos + 1, Color::Black, 0, "DUCK");
		render::get().text(fonts[INDICATORFONT2], 10, g_ctx.globals.indicator_pos, Color(255, 255, 255, 150), 0, "DUCK");
		g_ctx.globals.indicator_pos += 40;
	}
}

void otheresp::draw_indicators()
{
	if (!g_ctx.local()->is_alive()) //-V807
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto h = height / 2 + 50;

	for (auto& indicator : m_indicators)
	{
		render::get().gradient(5, h - 15, 30, 30, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(35, h - 15, 30, 30, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[INDICATORFONT2], 10, h, indicator.m_color, HFONT_CENTERED_Y, indicator.m_text.c_str());
		h += 30;
	}

	if (g_cfg.misc.holo_panel)
	{
		otheresp::holopanel();
	}

	m_indicators.clear();
}

void otheresp::hitmarker_paint()
{
	if (!g_cfg.esp.hitmarker[0] && !g_cfg.esp.hitmarker[1])
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (!g_ctx.local()->is_alive())
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (hitmarker.hurt_time + 0.7f > m_globals()->m_curtime)
	{
		if (g_cfg.esp.hitmarker[0])
		{
			static int width, height;
			m_engine()->GetScreenSize(width, height);

			auto alpha = (int)((hitmarker.hurt_time + 0.7f - m_globals()->m_curtime) * 255.0f);
			hitmarker.hurt_color.SetAlpha(alpha);

			auto offset = 7.0f - (float)alpha / 255.0f * 7.0f;

			render::get().line(width / 2 + 5 + offset, height / 2 - 5 - offset, width / 2 + 12 + offset, height / 2 - 12 - offset, hitmarker.hurt_color);
			render::get().line(width / 2 + 5 + offset, height / 2 + 5 + offset, width / 2 + 12 + offset, height / 2 + 12 + offset, hitmarker.hurt_color);
			render::get().line(width / 2 - 5 - offset, height / 2 + 5 + offset, width / 2 - 12 - offset, height / 2 + 12 + offset, hitmarker.hurt_color);
			render::get().line(width / 2 - 5 - offset, height / 2 - 5 - offset, width / 2 - 12 - offset, height / 2 - 12 - offset, hitmarker.hurt_color);
		}

		if (g_cfg.esp.hitmarker[1])
		{
			Vector world;

			if (math::world_to_screen(hitmarker.point, world))
			{
				auto alpha = (int)((hitmarker.hurt_time + 0.7f - m_globals()->m_curtime) * 255.0f);
				hitmarker.hurt_color.SetAlpha(alpha);

				auto offset = 7.0f - (float)alpha / 255.0f * 7.0f;

				render::get().line(world.x + 5 + offset, world.y - 5 - offset, world.x + 12 + offset, world.y - 12 - offset, hitmarker.hurt_color);
				render::get().line(world.x + 5 + offset, world.y + 5 + offset, world.x + 12 + offset, world.y + 12 + offset, hitmarker.hurt_color);
				render::get().line(world.x - 5 - offset, world.y + 5 + offset, world.x - 12 - offset, world.y + 12 + offset, hitmarker.hurt_color);
				render::get().line(world.x - 5 - offset, world.y - 5 - offset, world.x - 12 - offset, world.y - 12 - offset, hitmarker.hurt_color);
			}
		}
	}
}

void otheresp::damage_marker_paint()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		if (damage_marker[i].hurt_time + 2.0f > m_globals()->m_curtime)
		{
			Vector screen;

			if (!math::world_to_screen(damage_marker[i].position, screen))
				continue;

			auto alpha = (int)((damage_marker[i].hurt_time + 2.0f - m_globals()->m_curtime) * 127.5f);
			damage_marker[i].hurt_color.SetAlpha(alpha);

			render::get().text(fonts[DAMAGE_MARKER], screen.x, screen.y, damage_marker[i].hurt_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, "%i", damage_marker[i].damage);
		}
	}
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device);

void otheresp::spread_crosshair(LPDIRECT3DDEVICE9 device)
{
	//
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2 = nullptr;
	std::vector <CUSTOMVERTEX2> circle(resolution + 2);

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0.0f;

	circle[0].rhw = 1.0f;
	circle[0].color = color2;

	for (auto i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - radius * cos(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - radius * sin(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0.0f;

		circle[i].rhw = 1.0f;
		circle[i].color = color;
	}

	device->CreateVertexBuffer((resolution + 2) * sizeof(CUSTOMVERTEX2), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB2, nullptr); //-V107

	if (!g_pVB2)
		return;

	void* pVertices;

	g_pVB2->Lock(0, (resolution + 2) * sizeof(CUSTOMVERTEX2), (void**)&pVertices, 0); //-V107
	memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(CUSTOMVERTEX2));
	g_pVB2->Unlock();

	device->SetTexture(0, nullptr);
	device->SetPixelShader(nullptr);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	device->SetStreamSource(0, g_pVB2, 0, sizeof(CUSTOMVERTEX2));
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);

	g_pVB2->Release();
}

/*void otheresp::automatic_peek_indicator()
{
	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	static auto position = ZERO;

	if (!g_ctx.globals.start_position.IsZero())
		position = g_ctx.globals.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 0.0f;

	if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18) || alpha)
	{
		if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
			alpha += 4.3f * m_globals()->m_frametime; //-V807
		else
			alpha -= 4.3f * m_globals()->m_frametime;

		auto color_main = Color(g_cfg.misc.automatic_peek_color.r(), g_cfg.misc.automatic_peek_color.g(), g_cfg.misc.automatic_peek_color.b(), (int)((alpha * 197) / 3));
		auto color_outline = Color(255, 255, 230, (int)(255));

		alpha = math::clamp(alpha, 0.0f, 1.0f);

		render::get().Draw3DFilledCircle(position, alpha * 15.f, Color(color_main));
		render::get().Draw3DCircle(position, alpha * 15.f, Color(color_outline));

		Vector screen;

		if (math::world_to_screen(position, screen))
		{
			static auto offset = 30.0f;

			if (!g_ctx.globals.fired_shot)
			{
				static auto switch_offset = false;

				if (offset <= 30.0f && offset >= 55.0f)
					switch_offset = !switch_offset;

				offset += switch_offset ? 22.0f * m_globals()->m_frametime : -22.0f * m_globals()->m_frametime;
				offset = math::clamp(offset, 30.0f, 55.0f);
			}
		}
	}
}*/

void otheresp::automatic_peek_indicator()
{
	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	static auto position = ZERO;

	if (!g_ctx.globals.start_position.IsZero())
		position = g_ctx.globals.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 4.0f;

	if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18) || alpha)
	{
		if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
			alpha += 9.0f * m_globals()->m_frametime; //-V807
		else
			alpha -= 9.0f * m_globals()->m_frametime;

		alpha = math::clamp(alpha, 0.0f, 1.0f);
		//render::get().Draw3DFilledCircle(position, 25.0f, g_cfg.esp.molotov_timer ? Color(183, 206, 232, (int)(alpha * 55.0f)) : Color(183, 206, 232, (int)(alpha * 55.0f)));
		render::get().Draw3DFilledCircle(position, alpha * 20.f, g_ctx.globals.fired_shot ? Color(40, 220, 5, (int)(alpha * 135.0f)) : Color(200, 200, 200, (int)(alpha * 125.0f)));
		Vector screen;

		if (math::world_to_screen(position, screen))
		{
			static auto offset = 30.0f;

			if (!g_ctx.globals.fired_shot)
			{
				static auto switch_offset = false;

				if (offset <= 30.0f || offset >= 55.0f)
					switch_offset = !switch_offset;

				offset += switch_offset ? 22.0f * m_globals()->m_frametime : -22.0f * m_globals()->m_frametime;
				offset = math::clamp(offset, 30.0f, 55.0f);
			}
		}
	}
}

void otheresp::AddTrails()
{

}