// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "aim.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"

void aim::run(CUserCmd* cmd)
{
	backup.clear();
	targets.clear();
	scanned_targets.clear();
	final_target.reset();
	should_stop = false;

	if (!g_cfg.ragebot.enable)
		return;

	automatic_revolver(cmd);
	prepare_targets();

	if (g_ctx.globals.weapon->is_non_aim())
		return;

	if (g_ctx.globals.current_weapon == -1)
		return;

	scan_targets();

	if (!should_stop && g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_PREDICTIVE])
	{
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
			scan(target.last_record, last_data, predicted_eye_pos, true);

			if (!last_data.valid())
				continue;

			should_stop = true;
			break;
		}
	}

	if (!automatic_stop(cmd))
		return;

	if (scanned_targets.empty())
		return;

	find_best_target();

	if (!final_target.data.valid())
		return;



	fire(cmd);
}

float aim::LagFix()
{
	float updaterate = m_cvar()->FindVar("cl_updaterate")->GetFloat();
	auto minupdate = m_cvar()->FindVar("sv_minupdaterate");
	auto maxupdate = m_cvar()->FindVar("sv_maxupdaterate");

	if (minupdate && maxupdate)
		updaterate = maxupdate->GetFloat();

	float ratio = m_cvar()->FindVar("cl_interp_ratio")->GetFloat();

	if (ratio == 0)
		ratio = 1.0f;

	float lerp = m_cvar()->FindVar("cl_interp")->GetFloat();
	auto cmin = m_cvar()->FindVar("sv_client_min_interp_ratio");
	auto cmax = m_cvar()->FindVar("sv_client_max_interp_ratio");

	if (cmin && cmax && cmin->GetFloat() != 1)
		ratio = math::clamp(ratio, cmin->GetFloat(), cmax->GetFloat());

	return max(lerp, ratio / updaterate);
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

	if (g_ctx.globals.weapon->can_fire(false))
	{
		if (r8cock_time <= server_time) //-V807
		{
			if (g_ctx.globals.weapon->m_flNextSecondaryAttack() <= server_time)
				r8cock_time = server_time + 0.234375f;
			else
				cmd->m_buttons |= IN_ATTACK2;
		}
		else
			cmd->m_buttons |= IN_ATTACK;
	}
	else
	{
		r8cock_time = server_time + 0.234375f;
		cmd->m_buttons &= ~IN_ATTACK;
	}

	g_ctx.globals.revolver_working = true;
}

void aim::prepare_targets()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		if (g_cfg.player_list.white_list[i])
			continue;

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

		targets.emplace_back(target(e, get_record(records, false), get_record(records, true)));
	}

	if (targets.size() >= 5)///Для оптимизации => больше фпсов
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
				--i;
		}
	}


	for (auto& target : targets)
		backup.emplace_back(adjust_data(target.e));
}

static bool compare_records(const optimized_adjust_data& first, const optimized_adjust_data& second)
{
	auto first_pitch = math::normalize_pitch(first.angles.x);
	auto second_pitch = math::normalize_pitch(second.angles.x);

	if (fabs(first_pitch - second_pitch) > 15.0f)
		return fabs(first_pitch) < fabs(second_pitch);
	else if (first.duck_amount != second.duck_amount) //-V550
		return first.duck_amount < second.duck_amount;
	else if (first.origin != second.origin)
		return first.origin.DistTo(g_ctx.local()->GetAbsOrigin()) < second.origin.DistTo(g_ctx.local()->GetAbsOrigin());

	return first.simulation_time > second.simulation_time;
}

adjust_data* aim::get_record(std::deque <adjust_data>* records, bool history)
{
	if (history)
	{
		std::deque <optimized_adjust_data> optimized_records; //-V826

		for (auto i = 0; i < records->size(); ++i)
		{
			auto record = &records->at(i);
			optimized_adjust_data optimized_record;

			optimized_record.i = i;
			optimized_record.player = record->player;
			optimized_record.simulation_time = record->simulation_time;
			optimized_record.duck_amount = record->duck_amount;
			optimized_record.angles = record->angles;
			optimized_record.origin = record->origin;

			optimized_records.emplace_back(optimized_record);
		}

		if (optimized_records.size() < 1)
			return nullptr;

		std::sort(optimized_records.begin(), optimized_records.end(), compare_records);

		for (auto& optimized_record : optimized_records)
		{
			auto record = &records->at(optimized_record.i);

			if (!record->valid())
				continue;

			return record;
		}
	}
	else
	{
		for (auto i = 0; i < records->size(); ++i)
		{
			auto record = &records->at(i);

			if (!record->valid())
				continue;

			return record;
		}
	}

	return nullptr;
}

int aim::get_minimum_damage(bool visible, int health)
{
	auto minimum_damage = 1;

	if (visible)
	{
		if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_visible_damage > 100)
			minimum_damage = health + g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_visible_damage - 100;
		else
			minimum_damage = math::clamp(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_visible_damage, 1, health);
	}
	else
	{
		if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage > 100)
			minimum_damage = health + g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage - 100;
		else
			minimum_damage = math::clamp(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage, 1, health);
	}

	if (key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon))
	{
		if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage > 100)
			minimum_damage = health + g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage - 100;
		else
			minimum_damage = math::clamp(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage, 1, health);
	}
	return minimum_damage;
}

void aim::scan_targets()
{
	if (targets.empty())
		return;

	for (auto& target : targets)
	{
		if (target.history_record->valid())
		{
			scan_data last_data;

			if (target.last_record->valid())
			{
				target.last_record->adjust_player();
				scan(target.last_record, last_data);
			}

			scan_data history_data;

			target.history_record->adjust_player();
			scan(target.history_record, history_data);

			if (last_data.valid() && last_data.damage > history_data.damage)
			{
				scanned_targets.emplace_back(scanned_target(target.last_record, last_data)); //test
				//scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
			}
			if (history_data.valid())
				//scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
				scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
		}
		else
		{
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

bool aim::automatic_stop(CUserCmd* cmd)
{
	if (!should_stop)
		return true;

	if (!g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop)
		return true;

	if (g_ctx.globals.slowwalking)
		return true;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND)) //-V807
		return true;

	if (g_ctx.globals.weapon->is_empty())
		return true;

	if (!g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !g_ctx.globals.weapon->can_fire(false))
		return true;

	auto animlayer = g_ctx.local()->get_animlayers()[1];

	if (animlayer.m_nSequence)
	{
		auto activity = g_ctx.local()->sequence_activity(animlayer.m_nSequence);

		if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight > 0.0f)
			return true;
	}

	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

	if (!weapon_info)
		return true;

	auto max_speed = 0.33f * (g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);

	if (engineprediction::get().backup_data.velocity.Length2D() < max_speed)
		slowwalk::get().create_move(cmd);
	else
	{
		if (!g_cfg.ragebot.autostop_fixer || force_stop)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(engineprediction::get().backup_data.velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();



			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			cmd->m_forwardmove = negative_forward_direction.x;
			cmd->m_sidemove = negative_side_direction.y;
		}
		else
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(engineprediction::get().backup_data.velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			cmd->m_forwardmove = negative_forward_direction.x;
			cmd->m_sidemove = negative_side_direction.y;

			if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_FORCE_ACCURACY])
				return false;
		}
	}

	return true;
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

	auto force_safe_points = key_binds::get().get_key_bind_state(3) || g_cfg.player_list.force_safe_points[record->i] || g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].max_misses && g_ctx.globals.missed_shots[record->i] >= g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].max_misses_amount;
	auto best_damage = 0;

	auto minimum_damage = get_minimum_damage(false, record->player->m_iHealth());
	auto minimum_visible_damage = get_minimum_damage(true, record->player->m_iHealth());

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

	for (auto& hitbox : hitboxes)
	{
		auto current_points = get_points(record, hitbox);

		for (auto& point : current_points)
		{
			point.safe = record->bot || (hitbox_intersection(record->player, record->matrixes_data.zero, hitbox, shoot_position, point.point) && hitbox_intersection(record->player, record->matrixes_data.first, hitbox, shoot_position, point.point) && hitbox_intersection(record->player, record->matrixes_data.second, hitbox, shoot_position, point.point));

			if (!force_safe_points || point.safe)
				points.emplace_back(point);
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

			if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].prefer_body_aim && best_damage >= 1 && record->flags & FL_ONGROUND)
				break;
		}

		auto fire_data = autowall::get().wall_penetration(shoot_position, point.point, record->player);

		if (!fire_data.valid)
			continue;

		if (fire_data.damage < 1)
			continue;

		if (!fire_data.visible && !g_cfg.ragebot.autowall)
			continue;

		if (get_hitgroup(fire_data.hitbox) != get_hitgroup(point.hitbox))
			continue;

		auto current_minimum_damage = fire_data.visible ? minimum_visible_damage : minimum_damage;

		if (fire_data.damage >= current_minimum_damage && fire_data.damage >= best_damage)
		{
			if (!should_stop)
			{
				should_stop = true;

				if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_LETHAL] && fire_data.damage < record->player->m_iHealth())
					should_stop = false;
				else if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_VISIBLE] && !fire_data.visible)
					should_stop = false;
				else if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_CENTER] && !point.center)
					should_stop = false;
			}

			if (force_safe_points && !point.safe)
				continue;

			if (((record->flags & FL_ONGROUND && g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].prefer_safe_points && point.safe) || (record->flags & FL_ONGROUND && point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST) || ((!(record->flags & FL_ONGROUND)) && point.hitbox == HITBOX_HEAD)) && fire_data.damage >= record->player->m_iHealth())
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

std::vector <scan_point> aim::get_points(adjust_data* record, int hitbox, bool from_aim)// надо добавить хтбоксы тут не все
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
	}
	else
	{
		auto scale = 0.0f;

		if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
		{
			if (hitbox == HITBOX_HEAD)
				scale = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].head_scale;
			else
				scale = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].body_scale;
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
			scale = math::clamp(radius / bbox->radius, 0.0f, 1.0f);
		}

		if (scale <= 0.0f) //-V648
		{
			math::vector_transform(center, record->matrixes_data.main[bbox->bone], center);
			points.emplace_back(scan_point(center, hitbox, true));

			return points;
		}

		auto final_radius = bbox->radius * scale;

		if (hitbox == HITBOX_HEAD)
		{
			auto pitch_down = math::normalize_pitch(record->angles.x) > 85.0f;
			auto backward = fabs(math::normalize_yaw(record->angles.y - math::calculate_angle(record->player->get_shoot_position(), g_ctx.local()->GetAbsOrigin()).y)) > 120.0f;

			points.emplace_back(scan_point(center, hitbox, !pitch_down || !backward));

			points.emplace_back(scan_point(Vector(bbox->bbmax.x + 0.70710678f * final_radius, bbox->bbmax.y - 0.70710678f * final_radius, bbox->bbmax.z), hitbox, false));
			points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y, bbox->bbmax.z + final_radius), hitbox, false));
			points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y, bbox->bbmax.z - final_radius), hitbox, false));

			points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y - final_radius, bbox->bbmax.z), hitbox, false));

			if (pitch_down && backward)
				points.emplace_back(scan_point(Vector(bbox->bbmax.x - final_radius, bbox->bbmax.y, bbox->bbmax.z), hitbox, false));
		}
		else if (hitbox >= HITBOX_PELVIS && hitbox <= HITBOX_UPPER_CHEST)
		{
			points.emplace_back(scan_point(center, hitbox, true));

			points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y, bbox->bbmax.z + final_radius), hitbox, false));
			points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y, bbox->bbmax.z - final_radius), hitbox, false));

			points.emplace_back(scan_point(Vector(center.x, bbox->bbmax.y - final_radius, center.z), hitbox, true));
		}
		else if (hitbox == HITBOX_RIGHT_CALF || hitbox == HITBOX_LEFT_CALF)
		{
			points.emplace_back(scan_point(center, hitbox, true));
			points.emplace_back(scan_point(Vector(bbox->bbmax.x - final_radius, bbox->bbmax.y, bbox->bbmax.z), hitbox, false));
		}
		else if (hitbox == HITBOX_RIGHT_THIGH || hitbox == HITBOX_LEFT_THIGH)
			points.emplace_back(scan_point(center, hitbox, true));
		else if (hitbox == HITBOX_RIGHT_UPPER_ARM || hitbox == HITBOX_LEFT_UPPER_ARM)
		{
			points.emplace_back(scan_point(center, hitbox, true));
			points.emplace_back(scan_point(Vector(bbox->bbmax.x + final_radius, center.y, center.z), hitbox, false));
		}
	}

	for (auto& point : points)
		math::vector_transform(point.point, record->matrixes_data.main[bbox->bone], point.point);

	return points;
}

/*static bool compare_points(const scan_point& first, const scan_point& second)
{
	return !first.center && first.hitbox == second.hitbox;
}*/


static bool compare_targets(const scanned_target& first, const scanned_target& second)
{
	if (g_cfg.player_list.high_priority[first.record->i] != g_cfg.player_list.high_priority[second.record->i])
		return g_cfg.player_list.high_priority[first.record->i];

	switch (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].selection_type)
	{
	case 1:
		return first.fov < second.fov;
	case 2:
		return first.distance < second.distance;
	case 3:
		return first.health < second.health;
	case 4:
		return first.data.damage > second.data.damage;
	}

	return false;
}

void aim::find_best_target()
{
	if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].selection_type)
		std::sort(scanned_targets.begin(), scanned_targets.end(), compare_targets);

	for (auto& target : scanned_targets)
	{
		if (target.fov > (float)g_cfg.ragebot.field_of_view)
			continue;

		final_target = target;
		final_target.record->adjust_player();
		break;
	}
}

void aim::fire(CUserCmd* cmd)
{
	if (!g_ctx.globals.weapon->can_fire(true))
		return;

	auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, final_target.data.point.point).Clamp();

	if (!g_cfg.ragebot.autoshoot)
		return;

	if (!g_cfg.ragebot.silent_aim)
		m_engine()->SetViewAngles(aim_angle);

	auto final_hitchance = 0;
	auto is_valid_hitchance = calculate_hitchance(aim_angle, final_hitchance);

	if (!is_valid_hitchance)
	{
		auto is_zoomable_weapon = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AUG || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SG556;

		if (g_cfg.ragebot.autoscope && is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
			cmd->m_buttons |= IN_ATTACK2;

		return;
	}
	auto backtrack_ticks = 0;
	auto net_channel_info = m_engine()->GetNetChannelInfo();





	if (net_channel_info) //p back track ticks а гдебэктрэк??
	{
		auto original_tickbase = g_ctx.globals.backup_tickbase;

		if (misc::get().double_tap_enabled && misc::get().double_tap_key)
			original_tickbase = g_ctx.globals.backup_tickbase + g_cfg.ragebot.shift_amount;

		static auto sv_maxunlag = m_cvar()->FindVar(("sv_maxunlag"));

		auto correct = math::clamp(net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());
		auto delta_time = correct - (TICKS_TO_TIME(original_tickbase) - final_target.record->simulation_time);

		backtrack_ticks = TIME_TO_TICKS(fabs(delta_time));
	}




	static auto get_hitbox_name = [](int hitbox, bool shot_info = false) -> std::string
	{
		switch (hitbox)
		{
		case HITBOX_HEAD:
			return shot_info ? ("Head") : ("head");
		case HITBOX_LOWER_CHEST:
			return shot_info ? ("Lower chest") : ("lower chest");
		case HITBOX_CHEST:
			return shot_info ? ("Chest") : ("chest");
		case HITBOX_UPPER_CHEST:
			return shot_info ? ("Upper chest") : ("upper chest");
		case HITBOX_STOMACH:
			return shot_info ? ("Stomach") : ("stomach");
		case HITBOX_PELVIS:
			return shot_info ? ("Pelvis") : ("pelvis");
		case HITBOX_RIGHT_UPPER_ARM:
			return shot_info ? ("Right upper arm") : ("right upper arm");
		case HITBOX_RIGHT_FOREARM:
			return shot_info ? ("Right forearm") : ("right forearm");
		case HITBOX_RIGHT_HAND:
			return shot_info ? ("Left arm") : ("left arm");
		case HITBOX_LEFT_UPPER_ARM:
			return shot_info ? ("Left upper arm") : ("left upper arm");
		case HITBOX_LEFT_FOREARM:
			return shot_info ? ("Left forearm") : ("left forearm");
		case HITBOX_LEFT_HAND:
			return shot_info ? ("Right arm") : ("right arm");
		case HITBOX_RIGHT_THIGH:
			return shot_info ? ("Right thigh") : ("right thigh");
		case HITBOX_RIGHT_CALF:
			return shot_info ? ("Left leg") : ("left leg");
		case HITBOX_LEFT_THIGH:
			return shot_info ? ("Left thigh") : ("left thigh");
		case HITBOX_LEFT_CALF:
			return shot_info ? ("Right leg") : ("right leg");
		case HITBOX_RIGHT_FOOT:
			return shot_info ? ("Left foot") : ("left foot");
		case HITBOX_LEFT_FOOT:
			return shot_info ? ("Right foot") : ("right foot");
		}
	};




	cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time + util::get_interpolation());
	cmd->m_viewangles = aim_angle;
	cmd->m_buttons |= IN_ATTACK;
	last_target_index = final_target.record->i;
	last_shoot_position = g_ctx.globals.eye_pos;
	last_target[last_target_index] = Last_target
	{
		*final_target.record, final_target.data, final_target.distance
	};

	static float fire_delay_time = 0;

	if (!g_ctx.local()->m_iShotsFired() && key_binds::get().get_key_bind_state(0) && fire_delay_time < g_cfg.ragebot.fire_delay_time)
	{
		fire_delay_time += 1;
		return;
	}
	else if (!g_ctx.local()->m_iShotsFired())
		fire_delay_time = 0;


	/**/

	player_info_t player_info;
	m_engine()->GetPlayerInfo(final_target.record->i, &player_info);


	//if (g_cfg.ragebot.fire_delay && )
	//	return;

	auto shot = &g_ctx.shots.emplace_back();

	shot->last_target = last_target_index;
	shot->side = final_target.record->side;
	shot->fire_tick = m_globals()->m_tickcount;
	shot->shot_info.target_name = player_info.szName;
	shot->shot_info.client_hitbox = get_hitbox_name(final_target.data.hitbox, true);
	shot->shot_info.client_damage = final_target.data.damage;
	shot->shot_info.hitchance = final_hitchance;
	shot->shot_info.backtrack_ticks = backtrack_ticks;
	shot->shot_info.aim_point = final_target.data.point.point;

	g_ctx.globals.aimbot_working = true;
	g_ctx.globals.revolver_working = false;
	g_ctx.globals.last_aimbot_shot = m_globals()->m_tickcount;


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


	std::stringstream log;

	log << ("Attemp Fire: ") + (std::string)player_info.szName + (", ");
	//log << ("hitchance: ") + (final_hitchance == 101 ? ("MA") : std::to_string(final_hitchance)) + (", ");
	//log << ("hitbox: ") + get_hitbox_name(final_target.data.hitbox) + (", ");
	//log << ("damage: ") + std::to_string(final_target.data.damage) + (", ");
	//log << ("safe: ") + std::to_string((bool)final_target.data.point.safe) + (", ");
	//log << ("backtrack: ") + std::to_string(backtrack_ticks) + (", ");
	log << ("resolver type: [") + get_resolver_type(final_target.record->type) + get_resolver_mode(final_target.record->curMode) + get_resolver_side(final_target.record->side) + ("]");
	if (final_target.record->flipped_s)
		log << ("(Flipped)");

	if (g_cfg.misc.events_to_log[EVENTLOG_HIT] && final_target.record->type != ORIGINAL && final_target.record->curSide != SIDE_NONE && final_target.record->curMode != NO_MODE) // temporary fix for overloading eventlog with phantom hits when enemy is using defensive
		eventlogs::get().add(log.str());

}





static int clip_ray_to_hitbox(const Ray_t& ray, mstudiobbox_t* hitbox, matrix3x4_t& matrix, trace_t& trace)
{
	static auto fn = util::FindSignature(("client.dll"), ("55 8B EC 83 E4 F8 F3 0F 10 42"));

	trace.fraction = 1.0f;
	trace.startsolid = false;

	return reinterpret_cast <int(__fastcall*)(const Ray_t&, mstudiobbox_t*, matrix3x4_t&, trace_t&)> (fn)(ray, hitbox, matrix, trace);
}

bool aim::hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end, float* safe)
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

		math::vector_transform(start, matrix[studio_hitbox->bone], min);
		math::vector_rotate(end, matrix[studio_hitbox->bone], max);

		if (math::intersect_line_with_bb(min, max, studio_hitbox->bbmin, studio_hitbox->bbmax))
			return true;
	}

	return false;
}

static std::vector<std::tuple<float, float, float>> precomputed_seeds = {};

__forceinline void aim::build_seed_table()
{
	if (!precomputed_seeds.empty())
		return;

	for (auto i = 0; i < 255; i++)
	{
		math::random_seed(i + 1);

		const auto pi_seed = math::random_float(0.f, twopi);

		precomputed_seeds.emplace_back(math::random_float(0.f, 1.f),
			sin(pi_seed), cos(pi_seed));
	}
}

bool aim::calculate_hitchance(const Vector& aim_angle, int& final_hitchance)
{
	build_seed_table();

	const auto info = g_ctx.globals.weapon->get_csweapon_info();

	if (!info)
	{
		final_hitchance = 0;
		return true;
	}

	const auto hitchance_cfg = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount;

	if ((g_ctx.globals.eye_pos - final_target.data.point.point).Length() > info->flRange)
	{
		final_hitchance = 0;
		return true;
	}

	static auto nospread = m_cvar()->FindVar(crypt_str("weapon_accuracy_nospread"));

	if (nospread->GetBool())
	{
		final_hitchance = INT_MAX;
		return true;
	}

	if (precomputed_seeds.empty())
	{
		final_hitchance = 0;
		return false;
	}

	const auto round_acc = [](const float accuracy) { return roundf(accuracy * 1000.f) / 1000.f; };
	const auto sniper = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1
		|| g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08;
	const auto crouched = g_ctx.local()->m_fFlags() & FL_DUCKING;

	const auto weapon_inaccuracy = g_ctx.globals.weapon->get_inaccuracy();

	if (crouched)
	{
		if (round_acc(weapon_inaccuracy) == round_acc(sniper ? info->flInaccuracyCrouchAlt : info->flInaccuracyCrouch))
		{
			final_hitchance = INT_MAX;
			return true;
		}
	}
	else
	{
		if (round_acc(weapon_inaccuracy) == round_acc(sniper ? info->flInaccuracyStandAlt : info->flInaccuracyStand))
		{
			final_hitchance = INT_MAX;
			return true;
		}
	}

	static auto weapon_recoil_scale = m_cvar()->FindVar("weapon_recoil_scale");

	auto forward = ZERO;
	auto right = ZERO;
	auto up = ZERO;

	math::angle_vectors(aim_angle, &forward, &right, &up);

	math::fast_vec_normalize(forward);
	math::fast_vec_normalize(right);
	math::fast_vec_normalize(up);

	auto current = 0;

	Vector total_spread, spread_angle, end;
	float inaccuracy, spread_x, spread_y;
	std::tuple<float, float, float>* seed;

	for (auto i = 0u; i < 256; i++)
	{
		seed = &precomputed_seeds[i];

		inaccuracy = std::get<0>(*seed) * weapon_inaccuracy;
		spread_x = std::get<2>(*seed) * inaccuracy;
		spread_y = std::get<1>(*seed) * inaccuracy;
		total_spread = (forward + right * spread_x + up * spread_y);
		total_spread.Normalize();

		math::vector_angles(total_spread, spread_angle);

		math::angle_vectors(spread_angle, end);

		end = g_ctx.globals.eye_pos + (end * info->flRange);

		trace_t trace;

		Ray_t ray;

		ray.Init(g_ctx.globals.eye_pos, end);
		m_trace()->ClipRayToEntity(ray, MASK_SHOT | CONTENTS_GRATE, final_target.record->player, &trace);

		if (auto ent = trace.hit_entity; ent) {

			if (ent == final_target.record->player)
				current++;

		}

		if ((static_cast<float>(current) / 256.f) * 100.f >= hitchance_cfg)
		{
			final_hitchance = (static_cast<float>(current) / 256.f) * 100.f;
			return true;
		}

		if ((static_cast<float>(current + 256 - i) / 256.f) * 100.f < hitchance_cfg)
		{
			final_hitchance = (static_cast<float>(current + 256 - i) / 256.f) * 100.f;
			return false;
		}
	}

	final_hitchance = (static_cast<float>(current) / 256.f) * 100.f;
	return (static_cast<float>(current) / 256.f) * 100.f >= hitchance_cfg;
}

/*
int BacktrackTicks()
{
	// получаем тикрейт сервера
	float timepertick = m_globals()->m_tickcount;//g_pGlobals->interval_per_tick; //g_pGlobals m_globals()->m_tickcount;
	if (timepertick == 0) timepertick += 0.001; //предотвращает краши игры
	float tickrate = 1 / timepertick;

	// вычисления
	int backticks = 200 / 1000 * tickrate; // поключение слайдера 1-200мс где 200 можно слайдер))
	if (backticks < 1) return 1; //предотвращение крашей
	else return backticks;
}
*/


void aim::update_peek_state()
{
	g_ctx.globals.m_Peek.m_bIsPeeking = false;
	if (!g_cfg.ragebot.enable && (!g_cfg.antiaim.fakelag || !g_cfg.antiaim.triggers_fakelag_amount > 2))
		return;

	if (g_ctx.local()->m_vecVelocity().Length2D() > 5.0f)
		return;

	// predpos
	Vector predicted_eye_pos = g_ctx.globals.eye_pos + (engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick);

	for (auto i = 1; i <= m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));
		if (!e->valid(true))
			continue;

		auto records = &player_records[i];
		if (records->empty())
			continue;

		auto record = &records->front();
		if (!record->valid())
			continue;

		// apply player animated data
		record->adjust_player();

		// look all ticks for get first hitable
		for (int next_chock = 1; next_chock <= m_clientstate()->iChokedCommands; ++next_chock)
		{
			predicted_eye_pos *= next_chock;

			auto fire_data = autowall::get().wall_penetration(predicted_eye_pos, e->hitbox_position_matrix(HITBOX_HEAD, record->matrixes_data.first), e);
			if (!fire_data.valid || fire_data.damage < 1)
				continue;

			g_ctx.globals.m_Peek.m_bIsPeeking = true;
			m_debugoverlay()->AddBoxOverlay(predicted_eye_pos, Vector(-0.7f, -0.7f, -0.7f), Vector(0.7f, 0.7f, 0.7f), Vector(0.f, 0.f, 0.f), 0, 255, 0, 100, m_globals()->m_intervalpertick * 2);
		}
	}
}

void aim::PVSFix(ClientFrameStage_t stage)
{
	if (stage != FRAME_RENDER_START)
		return;

	for (int i = 1; i <= m_globals()->m_maxclients; i++)
	{
		auto local = g_ctx.local();
		auto local_idx = local->EntIndex();

		if (i == local_idx) continue;

		IClientEntity* pCurEntity = m_entitylist()->GetClientEntity(i);
		if (!pCurEntity) continue;

		*(int*)((uintptr_t)pCurEntity + 0xA30) = m_globals()->m_framecount; //we'll skip occlusion checks now
		*(int*)((uintptr_t)pCurEntity + 0xA28) = 0;//clear occlusion flags
	}
}


