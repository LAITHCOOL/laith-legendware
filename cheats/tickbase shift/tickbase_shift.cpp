#include "tickbase_shift.h"
#include "../misc/misc.h"
#include "../misc/fakelag.h"
#include "..\ragebot\aim.h"
#include "../misc/prediction_system.h"

void tickbase::double_tap_deffensive(CUserCmd* cmd)
{
	bool did_peek = false;
	auto predicted_eye_pos = g_ctx.globals.eye_pos + engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * 4.f;
	bool do_defensive = false;
	int shift_amount = 13;
	g_ctx.globals.should_fakelag = false;
	if (antiaim::get().type != ANTIAIM_STAND)
	{
		for (auto i = 1; i < m_globals()->m_maxclients; i++)
		{
			auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));
			if (!e->valid(true))
				continue;

			if (!e || i == -1)
				continue;

			FireBulletData_t fire_data = { };

			fire_data.damage = CAutoWall::GetDamage(predicted_eye_pos, g_ctx.local(), e->hitbox_position(HITBOX_HEAD), &fire_data);

			if (fire_data.damage < 1)
				continue;

			did_peek = true;



			if (did_peek)
			{
				do_defensive = true;
				did_peek = false;
			}

		}
	}

	if (do_defensive)
	{
		g_ctx.globals.tickbase_shift = shift_amount;
		g_ctx.globals.shifting_command_number = cmd->m_command_number;

		for (auto i = 0; i < g_ctx.globals.tickbase_shift; i++)
		{
			auto command = m_input()->GetUserCmd(cmd[i].m_command_number);
			auto v8 = m_input()->GetVerifiedUserCmd(cmd[i].m_command_number);

			memcpy(command, &cmd[i], sizeof(CUserCmd));

			if (command->m_tickcount != INT_MAX && m_clientstate()->iDeltaTick > 0)
				m_prediction()->Update(m_clientstate()->iDeltaTick, true, m_clientstate()->nLastCommandAck, m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands);

			command->m_predicted = command->m_tickcount != INT_MAX;

			v8->m_cmd = *command;
			v8->m_crc = command->GetChecksum();

			++m_clientstate()->pNetChannel->m_nChokedPackets;
			++m_clientstate()->pNetChannel->m_nOutSequenceNr;
			++m_clientstate()->iChokedCommands;
		}

	}
}

void tickbase::DoubleTap(CUserCmd* m_pcmd)
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
	if (!aim::get().should_stop && !(m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife())
		&& !util::is_button_down(MOUSE_LEFT) && g_ctx.globals.tocharge < shiftAmount && (m_pcmd->m_command_number - lastdoubletaptime) > recharge_time)
	{
		lastdoubletaptime = 0;
		g_ctx.globals.startcharge = true;
		g_ctx.globals.tochargeamount = shiftAmount;
		g_ctx.globals.dt_shots = 0;
	}
	else g_ctx.globals.startcharge = false;

	//Do the magic.
	bool restricted_weapon = (g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_TASER || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER
		|| g_ctx.globals.weapon->is_knife() || g_ctx.globals.weapon->is_grenade());
	if ((m_pcmd->m_buttons & IN_ATTACK) && !restricted_weapon && g_ctx.globals.tocharge == shiftAmount && weapon->m_flNextPrimaryAttack() <= m_pcmd->m_command_number - shiftTime)
	{
		misc::get().fix_autopeek(m_pcmd);

		if (g_ctx.globals.aimbot_working)
		{
			g_ctx.globals.double_tap_aim = true;
			g_ctx.globals.double_tap_aim_check = true;
		}
		g_ctx.globals.shift_ticks = shiftAmount;
		g_ctx.globals.m_shifted_command = m_pcmd->m_command_number;
		g_ctx.globals.shifting_command_number = m_pcmd->m_command_number; // used for tickbase fix 
		lastdoubletaptime = m_pcmd->m_command_number;
	}
}

bool tickbase::CanDoubleTap(bool check_charge) {
	//check if DT key is enabled.
	if (!g_cfg.ragebot.double_tap || g_cfg.ragebot.double_tap_key.key <= KEY_NONE || g_cfg.ragebot.double_tap_key.key >= KEY_MAX) {
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


void tickbase::HideShots(CUserCmd* m_pcmd)
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
