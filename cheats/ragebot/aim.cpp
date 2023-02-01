// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "aim.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"
#include "..\visuals\other_esp.h"
#include "../tickbase shift/tickbase_shift.h"



bool aim::SanityCheck(CUserCmd* cmd, bool weapon, int idx, bool check_weapon) {
	bool manual_override = util::is_button_down(MOUSE_LEFT); //this is just nice to have.
	bool has_revolver = (g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER);

	if (!weapon) {
		if (!g_cfg.ragebot.enable
			|| !cmd
			|| !g_ctx.local()
			|| manual_override)
			return false;

		if (check_weapon && !g_ctx.globals.weapon->can_fire(has_revolver))
			return false;

		return true;
	}
	else
	{
		if (g_ctx.globals.weapon->is_non_aim() || idx == -1)
			return false;

		if (check_weapon && !g_ctx.globals.weapon->can_fire(has_revolver))
			return false;

		return true;
	}
}

void aim::run(CUserCmd* cmd) {

	backup.clear();
	targets.clear();
	scanned_targets.clear();
	final_target.reset();
	should_stop = false;

	//vars
	int idx = g_ctx.globals.current_weapon;

	//return checks
	if (!SanityCheck(cmd, false, idx, false))
		return;

	automatic_revolver(cmd);
	prepare_targets();

	//return checks again
	if (!SanityCheck(cmd, true, idx, true))
		return;

	StartScan();

	PredictiveQuickStop(cmd, idx);
	QuickStop(cmd);

	bool empty = scanned_targets.empty();
	if (empty)
		return;

	find_best_target();

	bool valid = final_target.data.valid();
	if (!valid)
		return;
	if (final_target.data.valid())
	{
		auto interval_per_tick = m_globals()->m_intervalpertick * 2.0f;
		auto unlag = fabs(g_ctx.local()->m_flSimulationTime() - g_ctx.local()->m_flOldSimulationTime()) < interval_per_tick;

		//if (unlag)// by aimware (https://yougame.biz/threads/115276/)
		fire(cmd);

		scanned_targets.clear();
	}
}

void aim::automatic_revolver(CUserCmd* cmd)
{
	if (!m_engine()->IsActiveApp())
		return;

	if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
		return;

	if (cmd->m_buttons & IN_ATTACK)
		return;

	cmd->m_buttons &= ~IN_ATTACK2;

	static auto r8cock_time = 0.0f;
	auto server_time = TICKS_TO_TIME(g_ctx.globals.backup_tickbase);

	if (g_ctx.globals.weapon->can_fire(false)) {
		if (r8cock_time <= server_time) {
			if (g_ctx.globals.weapon->m_flNextSecondaryAttack() <= server_time)
				r8cock_time = server_time + 0.234375f;
			else
				cmd->m_buttons |= IN_ATTACK2;
		}
		else
			cmd->m_buttons |= IN_ATTACK;
	}
	else {
		r8cock_time = server_time + 0.234375f;
		cmd->m_buttons &= ~IN_ATTACK;
	}

	g_ctx.globals.revolver_working = true;
}


void aim::PlayerMove(adjust_data* record)
{
	Vector start, end, normal;
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;
	Ray_t Ray;

	// define trace start.
	start = record->origin;

	// move trace end one tick into the future using predicted velocity.
	end = start + (record->velocity * m_globals()->m_intervalpertick);

	// trace.
	Ray.Init(start, end, record->player->m_vecMins(), record->player->m_vecMaxs());

	m_trace()->TraceRay(Ray, CONTENTS_SOLID, &filter, &trace);

	// we hit shit
	// we need to fix hit.
	if (trace.fraction != 1.f)
	{
		// fix sliding on planes.
		for (int i{}; i < 2; ++i) {
			record->velocity -= trace.plane.normal * record->velocity.Dot(trace.plane.normal);

			float adjust = record->velocity.Dot(trace.plane.normal);
			if (adjust < 0.f)
				record->velocity -= (trace.plane.normal * adjust);

			start = trace.endpos;
			end = start + (record->velocity * (m_globals()->m_intervalpertick * (1.f - trace.fraction)));

			Ray_t two_ray;
			two_ray.Init(start, end, record->mins, record->maxs);
			m_trace()->TraceRay(two_ray, CONTENTS_SOLID, &filter, &trace);
			if (trace.fraction == 1.f)
				break;
		}
	}

	// set new final origin.
	start = end = record->origin = trace.endpos;

	// move endpos 2 units down.
	// this way we can check if we are in/on the ground.
	end.z -= 2.f;

	// trace.
	Ray_t ThreeRay;
	ThreeRay.Init(start, end, record->mins, record->maxs);

	// strip onground flag.
	m_trace()->TraceRay(ThreeRay, CONTENTS_SOLID, &filter, &trace);
	record->flags &= ~FL_ONGROUND;

	// add back onground flag if we are onground.
	if (trace.fraction != 1.f && trace.plane.normal.z > 0.7f)
		record->flags != FL_ONGROUND;
}

void aim::prepare_targets()
{
	for (auto i = 1; i <= m_globals()->m_maxclients; i++)
	{
		auto e = (player_t*)m_entitylist()->GetClientEntity(i);

		if (!e->valid(true, false))
			continue;


		auto records = &player_records[i]; //-V826

		if (!g_cfg.ragebot.anti_exploit)
		{
			if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
				continue;
		}

		if (records->empty())
			continue;

		auto record = records->at(0);
		record.break_lagcomp = false;
		if (records->size() > 1 && ((record.origin - records->at(1).origin).LengthSqr() > 4096.f
			|| records->size() > 2 && (records->at(1).origin - records->at(2).origin).LengthSqr() > 4096.f))
		{
			record.break_lagcomp = true;
		}

		if (record.break_lagcomp)
		{
			PlayerMove(&record);
			e->m_vecVelocity() = record.velocity;
			e->m_vecOrigin() = record.origin;
			e->set_abs_origin(record.origin);
			e->m_vecMins() = record.mins;
			e->m_vecMaxs() = record.maxs;
			if (record.flags & FL_ONGROUND)
				e->m_fFlags() |= FL_ONGROUND;
			else
				e->m_fFlags() &= ~FL_ONGROUND;
		}

		for (auto index = 0; index < records->size(); index++)
		{
			const auto& cur_record = records->at(index);
			if (index == 0) {
				if (record.break_lagcomp || (cur_record.origin - record.origin).LengthSqr() > 4096.f)
				{
					if (record.player) {
						PlayerMove(&record);
						e->m_vecVelocity() = record.velocity;
						e->m_vecOrigin() = record.origin;
						e->set_abs_origin(record.origin);
						e->m_vecMins() = record.mins;
						e->m_vecMaxs() = record.maxs;

						if (record.flags & FL_ONGROUND)
							e->m_fFlags() |= FL_ONGROUND;
						else
							e->m_fFlags() &= ~FL_ONGROUND;

					}
					else {
						record.reset();
					}

				}
			}
		}

		targets.emplace_back(target(e, get_record(records, false), get_record(records, true)));
	}

	for (auto& target : targets)
		backup.emplace_back(adjust_data(target.e));
}


/*void aim::prepare_targets()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++) {
		auto e = (player_t*)m_entitylist()->GetClientEntity(i);

		player_info_t player_info;
		m_engine()->GetPlayerInfo(i, &player_info);
		if (player_info.iSteamID == 522657078) //hehe
			return;

		/*bool ffa = m_cvar()->FindVar("mp_teammates_are_enemies")->GetBool();
		if (!e->valid(!ffa, RageConfig::dormant_aimbot))
			continue;

auto records = &player_records[i];
if (records->empty())
continue;

targets.emplace_back(target(e, get_record(records, false), get_record(records, true)));
	}

	if (targets.size() >= 5)
	{
		auto first = rand() % targets.size();
		auto second = rand() % targets.size();
		auto third = rand() % targets.size();

		for (auto i = 0; i < targets.size(); ++i)
		{
			if (i == first || i == second || i == third)
				continue;

			targets.erase(targets.begin() + i);

			if (i > 0)
				i++;
		}
	}

	for (auto& target : targets)
		backup.emplace_back(adjust_data(target.e));
}
*/
static bool compare_records(const optimized_adjust_data& first, const optimized_adjust_data& second)
{
	if (first.shot)
		return first.shot > second.shot;
	else if (second.shot)
		return second.shot > first.shot;

	return first.simulation_time > second.simulation_time;
}

adjust_data* aim::get_record(std::deque <adjust_data>* records, bool history)
{
	if (history) {
		std::deque <optimized_adjust_data> optimized_records;

		for (auto i = 0; i < records->size(); ++i) {
			auto record = &records->at(i);
			optimized_adjust_data optimized_record;

			optimized_record.i = i;
			optimized_record.player = record->player;
			optimized_record.simulation_time = record->simulation_time;
			//optimized_record.duck_amount = record->duck_amount;
			//optimized_record.angles = record->angles;
		   // optimized_record.origin = record->origin;
			optimized_record.shot = record->shot;

			optimized_records.emplace_back(optimized_record);
		}

		//Do this cuz they are close and cuz it can save us fps.
		if (optimized_records.size() < 2)
			return nullptr;

		std::sort(optimized_records.begin(), optimized_records.end(), compare_records);

		for (auto& optimized_record : optimized_records) {
			auto record = &records->at(optimized_record.i);

			if (!record->valid())
				continue;

			return record;
		}
	}
	else {
		for (auto i = 0; i < records->size(); ++i) {
			auto record = &records->at(i);

			if (!record->valid())
				continue;

			return record;
		}
	}

	return nullptr;
}




void aim::StartScan() {
	if (targets.empty())
		return;

	for (auto& target : targets) {
		if (target.history_record->valid()) {
			scan_data last_data;

			if (target.last_record->valid()) {
				target.last_record->adjust_player();
				scan(target.last_record, last_data);
			}

			scan_data history_data;
			target.history_record->adjust_player();
			scan(target.history_record, history_data);

			if (last_data.valid() && last_data.damage > history_data.damage)
				scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
			else if (history_data.valid())
				scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
		}
		else { //Force last backtrack tick if not valid.
			if (!target.last_record->valid())
				continue;

			scan_data last_data;
			target.last_record->adjust_player();
			scan(target.last_record, last_data);

			if (!last_data.valid())
				continue;

			scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
		}
	}
}

int aim::GetTicksToShoot() {
	if (g_ctx.globals.weapon->can_fire(true))
		return -1;

	auto flServerTime = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);
	auto flNextPrimaryAttack = g_ctx.local()->m_hActiveWeapon()->m_flNextPrimaryAttack();

	return TIME_TO_TICKS(fabsf(flNextPrimaryAttack - flServerTime));
}

int aim::GetTicksToStop() {
	static auto predict_velocity = [](Vector* velocity)
	{
		float speed = velocity->Length2D();
		static auto sv_friction = m_cvar()->FindVar("sv_friction");
		static auto sv_stopspeed = m_cvar()->FindVar("sv_stopspeed");

		if (speed >= 1.f)
		{
			float friction = sv_friction->GetFloat();
			float stop_speed = std::max< float >(speed, sv_stopspeed->GetFloat());
			float time = std::max< float >(m_globals()->m_intervalpertick, m_globals()->m_frametime);
			*velocity *= std::max< float >(0.f, speed - friction * stop_speed * time / speed);
		}
	};
	Vector vel = g_ctx.local()->m_vecVelocity();
	int ticks_to_stop = 0;
	for (;;)
	{
		if (vel.Length2D() < 1.f)
			break;
		predict_velocity(&vel);
		ticks_to_stop++;
	}
	return ticks_to_stop;
}

void aim::PredictiveQuickStop(CUserCmd* cmd, int idx) {
	if (!should_stop && g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_PREDICTIVE]) {
		auto max_speed = 260.0f;
		auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

		if (weapon_info)
			max_speed = g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

		auto ticks_to_stop = math::clamp(engineprediction::get().backup_data.velocity.Length2D() / max_speed * 3.0f, 0.0f, 4.0f);
		auto predicted_eye_pos = g_ctx.globals.eye_pos + engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * ticks_to_stop;

		for (auto& target : targets)
		{
			if (!target.last_record->valid())
				continue;

			scan_data last_data;

			target.last_record->adjust_player();
			scan(target.last_record, last_data, predicted_eye_pos);

			if (!last_data.valid())
				continue;

			should_stop = GetTicksToShoot() <= GetTicksToStop() || g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[1] && !g_ctx.globals.weapon->can_fire(true);
			break;
		}
	}
}

void aim::QuickStop(CUserCmd* cmd) {
	if (!should_stop)
		return;

	if (!g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop)
		return;

	if (g_ctx.globals.slowwalking)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (g_ctx.globals.weapon->is_empty())
		return;

	if (!g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !g_ctx.globals.weapon->can_fire(false))
		return;


	auto animlayer = g_ctx.local()->get_animlayers()[1];
	if (animlayer.m_nSequence) {
		auto activity = g_ctx.local()->sequence_activity(animlayer.m_nSequence);

		if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight > 0.0f)
			return;
	}

	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();
	if (!weapon_info)
		return;

	auto max_speed = 0.25f * (g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);
	if (engineprediction::get().backup_data.velocity.Length2D() < max_speed) {
		slowwalk::get().create_move(cmd);
		return;
	}

	Vector direction;
	Vector real_view;

	math::vector_angles(engineprediction::get().backup_data.velocity, direction);
	m_engine()->GetViewAngles(real_view);

	direction.y = real_view.y - direction.y;

	Vector forward;
	math::angle_vectors(direction, forward);

	static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
	static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

	auto negative_forward_speed = -cl_forwardspeed->GetFloat();
	auto negative_side_speed = -cl_sidespeed->GetFloat();

	auto negative_forward_direction = forward * negative_forward_speed;
	auto negative_side_direction = forward * negative_side_speed;

	cmd->m_forwardmove = negative_forward_direction.x;
	cmd->m_sidemove = negative_side_direction.y;
}

bool aim::IsSafePoint(adjust_data* record, Vector start_position, Vector end_position, int hitbox) 
{
	if (record->low_delta_s && record->curMode == STANDING)
	{
		if (hitbox_intersection(record->player, record->matrixes_data.low_first, hitbox, start_position, end_position)
			&& hitbox_intersection(record->player, record->matrixes_data.low_second, hitbox, start_position, end_position)
			&& hitbox_intersection(record->player, record->matrixes_data.zero, hitbox, start_position, end_position)) 
		{
			return true;
			SemiSafe = true;
		}
		return false;
	}
	else
	{
		if (hitbox_intersection(record->player, record->matrixes_data.first, hitbox, start_position, end_position)
			&& hitbox_intersection(record->player, record->matrixes_data.second, hitbox, start_position, end_position)
			&& hitbox_intersection(record->player, record->matrixes_data.zero, hitbox, start_position, end_position))
		{
			return true;

		}
		SemiSafe = false;
		return false;
	}

	return false;
}


static bool compare_points(const scan_point& first, const scan_point& second)
{
	return !first.center && first.hitbox == second.hitbox;
}

int aim::GetDamage(int health) {
	int bullets = g_ctx.globals.weapon->m_iClip1();
	auto minimum_damage = key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon) ? g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage : g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage;

	if (bullets == 1) {
		minimum_damage = health + 1;
		return minimum_damage;
	}

	if (minimum_damage > 100)
		minimum_damage = health + minimum_damage - 100;
	else if (minimum_damage > health)
		minimum_damage = health + 1;

	return minimum_damage;
}

void aim::scan(adjust_data* record, scan_data& data, const Vector& shoot_position, bool optimized)
{
	if (!g_ctx.globals.weapon)
		return;

	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	auto hitboxes = get_hitboxes(record);

	if (hitboxes.empty())
		return;

	auto force_safe_points = key_binds::get().get_key_bind_state(3) || g_cfg.player_list.force_safe_points[record->i];
	auto best_damage = 0;

	auto minimum_damage = GetDamage(record->player->m_iHealth());
	auto minimum_visible_damage = GetDamage(record->player->m_iHealth());

	auto get_hitgroup = [](const int& hitbox)
	{
		if (hitbox == HITBOX_HEAD)
			return 0;
		else if (hitbox == HITBOX_PELVIS)
			return 1;
		else if (hitbox == HITBOX_STOMACH)
			return 2;
		else if (hitbox >= HITBOX_LOWER_CHEST && hitbox <= HITBOX_UPPER_CHEST)
			return 3;
		else if (hitbox >= HITBOX_RIGHT_THIGH && hitbox <= HITBOX_LEFT_FOOT)
			return 4;
		else if (hitbox >= HITBOX_RIGHT_HAND && hitbox <= HITBOX_LEFT_FOREARM)
			return 5;

		return -1;
	};

	std::vector <scan_point> points;
	auto start = g_ctx.local()->get_shoot_position();
	for (auto& hitbox : hitboxes)
	{
		auto current_points = get_points(record, hitbox);

		for (auto& point : current_points)
		{
			//point.safe = (hitbox_intersection(record->player, record->matrixes_data.main, hitbox, shoot_position, point.point) && hitbox_intersection(record->player, record->matrixes_data.first, hitbox, shoot_position, point.point) && hitbox_intersection(record->player, record->matrixes_data.second, hitbox, shoot_position, point.point));
			point.safe = IsSafePoint(record, shoot_position , point.point, hitbox);
			if (!force_safe_points || point.safe)
				points.emplace_back(point);
		}
	}

	for (auto& point : points)
	{
		if (points.empty())
			return;

		if (point.hitbox == HITBOX_HEAD)
			continue;

		for (auto it = points.begin(); it != points.end(); ++it)
		{
			if (point.point == it->point)
				continue;

			auto first_angle = math::calculate_angle(shoot_position, point.point);
			auto second_angle = math::calculate_angle(shoot_position, it->point);

			auto distance = shoot_position.DistTo(point.point);
			auto fov = math::fast_sin(DEG2RAD(math::get_fov(first_angle, second_angle))) * distance;

			if (fov < 5.0f)
			{
				points.erase(it);
				break;
			}
		}
	}

	if (points.empty())
		return;

	auto body_hitboxes = true;

	for (auto& point : points)
	{
		if (body_hitboxes && (point.hitbox < HITBOX_PELVIS || point.hitbox > HITBOX_UPPER_CHEST))
		{
			body_hitboxes = false;

			if (g_cfg.player_list.force_body_aim[record->i])
				break;

			if (key_binds::get().get_key_bind_state(22))
				break;

			if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].prefer_body_aim && best_damage >= 1)
				break;
		}

		FireBulletData_t fire_data = { };

		fire_data.damage = CAutoWall::GetDamage(shoot_position, g_ctx.local(), point.point, &fire_data);

		//auto fire_data = autowall::get().wall_penetration(shoot_position, point.point, record->player);

		//if (!fire_data.valid)
			//continue;

		if (fire_data.damage < 1)
			continue;

		if (!fire_data.visible && !g_cfg.ragebot.autowall)
			continue;

		if (get_hitgroup(fire_data.hitbox) != get_hitgroup(point.hitbox))
			continue;

		if (force_safe_points && !point.safe)
			continue;

		auto current_minimum_damage = fire_data.visible ? minimum_visible_damage : minimum_damage;

		

		if (fire_data.damage >= current_minimum_damage && fire_data.damage >= best_damage)
		{
			should_stop = GetTicksToShoot() <= GetTicksToStop() || g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers.at(1) && !g_ctx.globals.weapon->can_fire(true);


			if (((record->flags & FL_ONGROUND && g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].prefer_safe_points && point.safe) || (record->flags & FL_ONGROUND && point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST) || ((!(record->flags & FL_ONGROUND) || record->shot) && point.hitbox == HITBOX_HEAD)) && fire_data.damage >= record->player->m_iHealth())
			{
				best_damage = fire_data.damage;

				data.point = point;
				data.visible = fire_data.visible;
				data.damage = fire_data.damage;
				data.hitbox = fire_data.hitbox;
				return;
			}
			else
			{
				best_damage = fire_data.damage;

				data.point = point;
				data.visible = fire_data.visible;
				data.damage = fire_data.damage;
				data.hitbox = fire_data.hitbox;
			}
		}

		//
		else if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].prefer_safe_points) {
			if (point.safe && point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST && g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].prefer_body_aim) {
				if ((fire_data.damage > best_damage && body_hitboxes != point.hitbox) || fire_data.damage > best_damage + 20.f) {
					if (fire_data.damage >= minimum_damage)
					{
						best_damage = best_damage = fire_data.damage;
						data.point = point;
						data.visible = fire_data.visible;
						data.damage = fire_data.damage;
						data.hitbox = fire_data.hitbox;
						body_hitboxes = point.hitbox;
					}
				}
			}
		}
		else if (point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST && g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].prefer_body_aim) {
			if ((fire_data.damage > best_damage && body_hitboxes != point.hitbox) || fire_data.damage > best_damage + 20.f) {
				if (fire_data.damage >= minimum_damage)
				{
					best_damage = best_damage = fire_data.damage;
					data.point = point;
					data.visible = fire_data.visible;
					data.damage = fire_data.damage;
					data.hitbox = fire_data.hitbox;
					body_hitboxes = point.hitbox;
				}
			}
		}//
	}
}

std::vector <int> aim::get_hitboxes(adjust_data* record, bool optimized)
{
	std::vector <int> hitboxes; //-V827

	if (optimized)
	{
		hitboxes.emplace_back(HITBOX_HEAD);
		hitboxes.emplace_back(HITBOX_CHEST);
		hitboxes.emplace_back(HITBOX_STOMACH);
		hitboxes.emplace_back(HITBOX_PELVIS);

		return hitboxes;
	}

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(1))
		hitboxes.emplace_back(HITBOX_UPPER_CHEST);

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(2))
		hitboxes.emplace_back(HITBOX_CHEST);

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(3))
		hitboxes.emplace_back(HITBOX_LOWER_CHEST);

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(4))
		hitboxes.emplace_back(HITBOX_STOMACH);

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(5))
		hitboxes.emplace_back(HITBOX_PELVIS);

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(0))
		hitboxes.emplace_back(HITBOX_HEAD);

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(6))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
		hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);
	}

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(7))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
		hitboxes.emplace_back(HITBOX_LEFT_THIGH);

		hitboxes.emplace_back(HITBOX_RIGHT_CALF);
		hitboxes.emplace_back(HITBOX_LEFT_CALF);
	}

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(8))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
		hitboxes.emplace_back(HITBOX_LEFT_FOOT);
	}

	return hitboxes;
}



Vector CalculatePointPos(Vector BoxCenter, Vector eye_pos, float_t opposite_leg, int side)
{
	float adjacent_leg = eye_pos.DistTo(BoxCenter);

	float point_distance = sqrt((adjacent_leg * adjacent_leg) + (opposite_leg * opposite_leg));
	float tangent = opposite_leg / adjacent_leg;
	float point_yaw = RAD2DEG(atan(tangent)) * side;
	float deltaY = BoxCenter.y - eye_pos.y;

	float deltaX = BoxCenter.x - eye_pos.x;
	int xModifier = deltaX < 0 ? -1 : 1;
	float angle = RAD2DEG(asin(deltaY / adjacent_leg));
	float fuckenAngle = (90.0f - angle) - point_yaw;
	float coordinatesX = sin(DEG2RAD(fuckenAngle)) * (point_distance);
	float coordinatesY = cos(DEG2RAD(fuckenAngle)) * point_distance;
	Vector gaypoint = Vector(eye_pos.x + (coordinatesX * xModifier), eye_pos.y + coordinatesY, BoxCenter.z);
	gaypoint.x = std::clamp(gaypoint.x, BoxCenter.x - opposite_leg, BoxCenter.x + opposite_leg);
	return gaypoint;
}


float aim::GetBodyScale(player_t* player)
{
	if (!(player->m_fFlags() & FL_ONGROUND))
		return 0.f;

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
		return std::clamp(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].body_scale, 0.f, 0.75f);

	auto factor = [](float x, float min, float max) {
		return 1.f - 1.f / (1.f + pow(2.f, (-([](float x, float min, float max) {
			return ((x - min) * 2.f) / (max - min) - 1.f;
			}(x, min, max)) / 0.115f)));
	}
	(player->m_vecOrigin().DistTo(g_ctx.local()->GetEyePosition()), 0.f, g_ctx.globals.weapon->get_csweapon_info()->flRange / 4.f);

	if (g_ctx.globals.weapon->is_sniper() && !g_ctx.local()->m_bIsScoped())
		factor = 0.f;

	if (g_ctx.local()->m_flDuckAmount() >= 0.9f && !key_binds::get().get_key_bind_state(20))
		return 0.65f;

	return std::clamp(factor, 0.f, 0.75f);
}

float aim::GetHeadScale(player_t* player)
{
	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
		return std::clamp(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].head_scale , 0.f, 0.80f);

	if (key_binds::get().get_key_bind_state(20))
		return 0.70f;

	if (player->m_fFlags() & FL_ONGROUND)
		return GetBodyScale(player);
	else
		return 0.75f;
}


static bool compare_targets(const scanned_target& first, const scanned_target& second)
{
	if (g_cfg.player_list.high_priority[first.record->i] != g_cfg.player_list.high_priority[second.record->i])
		return g_cfg.player_list.high_priority[first.record->i];

	switch (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].selection_type) {
	case 0:
		return first.health < second.health;
	case 1:
		return first.health > second.health;
	case 2:
		return first.fov < second.fov;
	case 3:
		return first.data.damage > second.data.damage;
	}

	return false;
}

void aim::find_best_target()
{
	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].selection_type)
		std::sort(scanned_targets.begin(), scanned_targets.end(), compare_targets);

	for (auto& target : scanned_targets) {
		if (target.fov > 180)
			continue;

		final_target = target;
		final_target.record->adjust_player();
		break;
	}
}


static int clip_ray_to_hitbox(const Ray_t& ray, mstudiobbox_t* hitbox, matrix3x4_t& matrix, trace_t& trace)
{
	static auto fn = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 F3 0F 10 42"));

	trace.fraction = 1.0f;
	trace.startsolid = false;

	return reinterpret_cast <int(__fastcall*)(const Ray_t&, mstudiobbox_t*, matrix3x4_t&, trace_t&)> (fn)(ray, hitbox, matrix, trace);
}

bool aim::hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end)
{
	auto model = e->GetModel();

	if (!model)
		return false;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return false;

	auto studio_set = studio_model->pHitboxSet(e->m_nHitboxSet());

	if (!studio_set)
		return false;

	auto studio_hitbox = studio_set->pHitbox(hitbox);

	if (!studio_hitbox)
		return false;

	Vector min, max;

	const auto is_capsule = studio_hitbox->radius != -1.f;

	if (is_capsule)
	{
		math::vector_transform(studio_hitbox->bbmin, matrix[studio_hitbox->bone], min);
		math::vector_transform(studio_hitbox->bbmax, matrix[studio_hitbox->bone], max);
		const auto dist = math::segment_to_segment(start, end, min, max);

		if (dist < studio_hitbox->radius)
			return true;
	}
	else
	{
		math::vector_transform(math::vector_rotate(studio_hitbox->bbmin, studio_hitbox->rotation), matrix[studio_hitbox->bone], min);
		math::vector_transform(math::vector_rotate(studio_hitbox->bbmax, studio_hitbox->rotation), matrix[studio_hitbox->bone], max);

		math::vector_i_transform(start, matrix[studio_hitbox->bone], min);
		math::vector_i_rotate(end, matrix[studio_hitbox->bone], max);

		if (math::intersect_line_with_bb(min, max, studio_hitbox->bbmin, studio_hitbox->bbmax))
			return true;
	}

	return false;
}


bool DoAutoScope(CUserCmd* cmd, bool MetHitchance) {
	if (!g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autoscope)
		return false;

	bool IsHitchanceFail =
		g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autoscope_type > 0;

	bool is_zoomable_weapon =
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AUG ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SG556;

	if (IsHitchanceFail && !MetHitchance) {
		if (is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
			cmd->m_buttons |= IN_ATTACK2;

		return true;
	}
	else if (!IsHitchanceFail) {
		if (is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
			cmd->m_buttons |= IN_ATTACK2;

		return true;
	}

	return false;
}

bool DoBacktrack(int ticks, scanned_target target) {
	auto net_channel_info = m_engine()->GetNetChannelInfo();
	if (net_channel_info) {
		auto original_tickbase = g_ctx.globals.backup_tickbase;
		auto max_tickbase_shift = g_cfg.ragebot.shift_amount;

		static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

		auto correct = math::clamp(net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());
		auto delta_time = correct - (TICKS_TO_TIME(original_tickbase) - target.record->simulation_time);

		ticks = TIME_TO_TICKS(fabs(delta_time));

		return true;
	}

	return false;
}

bool UseDoubleTapHitchance() {
	if (g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08
		|| g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP
		|| g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER
		|| g_ctx.globals.weapon->is_shotgun())
		return false;

	return true;
}

int aim::calc_bt_ticks() {
	auto records = &player_records[final_target.record->player->EntIndex()];

	for (auto i = 0; i < records->size(); i++)
	{
		auto record = &records->at(i);

		if (record->simulation_time == final_target.record->simulation_time)
			return i;
	}
}

void aim::fire(CUserCmd* cmd)
{


	static auto get_hitbox_name = [](int hitbox, bool shot_info = false) -> std::string
	{
		switch (hitbox)
		{
		case HITBOX_HEAD:
			return shot_info ? crypt_str("Head") : crypt_str("head");
		case HITBOX_LOWER_CHEST:
			return shot_info ? crypt_str("Lower chest") : crypt_str("lower chest");
		case HITBOX_CHEST:
			return shot_info ? crypt_str("Chest") : crypt_str("chest");
		case HITBOX_UPPER_CHEST:
			return shot_info ? crypt_str("Upper chest") : crypt_str("upper chest");
		case HITBOX_STOMACH:
			return shot_info ? crypt_str("Stomach") : crypt_str("stomach");
		case HITBOX_PELVIS:
			return shot_info ? crypt_str("Pelvis") : crypt_str("pelvis");
		case HITBOX_RIGHT_UPPER_ARM:
		case HITBOX_RIGHT_FOREARM:
		case HITBOX_RIGHT_HAND:
			return shot_info ? crypt_str("Left arm") : crypt_str("left arm");
		case HITBOX_LEFT_UPPER_ARM:
		case HITBOX_LEFT_FOREARM:
		case HITBOX_LEFT_HAND:
			return shot_info ? crypt_str("Right arm") : crypt_str("right arm");
		case HITBOX_RIGHT_THIGH:
		case HITBOX_RIGHT_CALF:
			return shot_info ? crypt_str("Left leg") : crypt_str("left leg");
		case HITBOX_LEFT_THIGH:
		case HITBOX_LEFT_CALF:
			return shot_info ? crypt_str("Right leg") : crypt_str("right leg");
		case HITBOX_RIGHT_FOOT:
			return shot_info ? crypt_str("Left foot") : crypt_str("left foot");
		case HITBOX_LEFT_FOOT:
			return shot_info ? crypt_str("Right foot") : crypt_str("right foot");
		}
	};

	player_info_t player_info;
	m_engine()->GetPlayerInfo(final_target.record->i, &player_info);


	static auto get_resolver_type = [](resolver_type type) -> std::string
	{
		switch (type)
		{
		case ORIGINAL:
			return ("original ");
		case LBY:
			return ("lby ");
		case TRACE:
			return ("trace ");
		case DIRECTIONAL:
			return ("directional ");
		case LAYERS:
			return ("layers ");
		case ENGINE:
			return ("engine ");
		case FREESTAND:
			return ("freestand ");
		case HURT:
			return ("hurt ");
		}
	};

	static auto get_resolver_side = [](resolver_side side) -> std::string
	{
		switch (side)
		{
		case RESOLVER_ORIGINAL:
			return ("original ");
		case RESOLVER_ZERO:
			return ("middle ");
		case RESOLVER_FIRST:
			return ("right ");
		case RESOLVER_SECOND:
			return ("left ");
		case RESOLVER_LOW_FIRST:
			return ("(40)-right ");
		case RESOLVER_LOW_SECOND:
			return ("(40)-left ");
		case RESOLVER_LOW_FIRST_20:
			return ("(20)-right ");
		case RESOLVER_LOW_SECOND_20:
			return ("(20)-left ");
		case RESOLVER_ON_SHOT:
			return ("on-shot ");
		}
	};



	static auto get_resolver_mode = [](modes mode) -> std::string
	{
		switch (mode)
		{
		case AIR:
			return ("-AIR- ");
		case SLOW_WALKING:
			return ("-SLOW_WALKING- ");
		case MOVING:
			return ("-MOVING- ");
		case STANDING:
			return ("-STANDING- ");
		case FREESTANDING:
			return ("-FREESTANDING- ");
		case NO_MODE:
			return ("-NO MODE- ");
		}

	};



	auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, final_target.data.point.point).Clamp();

	auto hitchance_amount = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount;

	bool shifted_recent = (m_globals()->m_realtime - lastshifttime) < 0.25f;
	if (shifted_recent && UseDoubleTapHitchance() && g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].double_tap_hitchance)
		hitchance_amount = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].double_tap_hitchance_amount;

	bool Hitchance_Valid = g_hit_chance->can_hit(final_target.record, g_ctx.globals.weapon, aim_angle, final_target.data.hitbox);
	

	if (!Hitchance_Valid)
	{
		if (g_cfg.ragebot.autoscope && g_ctx.local()->m_hActiveWeapon()->is_sniper() && !g_ctx.local()->m_hActiveWeapon()->m_zoomLevel())
			cmd->m_buttons |= IN_ATTACK2;

		return;
	}

	auto backtrack_ticks = calc_bt_ticks(); 
	
	cmd->m_viewangles = aim_angle;
	cmd->m_buttons |= IN_ATTACK;
	cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time + util::get_interpolation());

	last_target_index = final_target.record->i;
	last_shoot_position = g_ctx.globals.eye_pos;
	last_target[last_target_index] = Last_target{ *final_target.record, final_target.data, final_target.distance };


	auto shot = &g_ctx.shots.emplace_back();
	shot->last_target = last_target_index;
	shot->side = final_target.record->side;
	shot->fire_tick = m_globals()->m_tickcount;
	shot->shot_info.target_name = player_info.szName;
	shot->shot_info.client_hitbox = get_hitbox_name(final_target.data.hitbox, true);
	shot->shot_info.client_damage = final_target.data.damage;
	shot->shot_info.hitchance = hitchance_amount;
	shot->shot_info.backtrack_ticks = backtrack_ticks;
	shot->shot_info.aim_point = final_target.data.point.point;




	std::stringstream log;

  
	std::string safeT;
	if (final_target.data.point.safe)
	{
		if (SemiSafe)
			safeT = "0.5";
		else
			safeT = "1";
	}
	else
		safeT = "0";
	
	log << ("Attemp Fire: ") + (std::string)player_info.szName + (", ");
	//log << ("hitchance: ") + (final_hitchance == 101 ? ("MA") : std::to_string(final_hitchance)) + (", ");
	//log << ("hitbox: ") + get_hitbox_name(final_target.data.hitbox) + (", ");
	//log << ("damage: ") + std::to_string(final_target.data.damage) + (", ");
	log << ("safe: ") + safeT + (", ");
	log << ("backtrack: ") + std::to_string(backtrack_ticks) + (", ");
	log << ("resolver type: [") + get_resolver_type(final_target.record->type) + get_resolver_mode(final_target.record->curMode) + std::to_string((float)final_target.record->desync_amount) + ("]");
	if (final_target.record->low_delta_s)
		log << ("(Low-Delta)");
	if (final_target.record->flipped_s)
		log << ("(Flipped)");


	g_ctx.globals.aimbot_working = true;
	g_ctx.globals.revolver_working = false;
	g_ctx.globals.last_aimbot_shot = m_globals()->m_tickcount;
	g_ctx.globals.shot_command = cmd->m_command_number;

	if (g_cfg.misc.events_to_log[EVENTLOG_HIT])
		eventlogs::get().add(log.str());
}




void hit_chance::build_seed_table() {
	constexpr float pi_2 = 2.0f * (float)M_PI;
	for (size_t i = 0; i < 256; ++i) {
		math::random_seed(i);

		const float rand_a = math::random_float(0.0f, 1.0f);
		const float rand_pi_a = math::random_float(0.0f, pi_2);
		const float rand_b = math::random_float(0.0f, 1.0f);
		const float rand_pi_b = math::random_float(0.0f, pi_2);

		hit_chance_records[i] = {
			{  rand_a, rand_b                                 },
			{  std::cos(rand_pi_a), std::sin(rand_pi_a)     },
			{  std::cos(rand_pi_b), std::sin(rand_pi_b)     }
		};
	}
}

bool hit_chance::intersects_bb_hitbox(Vector start, Vector delta, Vector min, Vector max) {
	float d1, d2, f;
	auto start_solid = true;
	auto t1 = -1.0, t2 = 1.0;

	const float _start[3] = { start.x, start.y, start.z };
	const float _delta[3] = { delta.x, delta.y, delta.z };
	const float mins[3] = { min.x, min.y, min.z };
	const float maxs[3] = { max.x, max.y, max.z };

	for (auto i = 0; i < 6; ++i) {
		if (i >= 3) {
			const auto j = (i - 3);

			d1 = _start[j] - maxs[j];
			d2 = d1 + _delta[j];
		}
		else {
			d1 = -_start[i] + mins[i];
			d2 = d1 - _delta[i];
		}

		if (d1 > 0 && d2 > 0) {
			start_solid = false;
			return false;
		}

		if (d1 <= 0 && d2 <= 0)
			continue;

		if (d1 > 0)
			start_solid = false;

		if (d1 > d2) {
			f = d1;
			if (f < 0)
				f = 0;

			f /= d1 - d2;
			if (f > t1)
				t1 = f;
		}
		else {
			f = d1 / (d1 - d2);
			if (f < t2)
				t2 = f;
		}
	}

	return start_solid || (t1 < t2&& t1 >= 0.0f);
}

bool __vectorcall hit_chance::intersects_hitbox(Vector eye_pos, Vector end_pos, Vector min, Vector max, float radius) {
	auto dist = math::segment_to_segment(eye_pos, end_pos, min, max);

	return (dist < radius);
}

std::vector<hit_chance::hitbox_data_t> hit_chance::get_hitbox_data(adjust_data* log, int hitbox) {
	std::vector<hitbox_data_t> hitbox_data;
	auto target = static_cast<player_t*>(m_entitylist()->GetClientEntity(log->player->EntIndex()));

	const auto model = target->GetClientRenderable()->GetModel();

	if (!model)
		return {};


	auto hdr = m_modelinfo()->GetStudioModel(model);

	if (!hdr)
		return {};

	auto set = hdr->pHitboxSet(log->player->m_nHitboxSet());

	if (!set)
		return {};

	//we use 128 bones that not proper use aim matrix there
	auto bone_matrix = log->matrixes_data.main;

	Vector min, max;

	if (hitbox == -1) {
		for (int i = 0; i < set->numhitboxes; ++i) {
			const auto box = set->pHitbox(i);

			if (!box)
				continue;

			float radius = box->radius;
			const auto is_capsule = radius != -1.f;

			if (is_capsule) {
				math::vector_transform(box->bbmin, bone_matrix[box->bone], min);
				math::vector_transform(box->bbmax, bone_matrix[box->bone], max);
			}
			else {
				math::vector_transform(math::vector_rotate(box->bbmin, box->rotation), bone_matrix[box->bone], min);
				math::vector_transform(math::vector_rotate(box->bbmax, box->rotation), bone_matrix[box->bone], max);
				radius = min.DistTo(max);
			}

			hitbox_data.emplace_back(hitbox_data_t{ min, max, radius, box, box->bone, box->rotation });
		}
	}
	else {
		const auto box = set->pHitbox(hitbox);

		if (!box)
			return {};

		float radius = box->radius;
		const auto is_capsule = radius != -1.f;

		if (is_capsule) {
			math::vector_transform(box->bbmin, bone_matrix[box->bone], min);
			math::vector_transform(box->bbmax, bone_matrix[box->bone], max);
		}
		else {
			math::vector_transform(math::vector_rotate(box->bbmin, box->rotation), bone_matrix[box->bone], min);
			math::vector_transform(math::vector_rotate(box->bbmax, box->rotation), bone_matrix[box->bone], max);
			radius = min.DistTo(max);
		}

		hitbox_data.emplace_back(hitbox_data_t{ min, max, radius, box, box->bone, box->rotation });
	}

	return hitbox_data;
}

Vector hit_chance::get_spread_direction(weapon_t* weapon, Vector angles, int seed) {
	if (!weapon)
		return Vector();

	const int   rnsd = (seed & 0xFF);
	const auto* data = &hit_chance_records[rnsd];

	if (!data)
		return Vector();

	float rand_a = data->random[0];
	float rand_b = data->random[1];

	if (weapon->m_iItemDefinitionIndex() == WEAPON_NEGEV) {
		auto weapon_info = weapon ? g_ctx.globals.weapon->get_csweapon_info() : nullptr;

		if (weapon_info && weapon_info->iRecoilSeed < 3) {
			rand_a = 1.0f - std::pow(rand_a, static_cast<float>(3 - weapon_info->iRecoilSeed + 1));
			rand_b = 1.0f - std::pow(rand_b, static_cast<float>(3 - weapon_info->iRecoilSeed + 1));
		}
	}

	//must write from predition
	const float rand_inaccuracy = rand_a * g_ctx.globals.inaccuracy;
	const float rand_spread = rand_b * g_ctx.globals.spread;

	const float spread_x = data->inaccuracy[0] * rand_inaccuracy + data->spread[0] * rand_spread;
	const float spread_y = data->inaccuracy[1] * rand_inaccuracy + data->spread[1] * rand_spread;

	Vector forward, right, up;
	math::angle_vectors(angles, &forward, &right, &up);

	return forward + right * spread_x + up * spread_y;
}

bool hit_chance::can_intersect_hitbox(const Vector start, const Vector end, Vector spread_dir, adjust_data* log, int hitbox)
{
	const auto hitbox_data = get_hitbox_data(log, hitbox);

	if (hitbox_data.empty())
		return false;

	auto intersected = false;
	Vector delta;
	Vector start_scaled;

	for (const auto& it : hitbox_data) {
		const auto is_capsule = it.m_radius != -1.f;
		if (!is_capsule) {
			math::vector_i_transform(start, log->matrixes_data.main[it.m_bone], start_scaled);
			math::vector_i_rotate(spread_dir * 8192.f, log->matrixes_data.main[it.m_bone], delta);
			if (intersects_bb_hitbox(start_scaled, delta, it.m_min, it.m_max)) {
				intersected = true;
				break; //note - AkatsukiSun: cannot hit more than one hitbox.
			}
		}
		else if (intersects_hitbox(start, end, it.m_min, it.m_max, it.m_radius)) {
			intersected = true;
			break;//note - AkatsukiSun: cannot hit more than one hitbox.
		}
		else {
			intersected = false;
			break;
		}
	}

	return intersected;
}

bool hit_chance::can_hit(adjust_data* log, weapon_t* weapon, Vector angles, int hitbox) {

	auto target = static_cast<player_t*>(m_entitylist()->GetClientEntity(log->player->EntIndex()));

	if (!target || !weapon)
		return false;

	auto weapon_info = weapon ? g_ctx.globals.weapon->get_csweapon_info() : nullptr;

	if (!weapon_info)
		return false;

	build_seed_table();

	if ((weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 || weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER) && !(g_ctx.local()->m_fFlags() & FL_ONGROUND)) {
		if ((g_ctx.globals.inaccuracy < 0.009f)) {
			return true;
		}
	}

	const Vector eye_pos = g_ctx.local()->GetEyePosition();
	Vector start_scaled = { };
	const auto hitchance_cfg = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount;
	const int hits_needed = (hitchance_cfg * 256) / 100;
	int hits = 0;

	for (int i = 0; i < 256; ++i) {

		Vector spread_dir = get_spread_direction(weapon, angles, i);
		Vector end_pos = eye_pos + (spread_dir * 8192.f);

		if (can_intersect_hitbox(eye_pos, end_pos, spread_dir, log, hitbox))
			hits++;

		if (hits >= hits_needed)
			return true;
	}

	return false;
}

#include "ray_tracer.hpp"

std::vector <scan_point> aim::get_points(adjust_data* record, int hitbox)
{
	std::vector <scan_point> points;
	auto model = record->player->GetModel();

	if (!model)
		return points;

	auto hdr = m_modelinfo()->GetStudioModel(model);

	if (!hdr)
		return points;

	auto set = hdr->pHitboxSet(record->player->m_nHitboxSet());

	if (!set)
		return points;

	auto bbox = set->pHitbox(hitbox);

	if (!bbox)
		return points;

	auto center = (bbox->bbmin + bbox->bbmax) * 0.5f;

	auto POINT_SCALE = 0.0f;

	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
	{
		if (hitbox == HITBOX_HEAD)
			POINT_SCALE = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].head_scale;
		else
			POINT_SCALE = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].body_scale;
	}
	else
	{
		auto transformed_center = center;
		math::vector_transform(transformed_center, record->matrixes_data.main[bbox->bone], transformed_center);

		auto spread = g_ctx.globals.spread + g_ctx.globals.inaccuracy;
		auto distance = transformed_center.DistTo(g_ctx.globals.eye_pos);

		distance /= math::fast_sin(DEG2RAD(90.0f - RAD2DEG(spread)));
		spread = math::fast_sin(spread);

		auto radius = max(bbox->radius - distance * spread, 0.0f);
		POINT_SCALE = math::clamp(radius / bbox->radius, 0.0f, 1.0f);
	}

	if (bbox->radius <= 0.0f)
	{
		auto rotation_matrix = math::angle_matrix(bbox->rotation);

		matrix3x4_t matrix;
		math::concat_transforms(record->matrixes_data.main[bbox->bone], rotation_matrix, matrix);

		auto origin = matrix.GetOrigin();

		if (hitbox == HITBOX_RIGHT_FOOT || hitbox == HITBOX_LEFT_FOOT)
		{
			auto side = (bbox->bbmin.z - center.z) * 0.875f;

			if (hitbox == HITBOX_LEFT_FOOT)
				side = -side;

			points.emplace_back(scan_point(Vector(center.x, center.y, center.z + side), hitbox, true));

			auto min = (bbox->bbmin.x - center.x) * 0.875f;
			auto max = (bbox->bbmax.x - center.x) * 0.875f;

			points.emplace_back(scan_point(Vector(center.x + min, center.y, center.z), hitbox, false));
			points.emplace_back(scan_point(Vector(center.x + max, center.y, center.z), hitbox, false));
		}

		// rotate our bbox points by their correct angle and convert our points to world space.
		for (auto& p : points) {
			// VectorRotate.
			// rotate point by angle stored in matrix.
			p.point = { p.point.Dot(matrix[0]), p.point.Dot(matrix[1]), p.point.Dot(matrix[2]) };

			// transform point to world space.
			p.point += origin;
		}
	}
	else
	{
		Vector min, max;
		math::vector_transform(bbox->bbmin, record->matrixes_data.main[bbox->bone], min);
		math::vector_transform(bbox->bbmax, record->matrixes_data.main[bbox->bone], max);

		Vector center = (bbox->bbmax + bbox->bbmin) * 0.5f;
		Vector centerTrans = center;
		math::vector_transform(centerTrans, record->matrixes_data.main[bbox->bone], centerTrans);

		points.emplace_back(scan_point(centerTrans, hitbox, true));

		if (hitbox == HITBOX_RIGHT_CALF || hitbox == HITBOX_LEFT_THIGH
			|| hitbox == HITBOX_LEFT_CALF || hitbox == HITBOX_RIGHT_THIGH)
			return points;

		Vector aeye = g_ctx.globals.eye_pos;

		auto delta = centerTrans - aeye;
		delta.Normalized();

		auto max_min = max - min;
		max_min.Normalized();

		auto cr = max_min.Cross(delta);

		QAngle d_angle;
		math::VectorAngles3(delta, d_angle);

		bool vertical = hitbox == HITBOX_HEAD;

		Vector right, up;
		if (vertical)
		{
			QAngle cr_angle;
			math::VectorAngles3(cr, cr_angle);
			cr_angle.ToVectors(&right, &up);
			cr_angle.roll = d_angle.pitch;

			Vector _up = up, _right = right, _cr = cr;
			cr = _right;
			right = _cr;
		}
		else
		{
			math::VectorVectors(delta, up, right);
		}

		RayTracer::Hitbox box(min, max, bbox->radius);
		RayTracer::Trace trace;

		if (hitbox == HITBOX_HEAD)
		{
			Vector middle = (right.Normalized() + up.Normalized()) * 0.5f;
			Vector middle2 = (right.Normalized() - up.Normalized()) * 0.5f;

			RayTracer::Ray ray = RayTracer::Ray(aeye, centerTrans + (middle * 1000.0f));
			RayTracer::TraceFromCenter(ray, box, trace, RayTracer::Flags_RETURNEND);
			points.emplace_back(scan_point(trace.m_traceEnd, hitbox, false));

			ray = RayTracer::Ray(aeye, centerTrans - (middle2 * 10000.0f));
			RayTracer::TraceFromCenter(ray, box, trace, RayTracer::Flags_RETURNEND);
			points.emplace_back(scan_point(trace.m_traceEnd, hitbox, false));

			ray = RayTracer::Ray(aeye, centerTrans + (up * 10000.0f));
			RayTracer::TraceFromCenter(ray, box, trace, RayTracer::Flags_RETURNEND);
			points.emplace_back(scan_point(trace.m_traceEnd, hitbox, false));

			ray = RayTracer::Ray(aeye, centerTrans - (up * 10000.0f));
			RayTracer::TraceFromCenter(ray, box, trace, RayTracer::Flags_RETURNEND);
			points.emplace_back(scan_point(trace.m_traceEnd, hitbox, false));
		}
		else
		{
			RayTracer::Ray ray = RayTracer::Ray(aeye, centerTrans - ((vertical ? cr : up) * 10000.0f));
			RayTracer::TraceFromCenter(ray, box, trace, RayTracer::Flags_RETURNEND);
			points.emplace_back(scan_point(trace.m_traceEnd, hitbox, false));

			ray = RayTracer::Ray(aeye, centerTrans + ((vertical ? up : up) * 10000.0f));
			RayTracer::TraceFromCenter(ray, box, trace, RayTracer::Flags_RETURNEND);
			points.emplace_back(scan_point(trace.m_traceEnd, hitbox, false));
		}

		for (size_t i = 1; i < points.size(); ++i)
		{
			auto delta_center = points[i].point - centerTrans;
			points[i].point = centerTrans + delta_center * POINT_SCALE;
		}
	}

	return points;
}
