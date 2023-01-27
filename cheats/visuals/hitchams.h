#pragma once
#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

struct hit_matrix_story {
	int ent_index;
	ModelRenderInfo_t info;
	DrawModelState_t state;
	matrix3x4_t pBoneToWorld[128] = {};
	float time;
	matrix3x4_t model_to_world;
};

class hit_chams : public singleton<hit_chams>
{
public:
	void add_matrix(player_t* player, matrix3x4_t* bones);
	void draw_hit_matrix();

	std::vector< hit_matrix_story > m_Hitmatrix;
};
