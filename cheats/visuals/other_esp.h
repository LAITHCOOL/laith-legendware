#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

struct m_indicator
{
	std::string m_text;
	Color m_color;

	m_indicator(const char* text, Color color) :
		m_text(text), m_color(color)
	{

	}
	m_indicator(std::string text, Color color) :
		m_text(text), m_color(color)
	{

	}

};

class otheresp : public singleton< otheresp >
{
public:
	void velocity();
	void penetration_reticle();
	void indicators();
	void draw_indicators();
	void hitmarker_paint();
	void damage_marker_paint();
	void spread_crosshair(LPDIRECT3DDEVICE9 device);
	void automatic_peek_indicator();
	void AddTrails();
	void holopanel(player_t* WhoUseThisBone = g_ctx.local(), int hitbox_id = HITBOX_STOMACH, bool autodir = true);

	struct Hitmarker
	{
		float hurt_time = FLT_MIN;
		Color hurt_color = Color::White;
		Vector point = ZERO;
	} hitmarker;

	struct C_TrailSegment
	{
		Vector position = Vector(0, 0, 0);
		float expiration = -1;

		std::vector< C_TrailSegment > trail_segments;
		Vector last_origin;
	}trail_data;

	struct Damage_marker
	{
		Vector position = ZERO;
		float hurt_time = FLT_MIN;
		Color hurt_color = Color::Red;
		int damage = -1;
		int hitgroup = -1;

		void reset()
		{
			position.Zero();
			hurt_time = FLT_MIN;
			hurt_color = Color::White;
			damage = -1;
			hitgroup = -1;
		}
	} damage_marker[65];

	std::vector<m_indicator> m_indicators;
};