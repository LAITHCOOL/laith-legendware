#pragma once
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
#include "../../utils/singleton.h"


enum ROTATE_MODE
{
	ROTATE_SERVER,
	ROTATE_LEFT,
	ROTATE_CENTER,
	ROTATE_RIGHT,
	ROTATE_LOW_LEFT,
	ROTATE_LOW_RIGHT
};


class C_LagRecord
{
public:
	float_t m_SimulationTime = 0.0f;
	float_t m_LowerBodyYaw = 0.0f;
	float_t m_DuckAmount = 0.0f;
	float_t m_BruteYaw = 0.0f;

	int32_t m_UpdateDelay = 0;
	int32_t m_RotationMode = ROTATE_SERVER;
	int32_t m_Flags = 0;
	int32_t m_AdjustTick = 0;

	QAngle m_EyeAngles = QAngle(0, 0, 0);
	QAngle m_AbsAngles = QAngle(0, 0, 0);
	Vector m_Velocity = Vector(0, 0, 0);
	Vector m_Origin = Vector(0, 0, 0);
	Vector m_AbsOrigin = Vector(0, 0, 0);
	Vector m_Mins = Vector(0, 0, 0);
	Vector m_Maxs = Vector(0, 0, 0);

	std::array < std::array < AnimationLayer, ANIMATION_LAYER_COUNT >, 6 > m_AnimationLayers = { };
	std::array < float_t, MAXSTUDIOPOSEPARAM > m_PoseParameters = { };
	std::array < std::array < matrix3x4_t, MAXSTUDIOBONES >, 4 > m_Matricies = { };

	bool m_bIsShooting = false;
	bool m_bAnimResolved = false;
	bool m_bJumped = false;
};


class C_Globals
{
public:


	struct
	{
		std::array < int32_t, 65 > m_MissedShots = { };
		std::array < int32_t, 65 > m_LastMissedShots = { };
		std::array < int32_t, 65 > m_BruteSide = { };
		std::array < int32_t, 65 > m_LastBruteSide = { };
		std::array < int32_t, 65 > m_LastTickbaseShift = { };
		std::array < float_t, 65 > m_LastValidTime = { };
		std::array < Vector, 65 > m_LastValidOrigin = { };
		std::array < bool, 65 > m_AnimResoled = { };
		std::array < bool, 65 > m_FirstSinceTickbaseShift = { };
	} m_ResolverData;

	
	struct
	{
		bool m_bAnimationUpdate = false;
		bool m_bSetupBones = false;
	} m_AnimationData;

	struct
	{
		bool m_bShotChams = false;
		bool m_bDrawModel = false;
		float_t m_flModelRotation = -180.0f;
	} m_Model;

	std::array < std::deque < C_LagRecord >, 65 > m_CachedPlayerRecords;
	struct
	{
		int m_PeekTick = -1;
		bool m_bIsPeeking = false;
		player_t* m_Player = NULL;
	} m_Peek;


};


inline C_Globals g_Globals = C_Globals();


