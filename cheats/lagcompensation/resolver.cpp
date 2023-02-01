#include "animation_system.h"
#include "..\ragebot\aim.h"
#include "../visuals/player_esp.h"

/*RESOLVER BY LAITH*/
void CResolver::initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
	player = e;
	player_record = record;

	original_pitch = math::normalize_pitch(pitch);
	original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);


}

void CResolver::initialize_yaw(player_t* e, adjust_data* record)
{
	player = e;

	player_record = record;

	player_record->left = b_yaw(player, player->m_angEyeAngles().y, 1);
	player_record->right = b_yaw(player, player->m_angEyeAngles().y, 2);
	player_record->middle = b_yaw(player, player->m_angEyeAngles().y, 3);
}

void CResolver::reset()
{
	player = nullptr;
	player_record = nullptr;

	side = false;
	fake = false;

	was_first_bruteforce = false;
	was_second_bruteforce = false;

	original_goal_feet_yaw = 0.0f;
	original_pitch = 0.0f;
}


bool CResolver::IsAdjustingBalance()
{


	for (int i = 0; i < 15; i++)
	{
		const int activity = player->sequence_activity(player_record->layers[i].m_nSequence);
		if (activity == 979)
		{
			return true;
		}
	}
	return false;
}

bool CResolver::is_breaking_lby(AnimationLayer cur_layer, AnimationLayer prev_layer)
{
	if (IsAdjustingBalance())
	{
		if (IsAdjustingBalance())
		{
			if ((prev_layer.m_flCycle != cur_layer.m_flCycle) || cur_layer.m_flWeight == 1.f)
			{
				return true;
			}
			else if (cur_layer.m_flWeight == 0.f && (prev_layer.m_flCycle > 0.92f && cur_layer.m_flCycle > 0.92f))
			{
				return true;
			}
		}
		return false;
	}

	return false;
}

static auto GetSmoothedVelocity = [](float min_delta, Vector a, Vector b) {
	Vector delta = a - b;
	float delta_length = delta.Length();

	if (delta_length <= min_delta)
	{
		Vector result;

		if (-min_delta <= delta_length)
			return a;
		else
		{
			float iradius = 1.0f / (delta_length + FLT_EPSILON);
			return b - ((delta * iradius) * min_delta);
		}
	}
	else
	{
		float iradius = 1.0f / (delta_length + FLT_EPSILON);
		return b + ((delta * iradius) * min_delta);
	}
};

inline float anglemod(float a)
{
	a = (360.f / 65536) * ((int)(a * (65536.f / 360.0f)) & 65535);
	return a;
}

float ApproachAngle(float target, float value, float speed)
{
	target = anglemod(target);
	value = anglemod(value);

	float delta = target - value;

	if (speed < 0)
		speed = -speed;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}



float CResolver::b_yaw(player_t* player, float angle, int n)
{

	auto animState = player->get_animation_state();

	Vector velocity = player->m_vecVelocity();
	float spd = velocity.LengthSqr();
	if (spd > std::powf(1.2f * 260.0f, 2.f)) {
		Vector velocity_normalized = velocity.Normalized();
		velocity = velocity_normalized * (1.2f * 260.0f);


	}

	float Resolveyaw = animState->m_flGoalFeetYaw;

	auto delta_time
		= fmaxf(m_globals()->m_curtime - animState->m_flLastClientSideAnimationUpdateTime, 0.f);

	float deltatime = fabs(delta_time);
	float stop_to_full_running_fraction = 0.f;
	bool is_standing = true;
	float v25 = std::clamp(player->m_flDuckAmount() + animState->m_fLandingDuckAdditiveSomething, 0.0f, 1.0f);
	float v26 = animState->m_fDuckAmount;
	float v27 = deltatime * 6.0f;
	float v28;

	// clamp
	if ((v25 - v26) <= v27) {
		if (-v27 <= (v25 - v26))
			v28 = v25;
		else
			v28 = v26 - v27;
	}
	else {
		v28 = v26 + v27;
	}

	float flDuckAmount = std::clamp(v28, 0.0f, 1.0f);

	Vector animationVelocity = velocity;
	float speed = std::fminf(animationVelocity.Length(), 260.0f);

	auto weapon = player->m_hActiveWeapon().Get();


	auto wpndata = weapon->get_csweapon_info();


	float flMaxMovementSpeed = 260.0f;
	if (weapon) {
		flMaxMovementSpeed = std::fmaxf(wpndata->flMaxPlayerSpeed, 0.001f);
	}

	float flRunningSpeed = speed / (flMaxMovementSpeed * 0.520f);
	float flDuckingSpeed_2 = speed / (flMaxMovementSpeed * 0.340f);

	flRunningSpeed = std::clamp(flRunningSpeed, 0.0f, 1.0f);

	float flYawModifier = (((stop_to_full_running_fraction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;
	if (flDuckAmount > 0.0f) {
		float flDuckingSpeed = std::clamp(flDuckingSpeed_2, 0.0f, 1.0f);
		flYawModifier += (flDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier);
	}

	float flMaxBodyYaw = 58.f * flYawModifier;
	float flMinBodyYaw = -58.f * flYawModifier;


	//float flMaxBodyYaw = (*(float*)(uintptr_t(animState) + 0x338) * flYawModifier);
		//float flMinBodyYaw = (*(float*)(uintptr_t(animState) + 0x334) * flYawModifier);

	float flEyeYaw = player->m_angEyeAngles().y;

	float flEyeDiff = std::remainderf(flEyeYaw - Resolveyaw, 360.f);

	if (flEyeDiff <= flMaxBodyYaw) {
		if (flMinBodyYaw > flEyeDiff)
			Resolveyaw = fabs(flMinBodyYaw) + flEyeYaw;
	}
	else {
		Resolveyaw = flEyeYaw - fabs(flMaxBodyYaw);
	}

	if (speed > 0.1f || fabs(velocity.z) > 100.0f) {
		Resolveyaw = ApproachAngle(
			flEyeYaw,
			Resolveyaw,
			((stop_to_full_running_fraction * 20.0f) + 30.0f)
			* deltatime);
	}
	else {
		Resolveyaw = ApproachAngle(
			player->m_flLowerBodyYawTarget(),
			Resolveyaw,
			deltatime * 100.0f);
	}

	if (stop_to_full_running_fraction > 0.0 && stop_to_full_running_fraction < 1.0)
	{
		const auto interval = m_globals()->m_intervalpertick * 2.f;

		if (is_standing)
			stop_to_full_running_fraction = stop_to_full_running_fraction - interval;
		else
			stop_to_full_running_fraction = interval + stop_to_full_running_fraction;

		stop_to_full_running_fraction = std::clamp(stop_to_full_running_fraction, 0.f, 1.f);
	}

	if (speed > 135.2f && is_standing)
	{
		stop_to_full_running_fraction = fmaxf(stop_to_full_running_fraction, .0099999998f);
		is_standing = false;
	}

	if (speed < 135.2f && !is_standing)
	{
		stop_to_full_running_fraction = fminf(stop_to_full_running_fraction, .99000001f);
		is_standing = true;
	}

	//float Left = flEyeYaw + flMinBodyYaw;
	//float Right = flEyeYaw + flMaxBodyYaw;
	float gfy = Resolveyaw;
	//brute_yaw = std::remainderf(brute_yaw, 360.f);

	if (n == 1)
		return flMinBodyYaw;
	else if (n == 2)
		return flMaxBodyYaw;
	else if (n == 3)
		return flEyeYaw;

	if (n == 4)
	{
		return speed;
	}// get player speed

}


bool CResolver::is_slow_walking()
{
	auto entity = player;
	//float large = 0;
	float velocity_2D[64], old_velocity_2D[64];
	if (entity->m_vecVelocity().Length2D() != velocity_2D[entity->EntIndex()] && entity->m_vecVelocity().Length2D() != NULL) {
		old_velocity_2D[entity->EntIndex()] = velocity_2D[entity->EntIndex()];
		velocity_2D[entity->EntIndex()] = entity->m_vecVelocity().Length2D();
	}
	//if (large == 0)return false;
	Vector velocity = entity->m_vecVelocity();
	Vector direction = entity->m_angEyeAngles();

	float speed = velocity.Length();
	direction.y = entity->m_angEyeAngles().y - direction.y;
	//method 1
	if (velocity_2D[entity->EntIndex()] > 1) {
		int tick_counter[64];
		if (velocity_2D[entity->EntIndex()] == old_velocity_2D[entity->EntIndex()])
			tick_counter[entity->EntIndex()] += 1;
		else
			tick_counter[entity->EntIndex()] = 0;

		while (tick_counter[entity->EntIndex()] > (1 / m_globals()->m_intervalpertick) * fabsf(0.1f))//should give use 100ms in ticks if their speed stays the same for that long they are definetely up to something..
			return true;
	}


	return false;
}
int last_ticks[65];
int CResolver::GetChokedPackets() {
	auto ticks = TIME_TO_TICKS(player->m_flSimulationTime() - player->m_flOldSimulationTime());
	if (ticks == 0 && last_ticks[player->EntIndex()] > 0) {
		return last_ticks[player->EntIndex()] - 1;
	}
	else {
		last_ticks[player->EntIndex()] = ticks;
		return ticks;
	}
}
void CResolver::layer_test()
{
	player_record->type = LAYERS;

	float center = (abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[0][6].m_flPlaybackRate)) * 1000.f;
	float positive_full = (abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[1][6].m_flPlaybackRate)) * 1000.f;
	float negative_full = (abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[2][6].m_flPlaybackRate)) * 1000.f;
	float positive_40 = (abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[3][6].m_flPlaybackRate)) * 1000.f;
	float negative_40 = (abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[4][6].m_flPlaybackRate)) * 1000.f;

	if ((positive_full > center && negative_full <= center) || (positive_40 > center && negative_40 <= center))
		player_record->curSide = LEFT;


	else if ((negative_full > center && positive_full <= center) || (negative_40 > center && positive_40 <= center))
		player_record->curSide = RIGHT;

	else
		get_side_trace();


}

float angle_diff_onetap(float a1, float a2)
{
	float i;

	for (; a1 > 180.0f; a1 = a1 - 180.0f)
		;
	for (; 180.0f > a1; a1 = a1 + 360.0f)
		;
	for (; a2 > 180.0f; a2 = a2 - 360.0f)
		;
	for (; 180.0f > a2; a2 = a2 + 360.0f)
		;
	for (i = a2 - a1; i > 180.0f; i = i - 360.0f)
		;
	for (; 180.0f > i; i = i + 360.0f)
		;
	return i;
}
void CResolver::get_side_standing()
{
	player_record->type = LBY;
	float EyeDelta = math::normalize_yaw(player->m_angEyeAngles().y - original_goal_feet_yaw);

	if (fabs(EyeDelta) > 25.0f)
	{

		if (EyeDelta > 25.0f)
			player_record->curSide = RIGHT;

		else if (EyeDelta < -25.0f)
			player_record->curSide = LEFT;
	}
	else
		get_side_trace();
}


float get_backward_side(player_t* player)
{
	return math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y;
}


void CResolver::detect_side()
{
	player_record->type = ENGINE;
	/* externs */
	Vector src3D, dst3D, forward, right, up, src, dst;
	float back_two, right_two, left_two;
	CGameTrace tr;
	CTraceFilter filter;

	/* angle vectors */
	math::angle_vectors(Vector(0, get_backward_side(player), 0), &forward, &right, &up);

	/* filtering */
	filter.pSkip = player;
	src3D = player->get_shoot_position();
	dst3D = src3D + (forward * 384);

	/* back engine tracers */
	m_trace()->TraceRay(Ray_t(src3D, dst3D), MASK_SHOT, &filter, &tr);
	back_two = (tr.endpos - tr.startpos).Length();

	/* right engine tracers */
	m_trace()->TraceRay(Ray_t(src3D + right * 35, dst3D + right * 35), MASK_SHOT, &filter, &tr);
	right_two = (tr.endpos - tr.startpos).Length();

	/* left engine tracers */
	m_trace()->TraceRay(Ray_t(src3D - right * 35, dst3D - right * 35), MASK_SHOT, &filter, &tr);
	left_two = (tr.endpos - tr.startpos).Length();

	/* fix side */
	if (left_two > right_two) {
		player_record->curSide = LEFT;
	}
	else if (right_two > left_two) {
		player_record->curSide = RIGHT;
	}
	else
		get_side_trace();
}

void CResolver::get_side_trace()
{
	auto m_side = false;
	auto trace = false;
	if (m_globals()->m_curtime - lock_side > 2.0f)
	{
		auto first_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first), player, g_ctx.local());
		auto second_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second), player, g_ctx.local());

		if (first_visible != second_visible)
		{
			trace = true;
			m_side = second_visible;
			lock_side = m_globals()->m_curtime;
		}
		else
		{
			auto first_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first));
			auto second_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second));

			if (fabs(first_position - second_position) > 1.0f)
				m_side = first_position > second_position;
		}
	}
	else
		trace = true;

	if (m_side)
	{
		player_record->type = trace ? TRACE : DIRECTIONAL;
		player_record->curSide = RIGHT;
	}
	else
	{
		player_record->type = trace ? TRACE : DIRECTIONAL;
		player_record->curSide = LEFT;
	}
}



bool CResolver::DesyncDetect()
{
	if (!player->is_alive())
		return false;
	if (player->get_max_desync_delta() < 10)
		return false;
	if (player->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
		return false;
	if (player->get_move_type() == MOVETYPE_NOCLIP || player->get_move_type() == MOVETYPE_LADDER)
		return false;

	return true;
}

bool CResolver::update_walk_data()
{
	auto e = player;


	auto anim_layers = player_record->layers;
	bool s_1 = false,
		s_2 = false,
		s_3 = false;

	for (int i = 0; i < e->animlayer_count(); i++)
	{
		anim_layers[i] = e->get_animlayers()[i];
		if (anim_layers[i].m_nSequence == 26 && anim_layers[i].m_flWeight < 0.47f)
			s_1 = true;
		if (anim_layers[i].m_nSequence == 7 && anim_layers[i].m_flWeight > 0.001f)
			s_2 = true;
		if (anim_layers[i].m_nSequence == 2 && anim_layers[i].m_flWeight == 0)
			s_3 = true;
	}
	bool  m_fakewalking;
	if (s_1 && s_2)
		if (s_3)
			m_fakewalking = true;
		else
			m_fakewalking = false;
	else
		m_fakewalking = false;

	return m_fakewalking;
}

void CResolver::setmode()
{
	auto e = player;

	//float speed = e->m_vecVelocity().Length2D();
	float speed = b_yaw(player, player->m_angEyeAngles().y, 4);

	auto cur_layer = player_record->layers;
	auto prev_layer = player_record->previous_layers;

	bool on_ground = e->m_fFlags() & FL_ONGROUND && !e->get_animation_state()->m_bInHitGroundAnimation;

	bool slow_walking1 = is_slow_walking();
	bool slow_walking2 = update_walk_data();

	bool flicked_lby = abs(player_record->layers[3].m_flWeight - player_record->previous_layers[7].m_flWeight) >= 1.1f;
	bool breaking_lby = is_breaking_lby(cur_layer[3], prev_layer[3]);


	bool ducking = player->get_animation_state()->m_fDuckAmount && e->m_fFlags() & FL_ONGROUND && !player->get_animation_state()->m_bInHitGroundAnimation;



	bool stand_anim = false;
	if (player_record->layers[3].m_flWeight == 0.f && player_record->layers[3].m_flCycle == 0.f)
		stand_anim = true;

	bool move_anim = false;
	//if (int(player_record->layers[6].m_flWeight * 1000.f) == int(previous_layers[6].m_flWeight * 1000.f))
	if ((player_record->layers[6].m_flWeight * 1000.f) == (previous_layers[6].m_flWeight * 1000.f))
		move_anim = true;

	auto animstate = player->get_animation_state();
	if (!animstate)
		return;

	auto valid_move = true;
	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.f)
		valid_move = animstate->m_flTimeSinceStartedMoving < 0.22f;


	if (!on_ground)
	{
		player_record->curMode = AIR;
	}
	else if ((/*micromovement check pog*/ (speed < 3.1f && ducking) || (speed < 1.2f && !ducking)))
	{
		player_record->curMode = STANDING;

	}
	else if (/*micromovement check pog*/ ((speed >= 3.1f && ducking) || (speed >= 1.2f && !ducking)))
	{
		if ((speed >= 1.2f && speed < 134.f) && !ducking && (slow_walking1 || slow_walking2))
			player_record->curMode = SLOW_WALKING;
		else
			player_record->curMode = MOVING;
	}
	else
		player_record->curMode = FREESTANDING;
}

bool CResolver::MatchShot()
{
	// do not attempt to do this in nospread mode.

	float shoot_time = -1.f;

	auto weapon = player->m_hActiveWeapon();
	if (weapon) {
		// with logging this time was always one tick behind.
		// so add one tick to the last shoot time.
		shoot_time = weapon->m_fLastShotTime() + m_globals()->m_intervalpertick;
	}

	// this record has a shot on it.
	if (TIME_TO_TICKS(shoot_time) == TIME_TO_TICKS(player->m_flSimulationTime()))
	{
		return true;
	}

	return false;
}

void CResolver::final_detection()
{
	switch (player_record->curMode)
	{
	case MOVING:
		layer_test();
		break;
	case STANDING:
		get_side_standing();
		break;
	case FREESTANDING:
		get_side_trace();
		break;
	case SLOW_WALKING:
		layer_test();
		break;

	}
}

void CResolver::missed_shots_correction(adjust_data* record, int missed_shots)
{

	switch (missed_shots)
	{
	case 0:
		restype[record->type].missed_shots_corrected[record->player->EntIndex()] = 0;
		break;
	case 1:
		restype[record->type].missed_shots_corrected[record->player->EntIndex()] = 1;
		break;
	case 2:
		restype[record->type].missed_shots_corrected[record->player->EntIndex()] = 2;
		break;
	case 3:
		restype[record->type].missed_shots_corrected[record->player->EntIndex()] = 3;
		break;
	}
}


void CResolver::resolve_desync()
{
	if (!DesyncDetect())
	{
		player_record->side = RESOLVER_ORIGINAL;
		player_record->desync_amount = 0;
		player_record->curMode = NO_MODE;
		player_record->curSide = NO_SIDE;
		return;
	}
	auto e = player;
	//
	auto negative = player_record->left;
	auto positive = player_record->right;
	auto gfy = player_record->middle;
	//

	bool mside;
	auto pWeapon = player->m_hActiveWeapon();
	auto simtime = player->m_flSimulationTime();
	auto oldsimtime = player->m_flOldSimulationTime();
	float m_flLastShotTime;
	bool m_shot;
	m_flLastShotTime = pWeapon ? pWeapon->m_fLastShotTime() : 0.f;
	m_shot = m_flLastShotTime > oldsimtime && m_flLastShotTime <= simtime;

	setmode();

	if (m_flLastShotTime <= simtime && m_shot || MatchShot())
	{
		player_record->side = RESOLVER_ON_SHOT;
		player_record->desync_amount = 0;
		player_record->curSide = NO_SIDE;
		player_record->shot = true;
		return;
	}

	if (player_record->curMode == AIR)
	{
		player_record->side = RESOLVER_ORIGINAL;
		player_record->desync_amount = 0;
		player_record->curMode = AIR;
		player_record->curSide = NO_SIDE;
		return;
	}

	final_detection();


	float LowDeltaFactor = 0.45f; //testing values :3

	missed_shots_correction(player_record, g_ctx.globals.missed_shots[player->EntIndex()]); // we brute each type (layers,lby...etc) seperatly :3

	// start bruting if we miss baby :3
	switch (restype[player_record->type].missed_shots_corrected[player_record->player->EntIndex()])
	{
	case 0:// skip :3 we resolve bellow with logic
		break;
	case 1:// low delta
		g_cfg.player_list.types[player_record->type].should_flip[e->EntIndex()] = false;
		g_cfg.player_list.types[player_record->type].low_delta[e->EntIndex()] = true;
		break;
	case 2:// flipped full delta
		g_cfg.player_list.types[player_record->type].should_flip[e->EntIndex()] = true;
		g_cfg.player_list.types[player_record->type].low_delta[e->EntIndex()] = false;
		break;
	case 3:// flipped low delta
		g_cfg.player_list.types[player_record->type].should_flip[e->EntIndex()] = true;
		g_cfg.player_list.types[player_record->type].low_delta[e->EntIndex()] = true;
		break;
	}

	if (g_ctx.globals.missed_shots[player->EntIndex()] > 3)
		g_ctx.globals.missed_shots[player->EntIndex()] = 0;

	if (restype[player_record->type].missed_shots_corrected[player_record->player->EntIndex()] > 3)
		restype[player_record->type].missed_shots_corrected[player_record->player->EntIndex()] = 0;

	if (player_record->curSide == LEFT)
	{
		player_record->desync_amount = negative;

		if (g_cfg.player_list.types[player_record->type].should_flip[e->EntIndex()])
			player_record->desync_amount = positive;

		if (g_cfg.player_list.types[player_record->type].low_delta[e->EntIndex()])
			player_record->desync_amount *= LowDeltaFactor;
	}

	else if (player_record->curSide == RIGHT)
	{
		player_record->desync_amount = positive;

		if (g_cfg.player_list.types[player_record->type].should_flip[e->EntIndex()])
			player_record->desync_amount = negative;

		if (g_cfg.player_list.types[player_record->type].low_delta[e->EntIndex()])
			player_record->desync_amount *= LowDeltaFactor;
	}

	// for logs xD
	player_record->flipped_s = g_cfg.player_list.types[player_record->type].should_flip[e->EntIndex()];
	player_record->low_delta_s = g_cfg.player_list.types[player_record->type].low_delta[e->EntIndex()];
	g_ctx.globals.mlog1[e->EntIndex()] = player_record->desync_amount;

	// set player's gfy to guessed desync value :3
	player->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + player_record->desync_amount);

}


