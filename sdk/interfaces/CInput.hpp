#pragma once

#include "../misc/CUserCmd.hpp"

#define MULTIPLAYER_BACKUP 150

class bf_write;
class bf_read;

class CInput
{
public:
	char            pad0[0xC];            // 0x0
	bool                m_fTrackIRAvailable;        // 0xC
	bool                m_fMouseInitialized;        // 0xD
	bool                m_fMouseActive;            // 0xE
	char            pad1[0x9A];            // 0xF
	bool                m_fCameraInThirdPerson;    // 0xA9
	char            pad2[0x2];            // 0xAA
	Vector                m_vecCameraOffset;        // 0xAC
	char            pad3[0x38];            // 0xB8
	CUserCmd* m_pCommands;            // 0xF0
	CVerifiedUserCmd* m_pVerifiedCommands;    // 0xF4

	CUserCmd* CInput::GetUserCmd(int sequence_number)
	{
		return &m_pCommands[sequence_number % MULTIPLAYER_BACKUP];
	}

	CUserCmd* CInput::GetUserCmd(int iSlot, int iSequenceNumber)
	{
		return call_virtual< CUserCmd* (__thiscall*)(void*, int, int) >(this, 8)(this, iSlot, iSequenceNumber);
	}

	CVerifiedUserCmd* GetVerifiedUserCmd(int sequence_number)
	{
		return &m_pVerifiedCommands[sequence_number % MULTIPLAYER_BACKUP];
	}
};