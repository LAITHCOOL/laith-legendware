#include "prediction_system.h"

void engineprediction::fix_local_commands()
{
	auto GetCorrectionTicks = []() -> int
	{
		float v1;
		float v4;
		float v6;

		static auto sv_clockcorrection_msecs = m_cvar()->FindVar("sv_clockcorrection_msecs");
		if (!sv_clockcorrection_msecs)
			return -1;

		v1 = sv_clockcorrection_msecs->GetFloat();
		v4 = 1.0f;
		v6 = v1 / 1000.0f;
		if (v6 <= 1.0f)
		{
			v4 = 0.0f;
			if (v6 >= 0.0f)
				v4 = v1 / 1000.0f;
		}

		return ((v4 / m_globals()->m_intervalpertick) + 0.5f);
	};

	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return;

	m_engine()->FireEvents();

	auto v18 = GetCorrectionTicks();
	if (v18 == -1)
		m_last_cmd_delta = 0;
	else
	{
		auto v19 = m_clientstate()->m_iServerTick;
		int m_sim_ticks = TIME_TO_TICKS(g_ctx.local()->m_flSimulationTime());

		if (m_sim_ticks > TIME_TO_TICKS(g_ctx.local()->m_flOldSimulationTime()) && (std::fabs(m_sim_ticks - v19) <= v18))
			m_last_cmd_delta = m_sim_ticks - v19;
	}
}


void engineprediction::store(StoredData_t* ndata, C_CommandContext* cctx, int m_tick)
{
	if (!g_ctx.local() || !g_ctx.local()->is_alive())
	{
		reset();
		return;
	}

	ndata->m_tickbase = g_ctx.local()->m_nTickBase();
	ndata->m_command_number = cctx->cmd.m_command_number;
	ndata->m_punch = g_ctx.local()->m_aimPunchAngle();
	ndata->m_punch_vel = g_ctx.local()->m_aimPunchAngleVel();
	ndata->m_viewPunchAngle = g_ctx.local()->m_viewPunchAngle();
	ndata->m_view_offset = g_ctx.local()->m_vecViewOffset();
	ndata->m_view_offset.z = std::fminf(std::fmaxf(ndata->m_view_offset.z, 46.0f), 64.0f);
	ndata->m_vecVelocity = g_ctx.local()->m_vecVelocity();
	ndata->m_vecOrigin = g_ctx.local()->m_vecOrigin();
	ndata->m_flFallVelocity = g_ctx.local()->m_flFallVelocity();
	ndata->m_flThirdpersonRecoil = g_ctx.local()->m_flThirdpersonRecoil();
	ndata->m_duck_amount = g_ctx.local()->m_flDuckAmount();
	ndata->m_velocity_modifier = g_ctx.local()->m_flVelocityModifier();
	ndata->m_tick = m_tick;
	ndata->m_is_filled = true;


}

void engineprediction::apply(int time)
{
	if (!g_ctx.local()->is_alive())
	{
		reset();
		return;
	}

	StoredData_t* data = &m_data[time % MULTIPLAYER_BACKUP];
	if (!data || !data->m_is_filled || data->m_command_number != time || (time - data->m_command_number) > MULTIPLAYER_BACKUP)
		return;

	Vector aim_punch_delta = g_ctx.local()->m_aimPunchAngle() - data->m_punch,
		aim_punch_vel_delta = g_ctx.local()->m_aimPunchAngleVel() - data->m_punch_vel,
		view_punch_delta = g_ctx.local()->m_viewPunchAngle() - data->m_viewPunchAngle;

	Vector view_offset_delta = g_ctx.local()->m_vecViewOffset() - data->m_view_offset,
		velocity_diff = g_ctx.local()->m_vecVelocity() - data->m_vecVelocity;

	float duck_amount_delta = g_ctx.local()->m_flDuckAmount() - data->m_duck_amount,
		modifier_delta = g_ctx.local()->m_flVelocityModifier() - data->m_velocity_modifier;

	if (std::abs(aim_punch_delta.x) <= 0.03125f && std::abs(aim_punch_delta.y) <= 0.03125f && std::abs(aim_punch_delta.z) <= 0.03125f)
		g_ctx.local()->m_aimPunchAngle() = data->m_punch;

	if (std::abs(aim_punch_vel_delta.x) <= 0.03125f && std::abs(aim_punch_vel_delta.y) <= 0.03125f && std::abs(aim_punch_vel_delta.z) <= 0.03125f)
		g_ctx.local()->m_aimPunchAngleVel() = data->m_punch_vel;

	if (std::abs(view_offset_delta.z) <= 0.03125f)
		g_ctx.local()->m_vecViewOffset().z = data->m_view_offset.z;

	if (std::abs(view_punch_delta.x) <= 0.03125f && std::abs(view_punch_delta.y) <= 0.03125f && std::abs(view_punch_delta.z) <= 0.03125f)
		g_ctx.local()->m_viewPunchAngle() = data->m_viewPunchAngle;

	if (std::abs(velocity_diff.x) <= 0.03125f && std::abs(velocity_diff.y) <= 0.03125f && std::abs(velocity_diff.z) <= 0.03125f)
		g_ctx.local()->m_vecVelocity() = data->m_vecVelocity;

	if (std::abs(g_ctx.local()->m_flThirdpersonRecoil() - data->m_flThirdpersonRecoil) <= 0.03125f)
		g_ctx.local()->m_flThirdpersonRecoil() = data->m_flThirdpersonRecoil;

	if (std::abs(duck_amount_delta) <= 0.03125f)
		g_ctx.local()->m_flDuckAmount() = data->m_duck_amount;

	if (std::abs(g_ctx.local()->m_flFallVelocity() - data->m_flFallVelocity) <= 0.03125f)
		g_ctx.local()->m_flFallVelocity() = data->m_flFallVelocity;

	if (std::abs(modifier_delta) <= 0.00625f)
		g_ctx.local()->m_flVelocityModifier() = data->m_velocity_modifier;
}

void engineprediction::detect_prediction_error(StoredData_t* data, int m_tick)
{
	if (!data || !g_ctx.local() || !g_ctx.local()->is_alive() || data->m_command_number != m_tick || !data->m_is_filled || data->m_tick > (m_globals()->m_tickcount + 8))
		return;

	static auto is_out_of_epsilon_float = [](float a, float b, float m_epsilon) -> bool {
		return std::fabsf(a - b) > m_epsilon;
	};

	static auto is_out_of_epsilon_vec = [](Vector a, Vector b, float m_epsilon) -> bool {
		return std::fabsf(a.x - b.x) > m_epsilon || std::fabsf(a.y - b.y) > m_epsilon || std::fabsf(a.z - b.z) > m_epsilon;
	};

	static auto is_out_of_epsilon_ang = [](Vector a, Vector b, float m_epsilon) -> bool {
		return std::fabsf(a.x - b.x) > m_epsilon || std::fabsf(a.y - b.y) > m_epsilon || std::fabsf(a.z - b.z) > m_epsilon;
	};

	if (is_out_of_epsilon_ang(g_ctx.local()->m_aimPunchAngle(), data->m_punch, 0.5f))
	{
		data->m_punch = g_ctx.local()->m_aimPunchAngle();
		m_prediction()->force_repredict();
	}
	else
		g_ctx.local()->m_aimPunchAngle() = data->m_punch;

	if (is_out_of_epsilon_ang(g_ctx.local()->m_aimPunchAngleVel(), data->m_punch_vel, 0.5f))
	{
		data->m_punch_vel = g_ctx.local()->m_aimPunchAngleVel();
		m_prediction()->force_repredict();
	}
	else
		g_ctx.local()->m_aimPunchAngleVel() = data->m_punch_vel;

	if (is_out_of_epsilon_float(g_ctx.local()->m_vecViewOffset().z, data->m_view_offset.z, 0.5f))
	{
		data->m_view_offset.z = g_ctx.local()->m_vecViewOffset().z;
		m_prediction()->force_repredict();
	}
	else
		g_ctx.local()->m_vecViewOffset().z = data->m_view_offset.z;

	if (is_out_of_epsilon_vec(g_ctx.local()->m_vecVelocity(), data->m_vecVelocity, 0.5f))
	{
		data->m_vecVelocity = g_ctx.local()->m_vecVelocity();
		m_prediction()->force_repredict();
	}
	else
		g_ctx.local()->m_vecVelocity() = data->m_vecVelocity;

	if (is_out_of_epsilon_float(g_ctx.local()->m_flVelocityModifier(), data->m_velocity_modifier, 0.00625f))
	{
		data->m_velocity_modifier = g_ctx.local()->m_flVelocityModifier();
		m_prediction()->force_repredict();
	}
	else
		g_ctx.local()->m_flVelocityModifier() = data->m_velocity_modifier;

	if ((g_ctx.local()->m_vecOrigin() - data->m_vecOrigin).LengthSqr() >= 1.f)
	{
		data->m_vecOrigin = g_ctx.local()->m_vecOrigin();
		m_prediction()->force_repredict();
	}
}


void engineprediction::reset()
{
	m_data.fill(StoredData_t());
}

namespace engine_prediction
{
	void update()
	{
		// https://github.com/click4dylan/CSGO_GameMovement_Reversed/blob/24d68f47761e5635437aee473860f13e6adbfe6b/IGameMovement.cpp#L703
		if (m_stored_variables.m_flVelocityModifier < 1.f)
		{
			*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x24)) = 1; //m_bPreviousAckHadErrors
			*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x1C)) = 0; //m_nCommandsPredicted
		}

		if (m_clientstate()->iDeltaTick > 0)
			m_prediction()->Update(m_clientstate()->iDeltaTick, m_clientstate()->iDeltaTick > 0, m_clientstate()->nLastCommandAck, m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands);
	}

	void StartCommand(player_t* player, CUserCmd* cmd)
	{
		memset(&m_stored_variables.m_MoveData, 0, sizeof(CMoveData));

		if (!m_stored_variables.prediction_player || !m_stored_variables.prediction_seed)
		{
			m_stored_variables.prediction_seed = *reinterpret_cast<std::uintptr_t*>(std::uintptr_t(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 47 40 A3"))) + 4);
			m_stored_variables.prediction_player = *reinterpret_cast<std::uintptr_t*>(std::uintptr_t(util::FindSignature(crypt_str("client.dll"), crypt_str("0F 5B C0 89 35"))) + 5);
		}

		*reinterpret_cast<int*>(m_stored_variables.prediction_seed) = cmd ? cmd->m_random_seed : -1;
		*reinterpret_cast<int*>(m_stored_variables.prediction_player) = std::uintptr_t(player);
		player->m_pCurrentCommand() = cmd;
		player->m_PlayerCommand() = cmd;
	}

	void FinishCommand(player_t* player)
	{
		if (m_stored_variables.prediction_player && m_stored_variables.prediction_seed)
		{
			*reinterpret_cast<int*>(m_stored_variables.prediction_seed) = 0xffffffff;
			*reinterpret_cast<int*>(m_stored_variables.prediction_player) = 0;
		}

		player->m_pCurrentCommand() = 0;
	}

	void UpdateButtonState(player_t* player, CUserCmd* cmd)
	{
		static auto button_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 86 ? ? ? ? 8B C8 33 CA")) + 0x2);

		static auto buttons_pres_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("F7 D2 89 86 ? ? ? ? 23 D1")) + 0x4);

		static auto buttons_rel_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("89 96 ? ? ? ? F3 0F 10 40 ? 8B 11")) + 0x2);

		static auto button_last_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("89 86 ?? ?? ?? ?? 8B C1 89 96 ?? ?? ?? ??")) + 0x2);

		const int original_buttons = cmd->m_buttons;
		int* player_buttons_last = reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + button_ptr);
		const int buttons_changed = original_buttons ^ *player_buttons_last;

		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + button_last_ptr) = *player_buttons_last;
		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + button_ptr) = original_buttons;
		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + buttons_pres_ptr) = original_buttons & buttons_changed;
		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + buttons_rel_ptr) = buttons_changed & ~original_buttons;
	}

	void BuildDataAfterPredict(player_t* player, CUserCmd* cmd, weapon_t* m_weapon)
	{
		player->get_max_desync_delta();

		m_weapon->update_accuracy_penality();

		m_stored_variables.m_flSpread = m_weapon->get_spread();
		m_stored_variables.m_flInaccuracy = m_weapon->get_inaccuracy();
	}

	void BackupData(player_t* player, CUserCmd* cmd)
	{
		engineprediction::get().backup_data.flags = m_stored_variables.unpred_flags = m_stored_variables.m_fFlags = player->m_fFlags();
		engineprediction::get().backup_data.velocity = m_stored_variables.unpred_vel = m_stored_variables.m_vecVelocity = player->m_vecVelocity();
		m_stored_variables.m_vecOrigin = player->m_vecOrigin();
		m_stored_variables.tickbase = player->m_nTickBase();
		m_stored_variables.m_in_prediction = m_prediction()->InPrediction;
		m_stored_variables.m_is_first_time_predicted = m_prediction()->IsFirstTimePredicted;
		m_stored_variables.m_iButtons = cmd->m_buttons;

		m_stored_variables.m_cur_time = m_globals()->m_curtime;
		m_stored_variables.m_frame_time = m_globals()->m_frametime;
		m_stored_variables.m_fTickcount = m_globals()->m_tickcount;
	}

	void predict(CUserCmd* cmd, player_t* player)
	{
		weapon_t* m_weapon_local = player->m_hActiveWeapon().Get();

		if (!m_engine()->IsInGame() || !player || !m_weapon_local)
			return;


		StartCommand(player, cmd);

		BackupData(player, cmd);

		m_globals()->m_curtime = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);
		m_globals()->m_frametime = m_prediction()->EnginePaused ? 0.f : m_globals()->m_intervalpertick;
		m_globals()->m_tickcount = g_ctx.globals.fixed_tickbase;

		m_prediction()->InPrediction = true;
		m_prediction()->IsFirstTimePredicted = false;


		static auto bf_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 86 ?? ?? ?? ?? 09 47 30 8B 86 ?? ?? ?? ??")) + 0x2);
		static auto bd_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 86 ? ? ? ? F7 D0 21 47 30")) + 0x2);

		auto buttons_forced = *reinterpret_cast<std::uint32_t*>(std::uintptr_t(player) + bf_ptr);
		auto buttons_disabled = *reinterpret_cast<std::uint32_t*>(std::uintptr_t(player) + bd_ptr);

		cmd->m_buttons |= buttons_forced;
		cmd->m_buttons &= ~buttons_disabled;

		if (cmd->m_weaponselect)
		{
			if (const auto weapon = reinterpret_cast<weapon_t*>(m_entitylist()->GetClientEntity(cmd->m_weaponselect)))
			{
				if (const auto weapon_data = weapon->get_csweapon_info())
					player->select_item(weapon_data->szWeaponName, cmd->m_weaponsubtype);
			}
		}

		static auto using_standard_weapons_in_vehicle = reinterpret_cast <bool(__thiscall*)(void*)>(util::FindSignature(crypt_str("client.dll"), crypt_str("56 57 8B F9 8B 97 ? ? ? ? 83 FA FF 74 43")));
		const auto vehicle = reinterpret_cast<player_t*>(player->m_hVehicle().Get());
		static int offset_impulse = netvars::get().get_offset(crypt_str("CCSPlayer"), crypt_str("m_nImpulse"));

		if (cmd->m_impulse)
		{
			if (!vehicle || using_standard_weapons_in_vehicle(player))
				*reinterpret_cast<std::uint32_t*>(std::uintptr_t(player) + offset_impulse) = cmd->m_impulse;
		}

		UpdateButtonState(player, cmd);

		m_prediction()->CheckMovingGround(player, m_globals()->m_frametime);
		m_prediction()->SetLocalViewAngles(cmd->m_viewangles);

		m_movehelper()->set_host(player);
		m_gamemovement()->StartTrackPredictionErrors(player);

		player->RunPreThink();

		player->RunThink();

		m_prediction()->SetupMove(player, cmd, m_movehelper(), &m_stored_variables.m_MoveData);

		if (!vehicle)
			m_gamemovement()->ProcessMovement(player, &m_stored_variables.m_MoveData);
		else
			call_virtual<void(__thiscall*)(void*, player_t*, CMoveData*)>(vehicle, 5)(vehicle, player, &m_stored_variables.m_MoveData);

		m_prediction()->FinishMove(player, cmd, &m_stored_variables.m_MoveData);

		m_movehelper()->process_impacts();

		player->RunPostThink();

		m_gamemovement()->FinishTrackPredictionErrors(player);
		m_movehelper()->set_host(nullptr);
		m_gamemovement()->Reset();

		m_prediction()->IsFirstTimePredicted = m_stored_variables.m_is_first_time_predicted;
		m_prediction()->InPrediction = m_stored_variables.m_in_prediction;

		BuildDataAfterPredict(g_ctx.local(), cmd, m_weapon_local);

		auto viewmodel = g_ctx.local()->m_hViewModel().Get();

		if (viewmodel)
		{
			engineprediction::get().viewmodel_data.weapon = viewmodel->m_hWeapon().Get();
			engineprediction::get().viewmodel_data.viewmodel_index = viewmodel->m_nViewModelIndex();
			engineprediction::get().viewmodel_data.sequence = viewmodel->m_nSequence();
			engineprediction::get().viewmodel_data.animation_parity = viewmodel->m_nAnimationParity();

			engineprediction::get().viewmodel_data.cycle = viewmodel->m_flCycle();
			engineprediction::get().viewmodel_data.animation_time = viewmodel->m_flAnimTime();
		}
	}

	void restore(player_t* player)
	{
		if (!m_engine()->IsInGame() || !player || !player->is_alive())
			return;

		FinishCommand(player);

		m_globals()->m_curtime = m_stored_variables.m_cur_time;
		m_globals()->m_frametime = m_stored_variables.m_frame_time;
		m_globals()->m_tickcount = m_stored_variables.m_fTickcount;
	}

	void patch_attack_packet(CUserCmd* cmd, bool restore)
	{
		static bool m_bLastAttack = false;
		static bool m_bInvalidCycle = false;
		static float m_flLastCycle = 0.f;

		if (!g_ctx.local())
			return;

		if (restore)
		{
			m_bLastAttack = cmd->m_weaponselect || (cmd->m_buttons & IN_ATTACK);
			m_flLastCycle = g_ctx.local()->m_flCycle();
		}
		else if (m_bLastAttack && !m_bInvalidCycle)
			m_bInvalidCycle = g_ctx.local()->m_flCycle() == 0.f && m_flLastCycle > 0.f;

		if (m_bInvalidCycle)
			g_ctx.local()->m_flCycle() = m_flLastCycle;
	}

	void UpdateVelocityModifier()
	{
		if (!g_ctx.local() || !g_ctx.local()->is_alive() || !m_clientstate())
			return;

		static int m_iLastCmdAck = 0;
		static float m_flNextCmdTime = 0.f;

		if (m_iLastCmdAck != m_clientstate()->nLastCommandAck || m_flNextCmdTime != m_clientstate()->flNextCmdTime)
		{
			if (m_stored_variables.m_flVelocityModifier != g_ctx.local()->m_flVelocityModifier())
			{
				*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x24)) = true;
				m_stored_variables.m_flVelocityModifier = g_ctx.local()->m_flVelocityModifier();
			}

			m_iLastCmdAck = m_clientstate()->nLastCommandAck;
			m_flNextCmdTime = m_clientstate()->flNextCmdTime;
		}
	}

	void ProcessInterpolation(bool bPostFrame, player_t* player, CUserCmd* cmd)
	{
		if (!g_ctx.local() || !g_ctx.local()->is_alive())
			return;

		auto net_channel_info = m_engine()->GetNetChannelInfo();
		if (!net_channel_info)
			return;

		auto choked = abs(TIME_TO_TICKS(player->m_flSimulationTime() - player->m_flOldSimulationTime()) - 1);  // choked противника

		auto outgoing = net_channel_info->GetLatency(FLOW_OUTGOING);
		auto ingoing = net_channel_info->GetLatency(FLOW_INCOMING);
		if (!bPostFrame)
		{
			m_stored_variables.m_FinalPredictedTick = g_ctx.local()->m_nFinalPredictedTick();

			int m_iTickCount = g_ctx.globals.fakeducking ? (14 - m_clientstate()->iChokedCommands) : m_globals()->m_tickcount;	  // вместо 14 мы должны калькулировать choked commands
			g_ctx.local()->m_nFinalPredictedTick() = m_iTickCount + TIME_TO_TICKS(outgoing + ingoing);		// outgoing первый потому что chokedcommands, а ingoing потому что tickcount
			return;
		}

		g_ctx.local()->m_nFinalPredictedTick() = m_stored_variables.m_FinalPredictedTick;

		predict(cmd, player);
	}
}