#pragma once

#include "../animation_system.h"
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


namespace g_BoneManager
{
	
		 void BuildMatrix(player_t* pPlayer, matrix3x4_t* aMatrix, bool bSafeMatrix);
	
}



//inline C_BoneManager g_BoneManager = C_BoneManager();