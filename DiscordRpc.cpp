#include "DiscordRpc.h"
#include <DiscordRPCSDK/Includes/discord_rpc.h>

void Discord::Initialize()
{
    DiscordEventHandlers Handle;
    memset(&Handle, 0, sizeof(Handle));
    Discord_Initialize("921751575247085609", &Handle, 1, NULL);
}

void Discord::Update()
{
    DiscordRichPresence discord;
    memset(&discord, 0, sizeof(discord));
    discord.details = "Playing CS:GO";
    discord.state = "With itzlaith_lw";
    discord.largeImageKey = "image2";
    discord.smallImageKey = "image1";
    Discord_UpdatePresence(&discord);
}