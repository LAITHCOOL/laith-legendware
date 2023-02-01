// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\cheats\misc\prediction_system.h"
#include "..\..\cheats\lagcompensation\local_animations.h"
#include "..\..\cheats\misc\misc.h"
#include "..\..\cheats\misc\logs.h"
#undef max

void FixRevolver(CUserCmd * cmd, player_t* player)
{
	if (!g_ctx.local()->is_alive())
		return;

	if (g_cfg.ragebot.enable)
	{
		auto weapon = player->m_hActiveWeapon().Get();

		if (weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
		{
			static float m_nTickBase[MULTIPLAYER_BACKUP];
			static bool m_nShotFired[MULTIPLAYER_BACKUP];
			static bool m_nCanFire[MULTIPLAYER_BACKUP];

			m_nTickBase[cmd->m_command_number % MULTIPLAYER_BACKUP] = player->m_nTickBase();
			m_nShotFired[cmd->m_command_number % MULTIPLAYER_BACKUP] = cmd->m_buttons & IN_ATTACK || cmd->m_buttons & IN_ATTACK2;
			m_nCanFire[cmd->m_command_number % MULTIPLAYER_BACKUP] = weapon->can_fire(false);

			int nLowestCommand = cmd->m_command_number - (int(1.0f / m_globals()->m_intervalpertick) >> 1);
			int nCheckCommand = cmd->m_command_number - 1;
			int nFireCommand = 0;

			while (nCheckCommand > nLowestCommand)
			{
				nFireCommand = nCheckCommand;

				if (!m_nShotFired[nCheckCommand % MULTIPLAYER_BACKUP])
					break;

				if (!m_nCanFire[nCheckCommand % MULTIPLAYER_BACKUP])
					break;

				nCheckCommand--;
			}

			if (nFireCommand)
			{
				int nOffset = 1 - (-0.03348f / m_globals()->m_intervalpertick);

				if (weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
				{
					if (cmd->m_command_number - nFireCommand >= nOffset)
						weapon->m_flPostponeFireReadyTime() = TICKS_TO_TIME(m_nTickBase[(nFireCommand + nOffset) % MULTIPLAYER_BACKUP]) + 0.2f;
				}
			}
		}
	}
};
float fix_velocity_modifier(player_t* e, int command_number, bool before)
{
	auto velocity_modifier = g_ctx.globals.last_velocity_modifier;
	auto delta = command_number - g_ctx.globals.last_velocity_modifier_tick;

	if (before)
		--delta;

	if (delta < 0 || g_ctx.globals.last_velocity_modifier == 1.0f)
		velocity_modifier = e->m_flVelocityModifier();
	else if (delta)
	{
		velocity_modifier = g_ctx.globals.last_velocity_modifier + (m_globals()->m_intervalpertick * (float)delta * 0.4f);
		velocity_modifier = math::clamp(velocity_modifier, 0.0f, 1.0f);
	}

	return velocity_modifier;
}
using RunCommand_t = void(__thiscall*)(void*, player_t*, CUserCmd*, IMoveHelper*);
void __fastcall hooks::hooked_runcommand(void* ecx, void* edx, player_t* player, CUserCmd* m_pcmd, IMoveHelper* move_helper)
{
	static auto original_fn = prediction_hook->get_func_address <RunCommand_t>(19);
	g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	if (!player || player != g_ctx.local() || !m_pcmd)
		return original_fn(ecx, player, m_pcmd, move_helper);

	int m_tickrate = ((int)(1.0f / m_globals()->m_intervalpertick));

	if (m_globals()->m_tickcount + m_tickrate + 8 <= m_pcmd->m_tickcount) //-V807
	{
		m_pcmd->m_predicted = true;
		if (m_engine()->GetNetChannelInfo())
		{
			auto serverTickcount = g_ctx.globals.fakeducking ? m_globals()->m_tickcount : m_globals()->m_tickcount;

			const auto outgoing = m_engine()->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);

			serverTickcount += (outgoing / m_globals()->m_intervalpertick/* + incoming*/) + 3;
			player->m_nFinalPredictedTick() = serverTickcount;
		}

		player->set_abs_origin(player->m_vecOrigin());
		if (m_globals()->m_frametime > 0.0f && !m_prediction()->EnginePaused)
		{
			++player->m_nTickBase();
			m_globals()->m_curtime = TICKS_TO_TIME(player->m_nTickBase());
		}
		return;
	}

	auto m_pWeapon = player->m_hActiveWeapon().Get();

	static float tickbase_records[MULTIPLAYER_BACKUP];
	static bool in_attack[MULTIPLAYER_BACKUP];
	static bool can_shoot[MULTIPLAYER_BACKUP];
	tickbase_records[m_pcmd->m_command_number % MULTIPLAYER_BACKUP] = player->m_nTickBase();
	in_attack[m_pcmd->m_command_number % MULTIPLAYER_BACKUP] = m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2;
	can_shoot[m_pcmd->m_command_number % MULTIPLAYER_BACKUP] = m_pWeapon->can_fire(false);

	auto m_flStoredVelMod = player->m_flVelocityModifier();
	auto m_nStoredTickbase = player->m_nTickBase();
	auto m_nStoredCurtime = m_globals()->m_curtime;
	auto backup_velocity_modifier = player->m_flVelocityModifier();

	/*
	if (misc::get().CanDoubleTap(false) && g_cfg.misc.break_lc)
	{
		if (m_pcmd->m_command_number == g_ctx.globals.shifting_command_number)
		player->m_nTickBase() -= g_ctx.globals.tickbase_shift;
	}
*/


	engine_prediction::patch_attack_packet(m_pcmd, true);

	engine_prediction::m_misc.m_bOverrideModifier = true;

	if (g_ctx.globals.last_velocity_modifier && m_pcmd->m_command_number == m_clientstate()->nLastCommandAck + 1)
		player->m_flVelocityModifier() = m_flStoredVelMod;

	if (m_pWeapon)
		FixRevolver(m_pcmd,player);

	original_fn(ecx, player, m_pcmd, move_helper);

	if (m_pWeapon)
		FixRevolver(m_pcmd, player);

	if (m_pcmd->m_command_number == g_ctx.globals.shifting_command_number)
	{
		player->m_nTickBase() = m_nStoredTickbase;
		m_globals()->m_curtime = m_nStoredCurtime;
	}
    
	player->m_flVelocityModifier() = fix_velocity_modifier(player, m_pcmd->m_command_number, false);
	if (!engine_prediction::m_misc.m_bOverrideModifier)
		player->m_flVelocityModifier() = backup_velocity_modifier;
	engine_prediction::patch_attack_packet(m_pcmd, false);
	player->m_vphysicsCollisionState() = false;

}

using InPrediction_t = bool(__thiscall*)(void*);

bool __stdcall hooks::hooked_inprediction()
{
	static auto original_fn = prediction_hook->get_func_address <InPrediction_t>(14);
	static auto maintain_sequence_transitions = (void*)util::FindSignature(crypt_str("client.dll"), crypt_str("84 C0 74 17 8B 87"));
	static auto setupbones_timing = (void*)util::FindSignature(crypt_str("client.dll"), crypt_str("84 C0 74 0A F3 0F 10 05 ? ? ? ? EB 05"));
	static void* calcplayerview_return = (void*)util::FindSignature(crypt_str("client.dll"), crypt_str("84 C0 75 0B 8B 0D ? ? ? ? 8B 01 FF 50 4C"));

	if (maintain_sequence_transitions && g_ctx.globals.setuping_bones && _ReturnAddress() == maintain_sequence_transitions)
		return true;

	if (setupbones_timing && _ReturnAddress() == setupbones_timing)
		return false;

	if (m_engine()->IsInGame()) {
		if (_ReturnAddress() == calcplayerview_return)
			return true;
	}

	return original_fn(m_prediction());
}



#include "../../cheats/ragebot/aim.h"
typedef void(__cdecl* clMove_fn)(float, bool);

void __cdecl hooks::Hooked_CLMove(float flAccumulatedExtraSamples, bool bFinalTick)
{
	if (g_ctx.globals.startcharge&& g_ctx.globals.tocharge < g_ctx.globals.tochargeamount)
	{
		g_ctx.globals.tocharge++;
		g_ctx.globals.ticks_allowed = g_ctx.globals.tocharge;
		return;
	}

	(clMove_fn(hooks::original_clmove)(flAccumulatedExtraSamples, bFinalTick));

	g_ctx.globals.isshifting = true;
	{
		for (g_ctx.globals.shift_ticks = min(g_ctx.globals.tocharge, g_ctx.globals.shift_ticks); g_ctx.globals.shift_ticks > 0; g_ctx.globals.shift_ticks--, g_ctx.globals.tocharge--)
		{
			aim::get().lastshifttime = m_globals()->m_realtime;
			(clMove_fn(hooks::original_clmove)(flAccumulatedExtraSamples, bFinalTick));
		}

		//g_ctx.globals.shifted_tick = true;
	}
	g_ctx.globals.isshifting = false;
}


using WriteUsercmdDeltaToBuffer_t = bool(__thiscall*)(void*, int, void*, int, int, bool);
void WriteUserCmd(void* buf, CUserCmd* incmd, CUserCmd* outcmd);

void WriteUserCmd(void* buf, CUserCmd* incmd, CUserCmd* outcmd)
{
	using WriteUserCmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	static auto Fn = (WriteUserCmd_t)util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 51 53 56 8B D9"));

	__asm
	{
		mov     ecx, buf
		mov     edx, incmd
		push    outcmd
		call    Fn
		add     esp, 4
	}
}

bool __fastcall hooks::hooked_writeusercmddeltatobuffer(void* ecx, void* edx, int slot, bf_write* buf, int from, int to, bool is_new_command)
{
	static auto original_fn = client_hook->get_func_address <WriteUsercmdDeltaToBuffer_t>(24);

	if (!g_ctx.globals.tickbase_shift)
		return original_fn(ecx, slot, buf, from, to, is_new_command);

	if (from != -1)
		return true;

	auto final_from = -1;

	uintptr_t frame_ptr;
	__asm mov frame_ptr, ebp;

	auto backup_commands = reinterpret_cast <int*> (frame_ptr + 0xFD8);
	auto new_commands = reinterpret_cast <int*> (frame_ptr + 0xFDC);

	auto newcmds = *new_commands;
	auto shift = g_ctx.globals.tickbase_shift;

	g_ctx.globals.tickbase_shift = 0;
	*backup_commands = 0;

	auto choked_modifier = newcmds + shift;

	if (choked_modifier > 62)
		choked_modifier = 62;

	*new_commands = choked_modifier;

	auto next_cmdnr = m_clientstate()->iChokedCommands + m_clientstate()->nLastOutgoingCommand + 1;
	auto final_to = next_cmdnr - newcmds + 1;

	if (final_to <= next_cmdnr)
	{
		while (original_fn(ecx, slot, buf, final_from, final_to, true))
		{
			final_from = final_to++;

			if (final_to > next_cmdnr)
				goto next_cmd;
		}

		return false;
	}
next_cmd:

	auto user_cmd = m_input()->GetUserCmd(final_from);

	if (!user_cmd)
		return true;

	CUserCmd to_cmd;
	CUserCmd from_cmd;

	from_cmd = *user_cmd;
	to_cmd = from_cmd;

	to_cmd.m_command_number++;
	to_cmd.m_tickcount += 200;

	if (newcmds > choked_modifier)
		return true;

	for (auto i = choked_modifier - newcmds + 1; i > 0; --i)
	{
		WriteUserCmd(buf, &to_cmd, &from_cmd);

		from_cmd = to_cmd;
		to_cmd.m_command_number++;
		to_cmd.m_tickcount++;
	}

	return true;
}



bool __fastcall hooks::hooked_sendnetmsg(INetChannel* pNetChan, void* edx, INetMessage& msg, bool bForceReliable, bool bVoice)
{
	using Fn = bool(__thiscall*)(INetChannel* pNetChan, INetMessage& msg, bool bForceReliable, bool bVoice);
	static auto ofc = client_hook->get_func_address <Fn>(40);

	if (g_ctx.local() && m_engine()->IsInGame()) {
		if (msg.GetType() == 14) // Return and don't send messsage if its FileCRCCheck
			return true;

		if (msg.GetGroup() == 9) // Fix lag when transmitting voice and fakelagging
			bVoice = true;
	}

	return ofc(pNetChan, msg, bForceReliable, bVoice);
}

void __fastcall hooks::Hooked_SetupMove(void* ecx, void* edx, player_t* player, CUserCmd* ucmd, IMoveHelper* moveHelper, void* pMoveData) 
{

	static auto SetupMove = prediction_hook->get_func_address < void(__thiscall*)(void*, player_t*, CUserCmd*, IMoveHelper*, void*) >(20);
	
	SetupMove(ecx, player, ucmd, moveHelper, pMoveData);

	if (!player || !m_engine()->IsConnected() || !m_engine()->IsInGame())
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND) && !(g_ctx.get_command()->m_buttons & IN_JUMP))
		return;

	player->m_vecVelocity() *= Vector(1.2f, 1.2f, 1.f);
}

using ProcessMovement_t = void(__thiscall*)(void*, player_t*, CMoveData*);
void __fastcall hooks::hooked_processmovement(void* ecx, void* edx, player_t* ent, CMoveData* data)
{
	static auto original_fn = game_movement_hook->get_func_address <ProcessMovement_t>(1);
	data->m_bGameCodeMovedPlayer = false;
	original_fn(ecx, ent, data);
}


bool __fastcall hooks::Hooked_IsPaused(void* ecx, void* edx) 
{
	static auto IsPaused = engine_hook->get_func_address < 	bool(__thiscall*)(void*, void*) >(90);

	static auto return_to_extrapolation = util::FindSignature(crypt_str("client.dll"), crypt_str("FF D0 A1 ?? ?? ?? ?? B9 ?? ?? ?? ?? D9 1D ?? ?? ?? ?? FF 50 34 85 C0 74 22 8B 0D ?? ?? ?? ??") + 0x29);
	static auto return_to_interpolate = util::FindSignature(crypt_str("client.dll"), crypt_str("84 C0 74 07 C6 05 ? ? ? ? ? 8B"));

	if (_ReturnAddress() == (void*)return_to_extrapolation)
		return true;

	if (_ReturnAddress() == (uint32_t*)return_to_interpolate)
		return true;

	return IsPaused(ecx, edx);
}