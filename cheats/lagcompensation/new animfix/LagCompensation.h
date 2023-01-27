#pragma once
#include <deque>
#include "BoneManager.h"
#include <Windows.h>
#include <deque>
#include <time.h>
#include "../animation_system.h"
#include "..\..\..\includes.hpp"
#include "..\..\..\sdk\structs.hpp"
#include "Networking.h"
#include "Animations.h"
#include "BoneManager.h"
#include "LagCompensation.h"
//#include "extra.h"
#include "LagHeader.h"


namespace g_LagCompensation
{

	 void Instance();
	 void FillRecord(player_t* Player, C_LagRecord& Record);
	 void ResetData();
	 void ClearRecords(int32_t iPlayerID) { g_Globals.m_CachedPlayerRecords[iPlayerID].clear(); };
	 //void SetBreakingLagCompensation(int32_t iPlayerID);
	// void UnsetBreakingLagCompensation(int32_t iPlayerID) ;
	 bool IsValidTime(float_t flTime);
	// bool IsBreakingLagCompensation(int32_t iPlayerID);
	 float_t GetLerpTime();
	// std::deque < C_LagRecord >& GetPlayerRecords(int32_t iPlayerID) { return g_Globals.m_CachedPlayerRecords[iPlayerID]; };
	// C_LagRecord& GetPreviousRecord(int32_t iPlayerID) { return g_Globals.m_CachedPlayerRecords[iPlayerID].back(); };

	//std::array < bool, 65 > m_BreakingLagcompensation = { };
};

//inline C_LagCompensation g_LagCompensation =  C_LagCompensation();