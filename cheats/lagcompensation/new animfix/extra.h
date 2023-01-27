
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







enum ROTATE_MODE
{
	ROTATE_SERVER,
	ROTATE_LEFT,
	ROTATE_CENTER,
	ROTATE_RIGHT,
	ROTATE_LOW_LEFT,
	ROTATE_LOW_RIGHT
};



class C_LagRecord1
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

	std::array < std::array < AnimationLayer, 13 >, 6 > m_AnimationLayers = { };
	std::array < float_t, 24 > m_PoseParameters = { };
	std::array < std::array < matrix3x4_t, MAXSTUDIOBONES >, 4 > m_Matricies = { };

	bool m_bIsShooting = false;
	bool m_bAnimResolved = false;
	bool m_bJumped = false;
};
class C_HitboxData
{
public:
	int32_t m_iHitbox = -1;
	float_t m_flDamage = 0.0f;
	Vector m_vecPoint = Vector(0, 0, 0);
	bool m_bIsSafeHitbox = false;
	bool m_bForcedToSafeHitbox = false;

	player_t* pPlayer = NULL;
	C_LagRecord1 LagRecord = C_LagRecord1();
};
class C_HitboxHitscanData
{
public:
	C_HitboxHitscanData(int32_t iHitbox = -1, bool bForceSafe = false)
	{
		this->m_iHitbox = iHitbox;
		this->m_flWeaponDamage = 0.0f;
		this->m_bForceSafe = bForceSafe;
	}

	int32_t m_iHitbox = -1;
	float_t m_flWeaponDamage = 0.0f;
	bool m_bForceSafe = false;
};
class C_TargetData
{
public:
	player_t* m_Player = NULL;
	C_LagRecord1 m_LagRecord = C_LagRecord1();
	C_HitboxData m_Hitbox = C_HitboxData();
};
class C_ShotData
{
public:
	C_TargetData m_Target = C_TargetData();

	Vector m_vecStartPosition = Vector(0, 0, 0);
	Vector m_vecEndPosition = Vector(0, 0, 0);

	int32_t m_iShotTick = 0;
	float m_Damage = 0;

	bool m_bDidIntersectPlayer = false;
	bool m_bHasBeenFired = false;
	bool m_bHasBeenRegistered = false;
	bool m_bHasBeenHurted = false;
	bool m_bHasMaximumAccuracy = false;

	std::vector < Vector > m_vecImpacts = { };
};
struct FuckThisSDK_t
{
	CUserCmd* m_pCmd = NULL;
	bool* m_pbSendPacket = NULL;

	player_t* m_Player = nullptr;
	bool m_bFakeDuck = false;
	bool m_bVisualFakeDuck = false;
	bool m_bIsPeeking = false;

	struct
	{
		std::array < AnimationLayer, 13 > GetAnimationLayers;
		std::array < AnimationLayer, 13 > m_FakeAnimationLayers;

		std::array < float_t, 24 > m_PoseParameters;
		std::array < float_t, 24 > m_FakePoseParameters;
		C_CSGOPlayerAnimationState m_FakeAnimationState;

		float_t m_flSpawnTime = 0.0f;
		float_t m_flLowerBodyYaw = 0.0f;
		float_t m_flNextLowerBodyYawUpdateTime = 0.0f;

		std::array < int32_t, 2 > m_iMoveType;
		std::array < int32_t, 2 > m_iFlags;

		std::array < Vector, MAXSTUDIOBONES > m_vecBoneOrigins;
		std::array < Vector, MAXSTUDIOBONES > m_vecFakeBoneOrigins;

		Vector m_vecNetworkedOrigin = Vector(0, 0, 0);
		Vector m_vecShootPosition = Vector(0, 0, 0);

		bool m_bDidShotAtChokeCycle = false;
		QAngle m_angShotChokedAngle = QAngle(0, 0, 0);

		float_t m_flShotTime = 0.0f;
		QAngle m_angForcedAngles = QAngle(0, 0, 0);

		QAngle m_angFakeAngles = QAngle(0, 0, 0);

		std::array < matrix3x4_t, MAXSTUDIOBONES > m_aMainBones;
		std::array < matrix3x4_t, MAXSTUDIOBONES > m_aDesyncBones;
		std::array < matrix3x4_t, MAXSTUDIOBONES > m_aLagBones;
	} m_LocalData;

	struct
	{
		bool m_bAnimationUpdate = false;
		bool m_bSetupBones = false;
	} m_AnimationData;

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
	std::deque < C_ShotData > m_ShotData;
	struct
	{
		std::array < float_t, 256 > m_aInaccuracy;
		std::array < float_t, 256 > m_aSpread;
		std::array < float_t, 256 > m_FirstRandom;
		std::array < float_t, 256 > m_SecondRandom;

		float_t m_flInaccuracy = 0.0f;
		float_t m_flSpread = 0.0f;
		float_t m_flMinInaccuracy = 0.0f;

		bool m_bCanFire_Shift = false;
		bool m_bCanFire_Default = false;
		bool m_bRestoreAutoStop = false;
		bool m_bHasValidAccuracyData = false;
		bool m_bHasMaximumAccuracy = false;
		bool m_bDoingSecondShot = false;
	} m_AccuracyData;
	struct
	{
		int m_TicksToStop = 0;
	} m_MovementData;
	struct
	{
		C_TargetData m_CurrentTarget;
	} m_RageData;

	std::array < std::deque < C_LagRecord1 >, 65 > m_CachedPlayerRecords;
	int m_nMaxChoke = 14;

};

inline FuckThisSDK_t* g_FuckThisSDK = new FuckThisSDK_t();