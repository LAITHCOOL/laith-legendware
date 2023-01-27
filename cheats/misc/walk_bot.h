#pragma once

#include <vector>
#include "../../includes.hpp"

class walkbot : public singleton <walkbot>
{
public:
	void update(bool showdebug = false);
	void move(CUserCmd* cmd);
	void clear_dots();
	bool prishel = false;
	int marker = 0;
	void file(std::string addictive_name, int todo);
private:
	void refresh(std::string levelName, std::vector<Vector> spots, bool save);
	bool button_code = false;
	bool button_clicked = false;
	bool button_down = false;
	std::string mapname;
	std::vector<Vector> dots;
	int teamnum;
};

enum files_walkbot {
	file_overwrite,
	file_load,
	file_nothing
};