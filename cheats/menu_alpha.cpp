#include <ShlObj_core.h>
#include <unordered_map>
#include "menu_alpha.h"
#include "../ImGui/code_editor.h"
#include "../constchars.h"
#include "../cheats/misc/logs.h"

#include "..\cheats\misc\misc.h"

#include "../BASS/API.h"
#include "../BASS/bass.h"
#include <utils/imports.h>
#include "lagcompensation/animation_system.h"

#define ALPHA (ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar| ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)
#define NOALPHA (ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)

std::vector <std::string> files;
std::vector <std::string> scripts;
std::string editing_script;

auto selected_script = 0;
auto loaded_editing_script = false;

static int dpi_resize = 0.f;

static auto menu_setupped = false;
static auto should_update = true;

void fRageQuit()
{
	exit(0);
}

IDirect3DTexture9* all_skins[36];

std::string get_wep(int id, int custom_index = -1, bool knife = true)
{
	if (custom_index > -1)
	{
		if (knife)
		{
			switch (custom_index)
			{
			case 0: return crypt_str("weapon_knife");
			case 1: return crypt_str("weapon_bayonet");
			case 2: return crypt_str("weapon_knife_css");
			case 3: return crypt_str("weapon_knife_skeleton");
			case 4: return crypt_str("weapon_knife_outdoor");
			case 5: return crypt_str("weapon_knife_cord");
			case 6: return crypt_str("weapon_knife_canis");
			case 7: return crypt_str("weapon_knife_flip");
			case 8: return crypt_str("weapon_knife_gut");
			case 9: return crypt_str("weapon_knife_karambit");
			case 10: return crypt_str("weapon_knife_m9_bayonet");
			case 11: return crypt_str("weapon_knife_tactical");
			case 12: return crypt_str("weapon_knife_falchion");
			case 13: return crypt_str("weapon_knife_survival_bowie");
			case 14: return crypt_str("weapon_knife_butterfly");
			case 15: return crypt_str("weapon_knife_push");
			case 16: return crypt_str("weapon_knife_ursus");
			case 17: return crypt_str("weapon_knife_gypsy_jackknife");
			case 18: return crypt_str("weapon_knife_stiletto");
			case 19: return crypt_str("weapon_knife_widowmaker");
			}
		}
		else
		{
			switch (custom_index)
			{
			case 0: return crypt_str("ct_gloves"); //-V1037
			case 1: return crypt_str("studded_bloodhound_gloves");
			case 2: return crypt_str("t_gloves");
			case 3: return crypt_str("ct_gloves");
			case 4: return crypt_str("sporty_gloves");
			case 5: return crypt_str("slick_gloves");
			case 6: return crypt_str("leather_handwraps");
			case 7: return crypt_str("motorcycle_gloves");
			case 8: return crypt_str("specialist_gloves");
			case 9: return crypt_str("studded_hydra_gloves");
			}
		}
	}
	else
	{
		switch (id)
		{
		case 0: return crypt_str("knife");
		case 1: return crypt_str("gloves");
		case 2: return crypt_str("weapon_ak47");
		case 3: return crypt_str("weapon_aug");
		case 4: return crypt_str("weapon_awp");
		case 5: return crypt_str("weapon_cz75a");
		case 6: return crypt_str("weapon_deagle");
		case 7: return crypt_str("weapon_elite");
		case 8: return crypt_str("weapon_famas");
		case 9: return crypt_str("weapon_fiveseven");
		case 10: return crypt_str("weapon_g3sg1");
		case 11: return crypt_str("weapon_galilar");
		case 12: return crypt_str("weapon_glock");
		case 13: return crypt_str("weapon_m249");
		case 14: return crypt_str("weapon_m4a1_silencer");
		case 15: return crypt_str("weapon_m4a1");
		case 16: return crypt_str("weapon_mac10");
		case 17: return crypt_str("weapon_mag7");
		case 18: return crypt_str("weapon_mp5sd");
		case 19: return crypt_str("weapon_mp7");
		case 20: return crypt_str("weapon_mp9");
		case 21: return crypt_str("weapon_negev");
		case 22: return crypt_str("weapon_nova");
		case 23: return crypt_str("weapon_hkp2000");
		case 24: return crypt_str("weapon_p250");
		case 25: return crypt_str("weapon_p90");
		case 26: return crypt_str("weapon_bizon");
		case 27: return crypt_str("weapon_revolver");
		case 28: return crypt_str("weapon_sawedoff");
		case 29: return crypt_str("weapon_scar20");
		case 30: return crypt_str("weapon_ssg08");
		case 31: return crypt_str("weapon_sg556");
		case 32: return crypt_str("weapon_tec9");
		case 33: return crypt_str("weapon_ump45");
		case 34: return crypt_str("weapon_usp_silencer");
		case 35: return crypt_str("weapon_xm1014");
		default: return crypt_str("unknown");
		}
	}
}

IDirect3DTexture9* get_skin_preview(const char* weapon_name, const std::string& skin_name, IDirect3DDevice9* device)
{
	IDirect3DTexture9* skin_image = nullptr;
	std::string vpk_path;

	if (strcmp(weapon_name, crypt_str("unknown")) && strcmp(weapon_name, crypt_str("knife")) && strcmp(weapon_name, crypt_str("gloves"))) //-V526
	{
		if (skin_name.empty() || skin_name == crypt_str("default"))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/") + std::string(weapon_name) + crypt_str(".png");
		else
			vpk_path = crypt_str("resource/flash/econ/default_generated/") + std::string(weapon_name) + crypt_str("_") + std::string(skin_name) + crypt_str("_light_large.png");
	}
	else
	{
		if (!strcmp(weapon_name, crypt_str("knife")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/weapon_knife.png");
		else if (!strcmp(weapon_name, crypt_str("gloves")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/ct_gloves.png");
		else if (!strcmp(weapon_name, crypt_str("unknown")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/weapon_snowball.png");

	}
	const auto handle = m_basefilesys()->Open(vpk_path.c_str(), crypt_str("r"), crypt_str("GAME"));
	if (handle)
	{
		int file_len = m_basefilesys()->Size(handle);
		char* image = new char[file_len]; //-V121

		m_basefilesys()->Read(image, file_len, handle);
		m_basefilesys()->Close(handle);

		D3DXCreateTextureFromFileInMemory(device, image, file_len, &skin_image);
		delete[] image;
	}

	if (!skin_image)
	{
		std::string vpk_path;

		if (strstr(weapon_name, crypt_str("bloodhound")) != NULL || strstr(weapon_name, crypt_str("hydra")) != NULL)
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/ct_gloves.png");
		else
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/") + std::string(weapon_name) + crypt_str(".png");

		const auto handle = m_basefilesys()->Open(vpk_path.c_str(), crypt_str("r"), crypt_str("GAME"));

		if (handle)
		{
			int file_len = m_basefilesys()->Size(handle);
			char* image = new char[file_len]; //-V121

			m_basefilesys()->Read(image, file_len, handle);
			m_basefilesys()->Close(handle);

			D3DXCreateTextureFromFileInMemory(device, image, file_len, &skin_image);
			delete[] image;
		}
	}

	return skin_image;
}

// setup some styles and colors, window size and bg alpha
// dpi setup
void c_menu::menu_setup(ImGuiStyle& style) //-V688
{
	ImGui::StyleColorsClassic(); // colors setup
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once); // window pos setup
	ImGui::SetNextWindowBgAlpha(min(style.Alpha, 0.94f)); // window bg alpha setup

	styles.WindowPadding = style.WindowPadding;
	styles.WindowRounding = style.WindowRounding;
	styles.WindowMinSize = style.WindowMinSize;
	styles.ChildRounding = style.ChildRounding;
	styles.PopupRounding = style.PopupRounding;
	styles.FramePadding = style.FramePadding;
	styles.FrameRounding = style.FrameRounding;
	styles.ItemSpacing = style.ItemSpacing;
	styles.ItemInnerSpacing = style.ItemInnerSpacing;
	styles.TouchExtraPadding = style.TouchExtraPadding;
	styles.IndentSpacing = style.IndentSpacing;
	styles.ColumnsMinSpacing = style.ColumnsMinSpacing;
	styles.ScrollbarSize = style.ScrollbarSize;
	styles.ScrollbarRounding = style.ScrollbarRounding;
	styles.GrabMinSize = style.GrabMinSize;
	styles.GrabRounding = style.GrabRounding;
	styles.TabRounding = style.TabRounding;
	styles.TabMinWidthForUnselectedCloseButton = style.TabMinWidthForUnselectedCloseButton;
	styles.DisplayWindowPadding = style.DisplayWindowPadding;
	styles.DisplaySafeAreaPadding = style.DisplaySafeAreaPadding;
	styles.MouseCursorScale = style.MouseCursorScale;

	// setup skins preview
	for (auto i = 0; i < g_cfg.skins.skinChanger.size(); i++)
		if (!all_skins[i])
			all_skins[i] = get_skin_preview(get_wep(i, (i == 0 || i == 1) ? g_cfg.skins.skinChanger.at(i).definition_override_vector_index : -1, i == 0).c_str(), g_cfg.skins.skinChanger.at(i).skin_name, device); //-V810

	menu_setupped = true; // we dont want to setup menu again
}

// resize current style sizes
void c_menu::dpi_resize(float scale_factor, ImGuiStyle& style) //-V688
{
	style.WindowPadding = (styles.WindowPadding * scale_factor);
	style.WindowRounding = (styles.WindowRounding * scale_factor);
	style.WindowMinSize = (styles.WindowMinSize * scale_factor);
	style.ChildRounding = (styles.ChildRounding * scale_factor);
	style.PopupRounding = (styles.PopupRounding * scale_factor);
	style.FramePadding = (styles.FramePadding * scale_factor);
	style.FrameRounding = (styles.FrameRounding * scale_factor);
	style.ItemSpacing = (styles.ItemSpacing * scale_factor);
	style.ItemInnerSpacing = (styles.ItemInnerSpacing * scale_factor);
	style.TouchExtraPadding = (styles.TouchExtraPadding * scale_factor);
	style.IndentSpacing = (styles.IndentSpacing * scale_factor);
	style.ColumnsMinSpacing = (styles.ColumnsMinSpacing * scale_factor);
	style.ScrollbarSize = (styles.ScrollbarSize * scale_factor);
	style.ScrollbarRounding = (styles.ScrollbarRounding * scale_factor);
	style.GrabMinSize = (styles.GrabMinSize * scale_factor);
	style.GrabRounding = (styles.GrabRounding * scale_factor);
	style.TabRounding = (styles.TabRounding * scale_factor);
	if (styles.TabMinWidthForUnselectedCloseButton != FLT_MAX) //-V550
		style.TabMinWidthForUnselectedCloseButton = (styles.TabMinWidthForUnselectedCloseButton * scale_factor);
	style.DisplayWindowPadding = (styles.DisplayWindowPadding * scale_factor);
	style.DisplaySafeAreaPadding = (styles.DisplaySafeAreaPadding * scale_factor);
	style.MouseCursorScale = (styles.MouseCursorScale * scale_factor);
}

__forceinline void tab_start(const char* label, float x_pos, float y_pos, float x, float y)
{
	ImGui::SetCursorPos(ImVec2{ x_pos, y_pos });
	ImGui::BeginChild(label, ImVec2{ x, y }, false);
	ImGui::SetCursorPos(ImVec2{ 10, 10 });
	ImGui::BeginChild(label, ImVec2{ x - 20, y - 20 }, false);
}

__forceinline void tab_end()
{
	ImGui::EndChild();
	ImGui::EndChild();
}

__forceinline void subtab_start(const char* label, float size_x, float size_y)
{
	ImGui::SetCursorPos(ImVec2{ 50, 0 });
	ImGui::BeginChild(label, ImVec2{ size_x, 20 }, false);
}
__forceinline void subtab_end()
{
	ImGui::EndChild();
}

__forceinline void padding(float x, float y)
{
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x * c_menu::get().dpi_scale);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y * c_menu::get().dpi_scale);
}

/// <summary>
/// //////////////////////////////////////////
/// </summary>
/// <returns></returns>

std::string get_config_dir()
{
	std::string folder;
	static TCHAR path[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPath(NULL, 0x001a, NULL, NULL, path)))
		folder = std::string(path) + crypt_str("\\itzlaith_lw\\configs\\");

	CreateDirectory(folder.c_str(), NULL);
	return folder;
}

void load_config()
{
	if (cfg_manager->files.empty())
		return;

	cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), false);
	c_lua::get().unload_all_scripts();

	for (auto& script : g_cfg.scripts.scripts)
		c_lua::get().load_script(c_lua::get().get_script_id(script));

	scripts = c_lua::get().scripts;

	if (selected_script >= scripts.size())
		selected_script = scripts.size() - 1; //-V103

	for (auto& current : scripts)
	{
		if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			current.erase(current.size() - 5, 5);
		else if (current.size() >= 4)
			current.erase(current.size() - 4, 4);
	}

	for (auto i = 0; i < g_cfg.skins.skinChanger.size(); ++i)
		all_skins[i] = nullptr;

	g_cfg.scripts.scripts.clear();

	cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), true);
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Loaded ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void save_config()
{
	if (cfg_manager->files.empty())
		return;

	g_cfg.scripts.scripts.clear();

	for (auto i = 0; i < c_lua::get().scripts.size(); ++i)
	{
		auto script = c_lua::get().scripts.at(i);

		if (c_lua::get().loaded.at(i))
			g_cfg.scripts.scripts.emplace_back(script);
	}

	cfg_manager->save(cfg_manager->files.at(g_cfg.selected_config));
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Saved ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void remove_config()
{
	if (cfg_manager->files.empty())
		return;

	eventlogs::get().add(crypt_str("Removed ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);

	cfg_manager->remove(cfg_manager->files.at(g_cfg.selected_config));
	cfg_manager->config_files();

	files = cfg_manager->files;

	if (g_cfg.selected_config >= files.size())
		g_cfg.selected_config = files.size() - 1; //-V103

	for (auto& current : files)
		if (current.size() > 2)
			current.erase(current.size() - 3, 3);
}

void add_config()
{
	auto empty = true;

	for (auto current : g_cfg.new_config_name)
	{
		if (current != ' ')
		{
			empty = false;
			break;
		}
	}

	if (empty)
		g_cfg.new_config_name = crypt_str("config");

	eventlogs::get().add(crypt_str("Added ") + g_cfg.new_config_name + crypt_str(" config"),  false);

	if (g_cfg.new_config_name.find(crypt_str(".cfg")) == std::string::npos)
		g_cfg.new_config_name += crypt_str(".cfg");

	cfg_manager->save(g_cfg.new_config_name);
	cfg_manager->config_files();

	g_cfg.selected_config = cfg_manager->files.size() - 1; //-V103
	files = cfg_manager->files;

	for (auto& current : files)
		if (current.size() > 2)
			current.erase(current.size() - 3, 3);
}

// title of content child
void child_title(const char* label)
{
	ImGui::PushFont(c_menu::get().futura_large);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (12 * c_menu::get().dpi_scale));
	ImGui::Text(label);

	ImGui::PopStyleColor();
	ImGui::PopFont();
}

void draw_combo(const char* name, int& variable, const char* labels[], int count)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6 * c_menu::get().dpi_scale);
	ImGui::Text(name);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
	ImGui::Combo(std::string(crypt_str("##COMBO__") + std::string(name)).c_str(), &variable, labels, count);
}

void draw_combo(const char* name, int& variable, bool (*items_getter)(void*, int, const char**), void* data, int count)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6 * c_menu::get().dpi_scale);
	ImGui::Text(name);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
	ImGui::Combo(std::string(crypt_str("##COMBO__") + std::string(name)).c_str(), &variable, items_getter, data, count);
}

void draw_multicombo(std::string name, std::vector<int>& variable, const char* labels[], int count, std::string& preview)
{
	padding(-3, -6);
	ImGui::Text((crypt_str(" ") + name).c_str());
	padding(0, -5);

	auto hashname = crypt_str("##") + name; // we dont want to render name of combo

	for (auto i = 0, j = 0; i < count; i++)
	{
		if (variable[i])
		{
			if (j)
				preview += crypt_str(", ") + (std::string)labels[i];
			else
				preview = labels[i];

			j++;
		}
	}

	if (ImGui::BeginCombo(hashname.c_str(), preview.c_str())) // draw start
	{
		ImGui::Spacing();
		ImGui::BeginGroup();
		{

			for (auto i = 0; i < count; i++)
				ImGui::Selectable(labels[i], (bool*)&variable[i], ImGuiSelectableFlags_DontClosePopups);

		}
		ImGui::EndGroup();
		ImGui::Spacing();

		ImGui::EndCombo();
	}

	preview = crypt_str("None"); // reset preview to use later
}

bool LabelClick(const char* label, bool* v, const char* unique_id)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	// The concatoff/on thingies were for my weapon config system so if we're going to make that, we still need this aids.
	char Buf[64];
	_snprintf(Buf, 62, crypt_str("%s"), label);

	char getid[128];
	sprintf_s(getid, 128, crypt_str("%s%s"), label, unique_id);


	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(getid);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const ImRect check_bb(window->DC.CursorPos, ImVec2(label_size.y + style.FramePadding.y * 2 + window->DC.CursorPos.x, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2));
	ImGui::ItemSize(check_bb, style.FramePadding.y);

	ImRect total_bb = check_bb;

	if (label_size.x > 0)
	{
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		const ImRect text_bb(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), ImVec2(window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y));

		ImGui::ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
		total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
	}

	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
		*v = !(*v);

	if (*v)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(235 / 255.f, 72 / 255.f, 72 / 255.f, 255.f));
	if (label_size.x > 0.0f)
		ImGui::RenderText(ImVec2(check_bb.GetTL().x + 12, check_bb.GetTL().y), Buf);
	if (*v)
		ImGui::PopStyleColor();

	return pressed;

}

void draw_keybind(const char* label, key_bind* key_bind, const char* unique_id)
{
	// reset bind if we re pressing esc
	if (key_bind->key == KEY_ESCAPE)
		key_bind->key = KEY_NONE;

	auto clicked = false;
	auto text = (std::string)m_inputsys()->ButtonCodeToString(key_bind->key);

	if (key_bind->key <= KEY_NONE || key_bind->key >= KEY_MAX)
		text = crypt_str("None");

	// if we clicked on keybind
	if (hooks::input_shouldListen && hooks::input_receivedKeyval == &key_bind->key)
	{
		clicked = true;
		text = crypt_str("...");
	}

	auto textsize = ImGui::CalcTextSize(text.c_str()).x + 8 * c_menu::get().dpi_scale;
	auto labelsize = ImGui::CalcTextSize(label);

	ImGui::Text(label);
	ImGui::SameLine();

	ImGui::SetCursorPosX(ImGui::GetWindowSize().x - (ImGui::GetWindowSize().x - ImGui::CalcItemWidth()) - max(55 * c_menu::get().dpi_scale, textsize));
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3 * c_menu::get().dpi_scale);

	if (ImGui::KeybindButton(text.c_str(), unique_id, ImVec2(max(55 * c_menu::get().dpi_scale, textsize), 24 * c_menu::get().dpi_scale), clicked))
		clicked = true;


	if (clicked)
	{
		hooks::input_shouldListen = true;
		hooks::input_receivedKeyval = &key_bind->key;
	}

	static auto hold = false, toggle = false, alwayson = false;

	switch (key_bind->mode)
	{
	case ALWAYS:
		alwayson = true;
		toggle = false;
		hold = false;
		break;
	case HOLD:
		hold = true;
		toggle = false;
		alwayson = false;
		break;
	case TOGGLE:
		toggle = true;
		hold = false;
		alwayson = false;
		break;
	}

	//ImGui::SetNextWindowSize()
	if (ImGui::BeginPopup(unique_id))
	{
		if (LabelClick(crypt_str("Always"), &alwayson, unique_id))
		{
			if (alwayson) {
				hold = true;
				key_bind->mode = ALWAYS;
			}
			else if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else
			{
				alwayson = false;
				key_bind->mode = ALWAYS;
			}

			ImGui::CloseCurrentPopup();
		}
		if (LabelClick(crypt_str("Hold"), &hold, unique_id))
		{
			if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else if (alwayson) {
				alwayson = false;
				key_bind->mode = ALWAYS;
			}
			else
			{
				toggle = false;
				key_bind->mode = HOLD;
			}

			ImGui::CloseCurrentPopup();
		}
		if (LabelClick(crypt_str("Toggle"), &toggle, unique_id))
		{
			if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else if (alwayson) {
				alwayson = false;
				key_bind->mode = ALWAYS;
			}
			else
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void draw_semitabs(const char* labels[], int count, int& tab, ImGuiStyle& style)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (2 * c_menu::get().dpi_scale));

	// center of main child
	float offset = 343 * c_menu::get().dpi_scale;

	// text size padding + frame padding
	for (int i = 0; i < count; i++)
		offset -= (ImGui::CalcTextSize(labels[i]).x) / 2 + style.FramePadding.x * 2;

	// set new padding
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	ImGui::BeginGroup();

	for (int i = 0; i < count; i++)
	{
		// switch current tab
		if (ImGui::ContentTab(labels[i], tab == i))
			tab = i;

		// continue drawing on same line
		if (i + 1 != count)
		{
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.ItemSpacing.x);
		}
	}

	ImGui::EndGroup();
}

void lua_edit(std::string window_name)
{
	std::string file_path;

	auto get_dir = [&]() -> void
	{
		static TCHAR path[MAX_PATH];

		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
			file_path = std::string(path) + crypt_str("\\itzlaith_lw\\scripts\\");

		CreateDirectory(file_path.c_str(), NULL);
		file_path += window_name + crypt_str(".lua");
	};

	get_dir();
	const char* child_name = (window_name + window_name).c_str();

	ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_Once);
	ImGui::Begin(window_name.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 5.f);

	static TextEditor editor;

	if (!loaded_editing_script)
	{
		static auto lang = TextEditor::LanguageDefinition::Lua();

		editor.SetLanguageDefinition(lang);
		editor.SetReadOnly(false);

		std::ifstream t(file_path);

		if (t.good()) // does while exist?
		{
			std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			editor.SetText(str); // setup script content
		}

		loaded_editing_script = true;
	}

	// dpi scale for font
	// we dont need to resize it for full scale
	ImGui::SetWindowFontScale(1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f));

	// new size depending on dpi scale
	ImGui::SetWindowSize(ImVec2(ImFloor(800 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))), ImFloor(700 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f)))));
	editor.Render(child_name, ImGui::GetWindowSize() - ImVec2(0, 66 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))));

	// seperate code with buttons
	ImGui::Separator();

	// set cursor pos to right edge of window
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowSize().x - (16.f * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))) - (250.f * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))));
	ImGui::BeginGroup();

	if (ImGui::CustomButton(crypt_str("Save"), (crypt_str("Save") + window_name).c_str(), ImVec2(125 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f)), 0), true, c_menu::get().super_ico, crypt_str("S")))
	{
		std::ofstream out;

		out.open(file_path);
		out << editor.GetText() << std::endl;
		out.close();
	}

	ImGui::SameLine();

	// TOOD: close button will close window (return in start of function)
	if (ImGui::CustomButton(crypt_str("Close"), (crypt_str("Close") + window_name).c_str(), ImVec2(125 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f)), 0)))
	{
		g_ctx.globals.focused_on_input = false;
		loaded_editing_script = false;
		editing_script.clear();
	}

	ImGui::EndGroup();

	ImGui::PopStyleVar();
	ImGui::End();
}

namespace ImGui
{
	bool Tab(const char* label, const ImVec2& size_arg, const bool selected)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		static float sizeplus = 0.f;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		ImVec2 pos = window->DC.CursorPos;

		ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::ItemSize(size, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

		ImGui::PushFont(c_menu::get().super_ico);
		if (selected)
			window->DrawList->AddText(ImVec2(bb.Min.x + size_arg.x / 2 - ImGui::CalcTextSize(label).x / 2, bb.Min.y + size_arg.y / 2 - ImGui::CalcTextSize(label).y / 2), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b()), label);
		else
			window->DrawList->AddText(ImVec2(bb.Min.x + size_arg.x / 2 - ImGui::CalcTextSize(label).x / 2, bb.Min.y + size_arg.y / 2 - ImGui::CalcTextSize(label).y / 2), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b()), label);
		ImGui::PopFont();

		return pressed;
	}
	bool sub_tab(const char* label, const float x, const bool selected)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImVec2 size_arg = { x, 20 };
		static float sizeplus = 0.f;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		ImVec2 pos = window->DC.CursorPos;

		ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::ItemSize(size, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

		ImGui::PushFont(c_menu::get().futura_small);
		if (selected)
			window->DrawList->AddText(ImVec2(bb.Min.x + size_arg.x / 2 - ImGui::CalcTextSize(label).x / 2, bb.Min.y + size_arg.y / 2 - ImGui::CalcTextSize(label).y / 2), ImColor(255, 255, 255, 255), label);
		else if (hovered)
			window->DrawList->AddText(ImVec2(bb.Min.x + size_arg.x / 2 - ImGui::CalcTextSize(label).x / 2, bb.Min.y + size_arg.y / 2 - ImGui::CalcTextSize(label).y / 2), ImColor(120, 120, 120, 255), label);
		else
			window->DrawList->AddText(ImVec2(bb.Min.x + size_arg.x / 2 - ImGui::CalcTextSize(label).x / 2, bb.Min.y + size_arg.y / 2 - ImGui::CalcTextSize(label).y / 2), ImColor(75, 75, 75, 255), label);
		ImGui::PopFont();

		return pressed;
	}
	bool TabName(const char* label)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		const auto size = ImGui::CalcTextSize(label);

		auto draw_list = ImGui::GetWindowDrawList();

		const auto width = ImGui::GetContentRegionAvailWidth();

		const auto cursor = ImGui::GetCursorScreenPos();

		const auto pos = ImVec2(cursor.x + width / 6 - size.x / 2, cursor.y);

		const auto start_col = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive);

		const auto col = ImGui::GetColorU32(start_col);

		const auto col2 = ImGui::GetColorU32(ImVec4(start_col.x, start_col.y, start_col.z, 0));

		draw_list->AddText(pos, ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), label);

		draw_list->AddRectFilledMultiColor(ImVec2(cursor.x, cursor.y + size.y / 2), ImVec2(pos.x - 5.f, (cursor.y + size.y / 2) + 2), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 0), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 44), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 144), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 0));

		draw_list->AddRectFilledMultiColor(ImVec2(pos.x + size.x + 5.f, cursor.y + size.y / 2), ImVec2(cursor.x + width, (cursor.y + size.y / 2) + 2), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 0), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 0), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 0), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 155));

		ImGui::NewLine();
	}
}

void c_menu::draw(bool is_open)
{
	// animation related code
	int m_alpha = is_open ? 1.f : 0.f;

	// set alpha in class to use later in widgets
	public_alpha = m_alpha;

	if (m_alpha <= 0.0001f)
		return;

	// set new alpha
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);

	// setup colors and some styles
	if (!menu_setupped)
		menu_setup(ImGui::GetStyle());

	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(ImGui::GetStyle().Colors[ImGuiCol_ScrollbarGrab].x, ImGui::GetStyle().Colors[ImGuiCol_ScrollbarGrab].y, ImGui::GetStyle().Colors[ImGuiCol_ScrollbarGrab].z, m_alpha));

	static float dpi_s = 1.f;
	switch (g_cfg.menu.size_menu) {
	case 0:
		dpi_s = 1.1f;
		break;
	case 1:
		dpi_s = 1.4f;
		break;
	case 2:
		dpi_s = 2.0f;
		break;
	}

	// default menu size
	const float x = 670 * dpi_s, y = 600 * dpi_s;
	const int tabs_count = 7;
	const char* tabs[tabs_count] =
	{
		"H",
		"A",
		"B",
		"C",
		"E",
		"F",
		"G"
	};
	int padding2 = 40;

	ImGuiStyle* Style = &ImGui::GetStyle();
	Style->Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
	Style->Colors[ImGuiCol_WindowBg] = ImColor(20, 20, 20);
	Style->Colors[ImGuiCol_ChildBg] = ImColor(20, 20, 20);
	Style->WindowBorderSize = 0;
	Style->WindowRounding = 0;
	Style->ChildRounding = 0;

	auto name = "itzlaith_lw";
	ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
	ImGuiWindowFlags NoMove = ImGuiWindowFlags_NoMove;
	ImGuiWindowFlags NoScroll = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	static auto type = 0;
	static auto rage_tab = 0;
	static auto visual_tab = 0;
	static auto player_tab = 0;
	static auto miscc_tab1 = 0;
	// static auto player = 0;
	static auto misc_tab = 0;

	auto player = g_cfg.player.teams;

	// start menu render
	ImGui::SetNextWindowSize(ImVec2(x, y));
	ImGui::Begin(crypt_str(name), nullptr, ImGuiWindowFlags_NoDecoration);
	{
		ImGui::SetCursorPos(ImVec2{ 0, 0 });
		Style->Colors[ImGuiCol_ChildBg] = ImColor(34, 34, 34);
		ImGui::BeginChild("##Tabs", ImVec2{ 40, y }, false);
		{
			ImGui::PushFont(c_menu::get().futura);
			ImGui::SetCursorPos(ImVec2{ 40 / 2 - ImGui::CalcTextSize("HW", NULL, true).x / 2, 20 });
			ImGui::Text(crypt_str("HW"));
			ImGui::PopFont();
			for (int i = 0; i < tabs_count; i++)
			{
				ImGui::SetCursorPos(ImVec2(0, 70 + padding2 * i));
				if (ImGui::Tab(tabs[i], ImVec2(40, 40), active_tab == i ? true : false))
					active_tab = i;
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2{ 40, y - 20 });
		ImGui::BeginChild("##footer", ImVec2{ x - 40, 20 }, false);
		{
			ImGui::Text(crypt_str("itzlaith_lw.uwu for Counter Strike: Global Offensive"));
		}
		ImGui::EndChild();

		float size_x = x - 60;
		float size_y = y - 60;

		if (active_tab == 1) 
		{
			subtab_start("subtab#1", size_x, size_y);
			if (ImGui::sub_tab("General ", (size_x - 40) / 2, rage_tab == 0 ? true : false)) rage_tab = 0; ImGui::SameLine(0);
			if (ImGui::sub_tab("Weapons ", (size_x - 40) / 2, rage_tab == 1 ? true : false)) rage_tab = 1;
			subtab_end();
		}

		if (active_tab == 3) 
		{
			subtab_start("subtab#2", size_x, size_y);
			if (ImGui::sub_tab("Model ", (size_x - 40) / 2, visual_tab == 0 ? true : false)) visual_tab = 0; ImGui::SameLine(0);
			if (ImGui::sub_tab("World ", (size_x - 40) / 2, visual_tab == 1 ? true : false)) visual_tab = 1;
			subtab_end();
		}

		if (active_tab == 4) 
		{
			subtab_start("subtab#4", size_x, size_y);
			if (ImGui::sub_tab("General ", (size_x - 40) / 2, miscc_tab1 == 0 ? true : false)) miscc_tab1 = 0; ImGui::SameLine(0);
			if (ImGui::sub_tab("Additives ", (size_x - 40) / 2, miscc_tab1 == 1 ? true : false)) miscc_tab1 = 1;
			subtab_end();
		}

		ImGui::SetCursorPos(ImVec2{ 50, 30 });
		Style->Colors[ImGuiCol_ChildBg] = ImColor(20, 20, 20);

		int out_padding = 10;
		ImGui::BeginChild("##func", ImVec2{ size_x, size_y }, false);
		{
			float size_a_x = size_x / 2; // ширина
			float size_a_y = size_y / 2 - out_padding / 2; // половина
			float size_d_x = size_x; // высота полная
			float size_d_y = size_y - out_padding / 2; // длинна полная

			float column_1 = 0;
			float column_2 = size_x / 2 + 10;

			Style->Colors[ImGuiCol_ChildBg] = ImColor(34, 34, 34);
			Style->ChildRounding = 2;

			if (active_tab == 1)
			{
				if (rage_tab == 0)
				{
					tab_start("1", column_1, 0, size_a_x, size_d_y);
					{
						ImGui::TabName(crypt_str("General"));

						ImGui::Checkbox(crypt_str("Enabled"), &g_cfg.ragebot.enable);

						if (g_cfg.ragebot.enable)
							g_cfg.legitbot.enabled = false;
						g_cfg.ragebot.zeus_bot = true;
						g_cfg.ragebot.knife_bot = true;

						//ImGui::SetCursorPosX(1);
						ImGui::SliderInt(crypt_str("Field of view"), &g_cfg.ragebot.field_of_view, 1, 180, false, crypt_str("%d°"));

						ImGui::Checkbox(crypt_str("Silent aim"), &g_cfg.ragebot.silent_aim);
						ImGui::Checkbox(crypt_str("Compensate recoil"), &g_cfg.ragebot.remove_recoil);
						ImGui::Checkbox(crypt_str("Automatic awall"), &g_cfg.ragebot.autowall);
						ImGui::Checkbox(crypt_str("Automatic fire"), &g_cfg.ragebot.autoshoot);
						ImGui::Checkbox(crypt_str("Automatic scope"), &g_cfg.ragebot.autoscope);
					}
					tab_end();

					tab_start("2", column_2, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Exploit"));

						ImGui::Checkbox(crypt_str("Hide shots"), &g_cfg.antiaim.hide_shots);
						if (g_cfg.antiaim.hide_shots)
						{
							ImGui::SameLine();
							draw_keybind(crypt_str(""), &g_cfg.antiaim.hide_shots_key, crypt_str("##HOTKEY_HIDESHOTS"));
						}

						ImGui::Checkbox(crypt_str("Double tap"), &g_cfg.ragebot.double_tap);
						ImGui::SameLine();
						draw_keybind(crypt_str(""), &g_cfg.ragebot.double_tap_key, crypt_str("##HOTKEY_DOUBLETAP"));
						if (g_cfg.ragebot.double_tap)
						{




							if (&g_cfg.ragebot.double_tap_key)
							{
								//ImGui::Checkbox(crypt_str("Defensive double-tap"), &g_cfg.ragebot.defensive_doubletap);
								ImGui::Checkbox(crypt_str("Break LagComp"), &g_cfg.misc.break_lc);
								//ImGui::Checkbox(crypt_str("Defensive"), &g_cfg.ragebot.defensive_doubletap);
								ImGui::Checkbox(crypt_str("Custome Double Tap"), &g_cfg.ragebot.use_cs_shift_amount);
								if (g_cfg.ragebot.use_cs_shift_amount)
								{
									ImGui::SliderInt(crypt_str("Ticks To Shift"), &g_cfg.ragebot.shift_amount, 1, 17);
									ImGui::SliderFloat(crypt_str("Recharge Time"), &g_cfg.ragebot.recharge_time, 0.00f, 3.00f, crypt_str("%.2f"));
								}
								else
								{
									g_cfg.ragebot.shift_amount = 13;
									g_cfg.ragebot.recharge_time = 0.75f;

								}


							}
						}
					}
					tab_end();
				}
				else if (rage_tab == 1)
				{
					const char* rage_weapons[8] = { crypt_str("Revolver / Deagle"), crypt_str("Pistols"), crypt_str("SMGs"), crypt_str("Rifles"), crypt_str("Auto"), crypt_str("Scout"), crypt_str("AWP"), crypt_str("Heavy") };

					tab_start("3221", column_1, 0, size_a_x, size_d_y);
					{
						ImGui::TabName(crypt_str("Weapon"));

						draw_combo(crypt_str("Current weapon"), hooks::rage_weapon, rage_weapons, ARRAYSIZE(rage_weapons));

						draw_combo(crypt_str("Target selection"), g_cfg.ragebot.weapon[hooks::rage_weapon].selection_type, selection, ARRAYSIZE(selection));
						padding(0, 3);

						
						ImGui::Checkbox(crypt_str("Static point scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale);
						if (g_cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale)
						{
							draw_multicombo(crypt_str("Multipoint Hitboxes"), g_cfg.ragebot.weapon[hooks::rage_weapon].multipoints_hitboxes, multipoint_hitboxes, ARRAYSIZE(multipoint_hitboxes), preview);
							ImGui::SliderFloat(crypt_str("Head scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].head_scale, 0.0f, 1.0f, g_cfg.ragebot.weapon[hooks::rage_weapon].head_scale ? crypt_str("%.2f") : crypt_str("None"));
							ImGui::SliderFloat(crypt_str("Body scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].body_scale, 0.0f, 1.0f, g_cfg.ragebot.weapon[hooks::rage_weapon].body_scale ? crypt_str("%.2f") : crypt_str("None"));
							//ImGui::SliderFloat(crypt_str("Limps scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].limp_scale, 0.0f, 1.0f, g_cfg.ragebot.weapon[hooks::rage_weapon].limp_scale ? crypt_str("%.2f") : crypt_str("None"));
						}
						

						//ImGui::Checkbox(crypt_str("Adaptive point scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].adaptive_point_scale);

						ImGui::Checkbox(crypt_str("Enable max misses"), &g_cfg.ragebot.weapon[hooks::rage_weapon].max_misses);
						if (g_cfg.ragebot.weapon[hooks::rage_weapon].max_misses)
							ImGui::SliderInt(crypt_str("Max misses"), &g_cfg.ragebot.weapon[hooks::rage_weapon].max_misses_amount, 0, 6);

						ImGui::Checkbox(crypt_str("Ignore limbs when moving"), &g_cfg.ragebot.weapon[hooks::rage_weapon].rage_aimbot_ignore_limbs);

						ImGui::Checkbox(crypt_str("Prefer safe points"), &g_cfg.ragebot.weapon[hooks::rage_weapon].prefer_safe_points);
						ImGui::Checkbox(crypt_str("Prefer body aim"), &g_cfg.ragebot.weapon[hooks::rage_weapon].prefer_body_aim);
					}
					tab_end();

					tab_start("weapon#1", column_2, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Weapons"));

						draw_multicombo(crypt_str("Hitboxes"), g_cfg.ragebot.weapon[hooks::rage_weapon].hitboxes, hitboxes, ARRAYSIZE(hitboxes), preview);
						padding(0, 3);

						ImGui::Checkbox(crypt_str("Hitchance"), &g_cfg.ragebot.weapon[hooks::rage_weapon].hitchance);
						if (g_cfg.ragebot.weapon[hooks::rage_weapon].hitchance)
							ImGui::SliderInt(crypt_str("Hitchance amount"), &g_cfg.ragebot.weapon[hooks::rage_weapon].hitchance_amount, 1, 100);

						if (g_cfg.ragebot.double_tap && hooks::rage_weapon <= 4)
						{
							ImGui::Checkbox(crypt_str("Double tap hitchance"), &g_cfg.ragebot.weapon[hooks::rage_weapon].double_tap_hitchance);
							if (g_cfg.ragebot.weapon[hooks::rage_weapon].double_tap_hitchance)
								ImGui::SliderInt(crypt_str("Double tap hitchance"), &g_cfg.ragebot.weapon[hooks::rage_weapon].double_tap_hitchance_amount, 1, 100);
						}

						draw_keybind(crypt_str("Damage override"), &g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key, crypt_str("##HOTKEY__DAMAGE_OVERRIDE"));
						if (g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key > KEY_NONE && g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key < KEY_MAX)
							ImGui::SliderInt(crypt_str("Minimum override damage"), &g_cfg.ragebot.weapon[hooks::rage_weapon].minimum_override_damage, 1, 120, true);

						ImGui::SliderInt(crypt_str("Minimum damage"), &g_cfg.ragebot.weapon[hooks::rage_weapon].minimum_visible_damage, 1, 120, true);
						if (g_cfg.ragebot.autowall)
							ImGui::SliderInt(crypt_str("Minimum wall damage"), &g_cfg.ragebot.weapon[hooks::rage_weapon].minimum_damage, 1, 120, true);

						if (g_cfg.ragebot.double_tap && hooks::rage_weapon <= 4)
							ImGui::Checkbox(crypt_str("Adaptive two shot damage"), &g_cfg.ragebot.weapon[hooks::rage_weapon].adaptive_two_shot);
						draw_keybind(crypt_str("Force safe points"), &g_cfg.ragebot.safe_point_key, crypt_str("##HOKEY_FORCE_SAFE_POINTS"));
						ImGui::Spacing();
						draw_keybind(crypt_str("Force body aim"), &g_cfg.ragebot.body_aim_key, crypt_str("##HOKEY_FORCE_BODY_AIM"));
					}
					tab_end();

					tab_start("accuracy", column_2, size_a_y + 5, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Accuracy"));

						ImGui::Checkbox(crypt_str("Automatic stop"), &g_cfg.ragebot.weapon[hooks::rage_weapon].autostop);
						if (g_cfg.ragebot.weapon[hooks::rage_weapon].autostop)
							draw_multicombo(crypt_str("Modifiers"), g_cfg.ragebot.weapon[hooks::rage_weapon].autostop_modifiers, autostop_modifiers, ARRAYSIZE(autostop_modifiers), preview);
					}
					tab_end();
				}
			}

			if (active_tab == 2)
			{

				tab_start("3", column_1, 0, size_a_x, size_a_y);
				{
					ImGui::TabName(crypt_str("General"));

					ImGui::Checkbox(crypt_str("Enabled"), &g_cfg.antiaim.enable);



					draw_combo(crypt_str("Anti-aim type"), g_cfg.antiaim.antiaim_type, antiaim_type, ARRAYSIZE(antiaim_type));
					padding(0, 3);
					draw_combo(crypt_str("Movement type"), type, movement_type, ARRAYSIZE(movement_type));

					ImGui::Checkbox(crypt_str("Pitch null on landing"), &g_cfg.antiaim.pitch_on_land);

					draw_keybind(crypt_str("Manual back"), &g_cfg.antiaim.manual_back, crypt_str("##HOTKEY_INVERT_BACK"));
					draw_keybind(crypt_str("Manual left"), &g_cfg.antiaim.manual_left, crypt_str("##HOTKEY_INVERT_LEFT"));
					draw_keybind(crypt_str("Manual right"), &g_cfg.antiaim.manual_right, crypt_str("##HOTKEY_INVERT_RIGHT"));
					draw_keybind(crypt_str("Manual forward"), &g_cfg.antiaim.manual_forward, crypt_str("##HOTKEY_INVERT_FORWARD"));

					if (g_cfg.antiaim.manual_back.key > KEY_NONE && g_cfg.antiaim.manual_back.key < KEY_MAX || g_cfg.antiaim.manual_left.key > KEY_NONE && g_cfg.antiaim.manual_left.key < KEY_MAX || g_cfg.antiaim.manual_right.key > KEY_NONE && g_cfg.antiaim.manual_right.key < KEY_MAX)
					{
						ImGui::Checkbox(crypt_str("Manual anti-aim"), &g_cfg.antiaim.flip_indicator);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##invc"), &g_cfg.antiaim.flip_indicator_color, ALPHA);
					}
				}
				tab_end();

				tab_start("4", column_1, size_a_y + 10, size_a_x, size_a_y);
				{
					ImGui::TabName(crypt_str("Other"));

					ImGui::Checkbox(crypt_str("Enabled##fakelag"), &g_cfg.antiaim.fakelag);
					if (g_cfg.antiaim.fakelag)
					{
						draw_combo(crypt_str("Fake-lag type"), g_cfg.antiaim.fakelag_type, fakelags, ARRAYSIZE(fakelags));
						padding(0, 3);
						draw_multicombo(crypt_str("Fake-lag condition"), g_cfg.antiaim.fakelag_enablers, lagstrigger, ARRAYSIZE(lagstrigger), preview);

						auto enabled_fakelag_triggers = false;

						for (auto i = 0; i < ARRAYSIZE(lagstrigger); i++)
							if (g_cfg.antiaim.fakelag_enablers[i])
								enabled_fakelag_triggers = true;

						if (enabled_fakelag_triggers)
							ImGui::SliderInt(crypt_str("Fake-lag limit"), &g_cfg.antiaim.triggers_fakelag_amount, 1, 16);					
					}

					
					draw_combo(crypt_str("Legs type"), g_cfg.misc.legs_movement, legs, ARRAYSIZE(legs));
				}
				tab_end();

				tab_start("5", column_2, 0, size_a_x, size_d_y);
				{
					ImGui::TabName(crypt_str("Settings"));

					if (g_cfg.antiaim.antiaim_type)
					{
						draw_combo(crypt_str("Desync"), g_cfg.antiaim.desync, desync, ARRAYSIZE(desync));

						if (g_cfg.antiaim.desync)
							draw_combo(crypt_str("LBY type"), g_cfg.antiaim.legit_lby_type, lby_type, ARRAYSIZE(lby_type));
					}
					else
					{
						ImGui::Checkbox(crypt_str("Fake Flick"), &g_cfg.antiaim.flick);


						if (g_cfg.antiaim.flick)
							ImGui::SliderInt(crypt_str("Flick Speed"), &g_cfg.antiaim.flicktick, 5, 64, false, crypt_str("%d°"));
						ImGui::Checkbox(crypt_str("Roll"), &g_cfg.antiaim.roll_enabled);
						if (g_cfg.antiaim.roll_enabled)
							ImGui::SliderFloat(crypt_str("roll amount"), &g_cfg.antiaim.roll, -45.f, 45.f, crypt_str("%.2f"));

						ImGui::Checkbox(crypt_str("Static legs in air"), &g_cfg.antiaim.static_legs);
						ImGui::Checkbox(crypt_str("Freestand desync"), &g_cfg.antiaim.freestand);

						draw_combo(crypt_str("Pitch"), g_cfg.antiaim.type[type].pitch, pitch, ARRAYSIZE(pitch));
						padding(0, 3);
						draw_combo(crypt_str("Yaw"), g_cfg.antiaim.type[type].yaw, yaw, ARRAYSIZE(yaw));
						padding(0, 3);
						draw_combo(crypt_str("Base angle"), g_cfg.antiaim.type[type].base_angle, baseangle, ARRAYSIZE(baseangle));

						if (g_cfg.antiaim.type[type].yaw)
						{
							ImGui::SliderInt(g_cfg.antiaim.type[type].yaw == 1 ? crypt_str("Jitter range") : crypt_str("Spin range"), &g_cfg.antiaim.type[type].range, 1, 180);

							padding(0, 3);
						}

						//ImGui::Checkbox(crypt_str("Anti-Brute"), &g_cfg.antiaim.anti_brute);
						draw_keybind(crypt_str("Invert desync"), &g_cfg.antiaim.flip_desync, crypt_str("##HOTKEY_INVERT_DESYNC"));
						

						draw_combo(crypt_str("Desync"), g_cfg.antiaim.type[type].desync, desync, ARRAYSIZE(desync));

						if (g_cfg.antiaim.type[type].desync == 1)
						{
							
								

						}

						if (g_cfg.antiaim.type[type].desync)
						{
							if (type == ANTIAIM_STAND)
							{
								draw_combo(crypt_str("LBY type"), g_cfg.antiaim.lby_type, lby_type, ARRAYSIZE(lby_type));
							}

							if (type != ANTIAIM_STAND || !g_cfg.antiaim.lby_type)
							{
								ImGui::SliderInt(crypt_str("Desync range"), &g_cfg.antiaim.type[type].desync_range, -58 , 58);
								ImGui::SliderInt(crypt_str("Inverted desync range"), &g_cfg.antiaim.type[type].inverted_desync_range, -58 , 58 );
								ImGui::SliderInt(crypt_str("Body lean"), &g_cfg.antiaim.type[type].body_lean, 0, 100);
								ImGui::SliderInt(crypt_str("Inverted body lean"), &g_cfg.antiaim.type[type].inverted_body_lean, 0, 100);
							}
						}
					}
					tab_end();
				}
			}

			if (active_tab == 0)
			{
				const char* legit_weapons[6] = { crypt_str("Deagle"), crypt_str("Pistols"), crypt_str("Rifles"), crypt_str("SMGs"), crypt_str("Snipers"), crypt_str("Heavy") };
				const char* hitbox_legit[3] = { crypt_str("Nearest"), crypt_str("Head"), crypt_str("Body") };

				const char* track[3] = { crypt_str("Low"), crypt_str("Medium"), crypt_str("High") };

				tab_start("#1", column_1, 0, size_a_x, size_d_y);
				{
					ImGui::TabName(crypt_str("Legit bot"));

					padding(0, 6);
					ImGui::Checkbox(crypt_str("Enable"), &g_cfg.legitbot.enabled);
					ImGui::SameLine();
					draw_keybind(crypt_str(""), &g_cfg.legitbot.key, crypt_str("##HOTKEY_LGBT_KEY"));

					if (g_cfg.legitbot.enabled)
						g_cfg.ragebot.enable = false;

					ImGui::Checkbox(crypt_str("Friendly fire"), &g_cfg.legitbot.friendly_fire);
					ImGui::Checkbox(crypt_str("Automatic pistols"), &g_cfg.legitbot.autopistol);

					ImGui::Checkbox(crypt_str("Automatic scope"), &g_cfg.legitbot.autoscope);

					if (g_cfg.legitbot.autoscope)
						ImGui::Checkbox(crypt_str("Unscope after shot"), &g_cfg.legitbot.unscope);

					ImGui::Checkbox(crypt_str("Snipers in zoom only"), &g_cfg.legitbot.sniper_in_zoom_only);

					ImGui::Checkbox(crypt_str("Enable in air"), &g_cfg.legitbot.do_if_local_in_air);
					ImGui::Checkbox(crypt_str("Enable if flashed"), &g_cfg.legitbot.do_if_local_flashed);
					ImGui::Checkbox(crypt_str("Enable in smoke"), &g_cfg.legitbot.do_if_enemy_in_smoke);

					draw_keybind(crypt_str("Automatic fire key"), &g_cfg.legitbot.autofire_key, crypt_str("##HOTKEY_AUTOFIRE_LGBT_KEY"));
					ImGui::SliderInt(crypt_str("Automatic fire delay"), &g_cfg.legitbot.autofire_delay, 0, 12, false, (!g_cfg.legitbot.autofire_delay ? crypt_str("None") : (g_cfg.legitbot.autofire_delay == 1 ? crypt_str("%d tick") : crypt_str("%d ticks"))));
				}
				tab_end();

				tab_start("3224233452", column_2, 0, size_a_x, size_d_y);
				{
					ImGui::TabName(crypt_str("Weapon settings"));

					ImGui::Spacing();
					draw_combo(crypt_str("Current weapon"), hooks::legit_weapon, legit_weapons, ARRAYSIZE(legit_weapons));
					ImGui::Spacing();
					draw_combo(crypt_str("Hitbox"), g_cfg.legitbot.weapon[hooks::legit_weapon].priority, hitbox_legit, ARRAYSIZE(hitbox_legit));

					ImGui::Checkbox(crypt_str("Automatic stop"), &g_cfg.legitbot.weapon[hooks::legit_weapon].auto_stop);

					draw_combo(crypt_str("Backtrack tick"), g_cfg.legitbot.weapon[hooks::legit_weapon].accuracy_boost_type, track, ARRAYSIZE(track));

					draw_combo(crypt_str("Field of view type"), g_cfg.legitbot.weapon[hooks::legit_weapon].fov_type, LegitFov, ARRAYSIZE(LegitFov));
					ImGui::SliderFloat(crypt_str("Field of view amount"), &g_cfg.legitbot.weapon[hooks::legit_weapon].fov, 0.f, 30.f, crypt_str("%.2f"));

					ImGui::Spacing();

					ImGui::SliderFloat(crypt_str("Silent field of view"), &g_cfg.legitbot.weapon[hooks::legit_weapon].silent_fov, 0.f, 30.f, (!g_cfg.legitbot.weapon[hooks::legit_weapon].silent_fov ? crypt_str("None") : crypt_str("%.2f"))); //-V550

					ImGui::Spacing();

					draw_combo(crypt_str("Smooth type"), g_cfg.legitbot.weapon[hooks::legit_weapon].smooth_type, LegitSmooth, ARRAYSIZE(LegitSmooth));
					ImGui::SliderFloat(crypt_str("Smooth amount"), &g_cfg.legitbot.weapon[hooks::legit_weapon].smooth, 1.f, 12.f, crypt_str("%.1f"));

					ImGui::Spacing();

					draw_combo(crypt_str("RCS type"), g_cfg.legitbot.weapon[hooks::legit_weapon].rcs_type, RCSType, ARRAYSIZE(RCSType));
					ImGui::SliderFloat(crypt_str("RCS amount"), &g_cfg.legitbot.weapon[hooks::legit_weapon].rcs, 0.f, 100.f, crypt_str("%.0f%%"), 1.f);

					if (g_cfg.legitbot.weapon[hooks::legit_weapon].rcs > 0)
					{
						ImGui::SliderFloat(crypt_str("RCS custom field of view"), &g_cfg.legitbot.weapon[hooks::legit_weapon].custom_rcs_fov, 0.f, 30.f, (!g_cfg.legitbot.weapon[hooks::legit_weapon].custom_rcs_fov ? crypt_str("None") : crypt_str("%.2f"))); //-V550
						ImGui::SliderFloat(crypt_str("RCS Custom smooth"), &g_cfg.legitbot.weapon[hooks::legit_weapon].custom_rcs_smooth, 0.f, 12.f, (!g_cfg.legitbot.weapon[hooks::legit_weapon].custom_rcs_smooth ? crypt_str("None") : crypt_str("%.1f"))); //-V550
					}

					ImGui::Spacing();

					ImGui::SliderInt(crypt_str("Automatic wall damage"), &g_cfg.legitbot.weapon[hooks::legit_weapon].awall_dmg, 0, 100, false, (!g_cfg.legitbot.weapon[hooks::legit_weapon].awall_dmg ? crypt_str("None") : crypt_str("%d")));
					ImGui::SliderInt(crypt_str("Automatic fire hitchance"), &g_cfg.legitbot.weapon[hooks::legit_weapon].autofire_hitchance, 0, 100, false, (!g_cfg.legitbot.weapon[hooks::legit_weapon].autofire_hitchance ? crypt_str("None") : crypt_str("%d")));
					ImGui::SliderFloat(crypt_str("Target switch delay"), &g_cfg.legitbot.weapon[hooks::legit_weapon].target_switch_delay, 0.f, 1.f);
				}
				tab_end();
			}

			if (active_tab == 3)
			{
				if (visual_tab == 0)
				{
					tab_start("7", column_1, 0, size_a_x, size_d_y);
					{
						ImGui::TabName(crypt_str("ESP Model"));

						ImGui::Checkbox(crypt_str("Enabled"), &g_cfg.player.enable);
						draw_combo(crypt_str("Set team"), g_cfg.player.teams, player_teams, ARRAYSIZE(player_teams));

						if (player == ENEMY)
						{
							ImGui::Checkbox(crypt_str("Display Resolved Delta"), &g_cfg.player.show_resolved);
							ImGui::Checkbox(crypt_str("OOF arrows"), &g_cfg.player.arrows);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##arrowscolor"), &g_cfg.player.arrows_color, ALPHA);

							if (g_cfg.player.arrows)
							{
								ImGui::SliderInt(crypt_str("Arrows distance"), &g_cfg.player.distance, 1, 100);
								ImGui::SliderInt(crypt_str("Arrows size"), &g_cfg.player.size, 1, 100);
							}
						}

						ImGui::Checkbox(crypt_str("Bounding box"), &g_cfg.player.type[player].box);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##boxcolor"), &g_cfg.player.type[player].box_color, ALPHA);

						ImGui::Checkbox(crypt_str("Name"), &g_cfg.player.type[player].name);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##namecolor"), &g_cfg.player.type[player].name_color, ALPHA);

						ImGui::Checkbox(crypt_str("Health bar"), &g_cfg.player.type[player].health);
						ImGui::Checkbox(crypt_str("Health color"), &g_cfg.player.type[player].custom_health_color);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##healthcolor"), &g_cfg.player.type[player].health_color, ALPHA);

						for (auto i = 0, j = 0; i < ARRAYSIZE(flags); i++)
						{
							if (g_cfg.player.type[player].flags[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)flags[i];
								else
									preview = flags[i];

								j++;
							}
						}

						draw_multicombo(crypt_str("Flags"), g_cfg.player.type[player].flags, flags, ARRAYSIZE(flags), preview);
						draw_multicombo(crypt_str("Weapon"), g_cfg.player.type[player].weapon, weaponplayer, ARRAYSIZE(weaponplayer), preview);


						if (g_cfg.player.type[player].weapon[WEAPON_ICON] || g_cfg.player.type[player].weapon[WEAPON_TEXT])
						{
							ImGui::Text(crypt_str("Color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##weapcolor"), &g_cfg.player.type[player].weapon_color, ALPHA);
						}

						ImGui::Checkbox(crypt_str("Skeleton"), &g_cfg.player.type[player].skeleton);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##skeletoncolor"), &g_cfg.player.type[player].skeleton_color, ALPHA);

						ImGui::Checkbox(crypt_str("Ammo bar"), &g_cfg.player.type[player].ammo);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##ammocolor"), &g_cfg.player.type[player].ammobar_color, ALPHA);

						ImGui::Checkbox(crypt_str("Footsteps"), &g_cfg.player.type[player].footsteps);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##footstepscolor"), &g_cfg.player.type[player].footsteps_color, ALPHA);

						if (g_cfg.player.type[player].footsteps)
						{
							ImGui::SliderInt(crypt_str("Thickness"), &g_cfg.player.type[player].thickness, 1, 10);
							ImGui::SliderInt(crypt_str("Radius"), &g_cfg.player.type[player].radius, 50, 500);
						}

						if (player == ENEMY || player == TEAM)
						{
							ImGui::Checkbox(crypt_str("Snap lines"), &g_cfg.player.type[player].snap_lines);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##snapcolor"), &g_cfg.player.type[player].snap_lines_color, ALPHA);

							if (player == ENEMY)
							{
								if (g_cfg.ragebot.enable)
								{
									ImGui::Checkbox(crypt_str("Aimbot points"), &g_cfg.player.show_multi_points);
									ImGui::SameLine();
									ImGui::ColorEdit(crypt_str("##showmultipointscolor"), &g_cfg.player.show_multi_points_color, ALPHA);
								}

								ImGui::Checkbox(crypt_str("Aimbot hitboxes"), &g_cfg.player.lag_hitbox);
								ImGui::SameLine();
								ImGui::ColorEdit(crypt_str("##lagcompcolor"), &g_cfg.player.lag_hitbox_color, ALPHA);

								if (g_cfg.player.lag_hitbox)
								{
									draw_combo(("Aimbot hitboxes type"), g_cfg.player.lag_hitbox_type, lag_type, ARRAYSIZE(lag_type));
								}
							}

							draw_combo(crypt_str("Player model T"), g_cfg.player.player_model_t, player_model_t, ARRAYSIZE(player_model_t));
							draw_combo(crypt_str("Player model CT"), g_cfg.player.player_model_ct, player_model_ct, ARRAYSIZE(player_model_ct));
						}
						tab_end();

						tab_start("1wdew", column_2, 0, size_a_x, size_d_y);
						{
							ImGui::TabName(crypt_str("Model Chams"));

							draw_combo(crypt_str("Set team"), g_cfg.player.teams, player_teams, ARRAYSIZE(player_teams));

							if (player == LOCAL)
								draw_combo(crypt_str("Type"), g_cfg.player.local_chams_type, local_chams_type, ARRAYSIZE(local_chams_type));

							if (player != 2 || LOCAL || !g_cfg.player.local_chams_type)
								draw_multicombo(crypt_str("Chams"), g_cfg.player.type[player].chams, g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? chamsvisact : chamsvis, g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? ARRAYSIZE(chamsvisact) : ARRAYSIZE(chamsvis), preview);

							if (g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] || player == 2 || LOCAL && g_cfg.player.local_chams_type) //-V648
							{
								if (player == LOCAL && g_cfg.player.local_chams_type)
								{
									ImGui::Checkbox(crypt_str("Enable desync chams"), &g_cfg.player.fake_chams_enable);
									ImGui::Checkbox(crypt_str("Visualize fake-lag"), &g_cfg.player.visualize_lag);
									ImGui::Checkbox(crypt_str("Layered"), &g_cfg.player.layered);

									draw_combo(crypt_str("Player chams material"), g_cfg.player.fake_chams_type, chamstype, ARRAYSIZE(chamstype));

									ImGui::Text(crypt_str("Color "));
									ImGui::SameLine();
									ImGui::ColorEdit(crypt_str("##fakechamscolor"), &g_cfg.player.fake_chams_color, ALPHA);

									if (g_cfg.player.fake_chams_type != 6)
									{
										ImGui::Checkbox(crypt_str("Double material "), &g_cfg.player.fake_double_material);
										ImGui::SameLine();
										ImGui::ColorEdit(crypt_str("##doublecolormat"), &g_cfg.player.fake_double_material_color, ALPHA);
									}
								}
								else
								{
									draw_combo(crypt_str("Player chams material"), g_cfg.player.type[player].chams_type, chamstype, ARRAYSIZE(chamstype));

									if (g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE])
									{
										ImGui::Text(crypt_str("Visible "));
										ImGui::SameLine();
										ImGui::ColorEdit(crypt_str("##chamsvisible"), &g_cfg.player.type[player].chams_color, ALPHA);
									}

									if (g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] && g_cfg.player.type[player].chams[PLAYER_CHAMS_INVISIBLE])
									{
										ImGui::Text(crypt_str("Invisible "));
										ImGui::SameLine();
										ImGui::ColorEdit(crypt_str("##chamsinvisible"), &g_cfg.player.type[player].xqz_color, ALPHA);
									}

									if (g_cfg.player.type[player].chams_type != 6)
									{
										ImGui::Checkbox(crypt_str("Double material "), &g_cfg.player.fake_double_material);
										ImGui::SameLine();
										ImGui::ColorEdit(crypt_str("##doublecolormat"), &g_cfg.player.fake_double_material_color, ALPHA);
									}

									if (player == 0 || ENEMY)
									{
										ImGui::Checkbox(crypt_str("Backtrack chams"), &g_cfg.player.backtrack_chams);

										if (g_cfg.player.backtrack_chams)
										{
											draw_combo(crypt_str("Backtrack chams material"), g_cfg.player.backtrack_chams_material, chamstype, ARRAYSIZE(chamstype));

											ImGui::Text(crypt_str("Color "));
											ImGui::SameLine();
											ImGui::ColorEdit(crypt_str("##backtrackcolor"), &g_cfg.player.backtrack_chams_color, ALPHA);
										}
									}
									if (player == 0 || ENEMY || player == 1 || TEAM)
									{
										ImGui::Checkbox(crypt_str("Ragdoll chams"), &g_cfg.player.type[player].ragdoll_chams);

										if (g_cfg.player.type[player].ragdoll_chams)
										{
											draw_combo(crypt_str("Ragdoll chams material"), g_cfg.player.type[player].ragdoll_chams_material, chamstype, ARRAYSIZE(chamstype));

											ImGui::Text(crypt_str("Color "));
											ImGui::SameLine();
											ImGui::ColorEdit(crypt_str("##ragdollcolor"), &g_cfg.player.type[player].ragdoll_chams_color, ALPHA);
										}
									}
									else if (!g_cfg.player.local_chams_type)
									{
										ImGui::Checkbox(crypt_str("Transparency in scope"), &g_cfg.player.transparency_in_scope);

										if (g_cfg.player.transparency_in_scope)
											ImGui::SliderFloat(crypt_str("Alpha"), &g_cfg.player.transparency_in_scope_amount, 0.0f, 1.0f);
									}

									if (player == 0 || ENEMY || player == 1 || TEAM || player == 2 || LOCAL)
									{
										ImGui::Checkbox(crypt_str("Glow"), &g_cfg.player.type[player].glow);

										if (g_cfg.player.type[player].glow)
										{
											draw_combo(crypt_str("Glow type"), g_cfg.player.type[player].glow_type, glowtype, ARRAYSIZE(glowtype));
											ImGui::Text(crypt_str("Color "));
											ImGui::SameLine();
											ImGui::ColorEdit(crypt_str("##glowcolor"), &g_cfg.player.type[player].glow_color, ALPHA);
										}
									}

									if (player == LOCAL)
									{
										ImGui::Checkbox(crypt_str("Arms chams"), &g_cfg.esp.arms_chams);
										ImGui::SameLine();
										ImGui::ColorEdit(crypt_str("##armscolor"), &g_cfg.esp.arms_chams_color, ALPHA);

										if (g_cfg.esp.arms_chams)
										{
											draw_combo(crypt_str("Arms chams material"), g_cfg.esp.arms_chams_type, chamstype, ARRAYSIZE(chamstype));

											if (g_cfg.esp.arms_chams_type != 6)
											{
												ImGui::Checkbox(crypt_str("Double material "), &g_cfg.player.fake_double_material);
												ImGui::SameLine();
												ImGui::ColorEdit(crypt_str("##doublecolormat"), &g_cfg.player.fake_double_material_color, ALPHA);
											}
										}

										ImGui::Checkbox(crypt_str("Weapon chams"), &g_cfg.esp.weapon_chams);
										ImGui::SameLine();
										ImGui::ColorEdit(crypt_str("##weaponchamscolors"), &g_cfg.esp.weapon_chams_color, ALPHA);

										if (g_cfg.esp.weapon_chams)
										{
											draw_combo(crypt_str("Weapon chams material"), g_cfg.esp.weapon_chams_type, chamstype, ARRAYSIZE(chamstype));

											if (g_cfg.esp.weapon_chams_type != 6)
											{
												ImGui::Checkbox(crypt_str("Double material "), &g_cfg.player.fake_double_material);
												ImGui::SameLine();
												ImGui::ColorEdit(crypt_str("##doublecolormat"), &g_cfg.player.fake_double_material_color, ALPHA);
											}
										}
									}
								}
							}
						}
						tab_end();
					}
				}
				else if (visual_tab == 1)
				{
					tab_start("world", column_1, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("World"));

						ImGui::Checkbox(crypt_str("Rain"), &g_cfg.esp.rain);
						ImGui::Checkbox(crypt_str("Full bright"), &g_cfg.esp.bright);
						ImGui::Checkbox(crypt_str("Draw gray"), &g_cfg.esp.drawgray);

						draw_combo(crypt_str("Skybox"), g_cfg.esp.skybox, skybox, ARRAYSIZE(skybox));

						ImGui::Text(crypt_str("Color "));
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##skyboxcolor"), &g_cfg.esp.skybox_color, NOALPHA);

						if (g_cfg.esp.skybox == 21)
						{
							static char sky_custom[64] = "\0";

							if (!g_cfg.esp.custom_skybox.empty())
								strcpy_s(sky_custom, sizeof(sky_custom), g_cfg.esp.custom_skybox.c_str());

							ImGui::Text(crypt_str("Name"));
							ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

							if (ImGui::InputText(crypt_str("##customsky"), sky_custom, sizeof(sky_custom)))
								g_cfg.esp.custom_skybox = sky_custom;

							ImGui::PopStyleVar();
						}

						ImGui::Checkbox(crypt_str("Color modulation"), &g_cfg.esp.nightmode);

						if (g_cfg.esp.nightmode)
						{
							ImGui::Text(crypt_str("World color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##worldcolor"), &g_cfg.esp.world_color, ALPHA);

							ImGui::Text(crypt_str("Props color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##propscolor"), &g_cfg.esp.props_color, ALPHA);
						}

						ImGui::Checkbox(crypt_str("World modulation"), &g_cfg.esp.world_modulation);

						if (g_cfg.esp.world_modulation)
						{
							ImGui::SliderFloat(crypt_str("Bloom"), &g_cfg.esp.bloom, 0.0f, 750.0f);
							ImGui::SliderFloat(crypt_str("Exposure"), &g_cfg.esp.exposure, 0.0f, 2000.0f);
							ImGui::SliderFloat(crypt_str("Ambient"), &g_cfg.esp.ambient, 0.0f, 1500.0f);
						}

						ImGui::Checkbox(crypt_str("Fog modulation"), &g_cfg.esp.fog);

						if (g_cfg.esp.fog)
						{
							ImGui::SliderInt(crypt_str("Distance"), &g_cfg.esp.fog_distance, 0, 2500);
							ImGui::SliderInt(crypt_str("Density"), &g_cfg.esp.fog_density, 0, 100);

							ImGui::Text(crypt_str("Color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##fogcolor"), &g_cfg.esp.fog_color, NOALPHA);
						}
					}
					tab_end();

					tab_start("render", column_1, size_a_y + 10, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Render"));
						ImGui::Checkbox(crypt_str("Enabled"), &g_cfg.player.enable);

						draw_multicombo(crypt_str("Indicators"), g_cfg.esp.indicators, indicators, ARRAYSIZE(indicators), preview);
						padding(0, 3);

						draw_multicombo(crypt_str("Removals"), g_cfg.esp.removals, removals, ARRAYSIZE(removals), preview);

						if (g_cfg.esp.removals[REMOVALS_ZOOM])
							ImGui::Checkbox(crypt_str("Fix zoom sensivity"), &g_cfg.esp.fix_zoom_sensivity);

						ImGui::Checkbox(crypt_str("Disable panorama blur"), &g_cfg.misc.disablepanoramablur);

						ImGui::Checkbox(crypt_str("Client impacts"), &g_cfg.esp.client_bullet_impacts);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##clientbulletimpacts"), &g_cfg.esp.client_bullet_impacts_color, ALPHA);

						ImGui::Checkbox(crypt_str("Server impacts"), &g_cfg.esp.server_bullet_impacts);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##serverbulletimpacts"), &g_cfg.esp.server_bullet_impacts_color, ALPHA);

						ImGui::Checkbox(crypt_str("Local tracers"), &g_cfg.esp.bullet_tracer);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##bulltracecolor"), &g_cfg.esp.bullet_tracer_color, ALPHA);

						ImGui::Checkbox(crypt_str("Enemy tracers"), &g_cfg.esp.enemy_bullet_tracer);
						ImGui::SameLine();

						ImGui::ColorEdit(crypt_str("##enemybulltracecolor"), &g_cfg.esp.enemy_bullet_tracer_color, ALPHA);
						draw_multicombo(crypt_str("Hit marker"), g_cfg.esp.hitmarker, hitmarkers, ARRAYSIZE(hitmarkers), preview);
						ImGui::Checkbox(crypt_str("Damage marker"), &g_cfg.esp.damage_marker);
						ImGui::Checkbox(crypt_str("Kill effect"), &g_cfg.esp.kill_effect);

						if (g_cfg.esp.kill_effect)
							ImGui::SliderFloat(crypt_str("Duration"), &g_cfg.esp.kill_effect_duration, 0.01f, 3.0f);
					}
					tab_end();

					tab_start("chelld", column_2, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Other"));
						draw_keybind(crypt_str("Thirdperson key"), &g_cfg.misc.thirdperson_toggle, crypt_str("##TPKEY__HOTKEY"));

						ImGui::Checkbox(crypt_str("Third person when dead"), &g_cfg.misc.thirdperson_when_spectating);

						if (g_cfg.misc.thirdperson_toggle.key > KEY_NONE && g_cfg.misc.thirdperson_toggle.key < KEY_MAX)
							ImGui::SliderInt(crypt_str("Third person distance"), &g_cfg.misc.thirdperson_distance, 50, 300);

						ImGui::SliderInt(crypt_str("Field of view"), &g_cfg.esp.fov, 0, 89);
						ImGui::Checkbox(crypt_str("Taser range"), &g_cfg.esp.taser_range);
						if (g_cfg.esp.taser_range)
						{
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##taser_range"), &g_cfg.esp.zeus_color, ALPHA);
							ImGui::Checkbox(crypt_str("Taser party"), &g_cfg.misc.zeusparty);
						}

						//ImGui::Checkbox(crypt_str("Velocity graph"), &g_cfg.esp.Velocity_graph);
						ImGui::Checkbox(crypt_str("Viewmodel in scope"), &g_cfg.esp.viewmodel_in_scope);
						ImGui::Checkbox(crypt_str("Holo panel"), &g_cfg.misc.holo_panel);

						ImGui::Text(crypt_str("Color Scope"));
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##color_scope"), &g_cfg.esp.new_scope_color, ALPHA);

						ImGui::Checkbox(crypt_str("Penetration crosshair"), &g_cfg.esp.penetration_reticle);
					}
					tab_end();

					tab_start("1332e", column_2, size_a_y + 10, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Visuals"));
						ImGui::Checkbox(crypt_str("Grenade prediction"), &g_cfg.esp.grenade_prediction);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##grenpredcolor"), &g_cfg.esp.grenade_prediction_color, ALPHA);

						if (g_cfg.esp.grenade_prediction)
						{
							ImGui::Checkbox(crypt_str("On click"), &g_cfg.esp.on_click);
							ImGui::Text(crypt_str("Tracer color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##tracergrenpredcolor"), &g_cfg.esp.grenade_prediction_tracer_color, ALPHA);
						}

						ImGui::Checkbox(crypt_str("Grenade projectiles"), &g_cfg.esp.grenade_proximity_warning);

						if (g_cfg.esp.grenade_proximity_warning)
						{
							ImGui::Checkbox(crypt_str("Rainbow tracer"), &g_cfg.esp.grenade_proximity_tracers_rainbow);
							ImGui::Checkbox(crypt_str("Offscreen warning"), &g_cfg.esp.offscreen_proximity);

							ImGui::Text(crypt_str("Tracer color"));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##grenade_color51235"), &g_cfg.esp.grenade_proximity_tracers_colors, ALPHA);

							ImGui::Text(crypt_str("Warning color"));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##grenade_color_warning#12"), &g_cfg.esp.grenade_proximity_warning_progress_color, ALPHA);

							ImGui::Text(crypt_str("Warning circle color"));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##grenade_color_warning#51"), &g_cfg.esp.grenade_proximity_warning_inner_color, ALPHA);
						}

						ImGui::Checkbox(crypt_str("Fire timer"), &g_cfg.esp.molotov_timer);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##molotovcolor"), &g_cfg.esp.molotov_timer_color, ALPHA);

						ImGui::Checkbox(crypt_str("Smoke timer"), &g_cfg.esp.smoke_timer);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##smokecolor"), &g_cfg.esp.smoke_timer_color, ALPHA);

						ImGui::Checkbox(crypt_str("Bomb indicator"), &g_cfg.esp.bomb_timer);
						draw_multicombo(crypt_str("Weapon ESP"), g_cfg.esp.weapon, weaponesp, ARRAYSIZE(weaponesp), preview);

						if (g_cfg.esp.weapon[WEAPON_ICON] || g_cfg.esp.weapon[WEAPON_TEXT] || g_cfg.esp.weapon[WEAPON_DISTANCE])
						{
							ImGui::Text(crypt_str("Color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##weaponcolor"), &g_cfg.esp.weapon_color, ALPHA);
						}

						if (g_cfg.esp.weapon[WEAPON_BOX])
						{
							ImGui::Text(crypt_str("Box color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##weaponboxcolor"), &g_cfg.esp.box_color, ALPHA);
						}

						if (g_cfg.esp.weapon[WEAPON_GLOW])
						{
							ImGui::Text(crypt_str("Glow color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##weaponglowcolor"), &g_cfg.esp.weapon_glow_color, ALPHA);
						}

						if (g_cfg.esp.weapon[WEAPON_AMMO])
						{
							ImGui::Text(crypt_str("Ammo bar color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##weaponammocolor"), &g_cfg.esp.weapon_ammo_color, ALPHA);
						}
					}
					tab_end();
				}
			}

			if (active_tab == 4)
			{
				if (miscc_tab1 == 0)
				{
					tab_start("misc#1", column_1, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("General"));

						ImGui::Checkbox(crypt_str("Anti-screenshot"), &g_cfg.misc.anti_screenshot);
						ImGui::Checkbox(crypt_str("Anti-untrusted"), &g_cfg.misc.anti_untrusted);
						ImGui::Checkbox(crypt_str("Rank reveal machmaking"), &g_cfg.misc.rank_reveal);
						ImGui::Checkbox(crypt_str("Auto accept machmaking"), &g_cfg.misc.auto_accept_matchmaking);
						ImGui::Checkbox(crypt_str("Unlock inventory access"), &g_cfg.misc.inventory_access);
						ImGui::Checkbox(crypt_str("Gravity ragdolls"), &g_cfg.misc.ragdolls);
						ImGui::Checkbox(crypt_str("Preserve killfeed"), &g_cfg.esp.preserve_killfeed);
						ImGui::Checkbox(crypt_str("sv_pure bypass"), &g_cfg.misc.sv_pure_bypass);

						// =-1-

						ImGui::SliderFloat(crypt_str("Aspect ratio amouth"), &g_cfg.misc.aspect_ratio_amount, 0.1f, 5.f);
					}
					tab_end();

					tab_start("misc#2", column_1, size_a_y + 10, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Movement"));

						ImGui::Checkbox(crypt_str("Automatic jump"), &g_cfg.misc.bunnyhop);
						draw_combo(crypt_str("Automatic strafes"), g_cfg.misc.airstrafe, strafes, ARRAYSIZE(strafes));
						ImGui::Checkbox(crypt_str("Crouch in air"), &g_cfg.misc.crouch_in_air);
						ImGui::Checkbox(crypt_str("Fast stop"), &g_cfg.misc.fast_stop);
						ImGui::Checkbox(crypt_str("Fast walk"), &g_cfg.misc.fast_walk);

						ImGui::Checkbox(crypt_str("Infinite duck"), &g_cfg.misc.noduck);

						if (g_cfg.misc.noduck)
							draw_keybind(crypt_str("Fake duck"), &g_cfg.misc.fakeduck_key, crypt_str("##FAKEDUCK__HOTKEY"));

						draw_keybind(crypt_str("Slow walk"), &g_cfg.misc.slowwalk_key, crypt_str("##SLOWWALK__HOTKEY"));
						draw_keybind(crypt_str("Auto peek"), &g_cfg.misc.automatic_peek, crypt_str("##AUTOPEEK__HOTKEY"));

						if (&g_cfg.misc.automatic_peek)
						{
							ImGui::Text(crypt_str("Auto peek color"));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##peek_color"), &g_cfg.misc.automatic_peek_color, ALPHA);
						}

						draw_keybind(crypt_str("Edge jump"), &g_cfg.misc.edge_jump, crypt_str("##EDGEJUMP__HOTKEY"));
					}
					tab_end();

					tab_start("misc#3", column_2, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Information"));

						ImGui::Checkbox(crypt_str("Watermark"), &g_cfg.menu.watermark);
						ImGui::Checkbox(crypt_str("Spectators list"), &g_cfg.misc.spectators_list);
						ImGui::Checkbox(crypt_str("Keybinds list"), &g_cfg.misc.keybinds);

						draw_combo(crypt_str("Hitsound"), g_cfg.esp.hitsound, sounds, ARRAYSIZE(sounds));

						draw_multicombo(crypt_str("Logs"), g_cfg.misc.events_to_log, events, ARRAYSIZE(events), preview);

						if (g_cfg.misc.events_to_log[EVENTLOG_HIT] || g_cfg.misc.events_to_log[EVENTLOG_ITEM_PURCHASES] || g_cfg.misc.events_to_log[EVENTLOG_BOMB])
						{
							draw_multicombo(crypt_str("Logs output"), g_cfg.misc.log_output, events_output, ARRAYSIZE(events_output), preview);

							ImGui::Text(crypt_str("Color "));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##logcolor"), &g_cfg.misc.log_color, ALPHA);
						}

						ImGui::Checkbox(crypt_str("Show CS:GO logs"), &g_cfg.misc.show_default_log);
					}
					tab_end();

					tab_start("misc#4", column_2, size_a_y + 10, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Spammer"));

						ImGui::Checkbox(crypt_str("Chat Spammer"), &g_cfg.misc.chat);
						ImGui::Checkbox(crypt_str("Clantag Spammer"), &g_cfg.misc.clantag_spammer);

						if (g_cfg.misc.clantag_spammer)
							draw_combo(crypt_str("Clantag changer"), g_cfg.misc.clantags_mode, clantags_mode, ARRAYSIZE(clantags_mode));
					}
					tab_end();

				}
				else if (miscc_tab1 == 1)
				{
					tab_start("misc#5", column_1, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Setup"));

						ImGui::Checkbox(crypt_str("OBS bypass"), &g_cfg.menu.obs_bypass);
						ImGui::Checkbox(crypt_str("Developer mode"), &g_cfg.scripts.developer_mode);

						draw_combo(crypt_str("DPI scale"), g_cfg.menu.size_menu, dpi, ARRAYSIZE(dpi));

						if (ImGui::CustomButton(crypt_str("unlock hidden convars"), crypt_str("##GAME__UNLOCK"), ImVec2(260 * dpi_s, 28 * dpi_scale)))
							misc::get().EnableHiddenCVars();

						if (ImGui::CustomButton(crypt_str("RAGE QUIT!"), crypt_str("##RAGE_QUIT"), ImVec2(260 * dpi_s, 28 * dpi_scale)))
							exit(0);
					}
					tab_end();

					tab_start("misc#7", column_2, 0, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Other"));

						
							ImGui::Checkbox(crypt_str("Anti-exploit"), &g_cfg.ragebot.anti_exploit);

						ImGui::Checkbox(crypt_str("Enable buybot"), &g_cfg.misc.buybot_enable);
						if (g_cfg.misc.buybot_enable)
						{
							draw_combo(crypt_str("Snipers"), g_cfg.misc.buybot1, mainwep, ARRAYSIZE(mainwep));
							padding(0, 3);
							draw_combo(crypt_str("Pistols"), g_cfg.misc.buybot2, secwep, ARRAYSIZE(secwep));
							padding(0, 3);
							draw_multicombo(crypt_str("Other"), g_cfg.misc.buybot3, grenades, ARRAYSIZE(grenades), preview);
						}
					}
					tab_end();

					tab_start("misc#8", column_1, size_a_y + 10, size_a_x, size_a_y);
					{
						ImGui::TabName(crypt_str("Viewmodel"));

						ImGui::TextColored(ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b()), crypt_str("Viewmodel"));
						ImGui::Spacing();
						ImGui::SliderInt(crypt_str("Viewmodel field of view"), &g_cfg.esp.viewmodel_fov, 0, 89);
						ImGui::SliderInt(crypt_str("Viewmodel X"), &g_cfg.esp.viewmodel_x, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel Y"), &g_cfg.esp.viewmodel_y, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel Z"), &g_cfg.esp.viewmodel_z, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel roll"), &g_cfg.esp.viewmodel_roll, -180, 180);
					}
					tab_end();
				}
			}
			if (active_tab == 5)
			{
				tab_start("lua#1", column_1, 0, size_a_x, size_d_y);
				{
					ImGui::TabName(crypt_str("Configs"));

					ImGui::PushItemWidth(220 * dpi_s);
					static auto should_update = true;

					if (should_update)
					{
						should_update = false;

						cfg_manager->config_files();
						files = cfg_manager->files;

						for (auto& current : files)
							if (current.size() > 2)
								current.erase(current.size() - 3, 3);
					}

					if (ImGui::CustomButton(crypt_str("Open configs folder"), crypt_str("##CONFIG__FOLDER"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						std::string folder;

						auto get_dir = [&folder]() -> void
						{
							static TCHAR path[MAX_PATH];

							if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
								folder = std::string(path) + crypt_str("\\itzlaith_lw\\configs\\");

							CreateDirectory(folder.c_str(), NULL);
						};

						get_dir();
						ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
					}

					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
					ImGui::ListBoxConfigArray(crypt_str("##CONFIGS"), &g_cfg.selected_config, files, 7);
					ImGui::PopStyleVar();

					if (ImGui::CustomButton(crypt_str("Refresh configs"), crypt_str("##CONFIG__REFRESH"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						cfg_manager->config_files();
						files = cfg_manager->files;

						for (auto& current : files)
							if (current.size() > 2)
								current.erase(current.size() - 3, 3);
					}

					static char config_name[64] = "\0";

					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
					ImGui::InputText(crypt_str("##configname"), config_name, sizeof(config_name));
					ImGui::PopStyleVar();

					if (ImGui::CustomButton(crypt_str("Create config"), crypt_str("##CONFIG__CREATE"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						g_cfg.new_config_name = config_name;
						add_config();
					}

					static auto next_save = false;
					static auto prenext_save = false;
					static auto clicked_sure = false;
					static auto save_time = m_globals()->m_realtime;
					static auto save_alpha = 1.0f;

					save_alpha = math::clamp(save_alpha + (4.f * ImGui::GetIO().DeltaTime * (!prenext_save ? 1.f : -1.f)), 0.01f, 1.f);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, save_alpha * public_alpha * (1.f - preview_alpha));

					ImGui::PopStyleVar();

					if (ImGui::CustomButton(crypt_str("Save config"), crypt_str("##CONFIG__SAVE"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
						save_config();
					if (ImGui::CustomButton(crypt_str("Load config"), crypt_str("##CONFIG__LOAD"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
						load_config();
					if (ImGui::CustomButton(crypt_str("Remove config"), crypt_str("##CONFIG__delete"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
						remove_config();

					ImGui::PopStyleVar();
					ImGui::PopItemWidth();
				}
				tab_end();

				auto player = players_section;

				static std::vector <Player_list_data> players;

				if (!g_cfg.player_list.refreshing)
				{
					players.clear();

					for (auto player : g_cfg.player_list.players)
						players.emplace_back(player);
				}

				static auto current_player = 0;

				tab_start("cfg#2", column_2, 0, size_a_x, size_d_y);
				{
					ImGui::TabName(crypt_str("Player list"));

					if (!players.empty())
					{
						std::vector <std::string> player_names;

						for (auto player : players)
							player_names.emplace_back(player.name);

						ImGui::PushItemWidth(291 * dpi_scale);
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
						ImGui::ListBoxConfigArray(crypt_str("##PLAYERLIST"), &current_player, player_names, 9);
						ImGui::PopStyleVar();
						ImGui::PopItemWidth();
					}

					if (!players.empty())
					{
						if (current_player >= players.size())
							current_player = players.size() - 1; //-V103

						ImGui::Checkbox(crypt_str("White list"), &g_cfg.player_list.white_list[players.at(current_player).i]); //-V106 //-V807

						if (!g_cfg.player_list.white_list[players.at(current_player).i]) //-V106
						{
							

							ImGui::Checkbox(crypt_str("High priority"), &g_cfg.player_list.high_priority[players.at(current_player).i]); //-V106
							ImGui::Checkbox(crypt_str("Force safe points"), &g_cfg.player_list.force_safe_points[players.at(current_player).i]); //-V106
							ImGui::Checkbox(crypt_str("Force body aim"), &g_cfg.player_list.force_body_aim[players.at(current_player).i]); //-V106


							
							


								//ImGui::Checkbox(crypt_str("Low delta"), &g_cfg.player_list.ml_modes[record_menu->curMode].ml_side[record_menu->curSide].low_delta[players.at(current_player).i]); //-V106
								//ImGui::Checkbox(crypt_str("Flip delta"), &g_cfg.player_list.ml_modes[record_menu->curMode].ml_side[record_menu->curSide].should_flip[players.at(current_player).i]); //-V106
							

							




						}

						ImGui::Checkbox(crypt_str("Override Resolver LowDelta Angle"), &g_cfg.player_list.set_cs_low); //-V106
						if (g_cfg.player_list.set_cs_low)
						{
							ImGui::SliderFloat(crypt_str("Custome Angle Amount"), &g_cfg.player_list.cs_low, 0.f, 60.f, crypt_str("%.0f%%"), 1.f);
						}
					}
					ImGui::EndChild();
				}
				ImGui::EndChild();
				tab_end();
			}

			if (active_tab == 6)
			{
				tab_start("lua#1", column_1, 0, size_a_x, size_d_y);
				{
					ImGui::TabName(crypt_str("Lua"));

					ImGui::PushItemWidth(220 * dpi_s);
					static auto should_update = true;

					if (should_update)
					{
						should_update = false;
						scripts = c_lua::get().scripts;

						for (auto& current : scripts)
						{
							if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
								current.erase(current.size() - 5, 5);
							else if (current.size() >= 4)
								current.erase(current.size() - 4, 4);
						}
					}

					if (ImGui::CustomButton(crypt_str("Open scripts folder"), crypt_str("##LUAS__FOLDER"), ImVec2(220 * dpi_s, 26)))
					{
						std::string folder;

						auto get_dir = [&folder]() -> void
						{
							static TCHAR path[MAX_PATH];

							if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
								folder = std::string(path) + crypt_str("\\itzlaith_lw\\scripts\\");

							CreateDirectory(folder.c_str(), NULL);
						};

						get_dir();
						ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
					}

					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

					if (scripts.empty())
						ImGui::ListBoxConfigArray(crypt_str("##LUAS"), &selected_script, scripts, 7);
					else
					{
						auto backup_scripts = scripts;

						for (auto& script : scripts)
						{
							auto script_id = c_lua::get().get_script_id(script + crypt_str(".lua"));

							if (script_id == -1)
								continue;

							if (c_lua::get().loaded.at(script_id))
								scripts.at(script_id) += crypt_str(" [loaded]");
						}

						ImGui::ListBoxConfigArray(crypt_str("##LUAS"), &selected_script, scripts, 7);
						scripts = std::move(backup_scripts);
					}

					ImGui::PopStyleVar();

					if (ImGui::CustomButton(crypt_str("Refresh scripts"), crypt_str("##LUA__REFRESH"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						c_lua::get().refresh_scripts();
						scripts = c_lua::get().scripts;

						if (selected_script >= scripts.size())
							selected_script = scripts.size() - 1; //-V103

						for (auto& current : scripts)
						{
							if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
								current.erase(current.size() - 5, 5);
							else if (current.size() >= 4)
								current.erase(current.size() - 4, 4);
						}
					}

					if (ImGui::CustomButton(crypt_str("Edit script"), crypt_str("##LUA__EDIT"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						loaded_editing_script = false;
						editing_script = scripts.at(selected_script);
					}

					if (ImGui::CustomButton(crypt_str("Load script"), crypt_str("##SCRIPTS__LOAD"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						c_lua::get().load_script(selected_script);
						c_lua::get().refresh_scripts();

						scripts = c_lua::get().scripts;

						if (selected_script >= scripts.size())
							selected_script = scripts.size() - 1; //-V103

						for (auto& current : scripts)
						{
							if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
								current.erase(current.size() - 5, 5);
							else if (current.size() >= 4)
								current.erase(current.size() - 4, 4);
						}

						eventlogs::get().add(crypt_str("Loaded ") + scripts.at(selected_script) + crypt_str(" script"), false); //-V106
					}

					if (ImGui::CustomButton(crypt_str("Unload script"), crypt_str("##SCRIPTS__UNLOAD"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						c_lua::get().unload_script(selected_script);
						c_lua::get().refresh_scripts();

						scripts = c_lua::get().scripts;

						if (selected_script >= scripts.size())
							selected_script = scripts.size() - 1; //-V103

						for (auto& current : scripts)
						{
							if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
								current.erase(current.size() - 5, 5);
							else if (current.size() >= 4)
								current.erase(current.size() - 4, 4);
						}

						eventlogs::get().add(crypt_str("Unloaded ") + scripts.at(selected_script) + crypt_str(" script"), false); //-V106
					}

					if (ImGui::CustomButton(crypt_str("Reload all scripts"), crypt_str("##SCRIPTS__RELOAD"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						c_lua::get().reload_all_scripts();
						c_lua::get().refresh_scripts();

						scripts = c_lua::get().scripts;

						if (selected_script >= scripts.size())
							selected_script = scripts.size() - 1; //-V103

						for (auto& current : scripts)
						{
							if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
								current.erase(current.size() - 5, 5);
							else if (current.size() >= 4)
								current.erase(current.size() - 4, 4);
						}
					}

					if (ImGui::CustomButton(crypt_str("Unload all scripts"), crypt_str("##SCRIPTS__UNLOADALL"), ImVec2(220 * dpi_s, 28 * dpi_scale)))
					{
						c_lua::get().unload_all_scripts();
						c_lua::get().refresh_scripts();

						scripts = c_lua::get().scripts;

						if (selected_script >= scripts.size())
							selected_script = scripts.size() - 1; //-V103

						for (auto& current : scripts)
						{
							if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
								current.erase(current.size() - 5, 5);
							else if (current.size() >= 4)
								current.erase(current.size() - 4, 4);
						}
					}

					ImGui::PopItemWidth();
				}
				tab_end();

				tab_start("lua#2", column_2, 0, size_a_x, size_d_y);
				{
					ImGui::TabName(crypt_str("Elements"));

					ImGui::Checkbox(crypt_str("Allow HTTP requests"), &g_cfg.scripts.allow_http);
					ImGui::Checkbox(crypt_str("Allow files read or write"), &g_cfg.scripts.allow_file);
					ImGui::Spacing();

					auto previous_check_box = false;

					for (auto& current : c_lua::get().scripts)
					{
						auto& items = c_lua::get().items.at(c_lua::get().get_script_id(current));

						for (auto& item : items)
						{
							std::string item_name;

							auto first_point = false;
							auto item_str = false;

							for (auto& c : item.first)
							{
								if (c == '.')
								{
									if (first_point)
									{
										item_str = true;
										continue;
									}
									else
										first_point = true;
								}

								if (item_str)
									item_name.push_back(c);
							}

							switch (item.second.type)
							{
							case NEXT_LINE:
								previous_check_box = false;
								break;
							case CHECK_BOX:
								previous_check_box = true;
								ImGui::Checkbox(item_name.c_str(), &item.second.check_box_value);
								break;
							case COMBO_BOX:
								previous_check_box = false;
								draw_combo(item_name.c_str(), item.second.combo_box_value, [](void* data, int idx, const char** out_text)
									{
										auto labels = (std::vector <std::string>*)data;
										*out_text = labels->at(idx).c_str(); //-V106
										return true;
									}, &item.second.combo_box_labels, item.second.combo_box_labels.size());
								break;
							case SLIDER_INT:
								previous_check_box = false;
								ImGui::SliderInt(item_name.c_str(), &item.second.slider_int_value, item.second.slider_int_min, item.second.slider_int_max);
								break;
							case SLIDER_FLOAT:
								previous_check_box = false;
								ImGui::SliderFloat(item_name.c_str(), &item.second.slider_float_value, item.second.slider_float_min, item.second.slider_float_max);
								break;
							case COLOR_PICKER:
								if (previous_check_box)
									previous_check_box = false;
								else
									ImGui::Text((item_name + ' ').c_str());

								ImGui::SameLine();
								ImGui::ColorEdit((crypt_str("##") + item_name).c_str(), &item.second.color_picker_value, ALPHA, true);
								break;
							}
						}
					}
					if (!editing_script.empty())
						lua_edit(editing_script);
				}
				tab_end();
			}

			ImGui::SetWindowSize(ImVec2(x, y));

			Style->ChildRounding = 0;
		}
		ImGui::EndChild();
	}

	ImGui::End();

	ImGui::PopStyleColor();
}