#pragma once

#include "LagCompensation.h"
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
#include "LagHeader.h"
//#include "extra.h"

enum ADVANCED_ACTIVITY : int
{
	ACTIVITY_NONE = 0,
	ACTIVITY_JUMP,
	ACTIVITY_LAND
};

template < class T >
__forceinline T Interpolate(const T& flCurrent, const T& flTarget, const int iProgress, const int iMaximum)
{
	return flCurrent + ((flTarget - flCurrent) / iMaximum) * iProgress;
}



namespace g_AnimationSync
{

	 void Instance();
	 void UpdatePlayerAnimations(player_t* pPlayer, C_LagRecord& LagRecord, C_LagRecord PreviousRecord, bool bHasPreviousRecord, int iRotationMode);
	 void MarkAsDormant(int iPlayerID) ;
	 void UnmarkAsDormant(int iPlayerID);
	 bool HasLeftOutOfDormancy(int iPlayerID);
	 void SetPreviousRecord(int iPlayerID, C_LagRecord LagRecord) 
	{ 
		m_PreviousRecord[iPlayerID] = LagRecord; 
	};
	 //C_LagRecord GetPreviousRecord(int iPlayerID);
	 bool GetCachedMatrix(player_t* pPlayer, matrix3x4_t* aMatrix);
	 void OnUpdateClientSideAnimation(player_t* pPlayer);

	//std::array < std::array < Vector, MAXSTUDIOBONES >, 65 > m_BoneOrigins;
	//std::array < std::array < matrix3x4_t, MAXSTUDIOBONES >, 65 > m_CachedMatrix = { };
	//std::array < C_LagRecord, 65 > m_PreviousRecord = { };
	//std::array < bool, 65 > m_LeftDormancy = { };
};

//inline C_AnimationSync g_AnimationSync =  C_AnimationSync();