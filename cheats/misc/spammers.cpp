#include "spammers.h"

void spammers::clan_tag()
{
    auto apply = [](const char* tag) -> void
    {
        using Fn = int(__fastcall*)(const char*, const char*);
        static auto fn = reinterpret_cast<Fn>(util::FindSignature(crypt_str("engine.dll"), crypt_str("53 56 57 8B DA 8B F9 FF 15")));

        fn(tag, tag);
    };

    static auto removed = false;

    if (!g_cfg.misc.clantag_spammer && !removed)
    {
        removed = true;
        apply(crypt_str(""));
        return;
    }

    auto nci = m_engine()->GetNetChannelInfo();

    if (!nci)
        return;

    static auto time = -1;

    auto ticks = TIME_TO_TICKS(nci->GetAvgLatency(FLOW_OUTGOING)) + (float)m_globals()->m_tickcount; //-V807
    auto intervals = 0.5f / m_globals()->m_intervalpertick;

    if (g_cfg.misc.clantag_spammer && g_cfg.misc.clantags_mode == 0)
    {
        auto main_time = (int)(ticks / intervals) % 6;

        if (main_time != time && !m_clientstate()->iChokedCommands)
        {
            auto tag = crypt_str("");

            switch (main_time)
            {
            case 0:
                tag = crypt_str("itzlaith_lw"); //>V1037
                break;
            case 1:
                tag = crypt_str("h3nta1war5"); //>V1037
                break;
            case 2:
                tag = crypt_str("h5n&ai#are"); //>V1037
                break;
            case 3:
                tag = crypt_str("h3n&4iw4re"); //>V1037
                break;
            case 4:
                tag = crypt_str("#e|\\|t4iwar5");
                break;
            case 5:
                tag = crypt_str("#$%&^w4r5"); //>V1037
                break;
            case 6:
                tag = crypt_str("hentai%$|^5");
                break;
            }

            apply(tag);
            time = main_time;
        }
        removed = false;
    }
    else if (g_cfg.misc.clantag_spammer && g_cfg.misc.clantags_mode == 1)
    {
        auto main_time = (int)(ticks / intervals) % 16;

        if (main_time != time && !m_clientstate()->iChokedCommands)
        {
            auto tag = crypt_str("");

            switch (main_time)
            {
            case 0:  
                tag = crypt_str("         ");
                break;
            case 1:  
                tag = crypt_str("F/       "); 
                break;
            case 2:  
                tag = crypt_str("F3\\      "); 
                break;
            case 3:  
                tag = crypt_str("FeD/     "); 
                break;
            case 4:  
                tag = crypt_str("F3DB\\    ");
                break;
            case 5:  
                tag = crypt_str("FeDB0/   "); 
                break;
            case 6:  
                tag = crypt_str("FEDB0Y/  "); 
                break;
            case 7:  
                tag = crypt_str("FeDBoY   "); 
                break;
            case 8:  
                tag = crypt_str("F3DB0Y\\  "); 
                break;
            case 9: 
                tag = crypt_str("FeDBo\\   "); 
                break;
            case 10:
                tag = crypt_str("F3DB\\    "); 
                break;
            case 11:
                tag = crypt_str("FeD\\     "); 
                break;
            case 12:
                tag = crypt_str("F3\\      ");
                break;
            case 13:
                tag = crypt_str("F\\       "); 
                break;
            case 14:
                tag = crypt_str("\\       "); 
                break;
            case 15:
                tag = crypt_str("         ");
                break;
            }

            apply(tag);
            time = main_time;
        }
        removed = false;
    }
}