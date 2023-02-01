// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "bullet_tracers.h"
#include "..\..\sdk\misc\BeamInfo_t.hpp"
#include "..\ragebot\aim.h"
#include "..\..\utils\ctx.hpp"
#include "..\misc\logs.h"

void bullettracers::draw_beam(bool local_tracer, const Vector& src, const Vector& end, Color color)
{
	if (src == ZERO)
		return;

	BeamInfo_t beam_info;
	Vector startP , endP;

	startP = src;

	endP = end;

	if (local_tracer)
	{
		//beam_info.m_vecStart.z -= 4.0f;
		startP.x += 3.0f;
	}



	const char* sprite;

	switch (g_cfg.esp.bullet_tracer_type)
	{
	case 0:
		sprite = "sprites/blueglow1.vmt";
		break;
	case 1:
		sprite = "sprites/bubble.vmt";
		break;
	case 2:
		sprite = "sprites/glow01.vmt";
		break;
	case 3:
		sprite = "sprites/physbeam.vmt";
		break;
	case 4:
		sprite = "sprites/purpleglow1.vmt";
		break;
	case 5:
		sprite = "sprites/purplelaser1.vmt";
		break;
	case 6:
		sprite = "sprites/radio.vmt";
		break;
	case 7:
		sprite = "sprites/white.vmt";
		break;

	}

	float red = (float)color.r();
	float green = (float)color.g();
	float blue = (float)color.b();
	float alpha = (float)color.b();

	if (g_cfg.esp.bullet_tracer_type == 8)
		m_debugoverlay()->AddLineOverlayAlpha(startP, endP, red, green, blue, alpha, false, 4);
	else
	{
		beam_info.m_vecStart = startP;
		beam_info.m_vecEnd = endP;
		beam_info.m_nType = TE_BEAMPOINTS;
		beam_info.m_pszModelName = crypt_str(sprite);
		//beam_info.m_nModelIndex = -1;
		beam_info.m_pszHaloName = crypt_str(sprite);
		beam_info.m_flHaloScale = 3.5f;
		beam_info.m_flLife = 4.5f;
		beam_info.m_flWidth = 4.0f;
		beam_info.m_flEndWidth = 4.4f;
		beam_info.m_flFadeLength = 1.0f;
		beam_info.m_flAmplitude = 0.0f;
		beam_info.m_flBrightness = alpha;
		beam_info.m_flSpeed = 0.0f;
		beam_info.m_nStartFrame = 0;
		beam_info.m_flFrameRate = 0.0f;
		beam_info.m_flRed = red;
		beam_info.m_flGreen = green;
		beam_info.m_flBlue = blue;
		beam_info.m_nSegments = 1;
		beam_info.m_bRenderable = true;
		beam_info.m_nFlags = 1;

		auto beam = m_viewrenderbeams()->CreateBeamPoints(beam_info);

		if (beam)
			m_viewrenderbeams()->DrawBeam(beam);
	}
}

void bullettracers::events(IGameEvent* event)
{
	auto event_name = event->GetName();

	if (!strcmp(event_name, crypt_str("bullet_impact")))
	{
		auto user_id = event->GetInt(crypt_str("userid"));
		auto user = m_engine()->GetPlayerForUserID(user_id);

		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(user));

		if (e->valid(false))
		{
			if (e == g_ctx.local())
			{
				auto new_record = true;
				Vector position(event->GetFloat(crypt_str("x")), event->GetFloat(crypt_str("y")), event->GetFloat(crypt_str("z")));

				for (auto& current : impacts)
				{
					if (e == current.e)
					{
						new_record = false;

						current.impact_position = position;
						current.time = m_globals()->m_curtime;
					}
				}

				if (new_record)
					impacts.push_back(
						impact_data
						{
							e,
							position,
							m_globals()->m_curtime
						});
			}
			else if (e->m_iTeamNum() != g_ctx.local()->m_iTeamNum())
			{
				auto new_record = true;
				Vector position(event->GetFloat(crypt_str("x")), event->GetFloat(crypt_str("y")), event->GetFloat(crypt_str("z")));

				for (auto& current : impacts)
				{
					if (e == current.e)
					{
						new_record = false;

						current.impact_position = position;
						current.time = m_globals()->m_curtime;
					}
				}

				if (new_record)
					impacts.push_back(
						impact_data
						{
							e,
							position,
							m_globals()->m_curtime
						});
			}
		}
	}
}

void bullettracers::draw_beams()
{
	if (impacts.empty())
		return;

	while (!impacts.empty())
	{
		if (impacts.begin()->impact_position.IsZero()) //-V807
		{
			impacts.erase(impacts.begin());
			continue;
		}

		if (fabs(m_globals()->m_curtime - impacts.begin()->time) > 4.0f) //-V807
		{
			impacts.erase(impacts.begin());
			continue;
		}

		if (!impacts.begin()->e->valid(false))
		{
			impacts.erase(impacts.begin());
			continue;
		}

		if (TIME_TO_TICKS(m_globals()->m_curtime) > TIME_TO_TICKS(impacts.begin()->time))
		{
			auto color = g_cfg.esp.enemy_bullet_tracer_color;

			if (impacts.begin()->e == g_ctx.local())
			{
				if (!g_cfg.esp.bullet_tracer)
				{
					impacts.erase(impacts.begin());
					continue;
				}

				color = g_cfg.esp.bullet_tracer_color;
			}
			else if (!g_cfg.esp.enemy_bullet_tracer)
			{
				impacts.erase(impacts.begin());
				continue;
			}

			draw_beam(impacts.begin()->e == g_ctx.local(), impacts.begin()->e == g_ctx.local() ? aim::get().last_shoot_position : impacts.begin()->e->get_shoot_position(), impacts.begin()->impact_position, color);
			impacts.erase(impacts.begin());
			continue;
		}

		break;
	}
}