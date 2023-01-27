#include "BoneManager.h"
//#include "extra.h"


namespace g_BoneManager
{
	void BuildMatrix(player_t* pPlayer, matrix3x4_t* aMatrix, bool bSafeMatrix)
	{
		std::array < AnimationLayer, 13 > aAnimationLayers;

		float_t flCurTime = m_globals()->m_curtime;
		float_t flRealTime = m_globals()->m_realtime;
		float_t flFrameTime = m_globals()->m_frametime;
		float_t flAbsFrameTime = m_globals()->m_absoluteframetime;
		int32_t iFrameCount = m_globals()->m_framecount;
		int32_t iTickCount = m_globals()->m_tickcount;
		float_t flInterpolation = m_globals()->m_interpolation_amount;

		m_globals()->m_curtime = pPlayer->m_flSimulationTime();
		m_globals()->m_realtime = pPlayer->m_flSimulationTime();
		m_globals()->m_frametime = m_globals()->m_intervalpertick;
		m_globals()->m_absoluteframetime = m_globals()->m_intervalpertick;
		m_globals()->m_framecount = INT_MAX;
		m_globals()->m_tickcount = TIME_TO_TICKS(pPlayer->m_flSimulationTime());
		m_globals()->m_interpolation_amount = 0.0f;

		int32_t nClientEffects = pPlayer->m_nClientEffects();
		int32_t nLastSkipFramecount = pPlayer->m_nLastSkipFramecount();
		int32_t nOcclusionMask = pPlayer->m_nOcclusionMask();
		int32_t nOcclusionFrame = pPlayer->m_nOcclusionFrame();
		int32_t iEffects = pPlayer->m_fEffects();
		bool bJiggleBones = pPlayer->m_bJiggleBones();
		bool bMaintainSequenceTransition = pPlayer->m_bMaintainSequenceTransition();
		Vector vecAbsOrigin = pPlayer->GetAbsOrigin();

		int32_t iMask = BONE_USED_BY_ANYTHING;
		if (bSafeMatrix)
			iMask = BONE_USED_BY_HITBOX;

		std::memcpy(aAnimationLayers.data(), pPlayer->get_animlayers(), sizeof(AnimationLayer) * 13);

		pPlayer->invalidate_bone_cache();
		pPlayer->GetBoneAccessor().m_ReadableBones = NULL;
		pPlayer->GetBoneAccessor().m_WritableBones = NULL;

		if (pPlayer->get_animation_state())
			pPlayer->get_animation_state()->m_pLastActiveWeapon = pPlayer->get_animation_state()->m_pActiveWeapon;

		pPlayer->m_nOcclusionFrame() = 0;
		pPlayer->m_nOcclusionMask() = 0;
		pPlayer->m_nLastSkipFramecount() = 0;

		if (pPlayer != g_ctx.local())
			pPlayer->set_abs_origin(pPlayer->GetOrigin());

		pPlayer->m_fEffects() |= 8;
		pPlayer->m_nClientEffects() |= 2;
		pPlayer->m_bJiggleBones() = false;
		pPlayer->m_bMaintainSequenceTransition() = false;

		if (pPlayer == g_ctx.local())
		{
			pPlayer->get_animlayers()[12].m_flWeight = 0.0f;
			if (pPlayer->sequence_activity(pPlayer->get_animlayers()[3].m_nSequence) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
			{
				pPlayer->get_animlayers()[3].m_flCycle = 0.0f;
				pPlayer->get_animlayers()[3].m_flWeight = 0.0f;
			}
		}

		g_Globals.m_AnimationData.m_bSetupBones = true;
		pPlayer->SetupBones(aMatrix, MAXSTUDIOBONES, iMask, pPlayer->m_flSimulationTime());
		g_Globals.m_AnimationData.m_bSetupBones = false;

		pPlayer->m_bMaintainSequenceTransition() = bMaintainSequenceTransition;
		pPlayer->m_nClientEffects() = nClientEffects;
		pPlayer->m_bJiggleBones() = bJiggleBones;
		pPlayer->m_fEffects() = iEffects;
		pPlayer->m_nLastSkipFramecount() = nLastSkipFramecount;
		pPlayer->m_nOcclusionFrame() = nOcclusionFrame;
		pPlayer->m_nOcclusionMask() = nOcclusionMask;

		if (pPlayer != g_ctx.local())
			pPlayer->set_abs_origin(vecAbsOrigin);

		std::memcpy(pPlayer->get_animlayers(), aAnimationLayers.data(), sizeof(AnimationLayer) * 13);

		m_globals()->m_curtime = flCurTime;
		m_globals()->m_realtime = flRealTime;
		m_globals()->m_frametime = flFrameTime;
		m_globals()->m_absoluteframetime = flAbsFrameTime;
		m_globals()->m_framecount = iFrameCount;
		m_globals()->m_tickcount = iTickCount;
		m_globals()->m_interpolation_amount = flInterpolation;
	}
}

