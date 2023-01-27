#pragma once
#include "..\includes.hpp"

class SimulationContext : public singleton < SimulationContext >
{
public:
	void TracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, CGameTrace& pm);
	void InitSimulationContext(player_t* player);
	void ExtrapolatePlayer(float yaw);
	void TryPlayerMove();
	void RebuildGameMovement(CUserCmd* ucmd);
private:
	bool walking;
	Vector m_vecOrigin;
	Vector m_vecVelocity;
	int simulationTicks;
	CGameTrace trace;
	float gravity;
	float sv_jump_impulse;
	float stepsize;
	Vector pMins;
	Vector pMaxs;
	player_t* player;
	CTraceFilterWorldOnly *filter;
	int flags;
	float flMaxSpeed;
	int buttons;
};