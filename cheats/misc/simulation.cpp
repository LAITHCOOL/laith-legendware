#include "simulation.h"

static CTraceFilterWorldOnly world_filter;

void VectorMAA(const Vector& start, float scale, const Vector& direction, Vector& dest) {
	dest.x = start.x + scale * direction.x;
	dest.y = start.y + scale * direction.y;
	dest.z = start.z + scale * direction.z;
}

void SimulationContext::TracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, CGameTrace& pm) {
	Ray_t ray;
	ray.Init(start, end, pMins, pMaxs);
	m_trace()->TraceRay(ray, fMask, filter, &pm);
}

void SimulationContext::InitSimulationContext(player_t* player) {
	static auto sv_maxspeed = m_cvar()->FindVar("sv_maxspeed");
	static auto sv_stepsize = m_cvar()->FindVar("sv_stepsize");

	//	 wihspeedClamped *= CS_PLAYER_SPEED_DUCK_MODIFIER;
	this->walking = false;
	this->m_vecOrigin = player->m_vecOrigin();
	this->m_vecVelocity = player->m_vecVelocity();
	this->simulationTicks = player->m_nTickBase();
	this->trace = CGameTrace{};
	this->gravity = m_cvar()->FindVar("sv_gravity")->GetFloat() * m_globals()->m_intervalpertick * 0.5f;
	this->sv_jump_impulse = m_cvar()->FindVar("sv_jump_impulse")->GetFloat();
	this->stepsize = sv_stepsize->GetFloat();
	this->pMins = player->GetCollideable()->OBBMins();
	this->pMaxs = player->GetCollideable()->OBBMaxs();
	this->player = player;
	this->filter = &world_filter;
	this->flags = player->m_fFlags() & 1 | player->m_fFlags() & 0xF8 | 2 * (((unsigned int)player->m_fFlags() >> 1) & 1);

	//static auto get_max_speed_idx = *( int* ) ( Memory::Scan( XorStr( "client_panorama.dll" ), XorStr( "8B 80 ?? ?? ?? ?? FF D0 8B 47 08" ) ) + 2 ) / 4;

#if 0
	auto maxSpeed = 250.0f;
	auto weapon = (C_WeaponCSBaseGun*)player->m_hActiveWeapon().Get();
	if (weapon) {
		auto weaponData = weapon->GetCSWeaponData();
		maxSpeed = weapon->m_weaponMode() == 0 ? weaponData->m_flMaxSpeed : weaponData->m_flMaxSpeed2;

		maxSpeed = ImLerp(maxSpeed * 0.34f, maxSpeed, 1.0f - player->m_flDuckAmount());
	}
#else
   //get_max_speed_idx
	float maxSpeed = call_virtual< float(__thiscall*)(void*) >(player, 274)(player);
#endif

	this->flMaxSpeed = std::fminf(maxSpeed, sv_maxspeed->GetFloat());
}

void SimulationContext::ExtrapolatePlayer(float yaw) {
	CUserCmd cmd;

	cmd.m_viewangles.y = yaw;

	if (*(int*)(uintptr_t(this->player) + 0x3970 /*move_state*/))
		cmd.m_forwardmove = 450.0f;

	if (this->flags & FL_DUCKING) {
		cmd.m_buttons |= IN_DUCK;
	}

	if (*(bool*)(uintptr_t(this->player) + 0x3915 /*m_bIsWaling*/)) {
		cmd.m_buttons |= IN_SPEED;
	}

	if ((this->flags & 5) == 5)
		cmd.m_buttons |= IN_JUMP;

	RebuildGameMovement(&cmd);
}

void SimulationContext::TryPlayerMove() {
#if 0
	auto ctx = this;
	auto numplanes = 0;
	auto v49 = 0;
	auto v47 = Source::m_pGlobalVars->interval_per_tick;
	Vector original_velocity = ctx->m_vecVelocity;
	Vector primal_velocity = ctx->m_vecVelocity;

	auto ClipVelocity = [](Vector& in, Vector& normal, Vector& out, float overbounce) {
		auto  angle = normal[2];

		// Determine how far along plane to slide based on incoming direction.
		auto backoff = DotProduct(in, normal) * overbounce;

		for (int i = 0; i < 3; i++) {
			auto change = normal[i] * backoff;
			out[i] = in[i] - change;
		}

		// iterate once to make sure we aren't still moving through the plane
		float adjust = DotProduct(out, normal);
		if (adjust < 0.0f) {
			out -= (normal * adjust);
		}
	};

	Vector planes[5];

	do {
		if (ctx->m_vecVelocity.LengthSquared() == 0.0f)
			break;

		TracePlayerBBox(this->m_vecOrigin, this->m_vecOrigin + this->m_vecVelocity * v47, 0x201400Bu, ctx->trace);
		if (ctx->trace.allsolid) {
			ctx->m_vecVelocity.Set();
			return;
		}

		if (ctx->trace.fraction > 0.0f) {
			ctx->m_vecOrigin = ctx->trace.endpos;
			if (ctx->trace.fraction == 1.0f)
				return;
		}

		planes[numplanes] = ctx->trace.plane.normal;
		numplanes++;

		v47 -= ctx->trace.fraction * v47;

		auto i = 0;
		if (numplanes > 0) {
			do {
				ClipVelocity(original_velocity, planes[i], ctx->m_vecVelocity, 1);
				auto j = 0;
				do {
					// Are we now moving against this plane?
					if (j != i && ctx->m_vecVelocity.Dot(planes[j]) < 0.0f)
						break;
					++j;
				} while (j < numplanes);

				if (j == numplanes)  // DidnXorStr( 't have to clip, so we' )re ok
					break;

				i++;
			} while (i < numplanes);
		}

		if (i == numplanes) {
			if (numplanes != 2) {
				ctx->m_vecVelocity.Set();
				return;
			}

			auto dir = planes[0].Cross(planes[1]);
			dir.Normalize();

			auto d = dir.Dot(ctx->m_vecVelocity);
			ctx->m_vecVelocity = dir * d;
		}

		auto d = ctx->m_vecVelocity.Dot(primal_velocity);
		if (d <= 0.f) {
			ctx->m_vecVelocity.Set();
			return;
		}

		++v49;
	} while (v49 < 4);
#else
	auto ClipVelocity = [](Vector& in, Vector& normal, Vector& out, float overbounce) {
		auto  angle = normal[2];

		// Determine how far along plane to slide based on incoming direction.
		auto backoff = math::dot_product(in, normal) * overbounce;

		for (int i = 0; i < 3; i++) {
			auto change = normal[i] * backoff;
			out[i] = in[i] - change;
		}

		// iterate once to make sure we aren't still moving through the plane
		float adjust = math::dot_product(out, normal);
		if (adjust < 0.0f) {
			out -= (normal * adjust);
		}
	};

#define MAX_CLIP_PLANES 5 

	int			bumpcount;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	CGameTrace	pm;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;

	blocked = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	original_velocity = this->m_vecVelocity;  // Store original velocity
	primal_velocity = this->m_vecVelocity;

	allFraction = 0;
	time_left = m_globals()->m_intervalpertick;   // Total time for this movement operation.

	new_velocity.Init();

	for (bumpcount = 0; bumpcount < 4; bumpcount++) {
		if (this->m_vecVelocity.Length() == 0.0)
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.

		VectorMAA(this->m_vecOrigin, time_left, this->m_vecVelocity, end);

		TracePlayerBBox(this->m_vecOrigin, end, MASK_PLAYERSOLID, pm);
		if (pm.fraction > 0.0f && pm.fraction < 0.0001f) {
			pm.fraction = 0.0f;
		}

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.allsolid) {
			// entity is trapped in another solid
			//this->m_vecVelocity.Set();
			this->m_vecVelocity = Vector(0, 0, 0);
			return;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if (pm.fraction > 0) {
			if (pm.fraction == 1) {
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				CGameTrace stuck;
				TracePlayerBBox(pm.endpos, pm.endpos, MASK_PLAYERSOLID, stuck);
				if (stuck.startsolid || stuck.fraction != 1.0f) {
					//Msg( XorStr( "Player will become stuck!!!\n" ) );
					//this->m_vecVelocity.Set();
					this->m_vecVelocity = Vector(0, 0, 0);
					break;
				}
			}

			// actually covered some distance
			this->m_vecOrigin = pm.endpos;
			original_velocity = this->m_vecVelocity;
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.fraction == 1) {
			break;		// moved the entire distance
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES) {
			// this shouldn't really happen
			//  Stop our movement if so.
			//this->m_vecVelocity.Set();
			this->m_vecVelocity = Vector(0, 0, 0);
			//Con_DPrintf(XorStr( "Too many planes 4\n" ));

			break;
		}

		// Set up next clipping plane
		planes[numplanes] = pm.plane.normal;
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if (numplanes == 1 &&
			player->get_move_type() == MOVETYPE_WALK &&
			!(this->flags & FL_ONGROUND)) {
			for (i = 0; i < numplanes; i++) {
				if (planes[i][2] > 0.7f) {
					// floor or slope
					ClipVelocity(original_velocity, planes[i], new_velocity, 1.0f);
					original_velocity = new_velocity;
				}
				else {
					ClipVelocity(
						original_velocity,
						planes[i],
						new_velocity,
						1.0f);
				}
			}

			this->m_vecVelocity = new_velocity;
			original_velocity = new_velocity;
		}
		else {
			for (i = 0; i < numplanes; i++) {
				ClipVelocity(original_velocity, planes[i], this->m_vecVelocity, 1);

				for (j = 0; j < numplanes; j++) {
					if (j != i) {
						// Are we now moving against this plane?
						if (this->m_vecVelocity.Dot(planes[j]) < 0.f)
							break;	// not ok
					}
				}

				if (j == numplanes)  // DidnXorStr( 't have to clip, so we' )re ok
					break;
			}

			// Did we go all the way through plane set
			if (i == numplanes) {
				if (numplanes != 2) {
					//this->m_vecVelocity.Set();
					this->m_vecVelocity = Vector(0, 0, 0);
					break;
				}

				dir = planes[0].Cross(planes[1]);
				dir.Normalize();
				d = dir.Dot(this->m_vecVelocity);
				this->m_vecVelocity = dir * d;
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = this->m_vecVelocity.Dot(primal_velocity);
			if (d <= 0.f) {
				//Con_DPrintf(XorStr( "Back\n" ));
				//this->m_vecVelocity.Set();
				this->m_vecVelocity = Vector(0, 0, 0);
				break;
			}
		}
	}

	if (allFraction == 0.f) {
		//this->m_vecVelocity.Set();
		this->m_vecVelocity = Vector(0, 0, 0);
	}
#endif
}

void SimulationContext::RebuildGameMovement(CUserCmd* ucmd) {

#define CS_PLAYER_SPEED_WALK_MODIFIER 0.52f
#define CS_PLAYER_SPEED_DUCK_MODIFIER 0.34f

	static auto sv_enablebunnyhopping = m_cvar()->FindVar("sv_enablebunnyhopping");
	static auto sv_walkable_normal = m_cvar()->FindVar("sv_enablebunnyhopping");

	this->buttons = ucmd->m_buttons;
	if (!this->walking) {
		this->walking = 1;
		if (ucmd->m_buttons & IN_SPEED) {
			auto v7 = this->flMaxSpeed * 0.52f;
			if (v7 + 25.0f > this->m_vecVelocity.Length())
				this->flMaxSpeed = v7;
		}
	}

	// StartGravity
	this->m_vecVelocity.z -= this->gravity;

	// CheckJumpButton
	if (ucmd->m_buttons & IN_JUMP && this->flags & FL_ONGROUND) {
		this->flags &= ~FL_ONGROUND;

		if (!sv_enablebunnyhopping->GetBool()) {
			auto v9 = this->flMaxSpeed * 1.1f;
			if (v9 > 0.0f) {
				auto v10 = this->m_vecVelocity.Length();
				if (v10 > v9) {
					auto v11 = v9 / v10;
					this->m_vecVelocity *= v11;
				}
			}
		}

		auto v12 = this->sv_jump_impulse;
		if (!(this->flags & FL_DUCKING))
			v12 += this->m_vecVelocity.z;
		this->m_vecVelocity.z = v12;
		this->m_vecVelocity.z = v12 - this->gravity;
	}

	// CheckParameters
	float  spd = (ucmd->m_forwardmove * ucmd->m_forwardmove) +
		(ucmd->m_sidemove * ucmd->m_sidemove) +
		(ucmd->m_upmove * ucmd->m_upmove);

	auto move = Vector2D(ucmd->m_forwardmove, ucmd->m_sidemove);
	if ((spd != 0.0) && (spd > flMaxSpeed * flMaxSpeed)) {
		float fRatio = flMaxSpeed / sqrt(spd);
		move *= fRatio;
	}

	if (this->flags & FL_ONGROUND && (this->flags & FL_DUCKING)) {
		float frac = 0.33333333f;
		move *= frac;
	}

	if (this->flags & FL_ONGROUND) {
		this->flMaxSpeed *= player->m_flVelocityModifier();

		this->m_vecVelocity.z = 0.0f;
		// Friction
		{
			static auto sv_stopspeed = m_cvar()->FindVar("sv_stopspeed");
			static auto sv_friction = m_cvar()->FindVar("sv_friction");

			auto speed = this->m_vecVelocity.Length2D();
			if (speed <= 0.1f) {
				this->m_vecVelocity.x = 0.0f;
				this->m_vecVelocity.y = 0.0f;
				this->m_vecVelocity.z = 0.0f;
			}
			else {
				auto control = std::fmaxf(sv_stopspeed->GetFloat(), speed);
				auto friction = sv_friction->GetFloat();
				auto drop = (control * friction) * m_globals()->m_intervalpertick;

				auto newspeed = std::fmaxf(speed - drop, 0.0f);
				if (newspeed != speed) {
					newspeed = newspeed / speed;
					this->m_vecVelocity *= newspeed;
				}
			}
		}

		// WalkMove
		{

			auto bOnGround = this->flags & FL_ONGROUND;

			Vector right;
			Vector forward;// = ucmd->m_viewangles.ToVectors(&right);
			math::angle_vectors(ucmd->m_viewangles, right);
			forward = right;

			if (forward.z != 0.0f) {
				forward.z = 0.0f;
				forward.Normalize();
			}

			if (right.z != 0.0f) {
				right.z = 0.0f;
				right.Normalize();
			}

			auto wishdir = forward * move.x + right * move.y;
			auto wishspeed = wishdir.Normalize();

			if (wishspeed > this->flMaxSpeed)
				wishspeed = this->flMaxSpeed;

			// accelerate
			this->m_vecVelocity.z = 0.0f;
			{
				static auto sv_accelerate_use_weapon_speed = m_cvar()->FindVar("sv_accelerate_use_weapon_speed");

				// See if we are changing direction a bit
				float currentspeed = this->m_vecVelocity.Dot(wishdir);

				// Reduce wishspeed by the amount of veer.
				float addspeed = wishspeed - currentspeed;

				if (addspeed > 0.0f) {
#if 0 // gamesense
					auto IsRunning = true;
					auto IsDucked = this->buttons & IN_DUCK || this->flags & FL_DUCKING; // m_bDucked
					if (!(this->buttons & IN_SPEED) || IsDucked)
						IsRunning = 0;

					auto InZoom = false;
					auto v192 = std::fmaxf(wishspeed, 250.0f);
					auto v193 = v192;
					auto v44 = v193;
					C_WeaponCSBaseGun* weapon = nullptr;
					if (sv_accelerate_use_weapon_speed->GetBool()
						&& (weapon = (C_WeaponCSBaseGun*)this->player->m_hActiveWeapon().Get())
						&& weapon != nullptr) {
						auto weaponData = weapon->GetCSWeaponData();

						float maxSpeed = weaponData->m_flMaxSpeed;
						if (weapon->m_weaponMode() != 0)
							maxSpeed = weaponData->m_flMaxSpeed2;

						if (weapon->m_zoomLevel() > 0)
							InZoom = (maxSpeed * 0.52f) < 110.f;

						auto ratio = maxSpeed / 250.0f;
						if (ratio < 1.0f)
							v192 *= ratio;

						if ((!IsDucked && !IsRunning || InZoom) && ratio <= 1.0) {
							v193 *= ratio;
							v44 = v193;
						}
					}

					auto v45 = v192;
					if (IsDucked) {
						if (!InZoom) {
							v193 *= 0.52f;
							v44 = v193;
						}
						v45 = v192 * 0.52f;
					}

					auto v49 = g_Vars.sv_accelerate->GetFloat();
					if (IsRunning) {
						if (!InZoom)
							v193 = v44 * 0.52f;

						v193 = v45 * 0.52f;
						auto v46 = std::fmaxf(currentspeed, 0.0f);
						if (v46 > (v193 - 5.0f) && (v46 - v193) >= -5.0f) {
							auto v47 = std::fminf(((v193 - (v46 + 5.0f)) * 0.2f) + 1.0f, 1.0f);
							auto v48 = std::fmaxf(v47, 0.0f);
							v49 *= v48;
						}
						else {
							v49 = g_Vars.sv_accelerate->GetFloat();
						}
					}

					auto v50 = std::fminf(addspeed, (Source::m_pGlobalVars->interval_per_tick * v49) * v193);
					this->m_vecVelocity += wishdir * v50;
#else
					// soufiw rebuild

					bool ducked = true;

					if (!(this->buttons & IN_DUCK)) {
						if ( /*!player->m_bDucking( ) && */!(player->m_fFlags() & FL_DUCKING))
							ducked = false;
					}
					bool in_speed = true;
					if (!(buttons & IN_SPEED) || ducked)
						in_speed = false;

					float clampedSpeed = fmaxf(currentspeed, 0.0f);
					float wihspeedClamped = fmaxf(wishspeed, 250.0f);

					player_t* Player = player;

					bool InZoom = false;

					float FinalSpeed = 0.0f;
					bool v60 = false;
					auto weapon = player->m_hActiveWeapon().Get();
					if (sv_accelerate_use_weapon_speed->GetBool() && weapon) {
						auto weaponData = weapon->get_csweapon_info();
						float maxSpeed = (player->m_bIsScoped() == 0) ? weaponData->flMaxPlayerSpeed : weaponData->flMaxPlayerSpeedAlt;
						int unknown2 = *(int*)((uintptr_t)weaponData + 456); // some zoom shit btw

						if (weapon->m_zoomLevel() <= 0 || unknown2 <= 1) {
							InZoom = false;
						}
						else {
							InZoom = maxSpeed * CS_PLAYER_SPEED_WALK_MODIFIER < 110.0f;
						}

						float maxSpeedNorm = maxSpeed * 0.004f; // 0.004 = 1.0 / 250.0
						float maxSpeedClamped = fminf(maxSpeedNorm, 1.0f);
						if (!ducked && !in_speed || InZoom)
							wihspeedClamped *= maxSpeedClamped;

						FinalSpeed = maxSpeedClamped * wihspeedClamped;
					}
					else {
						FinalSpeed = wihspeedClamped;
					}

					if (ducked) {
						if (!InZoom)
							wihspeedClamped *= CS_PLAYER_SPEED_DUCK_MODIFIER;
						FinalSpeed *= CS_PLAYER_SPEED_DUCK_MODIFIER;
					}

					if (in_speed) {
						if (!player->m_bHasHeavyArmor() /*&& !player->m_hCarriedHostage( ).Get( ) */ && !InZoom)
							wihspeedClamped *= CS_PLAYER_SPEED_WALK_MODIFIER;
						FinalSpeed *= CS_PLAYER_SPEED_WALK_MODIFIER;
					}

					auto accel = m_cvar()->FindVar("sv_accelerate")->GetFloat();
					float v27 = ((m_globals()->m_intervalpertick * accel) * wihspeedClamped);
					if (v60 && clampedSpeed > (FinalSpeed - v27)) {
						float v28 = 1.0f - (fmaxf(clampedSpeed - (FinalSpeed - v27), 0.0f) / fmaxf(FinalSpeed - (FinalSpeed - v27), 0.0f));
						if (v28 >= 0.0f) {
							accel = fminf(v28, 1.0f) * accel;
						}
						else {
							accel = 0.0f * accel;
						}
					}

					float v30 = 0.0f;
					if (in_speed && clampedSpeed > (FinalSpeed - 5.0f)) {
						float v29 = fmaxf(clampedSpeed - (FinalSpeed - 5.0f), 0.0f) / fmaxf(FinalSpeed - (FinalSpeed - 5.0f), 0.0f);
						if ((1.0f - v29) >= 0.0f)
							v30 = fminf(1.0f - v29, 1.0f) * accel;
						else
							v30 = 0.0f * accel;
					}
					else {
						v30 = accel;
					}

					float v31 = fminf(((m_globals()->m_intervalpertick * v30) * wihspeedClamped), addspeed);
					addspeed = v31;

#if 0
					if (Player) {
						static auto healthshot_healthboost_speed_multiplier = g_CVar->FindVar(XorStr("healthshot_healthboost_speed_multiplier"));
						float healthboost = healthshot_healthboost_speed_multiplier->GetFloat();

						// m_flHealthShotBoostExpirationTime - curtime
						float deltaBoostTime = *(float*)((uintptr_t)Player + 0xA3A8) - g_GlobalVars->curtime;
						if (deltaBoostTime >= 0.0f && fminf(deltaBoostTime, 1.0f) > 0.0f) {
							float v22 = 0.0f;
							if (deltaBoostTime >= 0.0f)
								v22 = fminf(deltaBoostTime, 1.0f);
							v31 *= (((healthboost - 1.0f) * v22) + 1.0f);
						}
					}
#endif

					this->m_vecVelocity += wishdir * v31;
#endif
				}
			}
			this->m_vecVelocity.z = 0.0f;

			auto speed = m_vecVelocity.Length();
			if (speed > this->flMaxSpeed) {
				m_vecVelocity = m_vecVelocity.Normalized() * this->flMaxSpeed;
			}

			// m_vecVelocity += m_vecBaseVelocity;

			// first try just moving to the destination	
			Vector dest;
			dest[0] = this->m_vecOrigin[0] + this->m_vecVelocity[0] * m_globals()->m_intervalpertick;
			dest[1] = this->m_vecOrigin[1] + this->m_vecVelocity[1] * m_globals()->m_intervalpertick;
			dest[2] = this->m_vecOrigin[2];

			// first try moving directly to the next spot
			TracePlayerBBox(this->m_vecOrigin, dest, MASK_PLAYERSOLID, this->trace);

			if (this->trace.fraction == 1.0f) {
				// StayOnGround
				this->m_vecOrigin = this->trace.endpos;

				CGameTrace tr;
				Vector start(this->m_vecOrigin);
				Vector end(this->m_vecOrigin);
				start.z += 2.0f;
				end.z -= this->stepsize;

				// See how far up we can go without getting stuck
				TracePlayerBBox(this->m_vecOrigin, start, MASK_PLAYERSOLID, tr);
				start = tr.endpos;

				// using trace.startsolid is unreliable here, it doesn't get set when
				// tracing bounding box vs. terrain

				// Now trace down from a known safe position
				TracePlayerBBox(start, end, MASK_PLAYERSOLID, tr);
				if (tr.fraction > 0.0f && tr.fraction < 1.0f && !tr.startsolid && tr.plane.normal.z >= sv_walkable_normal->GetFloat()) {
					float flDelta = std::fabsf(this->m_vecOrigin.z - tr.endpos.z);
					if (flDelta > 0.015625f) {
						this->m_vecOrigin = tr.endpos;
					}
				}
			}
			else if (bOnGround) {
				// Try sliding forward both on ground and up 16 pixels
				//  take the move that goes farthest
				auto vecPos = this->m_vecOrigin;
				auto vecVel = this->m_vecVelocity;

				// Slide move down.
				TryPlayerMove();

				auto vecDownPos = this->m_vecOrigin;
				auto vecDownVel = this->m_vecVelocity;

				// Reset original values.
				this->m_vecOrigin = vecPos;
				this->m_vecVelocity = vecVel;

				// Move up a stair height.
				Vector vecEndPos;
				vecEndPos = this->m_vecOrigin;
				vecEndPos.z += this->stepsize + 0.03125f;

				CGameTrace trace;
				TracePlayerBBox(this->m_vecOrigin, vecEndPos, MASK_PLAYERSOLID, trace);
				if (!trace.startsolid && !trace.allsolid) {
					this->m_vecOrigin = trace.endpos;
				}

				// Slide move up.
				TryPlayerMove();

				vecEndPos = this->m_vecOrigin;
				vecEndPos.z -= this->stepsize + 0.03125f;

				TracePlayerBBox(this->m_vecOrigin, vecEndPos, MASK_PLAYERSOLID, trace);
				if (!trace.startsolid && !trace.allsolid) {
					this->m_vecOrigin = trace.endpos;
				}

				// If we are not on the ground any more then use the original movement attempt.
				if (trace.plane.normal[2] < sv_walkable_normal->GetFloat()) {
					this->m_vecOrigin = vecDownPos;
					this->m_vecVelocity = vecDownVel;
				}
				else {
					// If the trace ended up in empty space, copy the end over to the origin.
					if (!trace.startsolid && !trace.allsolid) {
						this->m_vecOrigin = trace.endpos;
					}

					Vector vecUpPos;
					vecUpPos = this->m_vecOrigin;

					// decide which one went farther
					float flDownDist = (vecDownPos.x - vecPos.x) * (vecDownPos.x - vecPos.x) + (vecDownPos.y - vecPos.y) * (vecDownPos.y - vecPos.y);
					float flUpDist = (vecUpPos.x - vecPos.x) * (vecUpPos.x - vecPos.x) + (vecUpPos.y - vecPos.y) * (vecUpPos.y - vecPos.y);
					if (flDownDist > flUpDist) {
						this->m_vecOrigin = vecDownPos;
						this->m_vecVelocity = vecDownVel;
					}
					else {
						// copy z value from slide move
						this->m_vecVelocity.z = vecDownVel.z;
					}
				}

				CGameTrace tr;
				Vector start(this->m_vecOrigin);
				Vector end(this->m_vecOrigin);
				start.z += 2.0f;
				end.z -= this->stepsize;

				// See how far up we can go without getting stuck
				TracePlayerBBox(this->m_vecOrigin, start, MASK_PLAYERSOLID, tr);
				start = tr.endpos;

				// using trace.startsolid is unreliable here, it doesn't get set when
				// tracing bounding box vs. terrain

				// Now trace down from a known safe position
				TracePlayerBBox(start, end, MASK_PLAYERSOLID, tr);
				if (tr.fraction > 0.0f && tr.fraction < 1.0f && !tr.startsolid && tr.plane.normal.z >= sv_walkable_normal->GetFloat()) {
					float flDelta = std::fabsf(this->m_vecOrigin.z - tr.endpos.z);
					if (flDelta > 0.015625f) {
						this->m_vecOrigin = tr.endpos;
					}
				}
			}
		}
	}
	else {
		// AirMove
		Vector right;
		Vector forward;// = ucmd->m_viewangles.ToVectors(&right);
		math::angle_vectors(ucmd->m_viewangles, right);
		forward = right;

		right.z = 0.0f;
		forward.z = 0.0f;
		forward.Normalize();
		right.Normalize();

		auto wishdir = forward * move.x + right * move.y;
		auto wishspeed = wishdir.Normalize();

		static auto sv_air_max_wishspeed = m_cvar()->FindVar("sv_air_max_wishspeed");
		if (wishspeed > sv_air_max_wishspeed->GetFloat())
			wishspeed = sv_air_max_wishspeed->GetFloat();

		auto currentspeed = this->m_vecVelocity.Dot(wishdir);
		auto addspeed = wishspeed - currentspeed;
		if (addspeed > 0.0) {
			auto v169 = m_cvar()->FindVar("sv_airaccelerate")->GetFloat() * wishspeed;
			auto v170 = std::fminf(addspeed, v169 * m_globals()->m_intervalpertick);
			this->m_vecVelocity += wishdir * v170;
		}

		TryPlayerMove();
	}

	CGameTrace trace;
	TracePlayerBBox(this->m_vecOrigin, this->m_vecOrigin - Vector(0.0f, 0.0f, 2.0f), MASK_PLAYERSOLID, trace);
	if (trace.DidHit() && trace.plane.normal.z >= sv_walkable_normal->GetFloat())
		this->flags |= 1u;
	else
		this->flags &= 0xFEu;

	if (this->flags & 1)
		this->m_vecVelocity.z = 0.0f;
	else
		this->m_vecVelocity.z -= this->gravity;

	++this->simulationTicks;
}
