#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

class StoredData_t
{
public:
	Vector  m_punch, m_punch_vel, m_viewPunchAngle;
	Vector m_view_offset, m_vecVelocity, m_vecOrigin;
	float  m_duck_amount, m_flThirdpersonRecoil, m_flDuckSpeed, m_velocity_modifier, m_flFallVelocity;
	int	   m_tick = 0, m_command_number = INT_MAX, m_tickbase;
	bool   m_is_filled = false;

public:
	__forceinline StoredData_t() : m_tickbase{}, m_punch{ }, m_punch_vel{ }, m_view_offset{ }, m_vecVelocity{ }, m_vecOrigin{}, m_duck_amount{ }, m_flThirdpersonRecoil{ }, m_velocity_modifier{ }, m_flFallVelocity{ }, m_tick{}, m_command_number{}, m_is_filled{} {};
};

class engineprediction : public singleton <engineprediction>
{
	struct Backup_data
	{
		int flags = 0;
		Vector velocity = ZERO;
	};

	struct Viewmodel_data
	{
		weapon_t* weapon = nullptr;

		int viewmodel_index = 0;
		int sequence = 0;
		int animation_parity = 0;

		float cycle = 0.0f;
		float animation_time = 0.0f;
	};
public:
	std::array<StoredData_t, MULTIPLAYER_BACKUP> m_data;
	Backup_data backup_data;
	Viewmodel_data viewmodel_data;

	int	m_last_cmd_delta = 0;
	void fix_local_commands();

	void store(StoredData_t* data, C_CommandContext* cctx, int m_tick);
	void apply(int time);
	void reset();
	void detect_prediction_error(StoredData_t* data, int m_tick);
};

enum E_UpdateViewmodelStage
{
	CYCLE_NONE = 0,
	CYCLE_PRE_UPDATE = 1,
	CYCLE_UPDATE = 2
};

namespace engine_prediction
{
	inline static struct stored_vars
	{
		bool m_in_prediction, m_is_first_time_predicted;
		float m_frame_time, m_cur_time, m_flVelocityModifier, m_anim_speed, fixed_curtime, m_FinalPredictedTick,
			m_min_delta, m_max_delta, m_flSpread, m_flInaccuracy, m_flCalculatedInaccuracy, m_flRevolverAccuracy;
		int unpred_flags, tickbase, m_fFlags, m_iButtons, m_fTickcount;
		Vector unpred_vel, m_vecVelocity, m_vecOrigin, m_anim_vel;
		CMoveData m_MoveData;
		uintptr_t prediction_player;
		uintptr_t prediction_seed;
	}m_stored_variables;

	inline static struct Misc_t
	{
		int m_iRunCommandTickbase;
		bool m_bOverrideModifier;
	} m_misc;

	inline static struct viewmodel_t
	{
		float m_viewmodel_cycle, m_viewmodel_anim_time;
		int m_viewmodel_AnimationParity, m_viewmodel_Sequence, m_viewmodel_ViewModelIndex, m_viewmodel_update_stage;
		CBaseHandle m_viewmodel_Weapon;
	} m_stored_viewmodel;

	inline static struct m_revolver_t
	{
		float_t m_flPostponeFireReadyTime = 0.0f;
		std::array < bool, MULTIPLAYER_BACKUP > m_FireStates = { };
		std::array < bool, MULTIPLAYER_BACKUP > m_PostponeStates = { };
		std::array < bool, MULTIPLAYER_BACKUP > m_PrimaryAttack = { };
		std::array < bool, MULTIPLAYER_BACKUP > m_SecondaryAttack = { };
		std::array < int, MULTIPLAYER_BACKUP > m_TickRecords = { };
	} m_revolver;

	void update();
	void BackupData(player_t* player, CUserCmd* cmd);
	void UpdateButtonState(player_t* player, CUserCmd* cmd);
	void BuildDataAfterPredict(player_t* player, CUserCmd* cmd, weapon_t* m_weapon);
	void predict(CUserCmd* cmd, player_t* player);
	void restore(player_t* player);
	void FinishCommand(player_t* player);

	void update_viewmodel_data();
	void correct_viewmodel_data();
	void patch_attack_packet(CUserCmd* cmd, bool predicted);
	void FixRevolver(CUserCmd* cmd, player_t* player);
	void UpdateVelocityModifier();
	void ProcessInterpolation(bool bPostFrame);
}