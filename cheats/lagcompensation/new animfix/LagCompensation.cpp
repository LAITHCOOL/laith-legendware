#include "LagCompensation.h"
#include "Animations.h"


namespace g_LagCompensation
{
	void Instance()
	{
		for (int32_t iPlayerID = 1; iPlayerID <= 64; iPlayerID++)
		{
			player_t* pPlayer = static_cast <player_t*> (m_entitylist()->GetClientEntity(iPlayerID));
			if (!pPlayer || !pPlayer->is_player() || !pPlayer->is_alive() || pPlayer->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
			{
				g_AnimationSync::MarkAsDormant(iPlayerID);

				g_Globals.m_CachedPlayerRecords[iPlayerID].clear();
				continue;
			}

			if (pPlayer->IsDormant())
			{
				g_AnimationSync::MarkAsDormant(iPlayerID);
				continue;
			}

			if (g_AnimationSync::HasLeftOutOfDormancy(iPlayerID))
				g_Globals.m_CachedPlayerRecords[iPlayerID].clear();

			if (pPlayer->m_flOldSimulationTime() >= pPlayer->m_flSimulationTime())
			{
				if (pPlayer->m_flOldSimulationTime() > pPlayer->m_flSimulationTime())
					g_Globals.m_CachedPlayerRecords[iPlayerID].clear();

				continue;
			}

			bool bHasPreviousRecord = false;
			if (g_Globals.m_CachedPlayerRecords[iPlayerID].empty())
				bHasPreviousRecord = false;
			else if (TIME_TO_TICKS(fabs(pPlayer->m_flSimulationTime() - g_Globals.m_CachedPlayerRecords[iPlayerID].back().m_SimulationTime)) <= 17)
				bHasPreviousRecord = true;

			C_LagRecord LagRecord;
			 FillRecord(pPlayer, LagRecord);

			 UnsetBreakingLagCompensation(iPlayerID);
			if (bHasPreviousRecord)
			{
				if (LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(11).m_flCycle ==  GetPreviousRecord(iPlayerID).m_AnimationLayers.at(ROTATE_SERVER).at(11).m_flCycle)
					continue;

				g_AnimationSync::SetPreviousRecord(iPlayerID,  GetPreviousRecord(iPlayerID));
				if ((LagRecord.m_Origin -  GetPreviousRecord(iPlayerID).m_Origin).LengthSqr() > 4096.0f)
				{
					 SetBreakingLagCompensation(iPlayerID);
					 ClearRecords(iPlayerID);
				}
			}

			g_Globals.m_CachedPlayerRecords[iPlayerID].emplace_back(LagRecord);
			while (g_Globals.m_CachedPlayerRecords[iPlayerID].size() > 32)
				g_Globals.m_CachedPlayerRecords[iPlayerID].pop_front();
		}
	}

	void FillRecord(player_t* pPlayer, C_LagRecord& LagRecord)
	{
		std::memcpy(LagRecord.m_AnimationLayers.at(ROTATE_SERVER).data(), pPlayer->get_animlayers(), sizeof(AnimationLayer) * 13);

		LagRecord.m_UpdateDelay = 1;
		LagRecord.m_Flags = pPlayer->m_fFlags();
		LagRecord.m_EyeAngles = pPlayer->m_angEyeAngles1();
		LagRecord.m_LowerBodyYaw = pPlayer->m_flLowerBodyYawTarget();
		LagRecord.m_Mins = pPlayer->GetCollideable()->OBBMins();
		LagRecord.m_Maxs = pPlayer->GetCollideable()->OBBMaxs();
		LagRecord.m_SimulationTime = pPlayer->m_flSimulationTime();
		LagRecord.m_Origin = pPlayer->GetOrigin();
		LagRecord.m_AbsOrigin = pPlayer->GetAbsOrigin();
		LagRecord.m_DuckAmount = pPlayer->m_flDuckAmount();
		LagRecord.m_Velocity = pPlayer->m_vecVelocity();

		if (pPlayer->m_hActiveWeapon().Get())
			if (pPlayer->m_hActiveWeapon()->m_fLastShotTime() <= pPlayer->m_flSimulationTime())
				if (pPlayer->m_hActiveWeapon()->m_fLastShotTime() > pPlayer->m_flOldSimulationTime())
					LagRecord.m_bIsShooting = true;
	}

	void ResetData()
	{
		for (int32_t iPlayerID = 0; iPlayerID < 65; iPlayerID++)
		{
			if (!g_Globals.m_CachedPlayerRecords[iPlayerID].empty())
				g_Globals.m_CachedPlayerRecords[iPlayerID].clear();

			m_BreakingLagcompensation[iPlayerID] = false;
		}
	}

	bool IsValidTime(float_t flTime)
	{
		int32_t nTickbase = g_ctx.local()->m_nTickBase();
		if (nTickbase <= 0)
			return false;

		float_t flDeltaTime = fmin(g_Networking->GetLatency() +  GetLerpTime(), 0.2f) - TICKS_TO_TIME(nTickbase - TIME_TO_TICKS(flTime));
		if (fabs(flDeltaTime) > 0.2f)
			return false;

		int32_t nDeadTime = TICKS_TO_TIME(g_Networking->GetServerTick()) - 0.2f;
		if (TIME_TO_TICKS(flTime +  GetLerpTime()) < nDeadTime)
			return false;

		return true;
	}

	float_t GetLerpTime()
	{
		static auto m_SvMinUpdateRate = m_cvar()->FindVar(("sv_minupdaterate"));
		static auto m_SvMaxUpdateRate = m_cvar()->FindVar(("sv_maxupdaterate"));
		static auto m_SvClientMinInterpRatio = m_cvar()->FindVar(("sv_client_min_interp_ratio"));
		static auto m_SvClientMaxInterpRatio = m_cvar()->FindVar(("sv_client_max_interp_ratio"));
		static auto m_ClInterp = m_cvar()->FindVar(("cl_interp"));
		static auto m_ClInterpRatio = m_cvar()->FindVar(("cl_interp_ratio"));
		static auto m_ClUpdateRate = m_cvar()->FindVar(("cl_updaterate"));

		float_t flUpdateRate = std::clamp(m_ClUpdateRate->GetFloat(), m_SvMinUpdateRate->GetFloat(), m_SvMaxUpdateRate->GetFloat());
		float_t flLerpRatio = std::clamp(m_ClInterpRatio->GetFloat(), m_SvClientMinInterpRatio->GetFloat(), m_SvClientMaxInterpRatio->GetFloat());
		return std::clamp(flLerpRatio / flUpdateRate, m_ClInterp->GetFloat(), 1.0f);
	}
}

