#include "animation_system.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"

std::deque <adjust_data> player_records[65];

void lagcompensation::init()
{
	static auto threadedBoneSetup = m_cvar()->FindVar("cl_threaded_bone_setup");
	threadedBoneSetup->SetValue(1);

	static auto extrapolate = m_cvar()->FindVar("cl_extrapolate");
	extrapolate->SetValue(0);
}

float lagcompensation::GetAngle(player_t* player) {
	return math::normalize_yaw(player->m_angEyeAngles().y);
}

void lagcompensation::apply_interpolation_flags(player_t* e)
{
	auto map = e->var_mapping();
	if (map == nullptr)
		return;
	for (auto j = 0; j < map->m_nInterpolatedEntries; j++)
		map->m_Entries[j].m_bNeedsToInterpolate = false;
}

void lagcompensation::fsn(ClientFrameStage_t stage)
{
	if (!g_cfg.ragebot.enable)
		return;

	for (int i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (e == g_ctx.local())
			continue;

		if (!valid(i, e))
			continue;

		if (!e->is_alive())
			g_ctx.globals.missed_shots[e->EntIndex()] = 0;

		auto time_delta = abs(TIME_TO_TICKS(e->m_flSimulationTime()) - m_globals()->m_tickcount);

		if (time_delta > 1.0f / m_globals()->m_intervalpertick)
			continue;

		auto update = player_records[i].empty() || e->m_flSimulationTime() != e->m_flOldSimulationTime();
		if (update && !player_records[i].empty())
		{
			auto server_tick = m_clientstate()->m_iServerTick - i % m_globals()->m_timestamprandomizewindow;
			auto current_tick = server_tick - server_tick % m_globals()->m_timestampnetworkingbase;

			if (TIME_TO_TICKS(e->m_flOldSimulationTime()) < current_tick && TIME_TO_TICKS(e->m_flSimulationTime()) == current_tick)
			{
				auto layer = &e->get_animlayers()[11];
				auto previous_layer = &player_records[i].front().layers[11];

				if (layer->m_flCycle == previous_layer->m_flCycle) //-V550
				{
					e->m_flSimulationTime() = e->m_flOldSimulationTime();
					update = false;
				}
			}
		}

		switch (stage)
		{
		case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
			apply_interpolation_flags(e);
			break;
		case FRAME_NET_UPDATE_END:
			if (update)
			{
				if (!player_records[i].empty() && (e->m_vecOrigin() - player_records[i].front().origin).LengthSqr() > 4096.0f) {
					for (auto& record : player_records[i])
						record.invalid = true;
				}

				player_records[i].emplace_front(adjust_data(e));
				update_player_animations(e);

				while (player_records[i].size() > 32)
					player_records[i].pop_back();
			}
			break;
		case FRAME_RENDER_START:
			e->set_abs_origin(e->m_vecOrigin());
			FixPvs();
			break;
		}

	}
}

void lagcompensation::extrapolation(player_t* player, Vector& origin, Vector& velocity, int& flags, bool on_ground, int ticks)
{
	static const auto sv_gravity = m_cvar()->FindVar(("sv_gravity"));
	static const auto sv_jump_impulse = m_cvar()->FindVar(("sv_jump_impulse"));

	if (!(flags & FL_ONGROUND))
		velocity.z -= TICKS_TO_TIME(sv_gravity->GetFloat());
	else if (player->m_fFlags() & FL_ONGROUND && !on_ground)
		velocity.z = sv_jump_impulse->GetFloat();

	const auto src = origin;
	auto end = src + velocity * m_globals()->m_intervalpertick;

	Ray_t r;
	r.Init(src, end, player->GetCollideable()->OBBMins(), player->GetCollideable()->OBBMaxs());

	CGameTrace t;
	CTraceFilter filter;
	filter.pSkip = player;

	m_trace()->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

	if (t.fraction != 1.f)
	{
		for (auto i = 0; i < ticks; i++)
		{
			velocity -= t.plane.normal * velocity.Dot(t.plane.normal);

			const auto dot = velocity.Dot(t.plane.normal);
			if (dot < 0.f)
				velocity -= Vector(dot * t.plane.normal.x,
					dot * t.plane.normal.y, dot * t.plane.normal.z);

			end = t.endpos + velocity * TICKS_TO_TIME(1.f - t.fraction);

			r.Init(t.endpos, end, player->GetCollideable()->OBBMins(), player->GetCollideable()->OBBMaxs());
			m_trace()->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

			if (t.fraction == 1.f)
				break;
		}
	}

	origin = end = t.endpos;
	end.z -= ticks;

	r.Init(origin, end, player->GetCollideable()->OBBMins(), player->GetCollideable()->OBBMaxs());
	m_trace()->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

	flags &= ~FL_ONGROUND;

	if (t.DidHit() && t.plane.normal.z > .7f)
		flags |= FL_ONGROUND;
}

void lagcompensation::extrapolate(player_t* player, Vector& origin, Vector& velocity, int& flags, bool wasonground)
{
	static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));
	static auto sv_jump_impulse = m_cvar()->FindVar(crypt_str("sv_jump_impulse"));

	if (!(flags & FL_ONGROUND))
		velocity.z -= (m_globals()->m_frametime * sv_gravity->GetFloat());
	else if (wasonground)
		velocity.z = sv_jump_impulse->GetFloat();

	const Vector mins = player->GetCollideable()->OBBMins();
	const Vector max = player->GetCollideable()->OBBMaxs();

	const Vector src = origin;
	Vector end = src + (velocity * m_globals()->m_frametime);

	Ray_t ray;
	ray.Init(src, end, mins, max);

	trace_t trace;
	CTraceFilter filter;
	filter.pSkip = (void*)(player);

	m_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

	if (trace.fraction != 1.f)
	{
		for (int i = 0; i < 2; i++)
		{
			velocity -= trace.plane.normal * velocity.Dot(trace.plane.normal);

			const float dot = velocity.Dot(trace.plane.normal);
			if (dot < 0.f)
			{
				velocity.x -= dot * trace.plane.normal.x;
				velocity.y -= dot * trace.plane.normal.y;
				velocity.z -= dot * trace.plane.normal.z;
			}

			end = trace.endpos + (velocity * (m_globals()->m_intervalpertick * (1.f - trace.fraction)));

			ray.Init(trace.endpos, end, mins, max);
			m_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

			if (trace.fraction == 1.f)
				break;
		}
	}

	origin = trace.endpos;
	end = trace.endpos;
	end.z -= 2.f;

	ray.Init(origin, end, mins, max);
	m_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

	flags &= ~(1 << 0);

	if (trace.DidHit() && trace.plane.normal.z > 0.7f)
		flags |= (1 << 0);
}

bool lagcompensation::valid(int i, player_t* e)
{
	if (!g_cfg.ragebot.enable || !e->valid(false))
	{
		if (!e->is_alive())
		{
			is_dormant[i] = false;
			player_resolver[i].reset();

			g_ctx.globals.fired_shots[i] = 0;
			g_ctx.globals.missed_shots[i] = 0;
		}
		else if (e->IsDormant())
			is_dormant[i] = true;

		player_records[i].clear();
		return false;
	}

	return true;
}

static auto ticksToTime(int ticks) noexcept { return static_cast<float>(ticks * m_globals()->m_intervalpertick); }
void lagcompensation::update_player_animations(player_t* e)
{
	auto animstate = e->get_animation_state();//get_animation_state

	if (!animstate)
		return;

	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(e->EntIndex(), &player_info))
		return;

	auto records = &player_records[e->EntIndex()]; //-V826

	if (records->empty())
		return;

	adjust_data* previous_record = nullptr;

	if (records->size() >= 2)
		previous_record = &records->at(1);

	auto record = &records->front();

	AnimationLayer animlayers[13];
	float pose_parametrs[24]; //for storing pose parameters like(static legs and etc)


	memcpy(pose_parametrs, &e->m_flPoseParameter(), 24 * sizeof(float));
	memcpy(animlayers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	auto backup_lower_body_yaw_target = e->m_flLowerBodyYawTarget();
	auto backup_duck_amount = e->m_flDuckAmount();
	auto backup_flags = e->m_fFlags();
	auto backup_eflags = e->m_iEFlags();

	auto backup_curtime = m_globals()->m_curtime; //-V807
	auto backup_frametime = m_globals()->m_frametime;
	auto backup_realtime = m_globals()->m_realtime;
	auto backup_framecount = m_globals()->m_framecount;
	auto backup_tickcount = m_globals()->m_tickcount;
	auto backup_interpolation_amount = m_globals()->m_interpolation_amount;

	m_globals()->m_curtime = e->m_flSimulationTime();
	m_globals()->m_frametime = m_globals()->m_intervalpertick;

	if (previous_record)
	{
		auto velocity = e->m_vecVelocity();
		auto was_in_air = e->m_fFlags() & FL_ONGROUND && previous_record->flags & FL_ONGROUND;

		auto time_difference = max(m_globals()->m_intervalpertick, e->m_flSimulationTime() - previous_record->simulation_time);
		auto origin_delta = e->m_vecOrigin() - previous_record->origin;

		auto animation_speed = 0.0f;

		if (!origin_delta.IsZero() && TIME_TO_TICKS(time_difference) > 0)
		{
			e->m_vecVelocity() = origin_delta * (1.0f / time_difference);

			if (e->m_fFlags() & FL_ONGROUND && animlayers[11].m_flWeight > 0.0f && animlayers[11].m_flWeight < 1.0f && animlayers[11].m_flCycle > previous_record->layers[11].m_flCycle)
			{
				auto weapon = e->m_hActiveWeapon().Get();

				if (weapon)
				{
					auto max_speed = 260.0f;
					auto weapon_info = e->m_hActiveWeapon().Get()->get_csweapon_info();

					if (weapon_info)
						max_speed = e->m_bIsScoped() ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

					auto modifier = 0.35f * (1.0f - animlayers[11].m_flWeight);

					if (modifier > 0.0f && modifier < 1.0f)
						animation_speed = max_speed * (modifier + 0.55f);
				}
			}

			if (animation_speed > 0.0f)
			{
				animation_speed /= e->m_vecVelocity().Length2D();

				e->m_vecVelocity().x *= animation_speed;
				e->m_vecVelocity().y *= animation_speed;
			}

			/*	if (record->flags & FL_ONGROUND)
			{
				if (record->layers[6].m_flWeight == 0.f)
					velocity = Vector(0.f, 0.f, 0.f);
				else
				{
					auto weight = record->layers[11].m_flWeight;

					auto average_speed = velocity.Length2D();

					auto speed_as_trans = 0.55f - (1.f - weight) * 0.35f;

					// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1205-L1206
					if (weight > 0.0f && weight < 1.0f || (average_speed < speed_as_trans * 0.34f))
					{
						velocity.x /= average_speed;
						velocity.y /= average_speed;

						velocity.x *= speed_as_trans;
						velocity.y *= speed_as_trans;
					}
				}
			}*/
		

			if (records->size() >= 3 && time_difference > m_globals()->m_intervalpertick)
			{
				auto previous_velocity = (previous_record->origin - records->at(2).origin) * (1.0f / time_difference);

				if (!previous_velocity.IsZero() && !was_in_air)
				{
					auto current_direction = math::normalize_yaw(RAD2DEG(atan2(e->m_vecVelocity().y, e->m_vecVelocity().x)));
					auto previous_direction = math::normalize_yaw(RAD2DEG(atan2(previous_velocity.y, previous_velocity.x)));

					auto average_direction = current_direction - previous_direction;
					average_direction = DEG2RAD(math::normalize_yaw(current_direction + average_direction * 0.5f));

					auto direction_cos = cos(average_direction);
					auto dirrection_sin = sin(average_direction);

					auto velocity_speed = e->m_vecVelocity().Length2D();

					e->m_vecVelocity().x = direction_cos * velocity_speed;
					e->m_vecVelocity().y = dirrection_sin * velocity_speed;
				}
			}

			if (!(e->m_fFlags() & FL_ONGROUND))
			{
				static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));

				auto fixed_timing = math::clamp(time_difference, m_globals()->m_intervalpertick, 1.0f);
				e->m_vecVelocity().z -= sv_gravity->GetFloat() * fixed_timing * 0.5f;
			}
			else
				e->m_vecVelocity().z = 0.0f;
		}
	}

	const auto simtime_delta = e->m_flSimulationTime() - e->m_flOldSimulationTime();
	const auto choked_ticks = ((simtime_delta / m_globals()->m_intervalpertick) + 0.5);
	const auto simulation_tick_delta = choked_ticks - 2;
	const auto delta_ticks = (std::clamp(TIME_TO_TICKS(m_engine()->GetNetChannelInfo()->GetLatency(1) + m_engine()->GetNetChannelInfo()->GetLatency(0)) + m_globals()->m_tickcount - TIME_TO_TICKS(e->m_flSimulationTime() + util::get_interpolation()), 0, 100)) - simulation_tick_delta;

	if (delta_ticks > 0 && records->size() >= 2)
	{
		auto ticks_left = static_cast<int>(simulation_tick_delta);

		ticks_left = std::clamp(ticks_left, 1, 10);

		do
		{
			auto data_origin = record->origin;
			auto data_velocity = record->velocity;
			auto data_flags = record->flags;

			extrapolate(e, data_origin, data_velocity, data_flags, !(e->m_fFlags() & FL_ONGROUND));

			record->simulation_time += m_globals()->m_intervalpertick;
			record->origin = data_origin;
			record->velocity = data_velocity;
			--ticks_left;
		} while (ticks_left > 0);
	}

	e->m_iEFlags() &= ~0x1000;

	if (e->m_fFlags() & FL_ONGROUND && e->m_vecVelocity().Length() > 0.0f && animlayers[6].m_flWeight <= 0.0f)
		e->m_vecVelocity().Zero();

	e->m_vecAbsVelocity() = e->m_vecVelocity();
	e->invalidate_physics_recursive(4);
	e->m_bClientSideAnimation() = true;

	if (is_dormant[e->EntIndex()])
	{
		is_dormant[e->EntIndex()] = false;

		if (e->m_fFlags() & FL_ONGROUND)
		{
			animstate->m_bOnGround = true;
			animstate->m_bInHitGroundAnimation = false;
		}

		animstate->time_since_in_air() = 0.0f;
		animstate->m_flGoalFeetYaw = math::normalize_yaw(GetAngle(e));
	}

	auto updated_animations = false;

	c_baseplayeranimationstate state;
	memcpy(&state, animstate, sizeof(c_baseplayeranimationstate));

	if (previous_record)
	{
		memcpy(&e->m_flPoseParameter(), pose_parametrs, 24 * sizeof(float));
		memcpy(e->get_animlayers(), previous_record->layers, e->animlayer_count() * sizeof(AnimationLayer));

		auto ticks_chocked = 1;
		auto simulation_ticks = TIME_TO_TICKS(e->m_flSimulationTime() - previous_record->simulation_time);

		if (simulation_ticks > 0 && simulation_ticks < 17)
			ticks_chocked = simulation_ticks;

		if (ticks_chocked > 1)
		{
			if (animstate->m_iLastClientSideAnimationUpdateFramecount == m_globals()->m_framecount)
				animstate->m_iLastClientSideAnimationUpdateFramecount -= 1;

			if (animstate->m_flLastClientSideAnimationUpdateTime == m_globals()->m_curtime)
				animstate->m_flLastClientSideAnimationUpdateTime += ticksToTime(1);



			if (animlayers[4].m_flCycle < 0.5f && (!(e->m_fFlags() & FL_ONGROUND) || !(previous_record->flags & FL_ONGROUND)))
			{
				land_time = e->m_flSimulationTime() - animlayers[4].m_flPlaybackRate * animlayers[4].m_flCycle;
				land_in_cycle = land_time >= previous_record->simulation_time;
			}

			auto duck_amount_per_tick = (e->m_flDuckAmount() - previous_record->duck_amount) / ticks_chocked;

			for (auto i = 0; i < ticks_chocked; ++i)
			{
				auto simulated_time = previous_record->simulation_time + TICKS_TO_TIME(i);

				if (duck_amount_per_tick != 0.f) //-V550
					e->m_flDuckAmount() = previous_record->duck_amount + duck_amount_per_tick * (float)i;

				on_ground = e->m_fFlags() & FL_ONGROUND;

				if (land_in_cycle && !is_landed)
				{
					if (land_time <= simulated_time)
					{
						is_landed = true;
						on_ground = true;
					}
					else
						on_ground = previous_record->flags & FL_ONGROUND;
				}

				//land fix creds lw4
				auto v490 = e->sequence_activity(record->layers[5].m_nSequence);

				if (record->layers[5].m_nSequence == previous_record->layers[5].m_nSequence && (previous_record->layers[5].m_flWeight != 0.0f || record->layers[5].m_flWeight == 0.0f)
					|| !(v490 == ACT_CSGO_LAND_LIGHT || v490 == ACT_CSGO_LAND_HEAVY)) {
					if ((record->flags & 1) != 0 && (previous_record->flags & FL_ONGROUND) == 0)
						e->m_fFlags() &= ~FL_ONGROUND;
				}
				else
					e->m_fFlags() |= FL_ONGROUND;

				auto simulated_ticks = TIME_TO_TICKS(simulated_time);

				m_globals()->m_realtime = simulated_time;
				m_globals()->m_curtime = simulated_time;
				m_globals()->m_framecount = simulated_ticks;
				m_globals()->m_tickcount = simulated_ticks;
				m_globals()->m_interpolation_amount = 0.0f;

				g_ctx.globals.updating_animation = true;
				e->update_clientside_animation();
				g_ctx.globals.updating_animation = false;

				m_globals()->m_realtime = backup_realtime;
				m_globals()->m_curtime = backup_curtime;
				m_globals()->m_framecount = backup_framecount;
				m_globals()->m_tickcount = backup_tickcount;
				m_globals()->m_interpolation_amount = backup_interpolation_amount;

				updated_animations = true;
			}
		}
	}

	if (!updated_animations)
	{
		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;
	}

	memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

	auto setup_matrix = [&](player_t* e, AnimationLayer* layers, const int& matrix) -> void
	{
		e->invalidate_physics_recursive(8);

		AnimationLayer backup_layers[13];
		memcpy(backup_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

		memcpy(e->get_animlayers(), layers, e->animlayer_count() * sizeof(AnimationLayer));

		switch (matrix)
		{
		case MAIN:
			e->setup_bones_rebuilt(record->matrixes_data.main, BONE_USED_BY_ANYTHING);
			break;
		case NONE:
			e->setup_bones_rebuilt(record->matrixes_data.zero, BONE_USED_BY_HITBOX);
			break;
		case FIRST:
			e->setup_bones_rebuilt(record->matrixes_data.first, BONE_USED_BY_HITBOX);
			break;
		case SECOND:
			e->setup_bones_rebuilt(record->matrixes_data.second, BONE_USED_BY_HITBOX);
			break;
		case LOW_FIRST:
			e->setup_bones_rebuilt(record->matrixes_data.low_first, BONE_USED_BY_HITBOX);
			break;
		case LOW_SECOND:
			e->setup_bones_rebuilt(record->matrixes_data.low_second, BONE_USED_BY_HITBOX);
			break;
		case LOW_FIRST_20:
			e->setup_bones_rebuilt(record->matrixes_data.low_first_20, BONE_USED_BY_HITBOX);
			break;
		case LOW_SECOND_20:
			e->setup_bones_rebuilt(record->matrixes_data.low_second_20, BONE_USED_BY_HITBOX);
			break;
		}

		memcpy(e->get_animlayers(), backup_layers, e->animlayer_count() * sizeof(AnimationLayer));
	};


	if (/*!record->bot &&*/ g_ctx.local()->is_alive() && e->m_iTeamNum() != g_ctx.local()->m_iTeamNum() && !g_cfg.legitbot.enabled)
	{
		player_resolver[e->EntIndex()].initialize_yaw(e, record);

		/*rebuild setup velocity for more accurate rotations for the resolver and safepoints*/
		auto eye = e->m_angEyeAngles().y;
		auto idx = e->EntIndex();
		float negative_full = record->left;
		float positive_full = record->right;
		float gfy = record->middle;

		float negative_40 = negative_full * 0.45f;
		float positive_40 = positive_full * 0.45f;


		if (g_cfg.player_list.set_cs_low)
		{
			//gonna add resolver override later 
		}

		// --- main --- \\

		animstate->m_flGoalFeetYaw = previous_goal_feet_yaw[e->EntIndex()];
        g_ctx.globals.updating_animation = true;
        e->update_clientside_animation();
        g_ctx.globals.updating_animation = false;
		previous_goal_feet_yaw[e->EntIndex()] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		// ------ \\



		// --- none --- \\

		animstate->m_flGoalFeetYaw = math::normalize_yaw(gfy);
        g_ctx.globals.updating_animation = true;
        e->update_clientside_animation();
        g_ctx.globals.updating_animation = false;
		setup_matrix(e, animlayers, NONE);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[0] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(player_resolver[e->EntIndex()].resolver_layers[0], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.zero, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		// --- \\



		// --- 60 delta --- \\

		animstate->m_flGoalFeetYaw = math::normalize_yaw(gfy + positive_full);
        g_ctx.globals.updating_animation = true;
        e->update_clientside_animation();
        g_ctx.globals.updating_animation = false;
		setup_matrix(e, animlayers, FIRST);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[1] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(player_resolver[e->EntIndex()].resolver_layers[1], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.first, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		// --- \\




		// --- -60 delta --- \\

		animstate->m_flGoalFeetYaw = math::normalize_yaw(gfy + negative_full);
        g_ctx.globals.updating_animation = true;
        e->update_clientside_animation();
        g_ctx.globals.updating_animation = false;
		setup_matrix(e, animlayers, SECOND);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[2] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(player_resolver[e->EntIndex()].resolver_layers[2], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.second, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		// --- \\


	
		// --- 40 --- \\

		animstate->m_flGoalFeetYaw = math::normalize_yaw(gfy + positive_40);;
        g_ctx.globals.updating_animation = true;
        e->update_clientside_animation();
        g_ctx.globals.updating_animation = false;
		setup_matrix(e, animlayers, LOW_FIRST);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[3] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(player_resolver[e->EntIndex()].resolver_layers[3], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.low_first, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		// --- \\




		// --- -40 delta --- \\

		animstate->m_flGoalFeetYaw = math::normalize_yaw(gfy + negative_40);
        g_ctx.globals.updating_animation = true;
        e->update_clientside_animation();
        g_ctx.globals.updating_animation = false;
		setup_matrix(e, animlayers, LOW_SECOND);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[4] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(player_resolver[e->EntIndex()].resolver_layers[4], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.low_second, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		// --- \\

		player_resolver[e->EntIndex()].initialize(e, record, previous_goal_feet_yaw[e->EntIndex()], e->m_angEyeAngles().x);
		player_resolver[e->EntIndex()].resolve_desync();

		//e->m_angEyeAngles().x = player_resolver[e->EntIndex()].resolve_pitch();
	}

    g_ctx.globals.updating_animation = true;
    e->update_clientside_animation();
    g_ctx.globals.updating_animation = false;

    setup_matrix(e, animlayers, MAIN);
    memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.main, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

    m_globals()->m_curtime = backup_curtime;
    m_globals()->m_frametime = backup_frametime;

    e->m_flLowerBodyYawTarget() = backup_lower_body_yaw_target;
    e->m_flDuckAmount() = backup_duck_amount;
    e->m_fFlags() = backup_flags;
    e->m_iEFlags() = backup_eflags;
    //e->set_abs_origin(backup_absorigin);
    //e->set_abs_angles(backup_absangles);

    memcpy(e->get_animlayers(), animlayers, e->animlayer_count() * sizeof(AnimationLayer));
    memcpy(player_resolver[e->EntIndex()].previous_layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

    record->store_data(e, false);

    if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
        record->invalid = true;
}

void lagcompensation::FixPvs()
{
	for (int i = 1; i <= m_globals()->m_maxclients; i++) {
		auto pCurEntity = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (pCurEntity == g_ctx.local())
			continue;

		if (!pCurEntity
			|| !pCurEntity->is_player()
			|| pCurEntity->EntIndex() == m_engine()->GetLocalPlayer())
			continue;

		*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pCurEntity) + 0xA30) = m_globals()->m_framecount;
		*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pCurEntity) + 0xA28) = 0;
	}
}