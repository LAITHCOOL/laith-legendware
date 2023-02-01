#pragma once

const char* AimType[] =
{
	"Hitbox",
	"Nearest hitbox"
};

const char* player_teams[] =
{
	"Enemy",
	"Team",
	"Local"
};

const char* LegitHitbox[] =
{
	"Head",
	"Neck",
	"Pelvis",
	"Stomach",
	"Lower chest",
	"Chest",
	"Upper chest"
};

const char* LegitSelection[] =
{
	"Near crosshair",
	"Lowest health",
	"Highest damage",
	"Lowest distance"
};

const char* antiaim_type[] =
{
	"Rage",
	"Legit"
};

const char* resolver_mode[] =
{
	"AIR",
	"SLOW_WALKING",
	"MOVING",
	"STANDING",
	"FREESTANDING"
};

const char* legs[] =
{
	"disabled",
	"default",
	"breaking"
};

const char* movement_type[] =
{
	"Stand",
	"Slow walk",
	"Move",
	"Air"
};

const char* condition_switch[] =
{
	"Stand",
	"Move",
	"Slow walk",
	"Air"
};

const char* LegitFov[] =
{
	"Static",
	"Dynamic"
};

const char* LegitSmooth[] =
{
	"Static",
	"Humanized"
};

const char* RCSType[] =
{
	"Always on",
	"On target"
};

const char* selection[] =
{
	"Cycle",
	"Near crosshair",
	"Lowest distance",
	"Lowest health",
	"Highest damage"
};
const char* autoscope_type[] =
{
	"Regular",
	"Dynamic",
};

const char* safe_points_conditions[] =
{
	"On lethal (soon)",
	"Visible",
	"In air",
	"In crouch",
	"After x misses (soon)",
	"If hp < x (soon)",
	"On limbs"
};
const char* bodyaimcond[] =
{
	"Double tap",
	"Prefer",
	"lethal"
};

const char* headaimcond[] =
{
	"On shot",
	"Running"
};

const char* headaimonlycond[] =
{
	"On shot",
	"Running"
};

const char* autostop_modifiers[] =
{
	"Between shots",
	"On lethal",
	"Only Visible",
	"Only Center",
	"Force accuracy"
};

const char* hitboxes[] =
{
	"Head",
	"Upper chest",
	"Chest",
	"Lower chest",
	"Stomach",
	"Pelvis",
	"Arms",
	"Legs",
};

const char* multipoint_hitboxes[] =
{
	"Head",
	"Chest",
	"Stomach",
	"Legs",
	"Feet"
};


const char* m_hitboxes[] =
{
	"Head",
	"Upper chest",
	"Chest",
	"Lower chest",
	"Stomach",
	"Pelvis",
	"Arms",
	"Legs",
	"Feet"
};

const char* pitch[] =
{
	"None",
	"Down",
	"Up",
	"Zero",
	"Random",
	"Jitter",
	"Fake down",
	"Fake up",
	"Fake jitter"
};


const char* beams[] =
{
"blueglow1",
"bubble",
"glow01",
"physbeam",
"purpleglow1",
"purplelaser1",
"radio",
"white",
"line (neverlose)",
};


const char* dt_type[] =
{
"Delayed",
"Defensive",
"Instant",
"Custome",
};


const char* yaw[] =
{
	"Static",
	"Jitter",
	"Spin"
};


const char* baseangle[] =
{
	"Local view",
	"At targets"
};

const char* desync[] =
{
	"None",
	"Static",
	"Jitter"
};

const char* lby_type[] =
{
	"Normal",
	"Opposite",
	"Sway",
	"Extended"
};
const char* proj_combo[] =
{
	"Icon",
	"Text",
	"Box",
	"Glow"
};

const char* weaponplayer[] =
{
	"Icon",
	"Text"
};

const char* weaponesp[] =
{
	"Icon",
	"Text",
	"Box",
	"Distance",
	"Glow",
	"Ammo"
};

const char* hitmarkers[] =
{
	"Crosshair",
	"World"
};

const char* glowtarget[] =
{
	"Enemy",
	"Teammate",
	"Local"
};

const char* glowtype[] =
{
	"Standard",
	"Pulse",
	"Inner"
};

const char* local_chams_type[] =
{
	"Real",
	"Desync"
};

const char* chamsvisact[] =
{
	"Visible",
	"Invisible"
};

const char* chamsvis[] =
{
	"Visible"
};

const char* chamstype[] =
{
	"Regular",
	"Metallic",
	"Flat",
	"Pulse",
	"Crystal",
	"Glass",
	"Circuit",
	"Wide glow",
	"Animated",
	"Velvet",
	"Wirerame"
};

const char* lag_type[] =
{
	"Regular",
	"Metallic",
	"Flat",
	"Pulse",
	"Crystal",
	"Glass",
	"Circuit",
	"Wide glow",
	"Animated",
	"Velvet",
	"Wirerame"
};

const char* flags[] =
{
	"Money",
	"Armor",
	"Have kits",
	"In scope",
	"Fakeduck",
	"Vulnerable",
	"Flashed",
	"Exploit",
	"Delay",
	"Bomb carrier"
};

const char* removals[] =
{
	"Scope",
	"Zoom",
	"Smoke",
	"Flash",
	"Recoil",
	"Landing bob",
	"Postprocessing",
	"Fog"
};

const char* indicators[] =
{
	"Fake amount",
	"Break LC",
	"Fake-lags",
	"Damage override",
	"Safe points",
	"Body aim",
	"Double tap",
	"Hide shots",
	"Anti-exploit"
};

const char* skybox[] =
{
	"None",
	"Tibet",
	"Baggage",
	"Italy",
	"Aztec",
	"Vertigo",
	"Daylight",
	"Daylight 2",
	"Clouds",
	"Clouds 2",
	"Gray",
	"Clear",
	"Canals",
	"Cobblestone",
	"Assault",
	"Clouds dark",
	"Night",
	"Night 2",
	"Night flat",
	"Dusty",
	"Rainy",
	"Custom"
};

const char* mainwep[] =
{
	"None",
	"Auto",
	"AWP",
	"SSG 08"
};

const char* secwep[] =
{
	"None",
	"Dual Berettas",
	"Deagle/Revolver",
	"Tec-9/Five-SeveN"
};

const char* strafes[] =
{
	"None",
	"Legit",
	"Directional",
	"Rage"
};

const char* events_output[] =
{
	"Console",
	"Chat"
};

const char* events[] =
{
	"Player hits",
	"Items",
	"Bomb logs"
};

const char* grenades[] =
{
	"Grenades",
	"Armor",
	"Taser",
	"Defuser"
};

const char* freestand[] =
{
	"Reverse",
	"Default"
};

const char* fakelags[] =
{
	"Static",
	"Random",
	"Dynamic",
	"Adaptive"
};

const char* lagstrigger[] =
{
	"Slow walk",
	"Move",
	"Air",
	"Peek"
};

const char* sounds[] =
{
	"None",
	"Mettalic",
	"Cod",
	"Bubble",
	"Stapler",
	"Bell",
	"Flick"
};

const char* player_model_t[] =
{
	"None",
	"Getaway Sally | The Professionals",
	"Number K | The Professionals",
	"Little Kev | The Professionals",
	"Safecracker Voltzmann | The Professionals",
	"Bloody Darryl The Strapped | The Professionals",
	"Sir Bloody Loudmouth Darryl | The Professionals",
	"Sir Bloody Darryl Royale | The Professionals",
	"Sir Bloody Skullhead Darryl | The Professionals",
	"Sir Bloody Silent Darryl | The Professionals",
	"Sir Bloody Miami Darryl | The Professionals",
	"Street Soldier | Phoenix",
	"Soldier | Phoenix",
	"Slingshot | Phoenix",
	"Enforcer | Phoenix",
	"Mr. Muhlik | Elite Crew",
	"Prof. Shahmat | Elite Crew",
	"Osiris | Elite Crew",
	"Ground Rebel | Elite Crew",
	"The Elite Mr. Muhlik | Elite Crew",
	"Trapper | Guerrilla Warfare",
	"Trapper Aggressor | Guerrilla Warfare",
	"Vypa Sista of the Revolution | Guerrilla Warfare",
	"Col. Mangos Dabisi | Guerrilla Warfare",
	"Arno The Overgrown | Guerrilla Warfare",
	"Medium Rare' Crasswater | Guerrilla Warfare",
	"Crasswater The Forgotten | Guerrilla Warfare",
	"Elite Trapper Solman | Guerrilla Warfare",
	"The Doctor' Romanov | Sabre",
	"Blackwolf | Sabre",
	"Maximus | Sabre",
	"Dragomir | Sabre",
	"Rezan The Ready | Sabre",
	"Rezan the Redshirt | Sabre",
	"Dragomir | Sabre Footsoldier",
};

const char* player_model_ct[] =
{
	"None",
	"Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman",
	"Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman",
	"Lieutenant Rex Krikey | SEAL Frogman",
	"Michael Syfers | FBI Sniper",
	"Operator | FBI SWAT",
	"Special Agent Ava | FBI",
	"Markus Delrow | FBI HRT",
	"Sous-Lieutenant Medic | Gendarmerie Nationale",
	"Chem-Haz Capitaine | Gendarmerie Nationale",
	"Chef d'Escadron Rouchard | Gendarmerie Nationale",
	"Aspirant | Gendarmerie Nationale",
	"Officer Jacques Beltram | Gendarmerie Nationale",
	"D Squadron Officer | NZSAS",
	"B Squadron Officer | SAS",
	"Seal Team 6 Soldier | NSWC SEAL",
	"Buckshot | NSWC SEAL",
	"Lt. Commander Ricksaw | NSWC SEAL",
	"Blueberries' Buckshot | NSWC SEAL",
	"3rd Commando Company | KSK",
	"Two Times' McCoy | TACP Cavalry",
	"Two Times' McCoy | USAF TACP",
	"Primeiro Tenente | Brazilian 1st Battalion",
	"Cmdr. Mae 'Dead Cold' Jamison | SWAT",
	"1st Lieutenant Farlow | SWAT",
	"John 'Van Healen' Kask | SWAT",
	"Bio-Haz Specialist | SWAT",
	"Sergeant Bombson | SWAT",
	"Chem-Haz Specialist | SWAT",
	"Lieutenant 'Tree Hugger' Farlow | SWAT"
};

const char* clantags_mode[] =
{
	"itzlaith_lw",
	"iliafedboy",
};

const char* dpi[] =
{
	"100 %",
	"150 %",
	"200 %"
};
