#include "..\..\includes.hpp"


class tickbase : public singleton <tickbase>
{
public:

	void DoubleTap(CUserCmd* m_pcmd);
	bool CanDoubleTap(bool check_charge);
	void HideShots(CUserCmd* m_pcmd);
	void double_tap_deffensive(CUserCmd* m_pcmd);
	void lagexploit(CUserCmd* m_pcmd);

	bool recharging_double_tap = false;


	bool double_tap_enabled = false;
	bool double_tap_key = false;
	bool double_tap_checkc = false;

	bool bDidPeek = false;
	int lastdoubletaptime = 0;
	bool hide_shots_enabled = false;
	bool hide_shots_key = false;

};