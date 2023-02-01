#pragma once

#include "..\autowall\autowall.h"
#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

class antiaim : public singleton <antiaim>
{
public:
	void fakeflickbylabeforrolix(CUserCmd* m_pcmd);
	void create_move(CUserCmd* m_pcmd);

	void desync_on_shot(CUserCmd* cmd);

	bool freestand_nix(float& ang);

	void get_best_target();

	

	int GetNearestPlayerToCrosshair();

	bool MatchShot(player_t* player);

	bool anti_brute(bool flip);
	float get_pitch(CUserCmd* m_pcmd);
	float get_yaw(CUserCmd* m_pcmd);
	bool condition(CUserCmd* m_pcmd, bool dynamic_check = true);

	bool should_break_lby(CUserCmd* m_pcmd, int lby_type);
	float at_targets();
	bool automatic_direction();
	void freestanding(CUserCmd* m_pcmd);
	void edge_anti_aim(CUserCmd* m_pcmd);
	void do_fr(CUserCmd* cmd);
	float corrected_tickbase(CUserCmd* cmd);
	void predict_lby_update(float sampletime, CUserCmd* ucmd, bool& sendpacket);
	void better_freestand();
	float best_head_yaw();
	bool flicker = false;
	void lagsync(CUserCmd* m_pcmd);
	int type = 0;
	int manual_side = -1;
	int final_manual_side = -1;
	bool flip = false;
	bool freeze_check = false;
	bool breaking_lby = false;
	float desync_angle = 0.0f;
	float free_stand = 0.0f;
	int m_right;
	int m_left;
	int m_back;
	bool dt_cond(CUserCmd* m_pcmd);
};

enum
{
	SIDE_NONE = -1,
	SIDE_BACK,
	SIDE_LEFT,
	SIDE_RIGHT,
	SIDE_FORWARD,
	SIDE_FR
};

