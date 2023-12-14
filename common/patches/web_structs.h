/*	EQEMu: Everquest Server Emulator

	Copyright (C) 2001-2016 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// Share this struct file across c bridge and make compatible for c/c++
#ifdef __cplusplus
namespace Web
{
	namespace structs
	{
		static const uint32 BUFF_COUNT = 25;
		static const uint32 ADVENTURE_COLLECT = 0;
		static const uint32 ADVENTURE_MASSKILL = 1;
		static const uint32 ADVENTURE_NAMED = 2;
		static const uint32 ADVENTURE_RESCUE = 3;
		static const uint32 MAX_PP_DISCIPLINES = 100;
		static const uint32 MAX_PLAYER_TRIBUTES = 5;
		static const uint32 TRIBUTE_NONE = 0xFFFFFFFF;
		static const uint32 MAX_GROUP_LEADERSHIP_AA_ARRAY = 16;
		static const uint32 MAX_RAID_LEADERSHIP_AA_ARRAY = 16;
		static const uint32 MAX_LEADERSHIP_AA_ARRAY = (MAX_GROUP_LEADERSHIP_AA_ARRAY + MAX_RAID_LEADERSHIP_AA_ARRAY);
		static const uint32 MAX_PP_LANGUAGE = 28;
		static const uint32 MAX_PP_SKILL = 100; // 100 - actual skills buffer size
		static const uint32 MAX_PP_INNATE_SKILL = 25;
		static const uint32 MAX_PP_AA_ARRAY = 240;
		static const uint32 MAX_GROUP_MEMBERS = 6;
		static const uint32 MAX_RECAST_TYPES = 20;
		static const uint32 COINTYPE_PP = 3;
		static const uint32 COINTYPE_GP = 2;
		static const uint32 COINTYPE_SP = 1;
		static const uint32 COINTYPE_CP = 0;
		static const uint32 MAX_NUMBER_GUILDS = 1500;
		static const uint32 MAX_TRIBUTE_TIERS = 10;
#endif

		struct LoginInfo_Struct
		{
			char *name;
			char *password;
			uint8 zoning; // 01 if zoning, 00 if not
		};

		struct EnterWorld_Struct
		{
			char name[64];
			uint32 tutorial;	// 01 on "Enter Tutorial", 00 if not
			uint32 return_home; // 01 on "Return Home", 00 if not
		};

		// yep, even tit had a version of the new inventory system, used by OP_MoveMultipleItems
		struct InventorySlot_Struct
		{
			int32 Type; // Worn and Normal inventory = 0, Bank = 1, Shared Bank = 2, Trade = 3, World = 4, Limbo = 5
			int32 Slot;
			int32 SubIndex; // no aug index in Tit
		};

		// unsure if they have a version of this, completeness though
		struct TypelessInventorySlot_Struct
		{
			int32 Slot;
			int32 SubIndex; // no aug index in Tit
		};

		struct NameApproval
		{
			char name[64];
			uint32 race;
			uint32 class_;
			uint32 deity;
		};
		
		/*
		** Entity identification struct
		** Size: 4 bytes
		** OPCodes: OP_DeleteSpawn, OP_Assist
		*/
		struct EntityId_Struct
		{
			uint32 entity_id;
		};

		struct Duel_Struct
		{
			uint32 duel_initiator;
			uint32 duel_target;
		};

		struct DuelResponse_Struct
		{
			uint32 target_id;
			uint32 entity_id;
		};

		struct AdventureInfo
		{
			uint32 QuestID;
			uint32 NPCID;
			bool in_use;
			uint32 status;
			bool ShowCompass;
			uint32 Objetive;	  // can be item to collect,mobs to kill,boss to kill and someone to rescue.
			uint32 ObjetiveValue; // number of items,or number of needed mob kills.
			char text[512];
			uint8 type;
			uint32 minutes;
			uint32 points;
			float x;
			float y;
			uint32 zoneid;
			uint32 zonedungeonid;
		};
		///////////////////////////////////////////////////////////////////////////////

		/*
		** Color_Struct
		** Size: 4 bytes
		** Used for convenience
		** Merth: Gave struct a name so gcc 2.96 would compile
		**
		*/
		struct Tint_Struct
		{
			uint8 blue;
			uint8 green;
			uint8 red;
			uint8 use_tint; // if there's a tint this is FF
		};

		struct TextureProfile
		{
			uint32 head;
			uint32 chest;
			uint32 arms;
			uint32 wrist;
			uint32 hands;
			uint32 legs;
			uint32 feet;
			uint32 primary;
			uint32 secondary;
		};

		struct TintProfile
		{
			struct Tint_Struct head;
			struct Tint_Struct chest;
			struct Tint_Struct arms;
			struct Tint_Struct wrist;
			struct Tint_Struct hands;
			struct Tint_Struct legs;
			struct Tint_Struct feet;
			struct Tint_Struct primary;
			struct Tint_Struct secondary;
		};

		struct CharSelectEquip_Struct
		{
			uint32 material;
			struct Tint_Struct color;
		};
		struct CharacterSelectEntry_Struct
		{
			char name[64];
			uint8 char_class;
			uint32 race;
			uint8 level;
			uint16 zone;
			uint16 instance;
			uint8 gender;
			uint8 face;
			struct CharSelectEquip_Struct equip[9];
			uint32 deity;
			uint32 primary_id_file;
			uint32 secondary_id_file;
			uint8 go_home;	// Seen 0 for new char and 1 for existing
			uint8 enabled;	// Originally labeled as 'CharEnabled' - unknown purpose and setting
			uint32 last_login;
			struct CharacterSelectEntry_Struct* next;
		};


		struct CharacterSelect_Struct
		{
			uint32 character_count;
			struct CharacterSelectEntry_Struct* characters;
		};

		/*
		** Generic Spawn Struct
		** Length: 383 Octets
		** Used in:
		**   spawnZoneStruct
		**   dbSpawnStruct
		**   petStruct
		**   newSpawnStruct
		*/
		/*
		showeq -> eqemu
		sed -e 's/_t//g' -e 's/seto_0xFF/set_to_0xFF/g'
		*/
		struct Spawn_Struct
		{

			uint8 gm; // 0=no, 1=gm

			uint8 aaitle; // 0=none, 1=general, 2=archtype, 3=class

			uint8 anon;	   // 0=normal, 1=anon, 2=roleplay
			uint8 face;	   // Face id for players
			char name[64]; // Player's Name
			uint16 deity;  // Player's Deity

			float size; // Model size

			uint8 NPC;		 // 0=player,1=npc,2=pc corpse,3=npc corpse,a
			uint8 invis;	 // Invis (0=not, 1=invis)
			uint8 haircolor; // Hair color
			uint8 curHp;	 // Current hp %%% wrong
			uint8 max_hp;	 // (name prolly wrong)takes on the value 100 for players, 100 or 110 for NPCs and 120 for PC corpses...
			uint8 findable;	 // 0=can't be found, 1=can be found

			uint16 deltaHeading; // change in heading
			uint16 x;			 // x coord
			uint16 y;			 // y coord
			uint16 animation;	 // animation
			uint16 z;			 // z coord
			uint16 deltaY;		 // change in y
			uint16 deltaX;		 // change in x
			uint16 heading;		 // heading
			uint16 deltaZ;		 // change in z
			uint8 eyecolor1;	 // Player's left eye color

			uint8 showhelm; // 0=no, 1=yes

			uint8 is_npc;	  // 0=no, 1=yes
			uint16 hairstyle; // Hair style
			uint8 beardcolor; // Beard color

			uint8 level;		// Spawn Level
			uint32 PlayerState; // PlayerState controls some animation stuff
			uint8 beard;		// Beard style
			char suffix[32];	// Player's suffix (of Veeshan, etc.)
			uint32 petOwnerId;	// If this is a pet, the spawn id of owner
			uint8 guildrank;	// 0=normal, 1=officer, 2=leader

			struct TextureProfile equipment;
			float runspeed; // Speed when running
			uint8 afk;		// 0=no, 1=afk
			uint32 guildID; // Current guild
			char title[32]; // Title

			uint8 helm;			  // Helm texture
			uint8 set_to_0xFF[8]; // ***Placeholder (all ff)
			uint32 race;		  // Spawn race

			char lastName[32]; // Player's Lastname
			float walkspeed;   // Speed when walking

			uint8 is_pet;	 // 0=no, 1=yes
			uint8 light;	 // Spawn's lightsource %%% wrong
			uint8 class_;	 // Player's class
			uint8 eyecolor2; // Left eye color
			uint8 flymode;
			uint8 gender;	// Gender (0=male, 1=female)
			uint8 bodytype; // Bodytype

			union
			{
				uint8 equip_chest2; // Second place in packet for chest texture (usually 0xFF in live packets)
									// Not sure why there are 2 of them, but it effects chest texture!
				uint8 mount_color;	// drogmor: 0=white, 1=black, 2=green, 3=red
									// horse: 0=brown, 1=white, 2=black, 3=tan
			};
			uint32 spawnId;		   // Spawn Id
			float bounding_radius; // used in melee, overrides calc
			struct TintProfile equipment_tint;
			uint8 lfg; // 0=off, 1=lfg on
		};

		/*
		** New Spawn
		** Length: 176 Bytes
		** OpCode: 4921
		*/
		struct NewSpawn_Struct
		{
			struct Spawn_Struct spawn; // Spawn Information
		};

		struct ClientZoneEntry_Struct
		{

			char char_name[64]; // Character Name
		};

		/*
		** Server Zone Entry Struct
		** Length: 452 Bytes
		** OPCodes: OP_ServerZoneEntry
		**
		*/
		struct ServerZoneEntry_Struct
		{
			struct NewSpawn_Struct player;
		};

		struct NewZone_Struct
		{
			char char_name[64];		  // Character Name
			char zone_short_name[32]; // Zone Short Name
			char zone_long_name[278]; // Zone Long Name
			uint8 ztype;			  // Zone type (usually FF)
			uint8 fog_red[4];		  // Zone fog (red)
			uint8 fog_green[4];		  // Zone fog (green)
			uint8 fog_blue[4];		  // Zone fog (blue)

			float fog_minclip[4];
			float fog_maxclip[4];
			float gravity;
			uint8 time_type;
			uint8 rain_chance[4];
			uint8 rain_duration[4];
			uint8 snow_chance[4];
			uint8 snow_duration[4];

			uint8 sky; // Sky Type

			float zone_exp_multiplier; // Experience Multiplier
			float safe_y;			   // Zone Safe Y
			float safe_x;			   // Zone Safe X
			float safe_z;			   // Zone Safe Z
			float max_z;			   // Guessed
			float underworld;		   // Underworld, min z (Not Sure?)
			float minclip;			   // Minimum View Distance
			float maxclip;			   // Maximum View DIstance

			char zone_short_name2[68];

			uint16 zone_id;
			uint16 zone_instance;
		};

		/*
		** Memorize Spell Struct
		** Length: 12 Bytes
		**
		*/
		struct MemorizeSpell_Struct
		{
			uint32 slot;	  // Spot in the spell book/memorized slot
			uint32 spell_id;  // Spell id (200 or c8 is minor healing, etc)
			uint32 scribing;  // 1 if memorizing a spell, set to 0 if scribing to book, 2 if un-memming
			uint32 reduction; // lowers reuse
		};

		/*
		** Make Charmed Pet
		** Length: 12 Bytes
		**
		*/
		struct Charm_Struct
		{
			uint32 owner_id;
			uint32 pet_id;
			uint32 command; // 1: make pet, 0: release pet
		};

		struct InterruptCast_Struct
		{
			uint32 spawnid;
			uint32 messageid;
			char message[0];
		};

		struct DeleteSpell_Struct
		{
			int16 spell_slot;

			uint8 success;
		};
		struct ManaChange_Struct
		{
			uint32 new_mana; // New Mana AMount
			uint32 stamina;
			uint32 spell_id;
			uint8 keepcasting; // won't stop the cast. Change mana while casting?
			uint8 padding[3];  // client doesn't read it, garbage data seems like
		};

		struct SwapSpell_Struct
		{
			uint32 from_slot;
			uint32 to_slot;
		};

		struct BeginCast_Struct
		{
			// len = 8
			uint16 caster_id;
			uint16 spell_id;
			uint32 cast_time; // in miliseconds
		};

		struct CastSpell_Struct
		{
			uint32 slot;
			uint32 spell_id;
			uint32 inventoryslot; // slot for clicky item, 0xFFFF = normal cast
			uint32 target_id;
		};

		/*
		** SpawnAppearance_Struct
		** Changes client appearance for all other clients in zone
		** Size: 8 bytes
		** Used in: OP_SpawnAppearance
		**
		*/
		struct SpawnAppearance_Struct
		{
			uint16 spawn_id;  // ID of the spawn
			uint16 type;	  // Values associated with the type
			uint32 parameter; // Type of data sent
		};

		// this is used inside profile
		struct SpellBuff_Struct
		{
			uint8 effect_type; // 0 = no buff, 2 = buff, 4 = inverse affects of buff
			uint8 level;
			uint8 bard_modifier;

			uint32 spellid;
			int32 duration;
			uint32 counters;  // single book keeping value (counters, rune/vie)
			uint32 player_id; // caster ID, pretty sure just zone ID
		};

		struct SpellBuffPacket_Struct
		{
			uint32 entityid;
			struct SpellBuff_Struct buff;
			uint32 slotid;
			uint32 bufffade;
		};

		struct ItemNamePacket_Struct
		{
			uint32 item_id;
			uint32 unkown004;
			char name[64];
		};

		// Length: 10
		struct ItemProperties_Struct
		{

			uint8 charges;
		};

		struct GMTrainee_Struct
		{
			uint32 npcid;
			uint32 playerid;
			uint32 skills[100];
		};

		struct GMTrainEnd_Struct
		{
			uint32 npcid;
			uint32 playerid;
		};

		struct GMSkillChange_Struct
		{
			uint16 npcid;

			uint16 skillbank; // 0 if normal skills, 1 if languages

			uint16 skill_id;
		};
		struct ConsentResponse_Struct
		{
			char grantname[64];
			char ownername[64];
			uint8 permission;
			char zonename[32];
		};

		/*
		** Name Generator Struct
		** Length: 72 bytes
		** OpCode: 0x0290
		*/
		struct NameGeneration_Struct
		{
			uint32 race;
			uint32 gender;
			char name[64];
		};

		/*
		** Character Creation struct
		** Length: 140 Bytes
		** OpCode: 0x0113
		*/
		struct CharCreate_Struct
		{
			uint32 class_;
			uint32 haircolor;  // Might be hairstyle
			uint32 beardcolor; // Might be beard
			uint32 beard;	   // Might be beardcolor
			uint32 gender;
			uint32 race;
			uint32 start_zone;
			// 0 = odus
			// 1 = qeynos
			// 2 = halas
			// 3 = rivervale
			// 4 = freeport
			// 5 = neriak
			// 6 = gukta/grobb
			// 7 = ogguk
			// 8 = kaladim
			// 9 = gfay
			// 10 = felwithe
			// 11 = akanon
			// 12 = cabalis
			// 13 = shar vahl
			uint32 hairstyle; // Might be haircolor
			uint32 deity;
			uint32 STR;
			uint32 STA;
			uint32 AGI;
			uint32 DEX;
			uint32 WIS;
			uint32 INT;
			uint32 CHA;
			uint32 face;
			uint32 eyecolor1; // its possiable we could have these switched
			uint32 eyecolor2; // since setting one sets the other we really can't check
			uint32 tutorial;
		};

		/*
		 *Used in PlayerProfile
		 */
		struct AA_Array
		{
			uint32 AA;
			uint32 value;
		};

		struct Disciplines_Struct
		{
			uint32 values[100];
		};

		struct Tribute_Struct
		{
			uint32 tribute;
			uint32 tier;
		};

		// Bandolier item positions
		enum
		{
			bandolierPrimary = 0,
			bandolierSecondary,
			bandolierRange,
			bandolierAmmo
		};

		// len = 72
		struct BandolierItem_Struct
		{
			uint32 ID;
			uint32 Icon;
			char Name[64];
		};

		// len = 320
		struct Bandolier_Struct
		{
			char Name[32];
			struct BandolierItem_Struct Items[4];
		};

		// len = 72
		struct PotionBeltItem_Struct
		{
			uint32 ID;
			uint32 Icon;
			char Name[64];
		};

		// len = 288
		struct PotionBelt_Struct
		{
			struct PotionBeltItem_Struct Items[4];
		};

		struct GroupLeadershipAA_Struct
		{
			union
			{
				struct
				{
					uint32 groupAAMarkNPC;
					uint32 groupAANPCHealth;
					uint32 groupAADelegateMainAssist;
					uint32 groupAADelegateMarkNPC;
					uint32 groupAA4;
					uint32 groupAA5;
					uint32 groupAAInspectBuffs;
					uint32 groupAA7;
					uint32 groupAASpellAwareness;
					uint32 groupAAOffenseEnhancement;
					uint32 groupAAManaEnhancement;
					uint32 groupAAHealthEnhancement;
					uint32 groupAAHealthRegeneration;
					uint32 groupAAFindPathToPC;
					uint32 groupAAHealthOfTargetsTarget;
					uint32 groupAA15;
				};
				uint32 ranks[16];
			};
		};

		struct RaidLeadershipAA_Struct
		{
			union
			{
				struct
				{
					uint32 raidAAMarkNPC;
					uint32 raidAANPCHealth;
					uint32 raidAADelegateMainAssist;
					uint32 raidAADelegateMarkNPC;
					uint32 raidAA4;
					uint32 raidAA5;
					uint32 raidAA6;
					uint32 raidAASpellAwareness;
					uint32 raidAAOffenseEnhancement;
					uint32 raidAAManaEnhancement;
					uint32 raidAAHealthEnhancement;
					uint32 raidAAHealthRegeneration;
					uint32 raidAAFindPathToPC;
					uint32 raidAAHealthOfTargetsTarget;
					uint32 raidAA14;
					uint32 raidAA15;
				};
				uint32 ranks[16];
			};
		};

		struct LeadershipAA_Struct
		{
			union
			{
				struct
				{
					struct GroupLeadershipAA_Struct group;
					struct RaidLeadershipAA_Struct raid;
				};
				uint32 ranks[32];
			};
		};

		/**
		 * A bind point.
		 * Size: 20 Octets
		 */
		struct BindStruct
		{
			uint32 zone_id;
			float x;
			float y;
			float z;
			float heading;
		};

		/*
		** Player Profile
		**
		** Length: 4308 bytes
		** OpCode: 0x006a
		*/

		struct PVPStatsEntry_Struct
		{
			char Name[64];
			uint32 Level;
			uint32 Race;
			uint32 Class;
			uint32 Zone;
			uint32 Time;
			uint32 Points;
		};

		struct PlayerProfile_Struct
		{
			uint32 checksum; //
			uint32 gender;	 // Player Gender - 0 Male, 1 Female
			uint32 race;	 // Player race
			uint32 class_;	 // Player class

			uint8 level;  // Level of player
			uint8 level1; // Level of player (again?)

			struct BindStruct binds[5]; // Bind points (primary is first)
			uint32 deity;				// deity
			uint32 intoxication;		// Alcohol level (in ticks till sober?)
			uint32 spellSlotRefresh[9]; // Refresh time (millis)
			uint32 abilitySlotRefresh;
			uint8 haircolor;  // Player hair color
			uint8 beardcolor; // Player beard color
			uint8 eyecolor1;  // Player left eye color
			uint8 eyecolor2;  // Player right eye color
			uint8 hairstyle;  // Player hair style
			uint8 beard;	  // Player beard type

			struct TextureProfile item_material; // Item texture/material of worn items

			struct TintProfile item_tint;  // RR GG BB 00
			struct AA_Array aa_array[240]; // AAs
			uint32 points;				   // Unspent Practice points
			uint32 mana;				   // Current mana
			uint32 cur_hp;				   // Current HP without +HP equipment
			uint32 STR;					   // Strength
			uint32 STA;					   // Stamina
			uint32 CHA;					   // Charisma
			uint32 DEX;					   // Dexterity
			uint32 INT;					   // Intelligence
			uint32 AGI;					   // Agility
			uint32 WIS;					   // Wisdom
			uint8 face;					   // Player face

			uint32 spell_book[400]; // List of the Spells in spellbook

			uint32 mem_spells[9]; // List of spells memorized

			uint32 platinum;		// Platinum Pieces on player
			uint32 gold;			// Gold Pieces on player
			uint32 silver;			// Silver Pieces on player
			uint32 copper;			// Copper Pieces on player
			uint32 platinum_cursor; // Platinum Pieces on cursor
			uint32 gold_cursor;		// Gold Pieces on cursor
			uint32 silver_cursor;	// Silver Pieces on cursor
			uint32 copper_cursor;	// Copper Pieces on cursor
			uint32 skills[100];		// [400] List of skills	// 100 dword buffer
			uint32 InnateSkills[25];

			uint32 toxicity;					   // Potion Toxicity (15=too toxic, each potion adds 3)
			uint32 thirst_level;				   // Drink (ticks till next drink)
			uint32 hunger_level;				   // Food (ticks till next eat)
			struct SpellBuff_Struct buffs[25];	   // Buffs currently on the player
			struct Disciplines_Struct disciplines; // Known disciplines
			uint32 recastTimers[20];			   // Timers (GMT of last use)

			uint32 endurance;	   // Current endurance
			uint32 aapoints_spent; // Number of spent AA points
			uint32 aapoints;	   // Unspent AA points

			struct Bandolier_Struct bandoliers[4]; // bandolier contents

			struct PotionBelt_Struct potionbelt; // potion belt

			uint32 available_slots;

			char name[64];		  // Name of player
			char last_name[32];	  // Last name of player
			uint32 guild_id;	  // guildid
			uint32 birthday;	  // character birthday
			uint32 lastlogin;	  // character last save time
			uint32 timePlayedMin; // time character played
			uint8 pvp;			  // 1=pvp, 0=not pvp
			uint8 anon;			  // 2=roleplay, 1=anon, 0=not anon
			uint8 gm;			  // 0=no, 1=yes (guessing!)
			uint8 guildrank;	  // 0=member, 1=officer, 2=guildleader
			uint32 guildbanker;

			uint32 exp; // Current Experience

			uint32 timeentitledonaccount; // In days, displayed in /played
			uint8 languages[28];		  // List of languages

			float x;	   // Players x position
			float y;	   // Players y position
			float z;	   // Players z position
			float heading; // Players heading

			uint32 platinum_bank;	// Platinum Pieces in Bank
			uint32 gold_bank;		// Gold Pieces in Bank
			uint32 silver_bank;		// Silver Pieces in Bank
			uint32 copper_bank;		// Copper Pieces in Bank
			uint32 platinum_shared; // Shared platinum pieces

			uint32 expansions; // Bitmask for expansions

			uint32 autosplit; // 0 = off, 1 = on

			uint16 zone_id;			  // see zones.h
			uint16 zoneInstance;	  // Instance id
			char groupMembers[6][64]; // all the members in group, including self
			char groupLeader[64];	  // Leader of the group ?

			uint32 entityid;
			uint32 leadAAActive; // 0 = leader AA off, 1 = leader AA on

			int32 ldon_points_guk;		 // Earned GUK points
			int32 ldon_points_mir;		 // Earned MIR points
			int32 ldon_points_mmc;		 // Earned MMC points
			int32 ldon_points_ruj;		 // Earned RUJ points
			int32 ldon_points_tak;		 // Earned TAK points
			int32 ldon_points_available; // Available LDON points

			uint32 tribute_time_remaining; // Time remaining on tribute (millisecs)
			uint32 career_tribute_points;  // Total favor points for this char

			uint32 tribute_points; // Current tribute points

			uint32 tribute_active;			   // 0 = off, 1=on
			struct Tribute_Struct tributes[5]; // Current tribute loadout

			double group_leadership_exp;
			double raid_leadership_exp;
			uint32 group_leadership_points;				 // Unspent group lead AA points
			uint32 raid_leadership_points;				 // Unspent raid lead AA points
			struct LeadershipAA_Struct leader_abilities; // Leader AA ranks

			uint32 air_remaining; // Air supply (seconds)
			uint32 PVPKills;
			uint32 PVPDeaths;
			uint32 PVPCurrentPoints;
			uint32 PVPCareerPoints;
			uint32 PVPBestKillStreak;
			uint32 PVPWorstDeathStreak;
			uint32 PVPCurrentKillStreak;
			struct PVPStatsEntry_Struct PVPLastKill;
			struct PVPStatsEntry_Struct PVPLastDeath;
			uint32 PVPNumberOfKillsInLast24Hours;
			struct PVPStatsEntry_Struct PVPRecentKills[50];
			uint32 expAA; // Exp earned in current AA point

			uint32 currentRadCrystals;	// Current count of radiant crystals
			uint32 careerRadCrystals;	// Total count of radiant crystals ever
			uint32 currentEbonCrystals; // Current count of ebon crystals
			uint32 careerEbonCrystals;	// Total count of ebon crystals ever
			uint8 groupAutoconsent;		// 0=off, 1=on
			uint8 raidAutoconsent;		// 0=off, 1=on
			uint8 guildAutoconsent;		// 0=off, 1=on

			uint32 level3;
			uint32 showhelm; // 0=no, 1=yes
		};

		/*
		** Client Target Struct
		** Length: 2 Bytes
		** OpCode: 6221
		*/
		struct ClientTarget_Struct
		{
			uint32 new_target; // Target ID
		};

		/*
		** Target Rejection Struct
		** Length: 12 Bytes
		** OpCode: OP_TargetReject
		*/
		struct TargetReject_Struct
		{
		};

		struct PetCommand_Struct
		{
			uint32 command;
			uint32 target;
		};

		/*
		** Delete Spawn
		** Length: 4 Bytes
		** OpCode: OP_DeleteSpawn
		*/
		struct DeleteSpawn_Struct
		{
			uint32 spawn_id; // Spawn ID to delete
		};

		/*
		** Channel Message received or sent
		** Length: 144 Bytes + Variable Length + 1
		** OpCode: OP_ChannelMessage
		**
		*/
		struct ChannelMessage_Struct
		{
			char targetname[64]; // Tell recipient
			char sender[64];	 // The senders name (len might be wrong)
			uint32 language;	 // Language
			uint32 chan_num;	 // Channel

			uint32 skill_in_language; // The players skill in this language? might be wrong
			char message[0];		  // Variable length message
		};

		struct SpecialMesg_Struct
		{
			char header[3];			// 04 04 00 <-- for #emote style msg
			uint32 msg_type;		// Color of text (see MT_*** below)
			uint32 target_spawn_id; // Who is it being said to?
			char sayer[1];			// Who is the source of the info
			char message[1];		// What is being said?
		};

		/*
		** When somebody changes what they're wearing
		**      or give a pet a weapon (model changes)
		** Length: 16 Bytes
		** Opcode: 9220
		*/
		struct WearChange_Struct
		{
			uint16 spawn_id;
			uint16 material;
			struct Tint_Struct color;
			uint8 wear_slot_id;
		};

		/*
		** Type:   Bind Wound Structure
		** Length: 8 Bytes
		*/
		// Fixed for 7-14-04 patch
		struct BindWound_Struct
		{
			uint16 to; // TargetID

			uint16 type;
		};

		/*
		** Type:   Zone Change Request (before hand)
		** Length: 88 bytes
		** OpCode: a320
		*/

		struct ZoneChange_Struct
		{
			char char_name[64]; // Character Name
			uint16 zoneID;
			uint16 instanceID;
			float y;
			float x;
			float z;
			uint32 zone_reason; // 0x0A == death, I think
			int32 success;		// =0 client->server, =1 server->client, -X=specific error
		};

		struct RequestClientZoneChange_Struct
		{
			uint16 zone_id;
			uint16 instance_id;
			float y;
			float x;
			float z;
			float heading;
			uint32 type;
		};

		struct Animation_Struct
		{
			uint16 spawnid;
			uint8 speed;
			uint8 action;
		};

		// this is what causes the caster to animate and the target to
		// get the particle effects around them when a spell is cast
		// also causes a buff icon
		struct Action_Struct
		{
			uint16 target;		   // id of target
			uint16 source;		   // id of caster
			uint16 level;		   // level of caster for spells, OSX dump says attack rating, guess spells use it for level
			uint32 instrument_mod; // OSX dump says base damage, spells use it for bard song (different from newer clients)
			float force;
			float hit_heading;
			float hit_pitch;
			uint8 type; // 231 (0xE7) for spells, skill

			uint16 spell; // spell id being cast
			uint8 spell_level;
			// this field seems to be some sort of success flag, if it's 4
			uint8 effect_flag; // if this is 4, a buff icon is made
		};

		// this is what prints the You have been struck. and the regular
		// melee messages like You try to pierce, etc.  It's basically the melee
		// and spell damage message
		struct CombatDamage_Struct
		{
			uint16 target;
			uint16 source;
			uint8 type; // slashing, etc.  231 (0xE7) for spells, skill
			uint16 spellid;
			uint32 damage;
			float force;
			float hit_heading; // see above notes in Action_Struct
			float hit_pitch;
		};

		/*
		** Consider Struct
		** Length: 24 Bytes
		** OpCode: 3721
		*/
		struct Consider_Struct
		{
			uint32 playerid; // PlayerID
			uint32 targetid; // TargetID
			uint32 faction;	 // Faction
			uint32 level;	 // Level
			int32 cur_hp;	 // Current Hitpoints
			int32 max_hp;	 // Maximum Hitpoints
			uint8 pvpcon;	 // Pvp con flag 0/1
		};

		/*
		** Spawn Death Blow
		** Length: 32 Bytes
		** OpCode: 0114
		*/
		struct Death_Struct
		{
			uint32 spawn_id;
			uint32 killer_id;
			uint32 corpseid;	 // was corpseid
			uint32 attack_skill; // was type
			uint32 spell_id;
			uint32 bindzoneid; // bindzoneid?
			uint32 damage;
		};

		struct BecomeCorpse_Struct
		{
			uint32 spawn_id;
			float y;
			float x;
			float z;
		};

		/*
		** Spawn position update
		**	struct sent from server->client to update position of
		**	another spawn's position update in zone (whether NPC or PC)
		**
		*/
		struct PlayerPositionUpdateServer_Struct
		{
			uint16 spawn_id;
			int16 delta_heading; // change in heading
			int16 x_pos;		 // x coord
			int16 y_pos;		 // y coord
			int16 animation;	 // animation
			int16 z_pos;		 // z coord
			int16 delta_y;		 // change in y
			int16 delta_x;		 // change in x
			int16 heading;		 // heading
			int32 delta_z;		 // change in z
		};

		/*
		** Player position update
		**	struct sent from client->server to update
		**	player position on server
		**
		*/
		struct PlayerPositionUpdateClient_Struct
		{
			uint16 spawn_id;
			uint16 sequence;	 // increments one each packet
			float y_pos;		 // y coord
			float delta_z;		 // Change in z
			float delta_x;		 // Change in x
			float delta_y;		 // Change in y
			int16 animation;	 // animation
			int16 delta_heading; // change in heading
			float x_pos;		 // x coord
			float z_pos;		 // z coord
			uint16 heading;		 // Directional heading
		};

		/*
		** Spawn HP Update
		** Length: 10 Bytes
		** OpCode: OP_HPUpdate
		*/
		struct SpawnHPUpdate_Struct
		{
			uint32 cur_hp;	// Id of spawn to update
			int32 max_hp;	// Maximum hp of spawn
			int16 spawn_id; // Current hp of spawn
		};
		struct SpawnHPUpdate_Struct2
		{
			int16 spawn_id;
			uint8 hp;
		};
		/*
		** Stamina
		** Length: 8 Bytes
		** OpCode: 5721
		*/
		struct Stamina_Struct
		{
			uint32 food;  // (low more hungry 127-0)
			uint32 water; // (low more thirsty 127-0)
		};

		/*
		** Level Update
		** Length: 12 Bytes
		*/
		struct LevelUpdate_Struct
		{
			uint32 level;	  // New level
			uint32 level_old; // Old level
			uint32 exp;		  // Current Experience
		};

		/*
		** Experience Update
		** Length: 14 Bytes
		** OpCode: 9921
		*/
		struct ExpUpdate_Struct
		{
			uint32 exp;	 // Current experience ratio from 0 to 330
			uint32 aaxp; // @BP ??
		};

		/*
		** Item Packet Struct - Works on a variety of opcodes
		** Packet Types: See ItemPacketType enum
		**
		*/
		enum ItemPacketType
		{
			ItemPacketViewLink = 0x00,
			ItemPacketTradeView = 0x65,
			ItemPacketLoot = 0x66,
			ItemPacketTrade = 0x67,
			ItemPacketCharInventory = 0x69,
			ItemPacketSummonItem = 0x6A,
			ItemPacketTributeItem = 0x6C,
			ItemPacketMerchant = 0x64,
			ItemPacketWorldContainer = 0x6B
		};
		struct ItemPacket_Struct
		{
			enum ItemPacketType PacketType;
			char SerializedItem[1];
		};

		struct BulkItemPacket_Struct
		{
			char SerializedItem[0];
		};

		struct Consume_Struct
		{
			uint32 slot;
			uint32 auto_consumed; // 0xffffffff when auto eating e7030000 when right click

			uint8 type; // 0x01=Food 0x02=Water
		};

		struct DeleteItem_Struct
		{
			uint32 from_slot;
			uint32 to_slot;
			uint32 number_in_stack;
		};

		struct MoveItem_Struct
		{
			uint32 from_slot;
			uint32 to_slot;
			uint32 number_in_stack;
		};

		struct MultiMoveItemSub_Struct
		{
			struct InventorySlot_Struct from_slot;
			uint32 number_in_stack; // so the amount we are moving from the source
			struct InventorySlot_Struct to_slot;
		};

		struct MultiMoveItem_Struct
		{
			uint32 count;
			struct MultiMoveItemSub_Struct moves[0];
		};

		//
		// from_slot/to_slot
		// -1 - destroy
		//  0 - cursor
		//  1 - inventory
		//  2 - bank
		//  3 - trade
		//  4 - shared bank
		//
		// cointype
		// 0 - copeer
		// 1 - silver
		// 2 - gold
		// 3 - platinum
		//

		struct MoveCoin_Struct
		{
			int32 from_slot;
			int32 to_slot;
			int32 cointype1;
			int32 cointype2;
			int32 amount;
		};
		struct TradeCoin_Struct
		{
			uint32 trader;
			uint8 slot;

			uint32 amount;
		};
		struct TradeMoneyUpdate_Struct
		{
			uint32 trader;
			uint32 type;
			uint32 amount;
		};
		/*
		** Surname struct
		** Size: 100 bytes
		*/
		struct Surname_Struct
		{
			char name[64];

			char lastname[32];
		};

		struct GuildsListEntry_Struct
		{
			char name[64];
		};

		struct GuildsList_Struct
		{
			uint8 head[64]; // First on guild list seems to be empty...
			struct GuildsListEntry_Struct Guilds[1500];
		};

		struct GuildUpdate_Struct
		{
			uint32 guildID;
			struct GuildsListEntry_Struct entry;
		};

		/*
		** Money Loot
		** Length: 22 Bytes
		** OpCode: 5020
		*/
		struct moneyOnCorpseStruct
		{
			uint8 response; // 0 = someone else is, 1 = OK, 2 = not at this time

			uint32 platinum; // Platinum Pieces
			uint32 gold;	 // Gold Pieces

			uint32 silver; // Silver Pieces
			uint32 copper; // Copper Pieces
		};

		// opcode = 0x5220
		//  size 292

		struct LootingItem_Struct
		{
			uint32 lootee;
			uint32 looter;
			uint16 slot_id;

			int32 auto_loot;
		};

		struct GuildManageStatus_Struct
		{
			uint32 guildid;
			uint32 oldrank;
			uint32 newrank;
			char name[64];
		};
		// Guild invite, remove
		struct GuildJoin_Struct
		{
			uint32 guildid;

			uint32 level;
			uint32 class_;
			uint32 rank; // 0 member, 1 officer, 2 leader
			uint32 zoneid;

			char name[64];
		};
		struct GuildInviteAccept_Struct
		{
			char inviter[64];
			char newmember[64];
			uint32 response;
			uint32 guildeqid;
		};
		struct GuildManageRemove_Struct
		{
			uint32 guildeqid;
			char member[64];
		};
		struct GuildCommand_Struct
		{
			char othername[64];
			char myname[64];
			uint16 guildeqid;

			uint32 officer;
		};

		// 4244 bytes. Is not really an 'OnLevelMessage', it causes a popup box to display in the client
		// Text looks like HTML.
		struct OnLevelMessage_Struct
		{
			char Title[128];
			char Text[4096];
			uint32 Buttons;
			uint32 Duration;
			uint32 PopupID;
		};

		// Opcode OP_GMZoneRequest
		// Size = 88 bytes
		struct GMZoneRequest_Struct
		{
			char charname[64];
			uint32 zone_id;
			float x;
			float y;
			float z;
			float heading;
			uint32 success; // 0 if command failed, 1 if succeeded?

			//		int8	success;		// =0 client->server, =1 server->client, -X=specific error
		};

		struct GMSummon_Struct
		{
			char charname[64];
			char gmname[64];
			uint32 success;
			uint32 zoneID;
			int32 y;
			int32 x;
			int32 z;
		};

		struct GMGoto_Struct
		{ // x,y is swapped as compared to summon and makes sense as own packet
			char charname[64];

			char gmname[64];
			uint32 success;
			uint32 zoneID;

			int32 y;
			int32 x;
			int32 z;
		};

		struct GMLastName_Struct
		{
			char name[64];
			char gmname[64];
			char lastname[64];

			// 0x01, 0x00 = Update the clients
		};

		// Combat Abilities
		struct CombatAbility_Struct
		{
			uint32 m_target; // the ID of the target mob
			uint32 m_atk;
			uint32 m_skill;
		};

		// Instill Doubt
		struct Instill_Doubt_Struct
		{
			uint8 i_id;

			uint8 i_atk;

			uint8 i_type;
		};

		struct GiveItem_Struct
		{
			uint16 to_entity;
			int16 to_equipSlot;
			uint16 from_entity;
			int16 from_equipSlot;
		};

		struct RandomReq_Struct
		{
			uint32 low;
			uint32 high;
		};

		struct RandomReply_Struct
		{
			uint32 low;
			uint32 high;
			uint32 result;
			char name[64];
		};

		struct LFG_Struct
		{
			/*
			Wrong size on OP_LFG. Got: 80, Expected: 68
			   0: 00 00 00 00 01 00 00 00 - 00 00 00 00 64 00 00 00  | ............d...
			  16: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  32: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  48: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  64: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			Wrong size on OP_LFG. Got: 80, Expected: 68
			   0: 00 00 00 00 01 00 00 00 - 3F 00 00 00 41 00 00 00  | ........?...A...
			  16: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  32: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  48: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  64: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			Wrong size on OP_LFG. Got: 80, Expected: 68
			   0: 00 00 00 00 01 00 00 00 - 3F 00 00 00 41 00 00 00  | ........?...A...
			  16: 46 72 75 62 20 66 72 75 - 62 20 66 72 75 62 00 00  | Frub frub frub..
			  32: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  48: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			  64: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
			*/

			uint32 value; // 0x00 = off 0x01 = on

			char name[64];
		};

		/*
		** LFG_Appearance_Struct
		** Packet sent to clients to notify when someone in zone toggles LFG flag
		** Size: 8 bytes
		** Used in: OP_LFGAppearance
		**
		*/
		struct LFG_Appearance_Struct
		{
			uint32 spawn_id; // ID of the client
			uint8 lfg;		 // 1=LFG, 0=Not LFG
		};

		// EverQuest Time Information:
		// 72 minutes per EQ Day
		// 3 minutes per EQ Hour
		// 6 seconds per EQ Tick (2 minutes EQ Time)
		// 3 seconds per EQ Minute

		struct TimeOfDay_Struct
		{
			uint8 hour;
			uint8 minute;
			uint8 day;
			uint8 month;
			uint32 year;
		};

		// Darvik: shopkeeper structs
		struct Merchant_Click_Struct
		{
			uint32 npcid; // Merchant NPC's entity id
			uint32 playerid;
			uint32 command; // 1=open, 0=cancel/close
			float rate;		// cost multiplier, dosent work anymore
		};
		/*

		0 is e7 from 01 to // MAYBE SLOT IN PURCHASE
		1 is 03
		2 is 00
		3 is 00
		4 is ??
		5 is ??
		6 is 00 from a0 to
		7 is 00 from 3f to */
		/*
		0 is F6 to 01
		1 is CE CE
		4A 4A
		00 00
		00 E0
		00 CB
		00 90
		00 3F
		*/

		struct Merchant_Sell_Struct
		{
			uint32 npcid;	 // Merchant NPC's entity id
			uint32 playerid; // Player's entity id
			uint32 itemslot;

			uint32 quantity;
			uint32 price;
		};
		struct Merchant_Purchase_Struct
		{
			uint32 npcid;	 // Merchant NPC's entity id
			uint32 itemslot; // Player's entity id
			uint32 quantity;
			uint32 price;
		};
		struct Merchant_DelItem_Struct
		{
			uint32 npcid;	 // Merchant NPC's entity id
			uint32 playerid; // Player's entity id
			uint32 itemslot;
		};
		struct Adventure_Purchase_Struct
		{
			uint32 some_flag; // set to 1 generally...
			uint32 npcid;
			uint32 itemid;
			uint32 variable;
		};

		struct Adventure_Sell_Struct
		{

			uint32 npcid;
			uint32 slot;
			uint32 charges;
			uint32 sell_price;
		};

		struct AdventurePoints_Update_Struct
		{
			uint32 ldon_available_points; // Total available points
			uint8 unkown_apu004[20];
			uint32 ldon_guk_points;		  // Earned Deepest Guk points
			uint32 ldon_mirugal_points;	  // Earned Mirugal' Mebagerie points
			uint32 ldon_mistmoore_points; // Earned Mismoore Catacombs Points
			uint32 ldon_rujarkian_points; // Earned Rujarkian Hills points
			uint32 ldon_takish_points;	  // Earned Takish points
		};

		struct AdventureFinish_Struct
		{
			uint32 win_lose; // Cofruben: 00 is a lose,01 is win.
			uint32 points;
		};
		// OP_AdventureRequest
		struct AdventureRequest_Struct
		{
			uint32 risk; // 1 normal,2 hard.
			uint32 entity_id;
		};
		struct AdventureRequestResponse_Struct
		{

			char text[2048];
			uint32 timetoenter;
			uint32 timeleft;
			uint32 risk;
			float x;
			float y;
			float z;
			uint32 showcompass;
		};

		/*struct Item_Shop_Struct {
			uint16 merchantid;
			uint8 itemtype;
			Item_struct item;

		};*/

		struct Illusion_Struct
		{
			uint32 spawnid;
			char charname[64];
			int race;
			uint8 gender;
			uint8 texture;
			uint8 helmtexture;

			uint32 face;
			uint8 hairstyle;
			uint8 haircolor;
			uint8 beard;
			uint8 beardcolor;
			float size;
		};

		struct ZonePoint_Entry
		{
			uint32 iterator;
			float y;
			float x;
			float z;
			float heading;
			uint16 zoneid;
			uint16 zoneinstance; // LDoN instance
		};

		struct ZonePoints
		{
			uint32 count;
			struct ZonePoint_Entry zpe[0]; // Always add one extra to the end after all zonepoints
		};

		struct SkillUpdate_Struct
		{
			uint32 skillId;
			uint32 value;
		};

		struct ZoneUnavail_Struct
		{
			// This actually varies, but...
			char zonename[16];
		};

		struct GroupGeneric_Struct
		{
			char name1[64];
			char name2[64];
		};

		struct GroupCancel_Struct
		{
			char name1[64];
			char name2[64];
			uint8 toggle;
		};

		struct GroupUpdate_Struct
		{
			uint32 action;
			char yourname[64];
			char membername[5][64];
			char leadersname[64];
		};

		struct GroupUpdate2_Struct
		{
			uint32 action;
			char yourname[64];
			char membername[5][64];
			char leadersname[64];
			struct GroupLeadershipAA_Struct leader_aas;
		};
		struct GroupJoin_Struct
		{
			uint32 action;
			char yourname[64];
			char membername[64];
		};

		struct FaceChange_Struct
		{
			uint8 haircolor;
			uint8 beardcolor;
			uint8 eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
			uint8 eyecolor2;
			uint8 hairstyle;
			uint8 beard; // vesuvias
			uint8 face;
			// vesuvias:
			// there are only 10 faces for barbs changing woad just
			// increase the face value by ten so if there were 8 woad
			// designs then there would be 80 barb faces
		};

		/*
		** Trade request from one client to another
		** Used to initiate a trade
		** Size: 8 bytes
		** Used in: OP_TradeRequest
		*/
		struct TradeRequest_Struct
		{
			uint32 to_mob_id;
			uint32 from_mob_id;
		};

		struct TradeAccept_Struct
		{
			uint32 from_mob_id;
		};

		/*
		** Cancel Trade struct
		** Sent when a player cancels a trade
		** Size: 8 bytes
		** Used In: OP_CancelTrade
		**
		*/
		struct CancelTrade_Struct
		{
			uint32 fromid;
			uint32 action;
		};

		struct PetitionUpdate_Struct
		{
			uint32 petnumber; // Petition Number
			uint32 color;	  // 0x00 = green, 0x01 = yellow, 0x02 = red
			uint32 status;
			time_t senttime; // 4 has to be 0x1F
			char accountid[32];
			char gmsenttoo[64];
			int32 quetotal;
			char charname[64];
		};

		struct Petition_Struct
		{
			uint32 petnumber;
			uint32 urgency;
			char accountid[32];
			char lastgm[32];
			uint32 zone;
			// char zone[32];
			char charname[64];
			uint32 charlevel;
			uint32 charclass;
			uint32 charrace;

			// time_t senttime; // Time?
			uint32 checkouts;
			uint32 unavail;

			time_t senttime;

			char petitiontext[1024];
			char gmtext[1024];
		};

		struct Who_All_Struct
		{ // 76 length total
			char whom[64];
			uint32 wrace; // FF FF = no race

			uint32 wclass;	 // FF FF = no class
			uint32 lvllow;	 // FF FF = no numbers
			uint32 lvlhigh;	 // FF FF = no numbers
			uint32 gmlookup; // FF FF = not doing /who all gm
		};

		struct Stun_Struct
		{					 // 4 bytes total
			uint32 duration; // Duration of stun
		};

		struct AugmentItem_Struct
		{
			int16 container_slot;

			int32 augment_slot;
		};

		// OP_Emote
		struct Emote_Struct
		{

			char message[1024];
		};

		// Inspect
		struct Inspect_Struct
		{
			uint32 TargetID;
			uint32 PlayerID;
		};
		// OP_InspectAnswer
		struct InspectResponse_Struct
		{ // Cofruben:need to send two of this for the inspect response.
			uint32 TargetID;
			uint32 playerid;
			char itemnames[22][64];
			uint32 itemicons[22];
			char text[288];
		};

		// OP_SetDataRate
		struct SetDataRate_Struct
		{
			float newdatarate;
		};

		// OP_SetServerFilter
		struct SetServerFilter_Struct
		{
			uint32 filters[29]; // see enum eqFilterType
		};

		// Op_SetServerFilterAck
		struct SetServerFilterAck_Struct
		{
			uint8 blank[8];
		};
		struct IncreaseStat_Struct
		{

			uint8 str;
			uint8 sta;
			uint8 agi;
			uint8 dex;
			uint8 int_;
			uint8 wis;
			uint8 cha;
			uint8 fire;
			uint8 cold;
			uint8 magic;
			uint8 poison;
			uint8 disease;

			uint8 str2;
			uint8 sta2;
			uint8 agi2;
			uint8 dex2;
			uint8 int_2;
			uint8 wis2;
			uint8 cha2;
			uint8 fire2;
			uint8 cold2;
			uint8 magic2;
			uint8 poison2;
			uint8 disease2;
		};

		struct GMName_Struct
		{
			char oldname[64];
			char gmname[64];
			char newname[64];
			uint8 badname;
		};

		struct GMDelCorpse_Struct
		{
			char corpsename[64];
			char gmname[64];
		};

		struct GMKick_Struct
		{
			char name[64];
			char gmname[64];
		};

		struct GMKill_Struct
		{
			char name[64];
			char gmname[64];
		};

		struct GMEmoteZone_Struct
		{
			char text[512];
		};

		// This is where the Text is sent to the client.
		// Use ` as a newline character in the text.
		// Variable length.
		struct BookText_Struct
		{
			uint8 window;	  // where to display the text (0xFF means new window)
			uint8 type;		  // type: 0=scroll, 1=book, 2=item info.. prolly others.
			char booktext[1]; // Variable Length
		};
		// This is the request to read a book.
		// This is just a "text file" on the server
		// or in our case, the 'name' column in our books table.
		struct BookRequest_Struct
		{
			uint8 window;	 // where to display the text (0xFF means new window)
			uint8 type;		 // type: 0=scroll, 1=book, 2=item info.. prolly others.
			char txtfile[1]; // Variable length
		};

		/*
		** Object/Ground Spawn struct
		** Used for Forges, Ovens, ground spawns, items dropped to ground, etc
		** Size: 92 bytes
		** OpCodes: OP_CreateObject
		** Last Updated: Oct-17-2003
		**
		*/
		struct Object_Struct
		{
			uint32 linked_list_addr[2]; // They are, get this, prev and next, ala linked list

			uint32 drop_id;		  // Unique object id for zone
			uint16 zone_id;		  // Redudant, but: Zone the object appears in
			uint16 zone_instance; //

			float heading;		  // heading
			float z;			  // z coord
			float x;			  // x coord
			float y;			  // y coord
			char object_name[32]; // Name of object, usually something like IT63_ACTORDEF

			// ShowEQ shows an extra field in here...
			uint32 object_type; // Type of object, not directly translated to OP_OpenObject

			uint32 spawn_id; // Spawn Id of client interacting with object
		};
		// 01 = generic drop, 02 = armor, 19 = weapon
		//[13:40] and 0xff seems to be indicative of the tradeskill/openable items that end up returning the old style item type in the OP_OpenObject

		/*
		** Click Object Struct
		** Client clicking on zone object (forge, groundspawn, etc)
		** Size: 8 bytes
		** Last Updated: Oct-17-2003
		**
		*/
		struct ClickObject_Struct
		{
			uint32 drop_id;
			uint32 player_id;
		};

		struct Shielding_Struct
		{
			uint32 target_id;
		};

		/*
		** Click Object Acknowledgement Struct
		** Response to client clicking on a World Container (ie, forge)
		**
		*/
		struct ClickObjectAck_Struct
		{
			uint32 player_id; // Entity Id of player who clicked object
			uint32 drop_id;	  // Zone-specified unique object identifier
			uint32 open;	  // 1=opening, 0=closing
			uint32 type;	  // See object.h, "Object Types"

			uint32 icon; // Icon to display for tradeskill containers

			char object_name[64]; // Object name to display
		};

		/*

		**
		*/
		struct CloseContainer_Struct
		{
			uint32 player_id; // Entity Id of player who clicked object
			uint32 drop_id;	  // Zone-specified unique object identifier
			uint32 open;	  // 1=opening, 0=closing
		};

		/*
		** Generic Door Struct
		** Length: 52 Octets
		** Used in:
		**    cDoorSpawnsStruct(f721)
		**
		*/
		struct Door_Struct
		{
			char name[32];
			float yPos; // y loc
			float xPos; // x loc
			float zPos; // z loc
			float heading;
			uint32 incline; // rotates the whole door
			uint16 size;	// 100 is normal, smaller number = smaller model

			uint8 doorId; // door's id #
			uint8 opentype;
			/*
			 *  Open types:
			 * 66 = PORT1414 (Qeynos)
			 * 55 = BBBOARD (Qeynos)
			 * 100 = QEYLAMP (Qeynos)
			 * 56 = CHEST1 (Qeynos)
			 * 5 = DOOR1 (Qeynos)
			 */
			uint8 state_at_spawn;
			uint8 invert_state; // if this is 1, the door is normally open
			uint32 door_param;
		};

		struct DoorSpawns_Struct
		{
			struct Door_Struct doors[0];
		};

		/*
		 OP Code: 	Op_ClickDoor
		 Size:		16
		*/
		struct ClickDoor_Struct
		{
			uint8 doorid;

			uint8 picklockskill;

			uint32 item_id;
			uint16 player_id;
		};

		struct MoveDoor_Struct
		{
			uint8 doorid;
			uint8 action;
		};

		struct BecomeNPC_Struct
		{
			uint32 id;
			int32 maxlevel;
		};

		struct Underworld_Struct
		{
			float speed;
			float y;
			float x;
			float z;
		};

		struct Resurrect_Struct
		{

			uint16 zone_id;
			uint16 instance_id;
			float y;
			float x;
			float z;
			char your_name[64];

			char rezzer_name[64];
			uint32 spellid;
			char corpse_name[64];
			uint32 action;
		};

		struct SetRunMode_Struct
		{
			uint8 mode;
		};

		// EnvDamage is EnvDamage2 without a few bytes at the end.

		struct EnvDamage2_Struct
		{
			uint32 id;

			uint32 damage;

			uint8 dmgtype; // FA = Lava; FC = Falling

			uint16 constant; // Always FFFF
		};

		// Bazaar Stuff =D

		struct BazaarWindowStart_Struct
		{
			uint8 Action;
		};

		struct BazaarWelcome_Struct
		{
			struct BazaarWindowStart_Struct beginning;
			uint32 traders;
			uint32 items;
		};

		struct BazaarSearch_Struct
		{
			struct BazaarWindowStart_Struct beginning;
			uint32 traderid;
			uint32 class_;
			uint32 race;
			uint32 stat;
			uint32 slot;
			uint32 type;
			char name[64];
			uint32 minprice;
			uint32 maxprice;
			uint32 minlevel;
			uint32 maxlevel;
		};
		struct BazaarInspect_Struct
		{
			uint32 item_id;

			char name[64];
		};
		struct BazaarReturnDone_Struct
		{
			uint32 type;
			uint32 traderid;
		};
		struct BazaarSearchResults_Struct
		{
			struct BazaarWindowStart_Struct Beginning;
			uint32 SellerID;
			uint32 NumItems; // Don't know. Don't know the significance of this field.
			uint32 SerialNumber;

			char ItemName[64];
			uint32 Cost;
			uint32 ItemStat;
		};

		struct ServerSideFilters_Struct
		{
			uint8 clientattackfilters; // 0) No, 1) All (players) but self, 2) All (players) but group
			uint8 npcattackfilters;	   // 0) No, 1) Ignore NPC misses (all), 2) Ignore NPC Misses + Attacks (all but self), 3) Ignores NPC Misses + Attacks (all but group)
			uint8 clientcastfilters;   // 0) No, 1) Ignore PC Casts (all), 2) Ignore PC Casts (not directed towards self)
			uint8 npccastfilters;	   // 0) No, 1) Ignore NPC Casts (all), 2) Ignore NPC Casts (not directed towards self)
		};

		/*
		** Client requesting item statistics
		** Size: 44 bytes
		** Used In: OP_ItemLinkClick
		** Last Updated: 2/15/2009
		**
		*/
		struct ItemViewRequest_Struct
		{
			uint32 item_id;
			uint32 augments[5];
			uint32 link_hash;
		};

		/*
		 *  Client to server packet
		 */
		struct PickPocket_Struct
		{
			// Size 18
			uint32 to;
			uint32 from;
			uint16 myskill;
			uint8 type; // -1 you are being picked, 0 failed , 1 = plat, 2 = gold, 3 = silver, 4 = copper, 5 = item

			uint32 coin;
			uint8 lastsix[2];
		};
		/*
		 * Server to client packet
		 */

		struct sPickPocket_Struct
		{
			// Size 28 = coin/fail
			uint32 to;
			uint32 from;
			uint32 myskill;
			uint32 type;
			uint32 coin;
			char itemname[64];
		};

		struct LogServer_Struct
		{
			// Op_Code OP_LOGSERVER

			char worldshortname[32];
		};

		struct ApproveWorld_Struct
		{
			// Size 544
			// Op_Code OP_ApproveWorld
		};

		struct ClientError_Struct
		{
			char type;

			char character_name[64];

			char message[31994];
		};

		struct MobHealth
		{
			uint8 hp;  // health percent
			uint16 id; // mobs id
		};

		struct Track_Struct
		{
			uint32 entityid;
			// uint16 padding002;
			float distance;
		};

		struct Tracking_Struct
		{
			struct Track_Struct Entrys[0];
		};

		/*
		** ZoneServerInfo_Struct
		** Zone server information
		** Size: 130 bytes
		** Used In: OP_ZoneServerInfo
		**
		*/
		struct ZoneServerInfo_Struct
		{
			char ip[128];
			uint16 port;
		};

		struct WhoAllPlayer
		{
			uint32 formatstring;
			uint32 pidstring;
			char *name;
			uint32 rankstring;
			char *guild;

			uint32 zonestring;
			uint32 zone;
			uint32 class_;
			uint32 level;
			uint32 race;
			char *account;
		};

		struct WhoAllReturnStruct
		{
			uint32 id;
			uint32 playerineqstring;
			char line[27];

			uint32 playersinzonestring;

			uint32 playercount; // 1
			struct WhoAllPlayer player[0];
		};

		struct Trader_Struct
		{
			uint32 code;
			uint32 itemid[160];

			uint32 itemcost[80];
		};

		struct ClickTrader_Struct
		{
			uint32 code;

			uint32 itemcost[80];
		};

		struct GetItems_Struct
		{
			uint32 items[80];
		};

		struct BecomeTrader_Struct
		{
			uint32 ID;
			uint32 Code;
		};

		struct Trader_ShowItems_Struct
		{
			uint32 code;
			uint32 traderid;
		};

		struct TraderBuy_Struct
		{
			uint32 Action;
			uint32 Price;
			uint32 TraderID;
			char ItemName[64];

			uint32 ItemID;
			uint32 AlreadySold;
			uint32 Quantity;
		};

		struct TraderItemUpdate_Struct
		{

			uint32 traderid;
			uint8 fromslot;
			uint8 toslot; // 7?
			uint16 charges;
		};

		struct MoneyUpdate_Struct
		{
			int32 platinum;
			int32 gold;
			int32 silver;
			int32 copper;
		};

		struct TraderDelItem_Struct
		{
			uint32 slotid;
			uint32 quantity;
		};

		struct TraderClick_Struct
		{
			uint32 traderid;

			uint32 approval;
		};

		struct FormattedMessage_Struct
		{

			uint32 string_id;
			uint32 type;
			char message[0];
		};
		struct SimpleMessage_Struct
		{
			uint32 string_id;
			uint32 color;
		};

		struct GuildMemberEntry_Struct
		{
			char name[1];		   // variable length
			uint32 level;		   // network byte order
			uint32 banker;		   // 1=yes, 0=no, network byte order
			uint32 class_;		   // network byte order
			uint32 rank;		   // network byte order
			uint32 time_last_on;   // network byte order
			uint32 tribute_enable; // network byte order
			uint32 total_tribute;  // total guild tribute donated, network byte order
			uint32 last_tribute;   // unix timestamp

			char public_note[1]; // variable length.
			uint16 zoneinstance; // network byte order
			uint16 zone_id;		 // network byte order
		};

		struct GuildMembers_Struct
		{						 // just for display purposes, this is not actually used in the message encoding.
			char player_name[1]; // variable length.
			uint32 count;		 // network byte order
			struct GuildMemberEntry_Struct member[0];
		};

		struct GuildMOTD_Struct
		{

			char name[64];
			char setby_name[64];

			char motd[512];
		};
		struct GuildUpdate_PublicNote
		{

			char name[64];
			char target[64];
			char note[100]; // we are cutting this off at 100, actually around 252
		};
		struct GuildDemoteStruct
		{
			char name[64];
			char target[64];
		};
		struct GuildRemoveStruct
		{
			char target[64];
			char name[64];

			uint32 leaderstatus; //?
		};
		struct GuildMakeLeader
		{
			char name[64];
			char target[64];
		};

		struct BugReport_Struct
		{
			char category_name[64];
			char character_name[64];
			char unused_0128[32];
			char ui_path[128];
			float pos_x;
			float pos_y;
			float pos_z;
			uint32 heading;
			uint32 unused_0304;
			uint32 time_played;
			char padding_0312[8];
			uint32 target_id;
			char padding_0324[140];

			char target_name[64];
			uint32 optional_info_mask;

			// this looks like a butchered 8k buffer with 2 trailing dword fields
			char unused_0536[2052];
			char bug_report[2050];
			char system_info[4098];
		};

		struct Make_Pet_Struct
		{ // Simple struct for getting pet info
			uint8 level;
			uint8 class_;
			uint16 race;
			uint8 texture;
			uint8 pettype;
			float size;
			uint8 type;
			uint32 min_dmg;
			uint32 max_dmg;
		};
		struct Ground_Spawn
		{
			float max_x;
			float max_y;
			float min_x;
			float min_y;
			float max_z;
			float heading;
			char name[16];
			uint32 item;
			uint32 max_allowed;
			uint32 respawntimer;
		};
		struct Ground_Spawns
		{
			struct Ground_Spawn spawn[50]; // Assigned max number to allow
		};

		// struct PetitionBug_Struct{
		//	uint32	petition_number;

		//	char	accountname[64];
		//	uint32	zoneid;
		//	char	name[64];
		//	uint32	level;
		//	uint32	class_;
		//	uint32	race;

		//	uint32	time;

		//	char	text[1028];
		//};

		struct ApproveZone_Struct
		{
			char name[64];
			uint32 zoneid;
			uint32 approve;
		};
		struct ZoneInSendName_Struct
		{

			char name[64];
			char name2[64];
		};
		struct ZoneInSendName_Struct2
		{

			char name[64];
		};

		struct StartTribute_Struct
		{
			uint32 client_id;
			uint32 tribute_master_id;
			uint32 response;
		};

		struct TributeLevel_Struct
		{
			uint32 level;			// backwards byte order!
			uint32 tribute_item_id; // backwards byte order!
			uint32 cost;			// backwards byte order!
		};

		struct TributeAbility_Struct
		{
			uint32 tribute_id; // backwards byte order!
			uint32 tier_count; // backwards byte order!
			struct TributeLevel_Struct tiers[10];
			char name[0];
		};

		struct GuildTributeAbility_Struct
		{
			uint32 guild_id;
			struct TributeAbility_Struct ability;
		};

		struct SelectTributeReq_Struct
		{
			uint32 client_id; //? maybe action ID?
			uint32 tribute_id;
		};

		struct SelectTributeReply_Struct
		{
			uint32 client_id; // echoed from request.
			uint32 tribute_id;
			char desc[0];
		};

		struct TributeInfo_Struct
		{
			uint32 active;		// 0 == inactive, 1 == active
			uint32 tributes[5]; //-1 == NONE
			uint32 tiers[5];	// all 00's
			uint32 tribute_master_id;
		};

		struct TributeItem_Struct
		{
			uint32 slot;
			uint32 quantity;
			uint32 tribute_master_id;
			int32 tribute_points;
		};

		struct TributePoint_Struct
		{
			int32 tribute_points;

			int32 career_tribute_points;
		};

		struct TributeMoney_Struct
		{
			uint32 platinum;
			uint32 tribute_master_id;
			int32 tribute_points;
		};

		struct Split_Struct
		{
			uint32 platinum;
			uint32 gold;
			uint32 silver;
			uint32 copper;
		};

		/*
		** New Combine Struct
		** Client requesting to perform a tradeskill combine
		** Size: 4 bytes
		** Used In: OP_TradeSkillCombine
		** Last Updated: Oct-15-2003
		**
		*/
		struct NewCombine_Struct
		{
			int16 container_slot;
			int16 guildtribute_slot;
		};

		// client requesting favorite recipies
		struct TradeskillFavorites_Struct
		{
			uint32 object_type;
			uint32 some_id;
			uint32 favorite_recipes[500];
		};

		// search request
		struct RecipesSearch_Struct
		{
			uint32 object_type; // same as in favorites
			uint32 some_id;		// same as in favorites
			uint32 mintrivial;
			uint32 maxtrivial;
			char query[56];
		};

		// one sent for each item, from server in reply to favorites or search
		struct RecipeReply_Struct
		{
			uint32 object_type;
			uint32 some_id; // same as in favorites
			uint32 component_count;
			uint32 recipe_id;
			uint32 trivial;
			char recipe_name[64];
		};

		// received and sent back as an ACK with different reply_code
		struct RecipeAutoCombine_Struct
		{
			uint32 object_type;
			uint32 some_id;

			uint32 recipe_id;
			uint32 reply_code; // 93 64 e1 00 (junk) in request
							   // 00 00 00 00 in successful reply
							   // f5 ff ff ff in 'you dont have all the stuff' reply
		};

		struct LevelAppearance_Struct
		{ // Sends a little graphic on level up
			uint32 spawn_id;
			uint32 parm1;
			uint32 value1a;
			uint32 value1b;
			uint32 parm2;
			uint32 value2a;
			uint32 value2b;
			uint32 parm3;
			uint32 value3a;
			uint32 value3b;
			uint32 parm4;
			uint32 value4a;
			uint32 value4b;
			uint32 parm5;
			uint32 value5a;
			uint32 value5b;
		};
		struct MerchantList
		{
			uint32 id;
			uint32 slot;
			uint32 item;
		};
		struct TempMerchantList
		{
			uint32 npcid;
			uint32 slot;
			uint32 item;
			uint32 charges; // charges/quantity
			uint32 origslot;
		};

		struct FindPerson_Point
		{
			float y;
			float x;
			float z;
		};

		struct FindPersonRequest_Struct
		{
			uint32 npc_id;
			struct FindPerson_Point client_pos;
		};

		// variable length packet of points
		struct FindPersonResult_Struct
		{
			struct FindPerson_Point dest;
			struct FindPerson_Point path[0]; // last element must be the same as dest
		};

		struct MobRename_Struct
		{
			char old_name[64];
			char old_name_again[64]; // not sure what the difference is
			char new_name[64];
		};

		struct PlayMP3_Struct
		{
			char filename[0];
		};

		// this is for custom title display in the skill window
		struct TitleEntry_Struct
		{
			uint32 skill_id;
			uint32 skill_value;
			char title[1];
		};

		struct Titles_Struct
		{
			uint32 title_count;
			struct TitleEntry_Struct titles[0];
		};

		struct TitleListEntry_Struct
		{
			char prefix[1];	 // variable length, null terminated
			char postfix[1]; // variable length, null terminated
		};

		struct TitleList_Struct
		{
			uint32 title_count;
			struct TitleListEntry_Struct titles[0]; // list of title structs
		};

		struct SetTitle_Struct
		{
			uint32 is_suffix; // guessed: 0 = prefix, 1 = suffix
			uint32 title_id;
		};

		struct SetTitleReply_Struct
		{
			uint32 is_suffix; // guessed: 0 = prefix, 1 = suffix
			char title[32];
			uint32 entity_id;
		};

		struct TaskDescription_Struct
		{
			uint32 activity_count; // not right.
			uint32 taskid;
			uint8 unk;
			uint32 id3;
			char name[1];		 // variable length, 0 terminated
			char desc[1];		 // variable length, 0 terminated
			uint32 reward_count; // not sure
			char reward_link[1]; // variable length, 0 terminated
		};

		struct TaskMemberList_Struct
		{
			uint32 gopher_id;
			uint32 member_count; // 1 less than the number of members
			char list_pointer[0];
		};

		struct TaskActivity_Struct
		{
			uint32 activity_count; // not right
			uint32 id3;
			uint32 taskid;
			uint32 activity_id;
			uint32 activity_type;
			char mob_name[1];  // variable length, 0 terminated
			char item_name[1]; // variable length, 0 terminated
			uint32 goal_count;
			char activity_name[1]; // variable length, 0 terminated... commonly empty
			uint32 done_count;
		};

		struct TaskHistoryEntry_Struct
		{
			uint32 task_id;
			char name[1];
			uint32 completed_time;
		};
		struct TaskHistory_Struct
		{
			uint32 completed_count;
			struct TaskHistoryEntry_Struct entries[0];
		};

		struct AcceptNewTask_Struct
		{
			uint32 task_id;		   // set to 0 for 'decline'
			uint32 task_master_id; // entity ID
		};

		// was all 0's from client, server replied with same op, all 0's
		struct CancelTask_Struct
		{
		};

		struct AvaliableTask_Struct
		{
			uint32 task_index;	   // no idea, seen 0x1
			uint32 task_master_id; // entity ID
			uint32 task_id;

			uint32 activity_count;	// not sure, seen 2
			char desc[1];			// variable length, 0 terminated
			uint32 reward_platinum; // not sure on these
			uint32 reward_gold;
			uint32 reward_silver;
			uint32 reward_copper;
			char some_name[1]; // variable length, 0 terminated
		};

		struct BankerChange_Struct
		{
			uint32 platinum;
			uint32 gold;
			uint32 silver;
			uint32 copper;
			uint32 platinum_bank;
			uint32 gold_bank;
			uint32 silver_bank;
			uint32 copper_bank;
		};

		struct LeadershipExpUpdate_Struct
		{
			double group_leadership_exp;
			uint32 group_leadership_points;

			double raid_leadership_exp;
			uint32 raid_leadership_points;
		};

		struct UpdateLeadershipAA_Struct
		{
			uint32 ability_id;
			uint32 new_rank;
			uint32 pointsleft;
		};

		/**
		 * Leadership AA update
		 * Length: 32 Octets
		 * OpCode: LeadExpUpdate
		 */
		struct leadExpUpdateStruct
		{

			uint32 group_leadership_exp;	// Group leadership exp value
			uint32 group_leadership_points; // Unspent group points

			uint32 raid_leadership_exp;	   // Raid leadership exp value
			uint32 raid_leadership_points; // Unspent raid points
		};

		struct RaidGeneral_Struct
		{
			uint32 action;		  //=10
			char player_name[64]; // should both be the player's name
			char leader_name[64];
			uint32 parameter;
		};

		struct RaidAddMember_Struct
		{
			struct RaidGeneral_Struct raidGen;
			uint8 _class;
			uint8 level;
			uint8 isGroupLeader;
		};

		struct RaidNote_Struct
		{
			struct RaidGeneral_Struct general;
			char note[64];
		};

		struct RaidMOTD_Struct
		{
			struct RaidGeneral_Struct general; // leader_name and action only used
			char motd[1024];				   // max size is 1024, but reply is variable
		};

		struct RaidLeadershipUpdate_Struct
		{
			uint32 action;
			char player_name[64];

			char leader_name[64];
			struct GroupLeadershipAA_Struct group; // unneeded
			struct RaidLeadershipAA_Struct raid;
		};

		struct RaidCreate_Struct
		{
			uint32 action; //=8
			char leader_name[64];
			uint32 leader_id;
		};

		struct RaidMemberInfo_Struct
		{
			uint8 group_number;
			char member_name[1]; // dyanmic length, null terminated '\0'

			uint8 _class;
			uint8 level;
			uint8 is_raid_leader;
			uint8 is_group_leader;
			uint8 main_tank; // not sure
		};

		struct RaidDetails_Struct
		{
			uint32 action; //=6,20
			char leader_name[64];

			struct LeadershipAA_Struct abilities; // ranks in backwards byte order

			uint32 leader_id;
		};

		struct RaidMembers_Struct
		{
			struct RaidDetails_Struct details;
			uint32 member_count; // including leader
			struct RaidMemberInfo_Struct members[1];
			struct RaidMemberInfo_Struct empty; // seem to have an extra member with a 0 length name on the end
		};

		struct DynamicWall_Struct
		{
			char name[32];
			float y;
			float x;
			float z;
			uint32 something;

			uint32 one_hundred; // 0x64

			uint32 something2;
		};

		// Bandolier actions
		enum
		{
			bandolierCreate = 0,
			bandolierRemove,
			bandolierSet
		};

		struct BandolierCreate_Struct
		{
			uint32 Action; // 0 for create
			uint8 Number;
			char Name[32];
		};

		struct BandolierDelete_Struct
		{
			uint32 Action;
			uint8 Number;
		};

		struct BandolierSet_Struct
		{
			uint32 Action;
			uint8 Number;
		};

		struct Arrow_Struct
		{
			uint32 type; // unsure on name, seems to be 0x1, dosent matter

			float src_y;
			float src_x;
			float src_z;

			float velocity;		// 4 is normal, 20 is quite fast
			float launch_angle; // 0-450ish, not sure the units, 140ish is straight
			float tilt;			// on the order of 125

			float arc;

			uint32 source_id;
			uint32 target_id; // entity ID
			uint32 item_id;	  // 1 to about 150ish

			char model_name[16];
		};

		// made a bunch of trivial structs for stuff for opcode finder to use
		struct Consent_Struct
		{
			char name[1]; // always at least a null
		};

		struct AdventureMerchant_Struct
		{

			uint32 entity_id;
		};

		struct Save_Struct
		{
		};

		struct GMToggle_Struct
		{

			uint32 toggle;
		};

		struct GroupInvite_Struct
		{
			char invitee_name[64];
			char inviter_name[64];
		};

		struct ColoredText_Struct
		{
			uint32 color;
			char msg[1];
		};

		struct UseAA_Struct
		{
			uint32 begin;
			uint32 ability;
			uint32 end;
		};

		struct AA_Ability
		{
			uint32 skill_id;
			uint32 base_value;
			uint32 limit_value;
			uint32 slot;
		};

		struct SendAA_Struct
		{
			uint32 id;

			uint32 hotkey_sid;
			uint32 hotkey_sid2;
			uint32 title_sid;
			uint32 desc_sid;
			uint32 class_type;
			uint32 cost;
			uint32 seq;
			uint32 current_level;	 // 1s, MQ2 calls this AARankRequired
			uint32 prereq_skill;	 // is < 0, abs() is category #
			uint32 prereq_minpoints; // min points in the prereq
			uint32 type;
			uint32 spellid;
			uint32 spell_type;
			uint32 spell_refresh;
			uint32 classes;
			uint32 max_level;
			uint32 last_id;
			uint32 next_id;
			uint32 cost2;

			uint32 total_abilities;
			struct AA_Ability abilities[0];
		};

		struct AA_List
		{
			struct SendAA_Struct *aa[0];
		};

		struct AA_Action
		{
			uint32 action;
			uint32 ability;
			uint32 target_id;
			uint32 exp_value;
		};

		struct AAExpUpdate_Struct
		{

			uint32 aapoints_unspent;
			uint8 aaxp_percent; //% of exp that goes to AAs
		};

		struct AltAdvStats_Struct
		{
			uint32 experience;
			uint16 unspent;

			uint8 percentage;
		};

		struct PlayerAA_Struct
		{
			struct AA_Array aa_list[240];
		};

		struct AATable_Struct
		{
			struct AA_Array aa_list[240];
		};

		struct Weather_Struct
		{
			uint32 val1; // generall 0x000000FF
			uint32 type; // 0x31=rain, 0x02=snow(i think), 0 = normal
			uint32 mode;
		};

		struct MobHealth_Struct
		{
			uint16 entity_id;
			uint8 hp;
		};

		struct LoadSpellSet_Struct
		{
			uint32 spell[9];
		};

		struct ApplyPoison_Struct
		{
			uint32 inventorySlot;
			uint32 success;
		};

		struct GuildMemberUpdate_Struct
		{
			uint32 guild_id; // not sure
			char member_name[64];
			uint16 zone_id;
			uint16 instance_id;
		};

		struct VeteranRewardItem
		{
			uint32 item_id;
			char item_name[256];
		};

		struct VeteranReward
		{
			uint32 claim_id;
			struct VeteranRewardItem item;
		};

		struct ExpeditionInvite_Struct
		{
			uint32 client_id;
			char inviter_name[64];
			char expedition_name[128];
			uint8 swapping;		// 0: adding 1: swapping
			char swap_name[64]; // if swapping, swap name being removed
			uint8 padding[3];
			uint16 dz_zone_id; // dz_id zone/instance pair, sent back in reply
			uint16 dz_instance_id;
		};

		struct ExpeditionInviteResponse_Struct
		{

			uint16 dz_zone_id; // dz_id pair sent in invite
			uint16 dz_instance_id;
			uint8 accepted;		// 0: declined 1: accepted
			uint8 swapping;		// 0: adding 1: swapping (sent in invite)
			char swap_name[64]; // swap name sent in invite
		};

		struct DynamicZoneInfo_Struct
		{
			uint32 client_id;
			uint32 assigned; // padded bool
			uint32 max_players;
			char dz_name[128];
			char leader_name[64];
		};

		struct DynamicZoneMemberEntry_Struct
		{
			char name[1];		 // variable length, null terminated, max 0x40 (64)
			uint8 online_status; // 0: unknown 1: Online, 2: Offline, 3: In Dynamic Zone, 4: Link Dead
		};

		struct DynamicZoneMemberList_Struct
		{
			uint32 client_id;
			uint32 member_count;
			struct DynamicZoneMemberEntry_Struct members[0]; // variable length
		};

		struct DynamicZoneMemberListName_Struct
		{
			uint32 client_id;
			uint32 add_name;
			char name[64];
		};

		struct ExpeditionLockoutTimerEntry_Struct
		{
			char expedition_name[1]; // variable length, null terminated, max 0x80 (128)
			uint32 seconds_remaining;
			int32 event_type;	// seen -1 (0xffffffff) for replay timers and 1 for event timers
			char event_name[1]; // variable length, null terminated, max 0x100 (256)
		};

		struct ExpeditionLockoutTimers_Struct
		{
			uint32 client_id;
			uint32 count;
			struct ExpeditionLockoutTimerEntry_Struct timers[0];
		};

		struct DynamicZoneLeaderName_Struct
		{
			uint32 client_id;
			char leader_name[64];
		};

		struct ExpeditionCommand_Struct
		{

			char name[64];
		};

		struct ExpeditionCommandSwap_Struct
		{

			char add_player_name[64]; // swap to (player must confirm)
			char rem_player_name[64]; // swap from
		};

		struct ExpeditionExpireWarning
		{
			uint32 client_id;
			uint32 minutes_remaining;
		};

		struct DynamicZoneCompassEntry_Struct
		{
			uint16 dz_zone_id; // target dz id pair
			uint16 dz_instance_id;
			uint32 dz_type; // 1: Expedition, 2: Tutorial (purple), 3: Task, 4: Mission, 5: Quest (green)
			uint32 dz_switch_id;
			float y;
			float x;
			float z;
		};

		struct DynamicZoneCompass_Struct
		{
			uint32 client_id;
			uint32 count;
			struct DynamicZoneCompassEntry_Struct entries[0];
		};

		struct DynamicZoneChooseZoneEntry_Struct
		{
			uint16 dz_zone_id; // dz_id pair
			uint16 dz_instance_id;
			uint32 dz_type;		 // 1: Expedition, 2: Tutorial, 3: Task, 4: Mission, 5: Quest -- sent back in reply
			char description[1]; // variable length, null terminated, max 0x80 (128)
			char leader_name[1]; // variable length, null terminated, max 0x40 (64)
		};

		struct DynamicZoneChooseZone_Struct
		{
			uint32 client_id;
			uint32 count;
			struct DynamicZoneChooseZoneEntry_Struct choices[0];
		};

		struct DynamicZoneChooseZoneReply_Struct
		{
			uint16 dz_zone_id;
			uint16 dz_instance_id;
			uint32 dz_type; // 1: Expedition, 2: Tutorial, 3: Task, 4: Mission, 5: Quest
		};

		struct LFGuild_SearchPlayer_Struct
		{
			uint32 Command;
			uint32 FromLevel;
			uint32 ToLevel;
			uint32 MinAA;
			uint32 TimeZone;
			uint32 Classes;
		};

		struct LFGuild_SearchGuild_Struct
		{
			uint32 Command;
			uint32 Level;
			uint32 AAPoints;
			uint32 TimeZone;
			uint32 Class;
		};

		struct LFGuild_PlayerToggle_Struct
		{
			uint32 Command;
			char Comment[256];
			uint32 TimeZone;
			uint8 Toggle;
			uint32 Expires;
		};

		struct LFGuild_GuildToggle_Struct
		{
			uint32 Command;
			char Comment[256];
			uint32 FromLevel;
			uint32 ToLevel;
			uint32 Classes;
			uint32 AACount;
			uint32 TimeZone;
			uint8 Toggle;
			uint32 Expires;
			char Name[64];
		};

		struct SayLinkBodyFrame_Struct
		{
			char ActionID[1];
			char ItemID[5];
			char Augment1[5];
			char Augment2[5];
			char Augment3[5];
			char Augment4[5];
			char Augment5[5];
			char IsEvolving[1];
			char EvolveGroup[4];
			char EvolveLevel[1];
			char Hash[8];
		};

		// Login structs

		struct WebLogin_Struct
		{
			char *username;
			char *password;
		};

		struct WebLoginServerRequest_Struct
		{
			uint32_t sequence;
		};

		struct WebLoginReply_Struct
		{
			char key[11];
			uint32_t error_str_id;
			uint32_t failed_attempts;
			int32_t lsid;
			bool success;
			bool show_player_count;
		};

		struct WebLoginWorldServer_Struct
		{
			char buffer[16];
			char ip[16];
			char long_name[64];
			char country_code[3];
			char language_code[3];
			uint32_t server_type;
			uint32_t server_id;
			uint32_t status;
			uint32_t players_online;
			struct WebLoginWorldServer_Struct *next;
		};

		struct WebLoginServerResponse_Struct
		{
			int32_t server_count;
			struct WebLoginWorldServer_Struct *servers;
		};

		struct WebPlayEverquestRequest_Struct
		{
			uint32_t server_id;
		};

		struct WebPlayEverquestResponse_Struct
		{
			uint32_t server_id;
			bool success;
			int32_t error_str_id;
		};

		struct WebInitiateConnection_Struct
		{
			bool login;
		};

		// Go interop
		struct WebSession_Struct
		{
			char *remote_addr;
			uint32_t remote_ip;
			uint32_t remote_port;
		};

#ifdef __cplusplus
	} // namespace Web

} // namespace structs
#endif
