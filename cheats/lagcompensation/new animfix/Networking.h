#pragma once
//#include "extra.h"
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

class C_Networking
{
public:
	virtual void OnPacketEnd(CClientState* pClientState);
	virtual void SaveNetvarData(int nCommand);
	virtual void RestoreNetvarData(int nCommand);
	virtual void UpdateLatency();
	virtual void ProcessInterpolation(bool bPostFrame);
	virtual void ResetData();
	virtual bool ShouldProcessPacketStart(int nCommand);
	virtual void PushCommand(int nCmd)
	{
		if (m_CommandList.size() > 32)
			m_CommandList.clear();

		m_CommandList.emplace_back(nCmd);
	}

	virtual void StartNetwork();
	virtual void FinishNetwork();

	virtual float_t GetLatency();
	virtual int32_t GetServerTick();
	virtual int32_t GetTickRate();
private:
	std::vector < int > m_CommandList;
	//std::array < C_NetvarData, MULTIPLAYER_BACKUP > m_aCompressData = { };

	float m_flInterp = 0.0f;
	int m_Sequence = 0;
	float_t m_Latency = 0.f;

	int32_t m_TickRate = 0;
	int32_t m_FinalPredictedTick = 0;
};

inline C_Networking* g_Networking = new C_Networking();