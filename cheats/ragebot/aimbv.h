#pragma once
#include "..\..\includes.hpp"
#include "..\lagcompensation\animation_system.h"
#include "../../sdk/math/unique_vector.h"

enum class HitscanMode : int {
	NORMAL = 0,
	LETHAL = 1,
	LETHAL2 = 3,
	PREFER = 4
};

struct HitscanBox_t {
	int         m_index;
	HitscanMode m_mode;

	__forceinline bool operator==(const HitscanBox_t& c) const {
		return m_index == c.m_index && m_mode == c.m_mode;
	}
};

class AimPlayer {
public:

	using hitboxcan_t = stdpp::unique_vector< HitscanBox_t >;
	using records_t = std::deque< std::shared_ptr< lagcompensation > >;
public:
	//resolver
	records_t m_records;
	//
	float	  m_spawn;
	hitboxcan_t m_hitboxes;
	int       m_shots;
	int       m_missed_shots;

	float     m_body_update;
	bool      m_moved;

	int m_stand_index;
	int m_stand_index2;
	int m_body_index;


	// data about the LBY proxy.
	float m_body;
	float m_old_body;


public:
	void PreferHitbox();
public:
	void reset() {

		m_spawn = 0.f;
		m_shots = 0;
		m_missed_shots = 0;
		m_hitboxes.clear();
	}
};

class target
{
public:
	player_t* e;

	adjust_data* last_record;
	adjust_data* history_record;

	target()
	{
		e = nullptr;

		last_record = nullptr;
		history_record = nullptr;
	}

	target(player_t* e, adjust_data* last_record, adjust_data* history_record) //-V818
	{
		this->e = e;

		this->last_record = last_record;
		this->history_record = history_record;
	}
};

class scan_point
{
public:
	Vector point;
	int hitbox;
	bool center;
	float safe;

	scan_point()
	{
		point.Zero();
		hitbox = -1;
		center = false;
		safe = 0.0f;
	}

	scan_point(const Vector& point, const int& hitbox, const bool& center) //-V818 //-V730
	{
		this->point = point;
		this->hitbox = hitbox;
		this->center = center;
	}

	void reset()
	{
		point.Zero();
		hitbox = -1;
		center = false;
		safe = 0.0f;
	}
};

class scan_data
{
public:
	scan_point point;
	bool visible;
	int damage;
	int hitbox;

	scan_data()
	{
		reset();
	}

	void reset()
	{
		point.reset();
		visible = false;
		damage = -1;
		hitbox = -1;
	}

	bool valid()
	{
		return damage >= 1 && hitbox != -1;
	}
};

struct Last_target
{
	adjust_data record;
	scan_data data;
	float distance;
};

class scanned_target
{
public:
	adjust_data* record;
	scan_data data;

	float fov;
	float distance;
	int health;

	scanned_target()
	{
		reset();
	}

	scanned_target(const scanned_target& data) //-V688
	{
		this->record = data.record;
		this->data = data.data;
		this->fov = data.fov;
		this->distance = data.distance;
		this->health = data.health;
	}

	scanned_target& operator=(const scanned_target& data) //-V688
	{
		this->record = data.record;
		this->data = data.data;
		this->fov = data.fov;
		this->distance = data.distance;
		this->health = data.health;

		return *this;
	}

	scanned_target(adjust_data* record, const scan_data& data) //-V688 //-V818
	{
		this->record = record;
		this->data = data;

		Vector viewangles;
		m_engine()->GetViewAngles(viewangles);

		auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, data.point.point); //-V688
		auto fov = math::get_fov(viewangles, aim_angle); //-V688

		this->fov = fov;
		this->distance = g_ctx.globals.eye_pos.DistTo(data.point.point);
		this->health = record->player->m_iHealth();
	}

	void reset()
	{
		record = nullptr;
		data.reset();

		fov = 0.0f;
		distance = 0.0f;
		health = 0;
	}
};

class aim : public singleton <aim>
{
	void automatic_revolver(CUserCmd* cmd);
	void prepare_targets();
	adjust_data* get_record(std::deque <adjust_data>* records, bool history);
	int get_minimum_damage(bool visible, int health);
	void scan_targets();
	bool automatic_stop(CUserCmd* cmd);

	void find_best_target();
	void fire(CUserCmd* cmd);

	//int hitchance(const Vector& aim_angle, float hit_chance);

	//bool hitchance(const Vector& aim_angle, IClientEntity* ent, float chance);
	int hitchance(const Vector& aim_angle);
	//bool hitchance(const Vector&, adjust_data*, int&);

	std::vector <scanned_target> scanned_targets;
	scanned_target final_target;
public:
	bool force_stop;
	bool HitTraces(adjust_data* _animation, const Vector position, const float chance, int box);

	void update_peek_state();

	void PVSFix(ClientFrameStage_t stage);

	bool get_multipoints(int ihitbox, std::vector<Vector>& points, matrix3x4_t mx[], bool& only_center, float force_pointscale);

	void run(CUserCmd* cmd);
	float LagFix();
	void two_shot(float damage);
	void scan(adjust_data* record, scan_data& data, const Vector& shoot_position = g_ctx.globals.eye_pos, bool optimized = false);
	std::vector <int> get_hitboxes(adjust_data* record, bool optimized = false);
	//std::vector<scan_point> get_points(adjust_data* record, int hitbox, Vector start);
	std::vector <scan_point> get_points(adjust_data* record, int hitbox, bool from_aim = true);

	bool hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end, float* safe = nullptr);

	void build_seed_table();

	bool calculate_hitchance(const Vector& aim_angle, int& final_hitchance);

	std::vector <target> targets;
	std::vector <adjust_data> backup;

	int last_target_index = -1;
	Last_target last_target[65];

	Vector last_shoot_position;
	bool should_stop;
};