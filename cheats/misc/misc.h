#include "..\..\includes.hpp"

class misc : public singleton <misc> 
{
public:


	int pressed_keys = 0;
	enum state
	{
		hold,
		toggle,
		always
	};

	std::string get_state(key_bind_mode mode)
	{
		if (mode == hold)
			return "hold";
		else if (mode == toggle)
			return "toggle";
		else if (mode == always)
			return "always";
	}
	void add_key(const char* name, bool main, key_bind key, int spacing, float alpha, bool condition = true, bool damage = false)
	{
		float animka = 1.f;
		auto p = ImGui::GetWindowPos() + ImGui::GetCursorPos() * animka;

		if (!condition || animka < 0)
			return;


		if (!main || animka < 0)
			return;

		if (key.mode == hold)
			ImGui::GetWindowDrawList()->AddCircle(ImVec2(p.x + 5, p.y + ImGui::CalcTextSize(name).y / 2), 3, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255));
		else if (key.mode == toggle)
			ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 5, p.y + ImGui::CalcTextSize(name).y / 2), 3, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255));
		else if (key.mode == always)
			ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 5, p.y + ImGui::CalcTextSize(name).y / 2), 3, ImColor(130, 20, 0, 255));

		spacing -= ImGui::CalcTextSize(damage ? std::to_string(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage).c_str() : get_state(key.mode).c_str()).x;
		ImGui::SetCursorPos(ImVec2(key.mode == -1 ? 5 : 15, ImGui::GetCursorPosY() * animka));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1));
		ImGui::Text(name, false); ImGui::SameLine(spacing - 5);
		ImGui::Text(damage ? std::to_string(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage).c_str() : get_state(key.mode).c_str(), false);
		ImGui::PopStyleColor(1);

		pressed_keys + 1;

	}

	void spectators_list();
	void key_binds();
	void dt_fakelags();
	void watermark();

	void NoDuck(CUserCmd* cmd);
	void AutoCrouch(CUserCmd* cmd);
	void SlideWalk(CUserCmd* cmd);
	void fast_stop(CUserCmd* m_pcmd);

	void automatic_peek(CUserCmd* cmd, float wish_yaw);
	void ViewModel();
	void PovArrows(player_t* e, Color color);

	void FullBright();
	void DrawGray();
	void NightmodeFix();

	void zeus_range();
	void ChatSpammer();

	void desync_arrows();
	void aimbot_hitboxes();
	void ragdolls();

	void rank_reveal();

	void double_tap(CUserCmd* m_pcmd);
	void DoubleTap(CUserCmd* m_pcmd);
	bool CanDoubleTap(bool check_charge);
	void HideShots(CUserCmd* m_pcmd);
	void break_prediction(CUserCmd* cmd);
	void hide_shots(CUserCmd* m_pcmd, bool should_work);

	void ax();

	void anti_anti_defensive(CUserCmd* m_pcmd);

	bool teleport(CUserCmd* m_pcmd);

	void dt_test(CUserCmd* m_pcmd);

	void double_tap_deffensive(CUserCmd* m_pcmd);
	void lagexploit(CUserCmd* m_pcmd);
	void BreakLagComp(CUserCmd* m_pcmd);
	void anti_defensive(CUserCmd* m_pcmd);

	bool recharging_double_tap = false;

	bool jumpbugged = false;

	bool double_tap_enabled = false;
	bool double_tap_key = false;
	bool double_tap_checkc = false;

	bool bDidPeek = false;
	int lastdoubletaptime = 0;
	bool hide_shots_enabled = false;
	bool hide_shots_key = false;
	void EnableHiddenCVars();
	int getCorrectTickbase(CUserCmd* m_pCmd, int tickbase) ;
};