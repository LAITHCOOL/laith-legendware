#include "Animations.h"
//#include "extra.h"


__forceinline float AngleNormalize(float angle)
{
	angle = fmodf(angle, 360.0f);
	if (angle > 180)
	{
		angle -= 360;
	}
	if (angle < -180)
	{
		angle += 360;
	}
	return angle;
}

namespace g_AnimationSync
{
	void Instance()
	{
		for (int32_t iPlayerID = 1; iPlayerID <= 64; iPlayerID++)
		{
			player_t* pPlayer = static_cast <player_t*>(m_entitylist()->GetClientEntity(iPlayerID));
			if (!pPlayer || !pPlayer->is_player() || !pPlayer->is_alive() || pPlayer->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
			{
				g_Globals.m_ResolverData.m_AnimResoled[iPlayerID] = false;
				g_Globals.m_ResolverData.m_MissedShots[iPlayerID] = 0;
				g_Globals.m_ResolverData.m_LastMissedShots[iPlayerID] = 0;

				continue;
			}

			bool bHasPreviousRecord = false;
			if (pPlayer->m_flOldSimulationTime() >= pPlayer->m_flSimulationTime())
			{
				if (pPlayer->m_flOldSimulationTime() > pPlayer->m_flSimulationTime())
					UnmarkAsDormant(iPlayerID);

				continue;
			}

			auto& LagRecords = g_Globals.m_CachedPlayerRecords[iPlayerID];
			if (LagRecords.empty())
				continue;

			C_LagRecord PreviousRecord = m_PreviousRecord[iPlayerID];
			if (TIME_TO_TICKS(fabs(pPlayer->m_flSimulationTime() - PreviousRecord.m_SimulationTime)) <= 17)
				bHasPreviousRecord = true;

			C_LagRecord& LatestRecord = LagRecords.back();
			if ( HasLeftOutOfDormancy(iPlayerID))
				bHasPreviousRecord = false;

			if (LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(11).m_flCycle == PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(11).m_flCycle)
			{
				pPlayer->m_flSimulationTime() = pPlayer->m_flOldSimulationTime();
				continue;
			}

			LatestRecord.m_UpdateDelay = TIME_TO_TICKS(pPlayer->m_flSimulationTime() -  GetPreviousRecord(iPlayerID).m_SimulationTime);
			if (LatestRecord.m_UpdateDelay > 17)
				LatestRecord.m_UpdateDelay = 1;

			if (LatestRecord.m_UpdateDelay < 1)
				continue;

			if (bHasPreviousRecord)
			{
				float_t flWeight = 1.0f - LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(11).m_flWeight;
				if (flWeight > 0.0f)
				{
					float_t flPreviousRate =  GetPreviousRecord(pPlayer->EntIndex()).m_AnimationLayers.at(ROTATE_SERVER).at(11).m_flPlaybackRate;
					float_t flCurrentRate = LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(11).m_flPlaybackRate;

					if (flPreviousRate == flCurrentRate)
					{
						int32_t iPreviousSequence =  GetPreviousRecord(pPlayer->EntIndex()).m_AnimationLayers.at(ROTATE_SERVER).at(11).m_nSequence;
						int32_t iCurrentSequence = LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(11).m_nSequence;

						if (iPreviousSequence == iCurrentSequence)
						{
							float_t flSpeedNormalized = (flWeight / 2.8571432f) + 0.55f;
							if (flSpeedNormalized > 0.0f)
							{
								float_t flSpeed = flSpeedNormalized * pPlayer->GetMaxPlayerSpeed();
								if (flSpeed > 0.0f)
									if (LatestRecord.m_Velocity.Length2D() > 0.0f)
										LatestRecord.m_Velocity = (LatestRecord.m_Velocity / LatestRecord.m_Velocity.Length()) * flSpeed;
							}
						}
					}
				}
			}

			std::array < AnimationLayer, 13 > AnimationLayers;
			std::array < float_t, 24 > PoseParameters;
			C_CSGOPlayerAnimationState AnimationState;

			// бэкапим данные
			std::memcpy(AnimationLayers.data(), pPlayer->get_animlayers(), sizeof(AnimationLayer) * 13);
			std::memcpy(PoseParameters.data(), pPlayer->GetBaseAnimating().data(), sizeof(float_t) * 24);
			std::memcpy(&AnimationState, pPlayer->get_animation_state1(), sizeof(AnimationState));

			// ротейтим игрока для сэйфпоинтов
			for (int32_t i = ROTATE_LEFT; i <= ROTATE_LOW_RIGHT; i++)
			{
				// ротейтим игрока твою мать 
				 UpdatePlayerAnimations(pPlayer, LatestRecord, PreviousRecord, bHasPreviousRecord, i);

				// сохраняем некоторые данные
				std::memcpy(LatestRecord.m_AnimationLayers.at(i).data(), pPlayer->get_animlayers(), sizeof(AnimationLayer) * 13);

				// сетапим лееры сервера
				std::memcpy(pPlayer->get_animlayers(), AnimationLayers.data(), sizeof(AnimationLayer) * 13);

				// сетапим кости
				if (i < ROTATE_LOW_LEFT)
					g_BoneManager::BuildMatrix(pPlayer, LatestRecord.m_Matricies[i].data(), true);

				// ресторим дефолтные данные
				std::memcpy(pPlayer->GetBaseAnimating().data(), PoseParameters.data(), sizeof(float_t) * 24);
				std::memcpy(pPlayer->get_animation_state1(), &AnimationState, sizeof(AnimationState));
			}

			if (!LatestRecord.m_bIsShooting)
			{
				if (LatestRecord.m_UpdateDelay > 1 && bHasPreviousRecord)
				{
					LatestRecord.m_RotationMode = g_Globals.m_ResolverData.m_LastBruteSide[iPlayerID];
					if (LatestRecord.m_Velocity.Length2D() > 1.0f)
					{
						float_t flLeftDelta = fabsf(LatestRecord.m_AnimationLayers.at(ROTATE_LEFT).at(6).m_flPlaybackRate - LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flPlaybackRate);
						float_t flLowLeftDelta = fabsf(LatestRecord.m_AnimationLayers.at(ROTATE_LOW_LEFT).at(6).m_flPlaybackRate - LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flPlaybackRate);
						float_t flLowRightDelta = fabsf(LatestRecord.m_AnimationLayers.at(ROTATE_LOW_RIGHT).at(6).m_flPlaybackRate - LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flPlaybackRate);
						float_t flRightDelta = fabsf(LatestRecord.m_AnimationLayers.at(ROTATE_RIGHT).at(6).m_flPlaybackRate - LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flPlaybackRate);
						float_t flCenterDelta = fabsf(LatestRecord.m_AnimationLayers.at(ROTATE_CENTER).at(6).m_flPlaybackRate - LatestRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flPlaybackRate);

						LatestRecord.m_bAnimResolved = false;
						{
							float flLastDelta = 0.0f;
							if (flLeftDelta > flCenterDelta)
							{
								LatestRecord.m_RotationMode = ROTATE_LEFT;
								flLastDelta = flLastDelta;
							}

							if (flRightDelta > flLastDelta)
							{
								LatestRecord.m_RotationMode = ROTATE_RIGHT;
								flLastDelta = flRightDelta;
							}

							if (flLowLeftDelta > flLastDelta)
							{
								LatestRecord.m_RotationMode = ROTATE_LOW_LEFT;
								flLastDelta = flLowLeftDelta;
							}

							if (flLowRightDelta > flLastDelta)
							{
								LatestRecord.m_RotationMode = ROTATE_LOW_RIGHT;
								flLastDelta = flLowRightDelta;
							}

							LatestRecord.m_bAnimResolved = true;
						}

						bool bIsValidResolved = true;
						if (bHasPreviousRecord)
						{
							if (fabs((LatestRecord.m_Velocity - PreviousRecord.m_Velocity).Length2D()) > 5.0f || PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(7).m_flWeight < 1.0f)
								bIsValidResolved = false;
						}

						g_Globals.m_ResolverData.m_AnimResoled[iPlayerID] = LatestRecord.m_bAnimResolved;
						if (LatestRecord.m_bAnimResolved && LatestRecord.m_RotationMode != ROTATE_SERVER)
							g_Globals.m_ResolverData.m_LastBruteSide[iPlayerID] = LatestRecord.m_RotationMode;

						if (LatestRecord.m_RotationMode == ROTATE_SERVER)
							LatestRecord.m_RotationMode = g_Globals.m_ResolverData.m_LastBruteSide[iPlayerID];
					}
					else
					{
						if (g_Globals.m_ResolverData.m_LastBruteSide[iPlayerID] < 0)
						{
							float_t flFeetDelta = AngleNormalize(math::AngleDiff(AngleNormalize(pPlayer->m_flLowerBodyYawTarget()), AngleNormalize(pPlayer->m_angEyeAngles().y)));
							if (flFeetDelta > 0.0f)
								LatestRecord.m_RotationMode = ROTATE_LEFT;
							else
								LatestRecord.m_RotationMode = ROTATE_RIGHT;

							g_Globals.m_ResolverData.m_LastBruteSide[iPlayerID] = LatestRecord.m_RotationMode;
						}
					}

					g_Globals.m_ResolverData.m_AnimResoled[iPlayerID] = LatestRecord.m_bAnimResolved;
					if (LatestRecord.m_RotationMode == g_Globals.m_ResolverData.m_BruteSide[iPlayerID])
					{
						if (g_Globals.m_ResolverData.m_MissedShots[iPlayerID] > 0)
						{
							int iNewRotation = 0;
							switch (LatestRecord.m_RotationMode)
							{
							case ROTATE_LEFT: iNewRotation = ROTATE_RIGHT; break;
							case ROTATE_RIGHT: iNewRotation = ROTATE_LEFT; break;
							case ROTATE_LOW_RIGHT: iNewRotation = ROTATE_LOW_LEFT; break;
							case ROTATE_LOW_LEFT: iNewRotation = ROTATE_LOW_RIGHT; break;
							}

							LatestRecord.m_RotationMode = iNewRotation;
						}
					}

					 UpdatePlayerAnimations(pPlayer, LatestRecord, PreviousRecord, bHasPreviousRecord, LatestRecord.m_RotationMode);
				}
				else
					 UpdatePlayerAnimations(pPlayer, LatestRecord, PreviousRecord, bHasPreviousRecord, ROTATE_SERVER);
			}
			else
				 UpdatePlayerAnimations(pPlayer, LatestRecord, PreviousRecord, bHasPreviousRecord, ROTATE_SERVER);

			// форсим правильные лееры
			std::memcpy(pPlayer->get_animlayers(), AnimationLayers.data(), sizeof(AnimationLayer) * 13);

			// сэйвим позы
			std::memcpy(LatestRecord.m_PoseParameters.data(), pPlayer->GetBaseAnimating().data(), sizeof(float_t) * 24);

			// сетапим кости
			g_BoneManager::BuildMatrix(pPlayer, LatestRecord.m_Matricies[ROTATE_SERVER].data(), false);

			// сэйвим кости
			for (int i = 0; i < MAXSTUDIOBONES; i++)
				m_BoneOrigins[iPlayerID][i] = pPlayer->GetAbsOrigin() - LatestRecord.m_Matricies[ROTATE_SERVER][i].GetOrigin();

			// кэшируем кости
			std::memcpy(m_CachedMatrix[iPlayerID].data(), LatestRecord.m_Matricies[ROTATE_SERVER].data(), sizeof(matrix3x4_t) * MAXSTUDIOBONES);

			// плеер вышел с дорманта
			 UnmarkAsDormant(iPlayerID);
		}
	}

	void UpdatePlayerAnimations(player_t* pPlayer, C_LagRecord& LagRecord, C_LagRecord PreviousRecord, bool bHasPreviousRecord, int iRotationMode)
	{
		float_t flCurTime = m_globals()->m_curtime;
		float_t flRealTime = m_globals()->m_realtime;
		float_t flAbsFrameTime = m_globals()->m_absoluteframetime;
		float_t flFrameTime = m_globals()->m_frametime;
		float_t iFrameCount = m_globals()->m_framecount;
		float_t iTickCount = m_globals()->m_tickcount;
		float_t flInterpolationAmount = m_globals()->m_interpolation_amount;

		float_t flLowerBodyYaw = LagRecord.m_LowerBodyYaw;
		float_t flDuckAmount = LagRecord.m_DuckAmount;
		int32_t iFlags = LagRecord.m_Flags;
		int32_t iEFlags = pPlayer->m_iEFlags();

		if ( HasLeftOutOfDormancy(pPlayer->EntIndex()))
		{
			float_t flLastUpdateTime = LagRecord.m_SimulationTime - m_globals()->m_intervalpertick;
			if (pPlayer->m_fFlags() & FL_ONGROUND)
			{
				pPlayer->get_animation_state1()->m_bLanding = false;
				pPlayer->get_animation_state1()->m_bOnGround = true;

				float_t flLandTime = 0.0f;
				if (LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flCycle > 0.0f &&
					LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flPlaybackRate > 0.0f)
				{
					int32_t iLandActivity = pPlayer->sequence_activity(LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_nSequence);
					if (iLandActivity == ACT_CSGO_LAND_LIGHT || iLandActivity == ACT_CSGO_LAND_HEAVY)
					{
						flLandTime = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flCycle / LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flPlaybackRate;
						if (flLandTime > 0.0f)
							flLastUpdateTime = LagRecord.m_SimulationTime - flLandTime;
					}
				}

				LagRecord.m_Velocity.z = 0.0f;
			}
			else
			{
				float_t flJumpTime = 0.0f;
				if (LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flCycle > 0.0f &&
					LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flPlaybackRate > 0.0f)
				{
					int32_t iJumpActivity = pPlayer->sequence_activity(LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_nSequence);
					if (iJumpActivity == ACT_CSGO_JUMP)
					{
						flJumpTime = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flCycle / LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flPlaybackRate;
						if (flJumpTime > 0.0f)
							flLastUpdateTime = LagRecord.m_SimulationTime - flJumpTime;
					}
				}

				pPlayer->get_animation_state1()->m_bOnGround = false;
				pPlayer->get_animation_state1()->m_flDurationInAir = flJumpTime - m_globals()->m_intervalpertick;
			}

			float_t flWeight = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flWeight;
			if (LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flPlaybackRate < 0.00001f)
				LagRecord.m_Velocity.Zero();
			else
			{
				float_t flPostVelocityLength = LagRecord.m_Velocity.Length();
				if (flWeight > 0.0f && flWeight < 0.95f)
				{
					float_t flMaxSpeed = pPlayer->GetMaxPlayerSpeed();
					if (flPostVelocityLength > 0.0f)
					{
						float_t flMaxSpeedMultiply = 1.0f;
						if (pPlayer->m_fFlags() & 6)
							flMaxSpeedMultiply = 0.34f;
						else if (pPlayer->m_bIsWalking())
							flMaxSpeedMultiply = 0.52f;

						LagRecord.m_Velocity = (LagRecord.m_Velocity / flPostVelocityLength) * (flWeight * (flMaxSpeed * flMaxSpeedMultiply));
					}
				}
			}

			pPlayer->get_animation_state1()->m_flLastUpdateTime = flLastUpdateTime;
		}

		if (bHasPreviousRecord)
		{
			pPlayer->get_animation_state1()->m_flStrafeChangeCycle = PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(7).m_flCycle;
			pPlayer->get_animation_state1()->m_flStrafeChangeWeight = PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(7).m_flWeight;
			pPlayer->get_animation_state1()->m_nStrafeSequence = PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(7).m_nSequence;
			pPlayer->get_animation_state1()->m_flPrimaryCycle = PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flCycle;
			pPlayer->get_animation_state1()->m_flMoveWeight = PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flWeight;
			pPlayer->get_animation_state1()->m_flAccelerationWeight = PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(12).m_flWeight;
			std::memcpy(pPlayer->get_animlayers(), PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).data(), sizeof(AnimationLayer) * 13);
		}
		else
		{
			pPlayer->get_animation_state1()->m_flStrafeChangeCycle = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(7).m_flCycle;
			pPlayer->get_animation_state1()->m_flStrafeChangeWeight = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(7).m_flWeight;
			pPlayer->get_animation_state1()->m_nStrafeSequence = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(7).m_nSequence;
			pPlayer->get_animation_state1()->m_flPrimaryCycle = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flCycle;
			pPlayer->get_animation_state1()->m_flMoveWeight = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(6).m_flWeight;
			pPlayer->get_animation_state1()->m_flAccelerationWeight = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(12).m_flWeight;
			std::memcpy(pPlayer->get_animlayers(), LagRecord.m_AnimationLayers.at(ROTATE_SERVER).data(), sizeof(AnimationLayer) * 13);
		}

		if (LagRecord.m_UpdateDelay > 1)
		{
			int32_t iActivityTick = 0;
			int32_t iActivityType = 0;

			if (LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flWeight > 0.0f && PreviousRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flWeight <= 0.0f)
			{
				int32_t iLandSequence = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_nSequence;
				if (iLandSequence > 2)
				{
					int32_t iLandActivity = pPlayer->sequence_activity(iLandSequence);
					if (iLandActivity == ACT_CSGO_LAND_LIGHT || iLandActivity == ACT_CSGO_LAND_HEAVY)
					{
						float_t flCurrentCycle = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flCycle;
						float_t flCurrentRate = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flPlaybackRate;

						if (flCurrentCycle > 0.0f && flCurrentRate > 0.0f)
						{
							float_t flLandTime = (flCurrentCycle / flCurrentRate);
							if (flLandTime > 0.0f)
							{
								iActivityTick = TIME_TO_TICKS(LagRecord.m_SimulationTime - flLandTime) + 1;
								iActivityType = ACTIVITY_LAND;
							}
						}
					}
				}
			}

			if (LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flCycle > 0.0f && LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flPlaybackRate > 0.0f)
			{
				int32_t iJumpSequence = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_nSequence;
				if (iJumpSequence > 2)
				{
					int32_t iJumpActivity = pPlayer->sequence_activity(iJumpSequence);
					if (iJumpActivity == ACT_CSGO_JUMP)
					{
						float_t flCurrentCycle = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flCycle;
						float_t flCurrentRate = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flPlaybackRate;

						if (flCurrentCycle > 0.0f && flCurrentRate > 0.0f)
						{
							float_t flJumpTime = (flCurrentCycle / flCurrentRate);
							if (flJumpTime > 0.0f)
							{
								iActivityTick = TIME_TO_TICKS(LagRecord.m_SimulationTime - flJumpTime) + 1;
								iActivityType = ACTIVITY_JUMP;
							}
						}
					}
				}
			}

			for (int32_t iSimulationTick = 1; iSimulationTick <= LagRecord.m_UpdateDelay; iSimulationTick++)
			{
				float_t flSimulationTime = PreviousRecord.m_SimulationTime + TICKS_TO_TIME(iSimulationTick);
				m_globals()->m_curtime = flSimulationTime;
				m_globals()->m_realtime = flSimulationTime;
				m_globals()->m_frametime = m_globals()->m_intervalpertick;
				m_globals()->m_absoluteframetime = m_globals()->m_intervalpertick;
				m_globals()->m_framecount = TIME_TO_TICKS(flSimulationTime);
				m_globals()->m_tickcount = TIME_TO_TICKS(flSimulationTime);
				m_globals()->m_interpolation_amount = 0.0f;

				pPlayer->m_flDuckAmount() = Interpolate(PreviousRecord.m_DuckAmount, LagRecord.m_DuckAmount, iSimulationTick, LagRecord.m_UpdateDelay);
				pPlayer->m_vecVelocity() = Interpolate(PreviousRecord.m_Velocity, LagRecord.m_Velocity, iSimulationTick, LagRecord.m_UpdateDelay);
				pPlayer->m_vecAbsVelocity() = pPlayer->m_vecVelocity();
				pPlayer->m_flLowerBodyYawTarget() = PreviousRecord.m_LowerBodyYaw;

				if (iSimulationTick < LagRecord.m_UpdateDelay)
				{
					int32_t iCurrentSimulationTick = TIME_TO_TICKS(flSimulationTime);
					if (iActivityType > ACTIVITY_NONE)
					{
						bool bIsOnGround = pPlayer->m_fFlags() & FL_ONGROUND;
						if (iActivityType == ACTIVITY_JUMP)
						{
							if (iCurrentSimulationTick == iActivityTick - 1)
								bIsOnGround = true;
							else if (iCurrentSimulationTick == iActivityTick)
							{
								// reset animation layer
								pPlayer->get_animlayers()[4].m_flCycle = 0.0f;
								pPlayer->get_animlayers()[4].m_nSequence = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_nSequence;
								pPlayer->get_animlayers()[4].m_flPlaybackRate = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(4).m_flPlaybackRate;

								// reset player ground state
								bIsOnGround = false;
							}

						}
						else if (iActivityType == ACTIVITY_LAND)
						{
							if (iCurrentSimulationTick == iActivityTick - 1)
								bIsOnGround = false;
							else if (iCurrentSimulationTick == iActivityTick)
							{
								// reset animation layer
								pPlayer->get_animlayers()[5].m_flCycle = 0.0f;
								pPlayer->get_animlayers()[5].m_nSequence = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_nSequence;
								pPlayer->get_animlayers()[5].m_flPlaybackRate = LagRecord.m_AnimationLayers.at(ROTATE_SERVER).at(5).m_flPlaybackRate;

								// reset player ground state
								bIsOnGround = true;
							}
						}

						if (bIsOnGround)
							pPlayer->m_fFlags() |= FL_ONGROUND;
						else
							pPlayer->m_fFlags() &= ~FL_ONGROUND;
					}

					if (iRotationMode)
					{
						LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw);
						switch (iRotationMode)
						{
						case ROTATE_LEFT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw - 60.0f); break;
						case ROTATE_RIGHT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw + 60.0f); break;
						case ROTATE_LOW_LEFT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw - 25.0f); break;
						case ROTATE_LOW_RIGHT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw + 25.0f); break;
						}

						pPlayer->get_animation_state1()->m_flFootYaw = LagRecord.m_BruteYaw;
					}
				}
				else
				{
					pPlayer->m_vecVelocity() = LagRecord.m_Velocity;
					pPlayer->m_vecAbsVelocity() = LagRecord.m_Velocity;
					pPlayer->m_flDuckAmount() = LagRecord.m_DuckAmount;
					pPlayer->m_flLowerBodyYawTarget() = LagRecord.m_LowerBodyYaw;
					pPlayer->m_fFlags() = LagRecord.m_Flags;
				}

				pPlayer->get_animation_state1()->m_nLastUpdateFrame = INT_MAX;

				bool bClientSideAnimation = pPlayer->m_bClientSideAnimation();
				pPlayer->m_bClientSideAnimation() = true;

				for (int32_t iLayer = NULL; iLayer < 13; iLayer++)
					pPlayer->get_animlayers()[iLayer].m_pOwner = pPlayer;

				g_Globals.m_AnimationData.m_bAnimationUpdate = true;
				pPlayer->update_clientside_animation();
				g_Globals.m_AnimationData.m_bAnimationUpdate = false;

				pPlayer->m_bClientSideAnimation() = bClientSideAnimation;
			}
		}
		else
		{
			m_globals()->m_curtime = LagRecord.m_SimulationTime;
			m_globals()->m_realtime = LagRecord.m_SimulationTime;
			m_globals()->m_frametime = m_globals()->m_intervalpertick;
			m_globals()->m_absoluteframetime = m_globals()->m_intervalpertick;
			m_globals()->m_framecount = TIME_TO_TICKS(LagRecord.m_SimulationTime);
			m_globals()->m_tickcount = TIME_TO_TICKS(LagRecord.m_SimulationTime);
			m_globals()->m_interpolation_amount = 0.0f;

			pPlayer->m_vecVelocity() = LagRecord.m_Velocity;
			pPlayer->m_vecAbsVelocity() = LagRecord.m_Velocity;

			if (iRotationMode)
			{
				LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw);
				switch (iRotationMode)
				{
				case ROTATE_LEFT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw - 60.0f); break;
				case ROTATE_RIGHT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw + 60.0f); break;
				case ROTATE_LOW_LEFT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw - 25.0f); break;
				case ROTATE_LOW_RIGHT: LagRecord.m_BruteYaw = AngleNormalize(LagRecord.m_BruteYaw + 25.0f); break;
				}

				pPlayer->get_animation_state1()->m_flFootYaw = LagRecord.m_BruteYaw;
			}

			if (pPlayer->get_animation_state1()->m_nLastUpdateFrame > m_globals()->m_framecount - 1)
				pPlayer->get_animation_state1()->m_nLastUpdateFrame = m_globals()->m_framecount - 1;

			bool bClientSideAnimation = pPlayer->m_bClientSideAnimation();
			pPlayer->m_bClientSideAnimation() = true;

			for (int32_t iLayer = NULL; iLayer < 13; iLayer++)
				pPlayer->get_animlayers()[iLayer].m_pOwner = pPlayer;

			g_Globals.m_AnimationData.m_bAnimationUpdate = true;
			pPlayer->update_clientside_animation();
			g_Globals.m_AnimationData.m_bAnimationUpdate = false;

			pPlayer->m_bClientSideAnimation() = bClientSideAnimation;
		}

		pPlayer->m_flLowerBodyYawTarget() = flLowerBodyYaw;
		pPlayer->m_flDuckAmount() = flDuckAmount;
		pPlayer->m_iEFlags() = iEFlags;
		pPlayer->m_fFlags() = iFlags;

		m_globals()->m_curtime = flCurTime;
		m_globals()->m_realtime = flRealTime;
		m_globals()->m_absoluteframetime = flAbsFrameTime;
		m_globals()->m_frametime = flFrameTime;
		m_globals()->m_framecount = iFrameCount;
		m_globals()->m_tickcount = iTickCount;
		m_globals()->m_interpolation_amount = flInterpolationAmount;

		return pPlayer->invalidate_physics_recursive(8);
	}

	bool GetCachedMatrix(player_t* pPlayer, matrix3x4_t* aMatrix)
	{
		std::memcpy(aMatrix, m_CachedMatrix[pPlayer->EntIndex()].data(), sizeof(matrix3x4_t) * pPlayer->m_CachedBoneData().Count());
		return true;
	}

	void OnUpdateClientSideAnimation(player_t* pPlayer)
	{
		for (int i = 0; i < MAXSTUDIOBONES; i++)
			m_CachedMatrix[pPlayer->EntIndex()][i].SetOrigin(pPlayer->GetAbsOrigin() - m_BoneOrigins[pPlayer->EntIndex()][i]);

		std::memcpy(pPlayer->m_CachedBoneData().Base(), m_CachedMatrix[pPlayer->EntIndex()].data(), sizeof(matrix3x4_t) * pPlayer->m_CachedBoneData().Count());
		std::memcpy(pPlayer->GetBoneAccessor().GetBoneArrayForWrite(), m_CachedMatrix[pPlayer->EntIndex()].data(), sizeof(matrix3x4_t) * pPlayer->m_CachedBoneData().Count());

		return pPlayer->SetupBones_AttachmentHelper();
	}
}

