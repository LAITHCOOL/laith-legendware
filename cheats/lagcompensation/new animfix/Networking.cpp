#include "Networking.h"

inline bool IsVectorValid(Vector vecOriginal, Vector vecCurrent)
{
	Vector vecDelta = vecOriginal - vecCurrent;
	for (int i = 0; i < 3; i++)
	{
		if (fabsf(vecDelta[i]) > 0.03125f)
			return false;
	}

	return true;
}
inline bool IsAngleValid(QAngle vecOriginal, QAngle vecCurrent)
{
	QAngle vecDelta = vecOriginal - vecCurrent;
	for (int i = 0; i < 3; i++)
	{
		if (fabsf(vecDelta[i]) > 0.03125f)
			return false;
	}

	return true;
}
inline bool IsFloatValid(float_t flOriginal, float_t flCurrent)
{
	if (fabsf(flOriginal - flCurrent) > 0.03125f)
		return false;

	return true;
}

void C_Networking::OnPacketEnd(CClientState* m_ClientState)
{
	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return;

	if (*(int32_t*)((DWORD)(m_ClientState)+0x16C) != *(int32_t*)((DWORD)(m_ClientState)+0x164))
		return;

	return this->RestoreNetvarData(m_clientstate()->nLastOutgoingCommand);
}

void C_Networking::StartNetwork()
{
	if (!m_clientstate() || !m_clientstate()->pNetChannel)
		return;

	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return;

	m_Sequence = m_clientstate()->pNetChannel->m_nOutSequenceNr;
}

bool C_Networking::ShouldProcessPacketStart(int nCommand)
{
	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return true;

	for (auto m_Cmd = m_CommandList.begin(); m_Cmd != m_CommandList.end(); m_Cmd++)
	{
		if (*m_Cmd != nCommand)
			continue;

		m_CommandList.erase(m_Cmd);
		return true;
	}

	return false;
}

void C_Networking::FinishNetwork()
{
	if (!m_clientstate() || !m_clientstate()->pNetChannel)
		return;

	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return;

	int32_t iChokedCommand = m_clientstate()->pNetChannel->m_nChokedPackets;
	int32_t iSequenceNumber = m_clientstate()->pNetChannel->m_nOutSequenceNr;

	m_clientstate()->pNetChannel->m_nChokedPackets = 0;
	m_clientstate()->pNetChannel->m_nOutSequenceNr = m_Sequence;

	m_clientstate()->pNetChannel->send_datagram();

	m_clientstate()->pNetChannel->m_nOutSequenceNr = iSequenceNumber;
	m_clientstate()->pNetChannel->m_nChokedPackets = iChokedCommand;
}

void C_Networking::ProcessInterpolation(bool bPostFrame)
{
	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return;

	if (!bPostFrame)
	{
		m_FinalPredictedTick = g_ctx.local()->m_nFinalPredictedTick();
		m_flInterp = m_globals()->m_interpolation_amount;

		g_ctx.local()->m_nFinalPredictedTick() = g_ctx.local()->m_nTickBase();
		

		return;
	}

	g_ctx.local()->m_nFinalPredictedTick() = m_FinalPredictedTick;
	m_globals()->m_interpolation_amount = 0.0f;
}

/*void C_Networking::SaveNetvarData(int nCommand)
{
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_nTickbase = g_ctx.local()->m_nTickBase();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_flDuckAmount = g_ctx.local()->m_flDuckAmount();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_flDuckSpeed = g_ctx.local()->m_flDuckSpeed();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_vecOrigin = g_ctx.local()->GetOrigin();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_vecVelocity = g_ctx.local()->m_vecVelocity();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_vecBaseVelocity = g_ctx.local()->m_vecBaseVelocity();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_flFallVelocity = g_ctx.local()->get_fall_velocity();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_vecViewOffset = g_ctx.local()->m_vecViewOffset();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_angAimPunchAngle = g_ctx.local()->m_aimPunchAngle1();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_vecAimPunchAngleVel = g_ctx.local()->m_aimPunchAngle1();
	m_aCompressData[nCommand % MULTIPLAYER_BACKUP].m_angViewPunchAngle = g_ctx.local()->m_viewPunchAngle1();
}

void C_Networking::RestoreNetvarData(int nCommand)
{
	auto aNetVars = &m_aCompressData[nCommand % MULTIPLAYER_BACKUP];
	if (aNetVars->m_nTickbase != g_ctx.local()->m_nTickBase())
		return;

	if (IsVectorValid(aNetVars->m_vecVelocity, g_ctx.local()->m_vecVelocity()))
		g_ctx.local()->m_vecVelocity() = aNetVars->m_vecVelocity;

	if (IsVectorValid(aNetVars->m_vecBaseVelocity, g_ctx.local()->m_vecBaseVelocity()))
		g_ctx.local()->m_vecBaseVelocity() = aNetVars->m_vecBaseVelocity;

	if (std::fabsf(g_ctx.local()->m_vecViewOffset().z - aNetVars->m_vecViewOffset.z) <= 0.25f)
		g_ctx.local()->m_vecViewOffset() = aNetVars->m_vecViewOffset;

	if (IsAngleValid(aNetVars->m_angAimPunchAngle, g_ctx.local()->m_aimPunchAngle1()))
		g_ctx.local()->m_aimPunchAngle1() = aNetVars->m_angAimPunchAngle;

	if (IsVectorValid(aNetVars->m_vecAimPunchAngleVel, g_ctx.local()->m_aimPunchAngle1()))
		g_ctx.local()->m_aimPunchAngle1() = aNetVars->m_vecAimPunchAngleVel;

	if (IsAngleValid(aNetVars->m_angViewPunchAngle, g_ctx.local()->m_viewPunchAngle1()))
		g_ctx.local()->m_viewPunchAngle1() = aNetVars->m_angViewPunchAngle;

	if (std::fabsf(aNetVars->m_flFallVelocity - g_ctx.local()->get_fall_velocity()) <= 0.5f)
		g_ctx.local()->get_fall_velocity() = aNetVars->m_flFallVelocity;

	if (IsFloatValid(aNetVars->m_flDuckAmount, g_ctx.local()->m_flDuckAmount()))
		g_ctx.local()->m_flDuckAmount() = aNetVars->m_flDuckAmount;

	if (IsFloatValid(aNetVars->m_flDuckSpeed, g_ctx.local()->m_flDuckSpeed()))
		g_ctx.local()->m_flDuckSpeed() = aNetVars->m_flDuckSpeed;

	if (g_ctx.local()->m_vecViewOffset().z > 64.0f)
		g_ctx.local()->m_vecViewOffset().z = 64.0f;
	else if (g_ctx.local()->m_vecViewOffset().z <= 46.05f)
		g_ctx.local()->m_vecViewOffset().z = 46.0f;
}
*/
void C_Networking::UpdateLatency()
{
	INetChannelInfo* pNetChannelInfo = m_engine()->GetNetChannelInfo();
	if (!pNetChannelInfo)
		return;

	m_Latency = pNetChannelInfo->GetLatency(0) + pNetChannelInfo->GetLatency(1);
}

int32_t C_Networking::GetServerTick()
{
	int32_t nExtraChoke = 0;
	if (g_Globals.m_bFakeDuck)
		nExtraChoke = 14 - m_clientstate()->iChokedCommands;

	return m_globals()->m_tickcount + (int)(((m_Latency) / m_globals()->m_intervalpertick) + 0.5f) + nExtraChoke;
}

int32_t C_Networking::GetTickRate()
{
	return (int32_t)(1.0f / m_globals()->m_intervalpertick);
}

float_t C_Networking::GetLatency()
{
	return m_Latency;
}

void C_Networking::ResetData()
{
	m_Latency = 0.0f;
	m_TickRate = 0;

	//m_aCompressData = { };
}