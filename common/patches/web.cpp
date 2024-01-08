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

#include "../global_define.h"
#include "../eqemu_config.h"
#include "../eqemu_logsys.h"
#include "web.h"
#include "../opcodemgr.h"

#include "../eq_stream_ident.h"
#include "../crc32.h"
#include "../races.h"

#include "../eq_packet_structs.h"
#include "../misc_functions.h"
#include "../strings.h"
#include "../item_instance.h"
#include "web_structs.h"
#include "../path_manager.h"
#include "../raid.h"

#include <sstream>

namespace Web
{
	static const char *name = "Web";
	static OpcodeManager *opcodes = nullptr;
	static Strategy struct_strategy;

	void SerializeItem(EQ::OutBuffer &ob, const EQ::ItemInstance *inst, int16 slot_id_in, uint8 depth);

	// server to client inventory location converters
	static inline int16 ServerToWebSlot(uint32 server_slot);
	static inline int16 ServerToWebCorpseSlot(uint32 server_corpse_slot);

	// client to server inventory location converters
	static inline uint32 WebToServerSlot(int16 web_slot);
	static inline uint32 WebToServerCorpseSlot(int16 web_corpse_slot);

	// server to client say link converter
	static inline void ServerToWebSayLink(std::string &web_saylink, const std::string &server_saylink);

	// client to server say link converter
	static inline void WebToServerSayLink(std::string &server_saylink, const std::string &web_saylink);

	static inline spells::CastingSlot ServerToWebCastingSlot(EQ::spells::CastingSlot slot);
	static inline EQ::spells::CastingSlot WebToServerCastingSlot(spells::CastingSlot slot, uint32 item_location);

	static inline int ServerToWebBuffSlot(int index);
	static inline int WebToServerBuffSlot(int index);

	void Register(EQStreamIdentifier &into)
	{
		auto Config = EQEmuConfig::get();
		// create our opcode manager if we havent already
		if (opcodes == nullptr)
		{
			opcodes = new PassthroughOpcodeManager();
		}

		// ok, now we have what we need to register.

		EQStreamInterface::Signature signature;
		std::string pname;

		// register our world signature.
		pname = std::string(name) + "_world";
		signature.ignore_eq_opcode = 0;
		signature.first_length = 1;
		signature.first_eq_opcode = opcodes->EmuToEQ(OP_WebInitiateConnection);
		into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);

		// register our zone signature.
		pname = std::string(name) + "_zone";
		signature.first_length = 1;
		signature.first_eq_opcode = opcodes->EmuToEQ(OP_WebInitiateConnection);
		into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);

		LogNetcode("[StreamIdentify] Registered patch [{}]", name);
	}

	void Reload()
	{
	}

	Strategy::Strategy() : StructStrategy(){
	// all opcodes default to passthrough.
#include "ss_register.h"
#include "web_ops.h"
						   }

						   std::string Strategy::Describe() const
	{
		std::string r;
		r += "Patch ";
		r += name;
		return (r);
	}

	const EQ::versions::ClientVersion Strategy::ClientVersion() const
	{
		return EQ::versions::ClientVersion::Web;
	}

#include "ss_define.h"

	// ENCODE methods
	EAT_ENCODE(OP_GuildMemberLevelUpdate); // added ;

	EAT_ENCODE(OP_ZoneServerReady); // added ;

	ENCODE(OP_Action)
	{
		ENCODE_LENGTH_EXACT(Action_Struct);
		SETUP_DIRECT_ENCODE(Action_Struct, structs::Action_Struct);

		OUT(target);
		OUT(source);
		OUT(level);
		OUT(instrument_mod);
		OUT(force);
		OUT(hit_heading);
		OUT(hit_pitch);
		OUT(type);
		// OUT(damage);
		OUT(spell);
		OUT(spell_level);
		OUT(effect_flag); // if this is 4, a buff icon is made

		FINISH_ENCODE();
	}

	ENCODE(OP_AdventureMerchantSell)
	{
		ENCODE_LENGTH_EXACT(Adventure_Sell_Struct);
		SETUP_DIRECT_ENCODE(Adventure_Sell_Struct, structs::Adventure_Sell_Struct);

		OUT(npcid);
		eq->slot = ServerToWebSlot(emu->slot);
		OUT(charges);
		OUT(sell_price);

		FINISH_ENCODE();
	}

	ENCODE(OP_ApplyPoison)
	{
		ENCODE_LENGTH_EXACT(ApplyPoison_Struct);
		SETUP_DIRECT_ENCODE(ApplyPoison_Struct, structs::ApplyPoison_Struct);

		eq->inventory_slot = ServerToWebSlot(emu->inventorySlot);
		OUT(success);

		FINISH_ENCODE();
	}

	ENCODE(OP_BazaarSearch)
	{
		if (((*p)->size == sizeof(BazaarReturnDone_Struct)) || ((*p)->size == sizeof(BazaarWelcome_Struct)))
		{

			EQApplicationPacket *in = *p;
			*p = nullptr;
			dest->FastQueuePacket(&in, ack_req);
			return;
		}

		// consume the packet
		EQApplicationPacket *in = *p;
		*p = nullptr;

		// store away the emu struct
		unsigned char *__emu_buffer = in->pBuffer;
		BazaarSearchResults_Struct *emu = (BazaarSearchResults_Struct *)__emu_buffer;

		// determine and verify length
		int entrycount = in->size / sizeof(BazaarSearchResults_Struct);
		if (entrycount == 0 || (in->size % sizeof(BazaarSearchResults_Struct)) != 0)
		{
			Log(Logs::General, Logs::Netcode, "[STRUCTS] Wrong size on outbound %s: Got %d, expected multiple of %d",
				opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(BazaarSearchResults_Struct));
			delete in;
			return;
		}

		// make the EQ struct.
		in->size = sizeof(structs::BazaarSearchResults_Struct) * entrycount;
		in->pBuffer = new unsigned char[in->size];
		structs::BazaarSearchResults_Struct *eq = (structs::BazaarSearchResults_Struct *)in->pBuffer;

		// zero out the packet. We could avoid this memset by setting all fields (including unknowns) in the loop.
		memset(in->pBuffer, 0, in->size);

		for (int i = 0; i < entrycount; i++, eq++, emu++)
		{
			eq->beginning.action = emu->Beginning.Action;
			eq->num_items = emu->NumItems;
			eq->serial_number = emu->SerialNumber;
			eq->seller_id = emu->SellerID;
			eq->cost = emu->Cost;
			eq->item_stat = emu->ItemStat;
			strcpy(eq->item_name, emu->ItemName);
		}

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_BecomeTrader)
	{
		ENCODE_LENGTH_EXACT(BecomeTrader_Struct);
		SETUP_DIRECT_ENCODE(BecomeTrader_Struct, structs::BecomeTrader_Struct);
		eq->id = emu->ID;
		eq->code = emu->Code;

		FINISH_ENCODE();
	}

	ENCODE(OP_Buff)
	{
		ENCODE_LENGTH_EXACT(SpellBuffPacket_Struct);
		SETUP_DIRECT_ENCODE(SpellBuffPacket_Struct, structs::SpellBuffPacket_Struct);

		OUT(entityid);
		OUT(buff.effect_type);
		OUT(buff.level);
		OUT(buff.bard_modifier);
		OUT(buff.spellid);
		OUT(buff.duration);
		OUT(buff.counters);
		OUT(buff.player_id);
		eq->slotid = ServerToWebBuffSlot(emu->slotid);
		OUT(bufffade);

		FINISH_ENCODE();
	}

	ENCODE(OP_ChannelMessage)
	{
		EQApplicationPacket *in = *p;
		*p = nullptr;

		ChannelMessage_Struct *emu = (ChannelMessage_Struct *)in->pBuffer;

		unsigned char *__emu_buffer = in->pBuffer;

		std::string old_message = emu->message;
		std::string new_message;
		ServerToWebSayLink(new_message, old_message);

		in->size = sizeof(ChannelMessage_Struct) + new_message.length() + 1;

		in->pBuffer = new unsigned char[in->size];

		char *OutBuffer = (char *)in->pBuffer;

		memcpy(OutBuffer, __emu_buffer, sizeof(ChannelMessage_Struct));

		OutBuffer += sizeof(ChannelMessage_Struct);

		VARSTRUCT_ENCODE_STRING(OutBuffer, new_message.c_str());

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_CharInventory)
	{
		// consume the packet
		EQApplicationPacket *in = *p;
		*p = nullptr;

		// store away the emu struct
		uchar *__emu_buffer = in->pBuffer;

		int itemcount = in->size / sizeof(EQ::InternalSerializedItem_Struct);
		if (itemcount == 0 || (in->size % sizeof(EQ::InternalSerializedItem_Struct)) != 0)
		{
			Log(Logs::General, Logs::Netcode, "[STRUCTS] Wrong size on outbound %s: Got %d, expected multiple of %d",
				opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(EQ::InternalSerializedItem_Struct));
			delete in;
			return;
		}

		EQ::InternalSerializedItem_Struct *eq = (EQ::InternalSerializedItem_Struct *)in->pBuffer;

		// do the transform...
		EQ::OutBuffer ob;
		EQ::OutBuffer::pos_type last_pos = ob.tellp();

		for (int r = 0; r < itemcount; r++, eq++)
		{
			SerializeItem(ob, (const EQ::ItemInstance *)eq->inst, ServerToWebSlot(eq->slot_id), 0);
			if (ob.tellp() == last_pos)
				LogNetcode("Web::ENCODE(OP_CharInventory) Serialization failed on item slot [{}] during OP_CharInventory.  Item skipped", eq->slot_id);

			last_pos = ob.tellp();
		}

		in->size = ob.size();
		in->pBuffer = ob.detach();

		delete[] __emu_buffer;

		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_ClientUpdate)
	{
		SETUP_DIRECT_ENCODE(PlayerPositionUpdateServer_Struct, structs::PlayerPositionUpdateServer_Struct);

		OUT(spawn_id);
		OUT(x_pos);
		OUT(delta_x);
		OUT(delta_y);
		OUT(z_pos);
		OUT(delta_heading);
		OUT(y_pos);
		OUT(delta_z);
		OUT(animation);
		OUT(heading);

		FINISH_ENCODE();
	}

	ENCODE(OP_Damage)
	{
		ENCODE_LENGTH_EXACT(CombatDamage_Struct);
		SETUP_DIRECT_ENCODE(CombatDamage_Struct, structs::CombatDamage_Struct);

		OUT(target);
		OUT(source);
		OUT(type);
		OUT(spellid);
		OUT(damage);
		OUT(force);
		OUT(hit_heading);
		OUT(hit_pitch);

		FINISH_ENCODE();
	}

	ENCODE(OP_DeleteCharge)
	{
		Log(Logs::Detail, Logs::Netcode, "Web::ENCODE(OP_DeleteCharge)");

		ENCODE_FORWARD(OP_MoveItem);
	}

	ENCODE(OP_DeleteItem)
	{
		ENCODE_LENGTH_EXACT(DeleteItem_Struct);
		SETUP_DIRECT_ENCODE(DeleteItem_Struct, structs::DeleteItem_Struct);

		eq->from_slot = ServerToWebSlot(emu->from_slot);
		eq->to_slot = ServerToWebSlot(emu->to_slot);
		OUT(number_in_stack);

		FINISH_ENCODE();
	}

	ENCODE(OP_DeleteSpawn)
	{
		SETUP_DIRECT_ENCODE(DeleteSpawn_Struct, structs::DeleteSpawn_Struct);

		OUT(spawn_id);

		FINISH_ENCODE();
	}

	ENCODE(OP_DzChooseZone)
	{
		SETUP_VAR_ENCODE(DynamicZoneChooseZone_Struct);

		SerializeBuffer buf;
		buf.WriteUInt32(emu->client_id);
		buf.WriteUInt32(emu->count);

		for (uint32 i = 0; i < emu->count; ++i)
		{
			buf.WriteUInt16(emu->choices[i].dz_zone_id);
			buf.WriteUInt16(emu->choices[i].dz_instance_id);
			buf.WriteUInt32(emu->choices[i].dz_type);
			buf.WriteString(emu->choices[i].description);
			buf.WriteString(emu->choices[i].leader_name);
		}

		__packet->size = buf.size();
		__packet->pBuffer = new unsigned char[__packet->size];
		memcpy(__packet->pBuffer, buf.buffer(), __packet->size);

		FINISH_ENCODE();
	}

	ENCODE(OP_DzExpeditionEndsWarning)
	{
		ENCODE_LENGTH_EXACT(ExpeditionExpireWarning);
		SETUP_DIRECT_ENCODE(ExpeditionExpireWarning, structs::ExpeditionExpireWarning);

		OUT(minutes_remaining);

		FINISH_ENCODE();
	}

	ENCODE(OP_DzExpeditionInfo)
	{
		ENCODE_LENGTH_EXACT(DynamicZoneInfo_Struct);
		SETUP_DIRECT_ENCODE(DynamicZoneInfo_Struct, structs::DynamicZoneInfo_Struct);

		OUT(client_id);
		OUT(assigned);
		OUT(max_players);
		strn0cpy(eq->dz_name, emu->dz_name, sizeof(eq->dz_name));
		strn0cpy(eq->leader_name, emu->leader_name, sizeof(eq->leader_name));

		FINISH_ENCODE();
	}

	ENCODE(OP_DzExpeditionInvite)
	{
		ENCODE_LENGTH_EXACT(ExpeditionInvite_Struct);
		SETUP_DIRECT_ENCODE(ExpeditionInvite_Struct, structs::ExpeditionInvite_Struct);

		OUT(client_id);
		strn0cpy(eq->inviter_name, emu->inviter_name, sizeof(eq->inviter_name));
		strn0cpy(eq->expedition_name, emu->expedition_name, sizeof(eq->expedition_name));
		OUT(swapping);
		strn0cpy(eq->swap_name, emu->swap_name, sizeof(eq->swap_name));
		OUT(dz_zone_id);
		OUT(dz_instance_id);

		FINISH_ENCODE();
	}

	ENCODE(OP_DzExpeditionLockoutTimers)
	{
		SETUP_VAR_ENCODE(ExpeditionLockoutTimers_Struct);

		SerializeBuffer buf;
		buf.WriteUInt32(emu->client_id);
		buf.WriteUInt32(emu->count);
		for (uint32 i = 0; i < emu->count; ++i)
		{
			buf.WriteString(emu->timers[i].expedition_name);
			buf.WriteUInt32(emu->timers[i].seconds_remaining);
			buf.WriteInt32(emu->timers[i].event_type);
			buf.WriteString(emu->timers[i].event_name);
		}

		__packet->size = buf.size();
		__packet->pBuffer = new unsigned char[__packet->size];
		memcpy(__packet->pBuffer, buf.buffer(), __packet->size);

		FINISH_ENCODE();
	}

	ENCODE(OP_DzSetLeaderName)
	{
		ENCODE_LENGTH_EXACT(DynamicZoneLeaderName_Struct);
		SETUP_DIRECT_ENCODE(DynamicZoneLeaderName_Struct, structs::DynamicZoneLeaderName_Struct);

		OUT(client_id);
		strn0cpy(eq->leader_name, emu->leader_name, sizeof(eq->leader_name));

		FINISH_ENCODE();
	}

	ENCODE(OP_DzMemberList)
	{
		SETUP_VAR_ENCODE(DynamicZoneMemberList_Struct);

		SerializeBuffer buf;
		buf.WriteUInt32(emu->client_id);
		buf.WriteUInt32(emu->member_count);
		for (uint32 i = 0; i < emu->member_count; ++i)
		{
			buf.WriteString(emu->members[i].name);
			buf.WriteUInt8(emu->members[i].online_status);
		}

		__packet->size = buf.size();
		__packet->pBuffer = new unsigned char[__packet->size];
		memcpy(__packet->pBuffer, buf.buffer(), __packet->size);

		FINISH_ENCODE();
	}

	ENCODE(OP_DzMemberListName)
	{
		ENCODE_LENGTH_EXACT(DynamicZoneMemberListName_Struct);
		SETUP_DIRECT_ENCODE(DynamicZoneMemberListName_Struct, structs::DynamicZoneMemberListName_Struct);

		OUT(client_id);
		OUT(add_name);
		strn0cpy(eq->name, emu->name, sizeof(eq->name));

		FINISH_ENCODE();
	}

	ENCODE(OP_DzMemberListStatus)
	{
		auto emu = reinterpret_cast<DynamicZoneMemberList_Struct *>((*p)->pBuffer);
		if (emu->member_count == 1)
		{
			ENCODE_FORWARD(OP_DzMemberList);
		}
	}

	ENCODE(OP_Emote)
	{
		EQApplicationPacket *in = *p;
		*p = nullptr;

		Emote_Struct *emu = (Emote_Struct *)in->pBuffer;

		unsigned char *__emu_buffer = in->pBuffer;

		std::string old_message = emu->message;
		std::string new_message;
		ServerToWebSayLink(new_message, old_message);

		// if (new_message.length() > 512) // length restricted in packet building function due vari-length name size (no nullterm)
		//	new_message = new_message.substr(0, 512);

		in->size = new_message.length() + 5;
		in->pBuffer = new unsigned char[in->size];

		char *OutBuffer = (char *)in->pBuffer;

		VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, emu->type);
		VARSTRUCT_ENCODE_STRING(OutBuffer, new_message.c_str());

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_FormattedMessage)
	{
		SETUP_DIRECT_ENCODE(FormattedMessage_Struct, structs::FormattedMessage_Struct);

		OUT(string_id);
		OUT(type);
		strn0cpy(eq->message, emu->message, sizeof(eq->message));

		FINISH_ENCODE();
	}

	ENCODE(OP_GroundSpawn)
	{
		// We are not encoding the spawn_id field here, but it doesn't appear to matter.
		//
		EQApplicationPacket *in = *p;
		*p = nullptr;

		// store away the emu struct
		unsigned char *__emu_buffer = in->pBuffer;
		Object_Struct *emu = (Object_Struct *)__emu_buffer;

		in->size = strlen(emu->object_name) + sizeof(structs::Object_Struct) - 1;
		in->pBuffer = new unsigned char[in->size];

		structs::Object_Struct *eq = (structs::Object_Struct *)in->pBuffer;

		eq->drop_id = emu->drop_id;
		eq->heading = emu->heading;
		eq->linked_list_addr[0] = 0;
		eq->linked_list_addr[1] = 0;
		strcpy(eq->object_name, emu->object_name);
		eq->object_type = emu->object_type;
		eq->spawn_id = 0;
		eq->z = emu->z;
		eq->x = emu->x;
		eq->y = emu->y;
		eq->zone_id = emu->zone_id;
		eq->zone_instance = emu->zone_instance;

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_GuildMemberList)
	{
		// consume the packet
		EQApplicationPacket *in = *p;
		*p = nullptr;

		// store away the emu struct
		unsigned char *__emu_buffer = in->pBuffer;
		Internal_GuildMembers_Struct *emu = (Internal_GuildMembers_Struct *)in->pBuffer;

		// make a new EQ buffer.
		uint32 pnl = strlen(emu->player_name);
		uint32 length = sizeof(structs::GuildMembers_Struct) + pnl +
						emu->count * sizeof(structs::GuildMemberEntry_Struct) + emu->name_length + emu->note_length;
		in->pBuffer = new uint8[length];
		in->size = length;
		// no memset since we fill every byte.

		uint8 *buffer;
		buffer = in->pBuffer;

		// easier way to setup GuildMembers_Struct
		// set prefix name
		strcpy((char *)buffer, emu->player_name);
		buffer += pnl;
		*buffer = '\0';
		buffer++;

		// add member count.
		*((uint32 *)buffer) = htonl(emu->count);
		buffer += sizeof(uint32);

		if (emu->count > 0)
		{
			Internal_GuildMemberEntry_Struct *emu_e = emu->member;
			const char *emu_name = (const char *)(__emu_buffer +
												  sizeof(Internal_GuildMembers_Struct) +				// skip header
												  emu->count * sizeof(Internal_GuildMemberEntry_Struct) // skip static length member data
			);
			const char *emu_note = (emu_name +
									emu->name_length + // skip name contents
									emu->count		   // skip string terminators
			);

			structs::GuildMemberEntry_Struct *e = (structs::GuildMemberEntry_Struct *)buffer;

			uint32 r;
			for (r = 0; r < emu->count; r++, emu_e++)
			{

				// the order we set things here must match the struct

				// nice helper macro
				/*#define SlideStructString(field, str) \
				strcpy(e->field, str.c_str()); \
				e = (GuildMemberEntry_Struct *) ( ((uint8 *)e) + str.length() )*/
#define SlideStructString(field, str)                                \
	{                                                                \
		int sl = strlen(str);                                        \
		memcpy(e->field, str, sl + 1);                               \
		e = (structs::GuildMemberEntry_Struct *)(((uint8 *)e) + sl); \
		str += sl + 1;                                               \
	}
#define PutFieldN(field) e->field = htonl(emu_e->field)

				SlideStructString(name, emu_name);
				PutFieldN(level);
				PutFieldN(banker);
				e->char_class = htonl(emu_e->class_);
				PutFieldN(rank);
				PutFieldN(time_last_on);
				PutFieldN(tribute_enable);
				PutFieldN(total_tribute);
				PutFieldN(last_tribute);
				SlideStructString(public_note, emu_note);
				e->zoneinstance = 0;
				e->zone_id = htons(emu_e->zone_id);

#undef SlideStructString
#undef PutFieldN

				e++;
			}
		}

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_Illusion)
	{
		ENCODE_LENGTH_EXACT(Illusion_Struct);
		SETUP_DIRECT_ENCODE(Illusion_Struct, structs::Illusion_Struct);

		OUT(spawnid);
		OUT_str(charname);

		if (emu->race > 473)
			eq->race = 1;
		else
			OUT(race);

		OUT(gender);
		OUT(texture);
		OUT(helmtexture);
		OUT(face);
		OUT(hairstyle);
		OUT(haircolor);
		OUT(beard);
		OUT(beardcolor);
		OUT(size);
		/*
		//Test code for identifying the structure
		uint8 ofs;
		uint8 val;
		ofs = emu->texture;
		val = emu->face;
		((uint8*)eq)[ofs % 168] = val;
		*/

		FINISH_ENCODE();
	}

	ENCODE(OP_InspectAnswer)
	{
		ENCODE_LENGTH_EXACT(InspectResponse_Struct);
		SETUP_DIRECT_ENCODE(InspectResponse_Struct, structs::InspectResponse_Struct);
		eq->target_id = emu->TargetID;
		eq->playerid = emu->playerid;

		for (int i = EQ::invslot::slotCharm; i <= EQ::invslot::slotWaist; ++i)
		{
			//strn0cpy(eq->itemnames[i], emu->itemnames[i], sizeof(eq->itemnames[i]));
			OUT(itemicons[i]);
		}

		// move ammo down to last element in web array
		//strn0cpy(eq->itemnames[invslot::slotAmmo], emu->itemnames[EQ::invslot::slotAmmo], sizeof(eq->itemnames[invslot::slotAmmo]));
		eq->itemicons[invslot::slotAmmo] = emu->itemicons[EQ::invslot::slotAmmo];

		strn0cpy(eq->text, emu->text, sizeof(eq->text));

		FINISH_ENCODE();
	}

	ENCODE(OP_InspectRequest)
	{
		ENCODE_LENGTH_EXACT(Inspect_Struct);
		SETUP_DIRECT_ENCODE(Inspect_Struct, structs::Inspect_Struct);

		eq->target_id = emu->TargetID;
		eq->player_id = emu->PlayerID;

		FINISH_ENCODE();
	}

	ENCODE(OP_ItemLinkResponse) { ENCODE_FORWARD(OP_ItemPacket); }

	ENCODE(OP_ItemPacket)
	{
		// consume the packet
		EQApplicationPacket *in = *p;
		*p = nullptr;

		// store away the emu struct
		uchar *__emu_buffer = in->pBuffer;

		EQ::InternalSerializedItem_Struct *int_struct = (EQ::InternalSerializedItem_Struct *)(&__emu_buffer[4]);

		EQ::OutBuffer ob;
		EQ::OutBuffer::pos_type last_pos = ob.tellp();

		ob.write((const char *)__emu_buffer, 4);

		SerializeItem(ob, (const EQ::ItemInstance *)int_struct->inst, ServerToWebSlot(int_struct->slot_id), 0);
		if (ob.tellp() == last_pos)
		{
			LogNetcode("Web::ENCODE(OP_ItemPacket) Serialization failed on item slot [{}]", int_struct->slot_id);
			delete in;
			return;
		}

		in->size = ob.size();
		in->pBuffer = ob.detach();

		delete[] __emu_buffer;

		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_SpawnDoor)
	{
		SETUP_DIRECT_ENCODE(Door_Struct, structs::DoorSpawns_Struct);
		int door_count = __packet->size / sizeof(Door_Struct);

		std::vector<structs::Door_Struct*> doors;
		for (int r = 0; r < door_count; r++)
		{
			auto door =  (structs::Door_Struct*)malloc(sizeof(structs::Door_Struct));
			*door = (structs::Door_Struct) {
				.name = "",
				.y_pos = emu[r].yPos,
				.x_pos = emu[r].xPos,
				.z_pos = emu[r].zPos,
				.heading = emu[r].heading,
				.incline = emu[r].incline,
				.size = emu[r].size,
				.door_id = emu[r].doorId,
				.opentype = emu[r].opentype,
				.state_at_spawn = emu[r].state_at_spawn,
				.invert_state = emu[r].invert_state,
				.door_param = emu[r].door_param,
				.next = nullptr};
			strncpy(door->name, emu[r].name, sizeof(door->name));
			if (!doors.empty())
			{
				doors.back()->next = door;
			}
			doors.push_back(door);
		}
		eq->count = door_count;
		eq->doors = doors.size() > 0 ? doors[0] : nullptr;
		FINISH_ENCODE();
	}

	ENCODE(OP_LeadershipExpUpdate)
	{
		SETUP_DIRECT_ENCODE(LeadershipExpUpdate_Struct, structs::LeadershipExpUpdate_Struct);

		OUT(group_leadership_exp);
		OUT(group_leadership_points);
		OUT(raid_leadership_exp);
		OUT(raid_leadership_points);

		FINISH_ENCODE();
	}

	ENCODE(OP_SendMaxCharacters)
	{
		SETUP_DIRECT_ENCODE(MaxCharacters_Struct, structs::Int_Struct);
		eq->value = emu->max_chars;
		FINISH_ENCODE();
	}

	ENCODE(OP_LFGuild)
	{
		EQApplicationPacket *in = *p;
		*p = nullptr;

		uint32 Command = in->ReadUInt32();

		if (Command != 0)
		{
			dest->FastQueuePacket(&in, ack_req);
			return;
		}

		auto outapp = new EQApplicationPacket(OP_LFGuild, sizeof(structs::LFGuild_Playertoggle_Struct));

		memcpy(outapp->pBuffer, in->pBuffer, sizeof(structs::LFGuild_Playertoggle_Struct));

		dest->FastQueuePacket(&outapp, ack_req);
		delete in;
	}

	ENCODE(OP_LootItem)
	{
		ENCODE_LENGTH_EXACT(LootingItem_Struct);
		SETUP_DIRECT_ENCODE(LootingItem_Struct, structs::LootingItem_Struct);

		Log(Logs::Detail, Logs::Netcode, "Web::ENCODE(OP_LootItem)");

		OUT(lootee);
		OUT(looter);
		eq->slot_id = ServerToWebCorpseSlot(emu->slot_id);
		OUT(auto_loot);

		FINISH_ENCODE();
	}

	ENCODE(OP_ManaChange)
	{
		ENCODE_LENGTH_EXACT(ManaChange_Struct);
		SETUP_DIRECT_ENCODE(ManaChange_Struct, structs::ManaChange_Struct);

		OUT(new_mana);
		OUT(stamina);
		OUT(spell_id);
		OUT(keepcasting);

		FINISH_ENCODE();
	}

	ENCODE(OP_MemorizeSpell)
	{
		ENCODE_LENGTH_EXACT(MemorizeSpell_Struct);
		SETUP_DIRECT_ENCODE(MemorizeSpell_Struct, structs::MemorizeSpell_Struct);

		// Since HT/LoH are translated up, we need to translate down only for memSpellSpellbar case
		if (emu->scribing == 3)
			eq->slot = static_cast<uint32>(ServerToWebCastingSlot(static_cast<EQ::spells::CastingSlot>(emu->slot)));
		else
			OUT(slot);
		OUT(spell_id);
		OUT(scribing);

		FINISH_ENCODE();
	}

	ENCODE(OP_MoveItem)
	{
		ENCODE_LENGTH_EXACT(MoveItem_Struct);
		SETUP_DIRECT_ENCODE(MoveItem_Struct, structs::MoveItem_Struct);

		Log(Logs::Detail, Logs::Netcode, "Web::ENCODE(OP_MoveItem)");

		eq->from_slot = ServerToWebSlot(emu->from_slot);
		eq->to_slot = ServerToWebSlot(emu->to_slot);
		OUT(number_in_stack);

		FINISH_ENCODE();
	}

	ENCODE(OP_NewSpawn) { ENCODE_FORWARD(OP_ZoneSpawns); }

	ENCODE(OP_OnLevelMessage)
	{
		ENCODE_LENGTH_EXACT(OnLevelMessage_Struct);
		SETUP_DIRECT_ENCODE(OnLevelMessage_Struct, structs::OnLevelMessage_Struct);

		strncpy(eq->title, emu->Title, sizeof(eq->title)); eq->title[sizeof(eq->title)-1];
		strncpy(eq->text, emu->Text, sizeof(eq->text)); eq->text[sizeof(eq->text)-1];
		eq->buttons = emu->Buttons;
		eq->duration = emu->Duration;

		FINISH_ENCODE();
	}

	ENCODE(OP_PetBuffWindow)
	{
		ENCODE_LENGTH_EXACT(PetBuff_Struct);
		SETUP_DIRECT_ENCODE(PetBuff_Struct, PetBuff_Struct);

		OUT(petid);
		OUT(buffcount);

		int EQBuffSlot = 0; // do we really want to shuffle them around like this?

		for (uint32 EmuBuffSlot = 0; EmuBuffSlot < PET_BUFF_COUNT; ++EmuBuffSlot)
		{
			if (emu->spellid[EmuBuffSlot])
			{
				eq->spellid[EQBuffSlot] = emu->spellid[EmuBuffSlot];
				eq->ticsremaining[EQBuffSlot++] = emu->ticsremaining[EmuBuffSlot];
			}
		}

		FINISH_ENCODE();
	}

	ENCODE(OP_PlayerProfile)
	{
		SETUP_DIRECT_ENCODE(PlayerProfile_Struct, structs::PlayerProfile_Struct);

		uint32 r;

		eq->available_slots = 0xffffffff;

		//	OUT(checksum);
		OUT(gender);
		OUT(race);
		eq->char_class = emu->class_;
		OUT(level);
		eq->level1 = emu->level;
		for (r = 0; r < 5; r++)
		{
			OUT(binds[r].zone_id);
			OUT(binds[r].x);
			OUT(binds[r].y);
			OUT(binds[r].z);
			OUT(binds[r].heading);
		}
		OUT(deity);
		OUT(intoxication);
		for(__i = 0; __i < spells::SPELL_GEM_COUNT; __i++) eq->spell_slot_refresh[__i] = emu->spellSlotRefresh[__i];
		eq->ability_slot_refresh = emu->abilitySlotRefresh;

		OUT(haircolor);
		OUT(beardcolor);
		OUT(eyecolor1);
		OUT(eyecolor2);
		OUT(hairstyle);
		OUT(beard);
		for (r = EQ::textures::textureBegin; r < EQ::textures::materialCount; r++)
		{

			// OUT(item_material.Slot[r].Material);
			// OUT(item_tint.Slot[r].Color);
		}
		for (r = 0; r < structs::MAX_PP_AA_ARRAY; r++)
		{
			OUT_DIFF(aa_array[r].aa, aa_array[r].AA);
			eq->aa_array[r].aa = emu->aa_array[r].AA;
			OUT(aa_array[r].value);
		}
		OUT(points);
		OUT(mana);
		OUT(cur_hp);
		eq->str = emu->STR;
		eq->sta = emu->STA;
		eq->cha = emu->CHA;
		eq->dex = emu->DEX;
		eq->intel = emu->INT;
		eq->agi = emu->AGI;
		eq->wis = emu->WIS;
		OUT(face);

		if (spells::SPELLBOOK_SIZE <= EQ::spells::SPELLBOOK_SIZE)
		{
			for (uint32 r = 0; r < spells::SPELLBOOK_SIZE; r++)
			{
				if (emu->spell_book[r] <= spells::SPELL_ID_MAX)
					eq->spell_book[r] = emu->spell_book[r];
				else
					eq->spell_book[r] = 0xFFFFFFFFU;
			}
		}
		else
		{
			for (uint32 r = 0; r < EQ::spells::SPELLBOOK_SIZE; r++)
			{
				if (emu->spell_book[r] <= spells::SPELL_ID_MAX)
					eq->spell_book[r] = emu->spell_book[r];
				else
					eq->spell_book[r] = 0xFFFFFFFFU;
			}
			// invalidate the rest of the spellbook slots
			memset(&eq->spell_book[EQ::spells::SPELLBOOK_SIZE], 0xFF, (sizeof(uint32) * (spells::SPELLBOOK_SIZE - EQ::spells::SPELLBOOK_SIZE)));
		}

		OUT_array(mem_spells, spells::SPELL_GEM_COUNT);
		OUT(platinum);
		OUT(gold);
		OUT(silver);
		OUT(copper);
		OUT(platinum_cursor);
		OUT(gold_cursor);
		OUT(silver_cursor);
		OUT(copper_cursor);

		OUT_array(skills, structs::MAX_PP_SKILL);			   // 1:1 direct copy (100 dword)
		OUT_DIFF_array(innate_skills, InnateSkills, structs::MAX_PP_INNATE_SKILL); // 1:1 direct copy (25 dword)

		OUT(toxicity);
		OUT(thirst_level);
		OUT(hunger_level);
		for (r = 0; r < structs::BUFF_COUNT; r++)
		{
			OUT(buffs[r].effect_type);
			OUT(buffs[r].level);
			OUT(buffs[r].bard_modifier);
			OUT(buffs[r].spellid);
			OUT(buffs[r].duration);
			OUT(buffs[r].counters);
			OUT(buffs[r].player_id);
		}
		for (r = 0; r < structs::MAX_PP_DISCIPLINES; r++)
		{
			OUT(disciplines.values[r]);
		}
		//	OUT_array(recastTimers, structs::MAX_RECAST_TYPES);
		OUT(endurance);
		OUT(aapoints_spent);
		OUT(aapoints);

		// Copy bandoliers where server and client indices converge
		for (r = 0; r < EQ::profile::BANDOLIERS_SIZE && r < profile::BANDOLIERS_SIZE; ++r)
		{
			OUT_DIFF_str(bandoliers[r].name, bandoliers[r].Name);
			for (uint32 k = 0; k < profile::BANDOLIER_ITEM_COUNT; ++k)
			{ // Will need adjusting if 'server != client' is ever true
				OUT_DIFF(bandoliers[r].items[k].id, bandoliers[r].Items[k].ID);
				OUT_DIFF(bandoliers[r].items[k].icon, bandoliers[r].Items[k].Icon);
				OUT_DIFF_str(bandoliers[r].items[k].name, bandoliers[r].Items[k].Name);
			}
		}
		// Nullify bandoliers where server and client indices diverge, with a client bias
		for (r = EQ::profile::BANDOLIERS_SIZE; r < profile::BANDOLIERS_SIZE; ++r)
		{
			eq->bandoliers[r].name[0] = '\0';
			for (uint32 k = 0; k < profile::BANDOLIER_ITEM_COUNT; ++k)
			{ // Will need adjusting if 'server != client' is ever true
				eq->bandoliers[r].items[k].id = 0;
				eq->bandoliers[r].items[k].icon = 0;
				eq->bandoliers[r].items[k].name[0] = '\0';
			}
		}

		// Copy potion belt where server and client indices converge
		for (r = 0; r < EQ::profile::POTION_BELT_SIZE && r < profile::POTION_BELT_SIZE; ++r)
		{
			OUT_DIFF(potionbelt.items[r].id, potionbelt.Items[r].ID);
			OUT_DIFF(potionbelt.items[r].icon, potionbelt.Items[r].Icon);
			OUT_DIFF_str(potionbelt.items[r].name, potionbelt.Items[r].Name);
		}
		// Nullify potion belt where server and client indices diverge, with a client bias
		for (r = EQ::profile::POTION_BELT_SIZE; r < profile::POTION_BELT_SIZE; ++r)
		{
			eq->potionbelt.items[r].id = 0;
			eq->potionbelt.items[r].icon = 0;
			eq->potionbelt.items[r].name[0] = '\0';
		}

		OUT_str(name);
		OUT_str(last_name);
		OUT(guild_id);
		OUT(birthday);
		OUT(lastlogin);
		OUT_DIFF(time_played_min, timePlayedMin);
		OUT(pvp);
		OUT(anon);
		OUT(gm);
		OUT(guildrank);
		OUT(guildbanker);
		OUT(exp);
		OUT(timeentitledonaccount);
		OUT_array(languages, structs::MAX_PP_LANGUAGE);
		OUT(x);
		OUT(y);
		OUT(z);
		OUT(heading);
		OUT(platinum_bank);
		OUT(gold_bank);
		OUT(silver_bank);
		OUT(copper_bank);
		OUT(platinum_shared);
		OUT(expansions);
		OUT(autosplit);
		OUT(zone_id);
		OUT_DIFF(zone_instance, zoneInstance);
		for (r = 0; r < structs::MAX_GROUP_MEMBERS; r++)
		{
			//OUT_str(groupMembers[r]);
		}
		
		//OUT_DIFF_str(group_leader, groupLeader);
		OUT(entityid);
		OUT_DIFF(lead_aa_active, leadAAActive);
		OUT(ldon_points_guk);
		OUT(ldon_points_mir);
		OUT(ldon_points_mmc);
		OUT(ldon_points_ruj);
		OUT(ldon_points_tak);
		OUT(ldon_points_available);
		OUT(tribute_time_remaining);
		OUT(career_tribute_points);
		OUT(tribute_points);
		OUT(tribute_active);
		for (r = 0; r < structs::MAX_PLAYER_TRIBUTES; r++)
		{
			OUT(tributes[r].tribute);
			OUT(tributes[r].tier);
		}
		OUT(group_leadership_exp);
		OUT(raid_leadership_exp);
		OUT(group_leadership_points);
		OUT(raid_leadership_points);
		//OUT_array(leader_abilities.ranks, structs::MAX_LEADERSHIP_AA_ARRAY);
		OUT(air_remaining);
		OUT_DIFF(pvp_kills, PVPKills);
		OUT_DIFF(pvp_deaths, PVPDeaths);
		OUT_DIFF(pvp_current_points, PVPCurrentPoints);
		OUT_DIFF(pvp_career_points, PVPCareerPoints);
		OUT_DIFF(pvp_best_kill_streak, PVPBestKillStreak);
		OUT_DIFF(pvp_worst_death_streak, PVPWorstDeathStreak);
		OUT_DIFF(pvp_current_kill_streak, PVPCurrentKillStreak);
		OUT_DIFF(exp_aa, expAA);
		OUT_DIFF(current_rad_crystals, currentRadCrystals);
		OUT_DIFF(career_rad_crystals, careerRadCrystals);
		OUT_DIFF(current_ebon_crystals, currentEbonCrystals);
		OUT_DIFF(career_ebon_crystals, careerEbonCrystals);
		OUT_DIFF(group_autoconsent, groupAutoconsent);
		OUT_DIFF(raid_autoconsent, raidAutoconsent);
		OUT_DIFF(guild_autoconsent, guildAutoconsent);
		eq->level3 = emu->level;
		eq->showhelm = emu->showhelm;

		const uint8 bytes[] = {
			0x78, 0x03, 0x00, 0x00, 0x1A, 0x04, 0x00, 0x00, 0x1A, 0x04, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00,
			0x19, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
			0x0F, 0x00, 0x00, 0x00, 0x1F, 0x85, 0xEB, 0x3E, 0x33, 0x33, 0x33, 0x3F, 0x09, 0x00, 0x00, 0x00,
			0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14};

		// set the checksum...
		CRC32::SetEQChecksum(__packet->pBuffer, sizeof(structs::PlayerProfile_Struct) - 4);

		FINISH_ENCODE();
	}

	ENCODE(OP_MarkRaidNPC)
	{
		ENCODE_LENGTH_EXACT(MarkNPC_Struct);
		SETUP_DIRECT_ENCODE(MarkNPC_Struct, MarkNPC_Struct);

		EQApplicationPacket *outapp = new EQApplicationPacket(OP_MarkNPC, sizeof(MarkNPC_Struct));
		MarkNPC_Struct *mnpcs = (MarkNPC_Struct *)outapp->pBuffer;
		mnpcs->TargetID = emu->TargetID;
		mnpcs->Number = emu->Number;
		dest->QueuePacket(outapp);
		safe_delete(outapp);

		FINISH_ENCODE();
	}

	ENCODE(OP_RaidUpdate)
	{
		EQApplicationPacket *inapp = *p;
		*p = nullptr;
		unsigned char *__emu_buffer = inapp->pBuffer;
		RaidGeneral_Struct *raid_gen = (RaidGeneral_Struct *)__emu_buffer;

		switch (raid_gen->action)
		{
		case raidAdd:
		{
			RaidAddMember_Struct *emu = (RaidAddMember_Struct *)__emu_buffer;

			auto outapp = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidAddMember_Struct));
			structs::RaidAddMember_Struct *eq = (structs::RaidAddMember_Struct *)outapp->pBuffer;

			OUT_DIFF(raid_gen.action, raidGen.action);
			OUT_DIFF(raid_gen.parameter, raidGen.parameter);
			OUT_DIFF_str(raid_gen.leader_name, raidGen.leader_name);
			OUT_DIFF_str(raid_gen.player_name, raidGen.player_name);
			eq->char_class = emu->_class;

			OUT(level);
			OUT_DIFF(is_group_leader, isGroupLeader);

			dest->FastQueuePacket(&outapp);
			break;
		}
		case raidSetMotd:
		{
			RaidMOTD_Struct *emu = (RaidMOTD_Struct *)__emu_buffer;

			auto outapp = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidMOTD_Struct));
			structs::RaidMOTD_Struct *eq = (structs::RaidMOTD_Struct *)outapp->pBuffer;

			OUT(general.action);
			OUT_str(general.player_name);
			OUT_str(general.leader_name);
			OUT_str(motd);

			dest->FastQueuePacket(&outapp);
			break;
		}
		case raidSetLeaderAbilities:
		case raidMakeLeader:
		{
			RaidLeadershipUpdate_Struct *emu = (RaidLeadershipUpdate_Struct *)__emu_buffer;

			auto outapp = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidLeadershipUpdate_Struct));
			structs::RaidLeadershipUpdate_Struct *eq = (structs::RaidLeadershipUpdate_Struct *)outapp->pBuffer;

			OUT(action);
			OUT_str(player_name);
			OUT_str(leader_name);
			memcpy(&eq->raid, &emu->raid, sizeof(RaidLeadershipAA_Struct));

			dest->FastQueuePacket(&outapp);
			break;
		}
		case raidSetNote:
		{
			auto emu = (RaidNote_Struct *)__emu_buffer;

			auto outapp = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidNote_Struct));
			auto eq = (structs::RaidNote_Struct *)outapp->pBuffer;

			OUT(general.action);
			OUT_str(general.leader_name);
			OUT_str(general.player_name);
			OUT_str(note);

			dest->FastQueuePacket(&outapp);
			break;
		}
		case raidNoRaid:
		{
			dest->QueuePacket(inapp);
			break;
		}
		default:
		{
			RaidGeneral_Struct *emu = (RaidGeneral_Struct *)__emu_buffer;

			auto outapp = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidGeneral_Struct));
			structs::RaidGeneral_Struct *eq = (structs::RaidGeneral_Struct *)outapp->pBuffer;

			OUT(action);
			OUT(parameter);
			OUT_str(leader_name);
			OUT_str(player_name);

			dest->FastQueuePacket(&outapp);
			break;
		}
		}
		safe_delete(inapp);
	}

	ENCODE(OP_ReadBook)
	{
		// no apparent slot translation needed
		EQApplicationPacket *in = *p;
		*p = nullptr;

		unsigned char *__emu_buffer = in->pBuffer;

		BookText_Struct *emu_BookText_Struct = (BookText_Struct *)__emu_buffer;

		in->size = sizeof(structs::BookText_Struct) + strlen(emu_BookText_Struct->booktext);
		in->pBuffer = new unsigned char[in->size];

		structs::BookText_Struct *eq_BookText_Struct = (structs::BookText_Struct *)in->pBuffer;

		eq_BookText_Struct->window = emu_BookText_Struct->window;
		eq_BookText_Struct->type = emu_BookText_Struct->type;
		strcpy(eq_BookText_Struct->booktext, emu_BookText_Struct->booktext);

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_RespondAA)
	{
		ENCODE_LENGTH_EXACT(AATable_Struct);
		SETUP_DIRECT_ENCODE(AATable_Struct, structs::AATable_Struct);

		unsigned int r;
		for (r = 0; r < structs::MAX_PP_AA_ARRAY; r++)
		{
			OUT_DIFF(aa_list[r].aa, aa_list[r].AA);
			OUT(aa_list[r].value);
		}

		FINISH_ENCODE();
	}

	ENCODE(OP_SendAATable)
	{
		EQApplicationPacket *inapp = *p;
		*p = nullptr;
		AARankInfo_Struct *emu = (AARankInfo_Struct *)inapp->pBuffer;

		auto outapp = new EQApplicationPacket(
			OP_SendAATable, sizeof(structs::SendAA_Struct) + emu->total_effects * sizeof(structs::AA_Ability));
		structs::SendAA_Struct *eq = (structs::SendAA_Struct *)outapp->pBuffer;

		inapp->SetReadPosition(sizeof(AARankInfo_Struct));
		outapp->SetWritePosition(sizeof(structs::SendAA_Struct));

		eq->id = emu->id;
		eq->id = emu->id;
		eq->hotkey_sid = emu->upper_hotkey_sid;
		eq->hotkey_sid2 = emu->lower_hotkey_sid;
		eq->desc_sid = emu->desc_sid;
		eq->title_sid = emu->title_sid;
		eq->class_type = emu->level_req;
		eq->cost = emu->cost;
		eq->seq = emu->seq;
		eq->current_level = emu->current_level;
		eq->type = emu->type;
		eq->spellid = emu->spell;
		eq->spell_type = emu->spell_type;
		eq->spell_refresh = emu->spell_refresh;
		eq->classes = emu->classes;
		eq->max_level = emu->max_level;
		eq->last_id = emu->prev_id;
		eq->next_id = emu->next_id;
		eq->cost2 = emu->total_cost;
		eq->count = emu->total_effects;

		for (auto i = 0; i < eq->count; ++i)
		{
			eq->abilities[i].skill_id = inapp->ReadUInt32();
			eq->abilities[i].base_value = inapp->ReadUInt32();
			eq->abilities[i].limit_value = inapp->ReadUInt32();
			eq->abilities[i].slot = inapp->ReadUInt32();
		}

		if (emu->total_prereqs > 0)
		{
			eq->prereq_skill = inapp->ReadUInt32();
			eq->prereq_minpoints = inapp->ReadUInt32();
		}

		dest->FastQueuePacket(&outapp);
		delete inapp;
	}

	ENCODE(OP_SendCharInfo)
	{
		ENCODE_LENGTH_ATLEAST(CharacterSelect_Struct);
		SETUP_DIRECT_ENCODE(CharacterSelect_Struct, structs::CharacterSelect_Struct);

		unsigned char *emu_ptr = __emu_buffer;
		emu_ptr += sizeof(CharacterSelect_Struct);
		CharacterSelectEntry_Struct *emu_cse = (CharacterSelectEntry_Struct *)nullptr;

		size_t char_index = 0;
		std::vector<Web::structs::CharacterSelectEntry_Struct*> characters;
		for (; char_index < emu->CharCount && char_index < 8; ++char_index)
		{
			emu_cse = (CharacterSelectEntry_Struct *)emu_ptr;
			auto character = (structs::CharacterSelectEntry_Struct*)malloc(sizeof(structs::CharacterSelectEntry_Struct));
			*character = (structs::CharacterSelectEntry_Struct) {
				.name = "",
				.char_class = emu_cse->Class,
				.race = emu_cse->Race > 473 ? 1 : emu_cse->Race,
				.level = emu_cse->Level,
				.zone = emu_cse->Zone,
				.instance = emu_cse->Instance,
				.gender = emu_cse->Gender,
				.face = emu_cse->Face,
				.equip = {},
				.deity = emu_cse->Deity,
				.primary_id_file = emu_cse->PrimaryIDFile,
				.secondary_id_file = emu_cse->SecondaryIDFile,
				.go_home = emu_cse->GoHome,
				.enabled = emu_cse->Enabled,
				.last_login = emu_cse->LastLogin,
				.next = nullptr,
			};

			memcpy(character->name, emu_cse->Name, 64);

			for (int index = 0; index < EQ::textures::materialCount; ++index)
			{
				auto color = emu_cse->Equip[index].Color;
				character->equip[index].color = {
					.blue = (uint8)(color),
					.green = (uint8)(color >> 8),
					.red = (uint8)(color >> 16),
					.use_tint = (uint8)(color >> 24),
				};
				character->equip[index].material = emu_cse->Equip[index].Material;
			}

			if (!characters.empty())
			{
				characters.back()->next = character;
			}

			characters.push_back(character);

			emu_ptr += sizeof(CharacterSelectEntry_Struct);
		}
		eq->characters = characters.size() > 0 ? characters[0] : nullptr;
		eq->character_count = characters.size();
		FINISH_ENCODE();
	}


	ENCODE(OP_SetFace)
	{
		auto emu = reinterpret_cast<FaceChange_Struct *>((*p)->pBuffer);

		EQApplicationPacket outapp(OP_Illusion, sizeof(structs::Illusion_Struct));
		auto buf = reinterpret_cast<structs::Illusion_Struct *>(outapp.pBuffer);

		buf->spawnid = emu->entity_id;
		buf->race = -1;		   // unchanged
		buf->gender = -1;	   // unchanged
		buf->texture = -1;	   // unchanged
		buf->helmtexture = -1; // unchanged
		buf->face = emu->face;
		buf->hairstyle = emu->hairstyle;
		buf->haircolor = emu->haircolor;
		buf->beard = emu->beard;
		buf->beardcolor = emu->beardcolor;
		buf->size = 0.0f; // unchanged

		safe_delete(*p); // not using the original packet

		dest->QueuePacket(&outapp, ack_req);
	}

	ENCODE(OP_ShopPlayerSell)
	{
		ENCODE_LENGTH_EXACT(Merchant_Purchase_Struct);
		SETUP_DIRECT_ENCODE(Merchant_Purchase_Struct, structs::Merchant_Purchase_Struct);

		OUT(npcid);
		eq->itemslot = ServerToWebSlot(emu->itemslot);
		OUT(quantity);
		OUT(price);

		FINISH_ENCODE();
	}

	ENCODE(OP_SpecialMesg)
	{
		EQApplicationPacket *in = *p;
		*p = nullptr;

		SerializeBuffer buf(in->size);
		buf.WriteInt8(in->ReadUInt8());	  // speak mode
		buf.WriteInt8(in->ReadUInt8());	  // journal mode
		buf.WriteInt8(in->ReadUInt8());	  // language
		buf.WriteInt32(in->ReadUInt32()); // message type
		buf.WriteInt32(in->ReadUInt32()); // target spawn id

		std::string name;
		in->ReadString(name); // NPC names max out at 63 chars

		buf.WriteString(name);

		buf.WriteInt32(in->ReadUInt32()); // loc
		buf.WriteInt32(in->ReadUInt32());
		buf.WriteInt32(in->ReadUInt32());

		std::string old_message;
		std::string new_message;

		in->ReadString(old_message);

		ServerToWebSayLink(new_message, old_message);

		buf.WriteString(new_message);

		auto outapp = new EQApplicationPacket(OP_SpecialMesg, buf);

		dest->FastQueuePacket(&outapp, ack_req);
		delete in;
	}

	ENCODE(OP_TaskDescription)
	{
		EQApplicationPacket *in = *p;
		*p = nullptr;

		unsigned char *__emu_buffer = in->pBuffer;

		char *InBuffer = (char *)in->pBuffer;
		char *block_start = InBuffer;

		InBuffer += sizeof(TaskDescriptionHeader_Struct);
		uint32 title_size = strlen(InBuffer) + 1;
		InBuffer += title_size;
		InBuffer += sizeof(TaskDescriptionData1_Struct);
		uint32 description_size = strlen(InBuffer) + 1;
		InBuffer += description_size;
		InBuffer += sizeof(TaskDescriptionData2_Struct);

		uint32 reward_size = strlen(InBuffer) + 1;
		InBuffer += reward_size;

		std::string old_message = InBuffer; // start item link string
		std::string new_message;
		ServerToWebSayLink(new_message, old_message);

		in->size = sizeof(TaskDescriptionHeader_Struct) + sizeof(TaskDescriptionData1_Struct) +
				   sizeof(TaskDescriptionData2_Struct) + sizeof(TaskDescriptionTrailer_Struct) +
				   title_size + description_size + reward_size + new_message.length() + 1;

		in->pBuffer = new unsigned char[in->size];

		char *OutBuffer = (char *)in->pBuffer;

		memcpy(OutBuffer, block_start, (InBuffer - block_start));
		OutBuffer += (InBuffer - block_start);

		VARSTRUCT_ENCODE_STRING(OutBuffer, new_message.c_str());

		InBuffer += strlen(InBuffer) + 1;

		memcpy(OutBuffer, InBuffer, sizeof(TaskDescriptionTrailer_Struct));
		// we have an extra DWORD in the trailer struct, client should ignore it so w/e

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_Track)
	{
		EQApplicationPacket *in = *p;
		*p = nullptr;

		unsigned char *__emu_buffer = in->pBuffer;
		Track_Struct *emu = (Track_Struct *)__emu_buffer;

		int EntryCount = in->size / sizeof(Track_Struct);

		if (EntryCount == 0 || ((in->size % sizeof(Track_Struct))) != 0)
		{
			LogNetcode("[STRUCTS] Wrong size on outbound [{}]: Got [{}], expected multiple of [{}]", opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(Track_Struct));
			delete in;
			return;
		}

		in->size = sizeof(structs::Track_Struct) * EntryCount;
		in->pBuffer = new unsigned char[in->size];
		structs::Track_Struct *eq = (structs::Track_Struct *)in->pBuffer;

		for (int i = 0; i < EntryCount; ++i, ++eq, ++emu)
		{
			OUT(entityid);
			// OUT(padding002);
			OUT(distance);
		}

		delete[] __emu_buffer;
		dest->FastQueuePacket(&in, ack_req);
	}

	ENCODE(OP_Trader)
	{
		if ((*p)->size != sizeof(TraderBuy_Struct))
		{
			EQApplicationPacket *in = *p;
			*p = nullptr;
			dest->FastQueuePacket(&in, ack_req);
			return;
		}

		ENCODE_FORWARD(OP_TraderBuy);
	}

	ENCODE(OP_TraderBuy)
	{
		ENCODE_LENGTH_EXACT(TraderBuy_Struct);
		SETUP_DIRECT_ENCODE(TraderBuy_Struct, structs::TraderBuy_Struct);

		OUT_DIFF(action, Action);
		OUT_DIFF(price, Price);
		OUT_DIFF(trader_id, TraderID);
		memcpy(eq->item_name, emu->ItemName, sizeof(eq->item_name));
		OUT_DIFF(item_id, ItemID);
		OUT_DIFF(quantity, Quantity);
		OUT_DIFF(already_sold, AlreadySold);

		FINISH_ENCODE();
	}

	ENCODE(OP_TributeItem)
	{
		ENCODE_LENGTH_EXACT(TributeItem_Struct);
		SETUP_DIRECT_ENCODE(TributeItem_Struct, structs::TributeItem_Struct);

		eq->slot = ServerToWebSlot(emu->slot);
		OUT(quantity);
		OUT(tribute_master_id);
		OUT(tribute_points);

		FINISH_ENCODE();
	}

	ENCODE(OP_VetRewardsAvaliable)
	{
		EQApplicationPacket *inapp = *p;
		unsigned char *__emu_buffer = inapp->pBuffer;

		uint32 count = ((*p)->Size() / sizeof(InternalVeteranReward));
		*p = nullptr;

		auto outapp_create =
			new EQApplicationPacket(OP_VetRewardsAvaliable, (sizeof(structs::VeteranReward) * count));
		uchar *old_data = __emu_buffer;
		uchar *data = outapp_create->pBuffer;
		for (uint32 i = 0; i < count; ++i)
		{
			structs::VeteranReward *vr = (structs::VeteranReward *)data;
			InternalVeteranReward *ivr = (InternalVeteranReward *)old_data;

			vr->claim_id = ivr->claim_id;
			vr->item.item_id = ivr->items[0].item_id;
			strcpy(vr->item.item_name, ivr->items[0].item_name);

			old_data += sizeof(InternalVeteranReward);
			data += sizeof(structs::VeteranReward);
		}

		dest->FastQueuePacket(&outapp_create);
		delete inapp;
	}

	ENCODE(OP_GuildsList)
	{
		EQApplicationPacket *in = *p;
		*p = nullptr;

		uint32 NumberOfGuilds = in->size / 64;
		char *InBuffer = (char *)in->pBuffer;

		auto names = std::vector<structs::StringList*>{};
		structs::StringList* last;
		for (unsigned int i = 0; i < NumberOfGuilds; ++i)
		{
			if (InBuffer[0])
			{
				auto guild_name = (structs::StringList *)malloc(sizeof(structs::StringList));
				*guild_name = (structs::StringList) {
					.str = InBuffer,
					.next = nullptr,
				};
				if (!names.empty())
				{
					names.back()->next = guild_name;
				}
				names.push_back(guild_name);
				last = guild_name;
			}
			InBuffer += 64;
		}


		structs::GuildsList_Struct guildlist_struct{
			.guilds = names.size() > 0 ? names.at(0) : nullptr
		};
		auto guildlist_packet =
			new EQApplicationPacket(OP_GuildsList, (sizeof(structs::GuildsList_Struct)));
		guildlist_packet->WriteData(&guildlist_struct, sizeof(structs::GuildsList_Struct));
		dest->FastQueuePacket(&guildlist_packet);
		delete[] in->pBuffer;
	}

	ENCODE(OP_WearChange)
	{
		ENCODE_LENGTH_EXACT(WearChange_Struct);
		SETUP_DIRECT_ENCODE(WearChange_Struct, structs::WearChange_Struct);

		OUT(spawn_id);
		OUT(material);
		// OUT(color.Color);
		OUT(wear_slot_id);

		FINISH_ENCODE();
	}

	ENCODE(OP_ZoneEntry) { ENCODE_FORWARD(OP_ZoneSpawns); }

	ENCODE(OP_ZoneSpawns)
	{
		SETUP_DIRECT_ENCODE(Spawn_Struct, structs::Spawns_Struct);
		int spawn_count = __packet->size / sizeof(Spawn_Struct);

		std::vector<structs::Spawn_Struct*> spawns;
		for (int r = 0; r < spawn_count; r++)
		{
			auto spawn = (structs::Spawn_Struct*)malloc(sizeof(structs::Spawn_Struct));
			*spawn = (structs::Spawn_Struct) {
			.gm = emu->gm,
			.aaitle = emu->aaitle,
			.anon = emu->anon,
			.face = emu->face,
			.name="",
			.deity = emu->deity,
			.size = emu->size,
			.npc = emu->NPC,
			.invis = emu->invis,
			.haircolor = emu->haircolor,
			.cur_hp = emu->curHp,
			.max_hp = emu->max_hp,
			.findable = emu->findable,
			.delta_heading = emu->deltaHeading,
			.x = emu->x,
			.y = emu->y,
			.animation = emu->animation,
			.z = emu->z,
			.delta_y = emu->deltaY,
			.delta_x = emu->deltaX,
			.heading = emu->heading,
			.delta_z = emu->deltaZ,
			.eyecolor1 = emu->eyecolor1,
			.showhelm = emu->showhelm,
			.is_npc = emu->is_npc,
			.hairstyle = emu->hairstyle,
			.beardcolor = emu->beardcolor,
			.level = emu->level,
			.player_state = emu->PlayerState,
			.beard = emu->beard,
			.suffix = "",
			.pet_owner_id = emu->petOwnerId,
			.guildrank = emu->guildrank,
			.equipment = {},
			.runspeed = emu->runspeed,
			.afk = emu->afk,
			.guild_id = emu->guildID,
			.title = "",
			.helm = emu->helm,
			.race = emu->race > 473 ? 1 : emu->race,
			.last_name = "",
			.walkspeed = emu->walkspeed,
			.is_pet = emu->is_pet,
			.light = emu->light,
			.char_class = emu->class_,
			.eyecolor2 = emu->eyecolor2,
			.flymode = emu->flymode,
			.gender = emu->gender,
			.bodytype = emu->bodytype,
			.equip_chest2 = emu->equip_chest2,
			.spawn_id = emu->spawnId,
			.lfg = emu->lfg,
			.next = nullptr,
				
			};

			strncpy(spawn->title, emu[r].title, sizeof(spawn->title));
			strncpy(spawn->last_name, emu[r].lastName, sizeof(spawn->last_name));
			strncpy(spawn->name, emu[r].name, sizeof(spawn->name));
			strncpy(spawn->suffix, emu[r].suffix, sizeof(spawn->suffix));

			for (int k = EQ::textures::textureBegin; k < EQ::textures::materialCount; k++)
			{
				//	eq->equipment.Slot[k].Material = emu->equipment.Slot[k].Material;
				//	eq->equipment_tint.Slot[k].Color = emu->equipment_tint.Slot[k].Color;
			}

			if (!spawns.empty())
			{
				spawns.back()->next = spawn;
			}
			spawns.push_back(spawn);
		}
		eq->spawn_count = spawn_count;
		eq->spawns = spawns.size() > 0 ?  spawns[0] : nullptr;
		FINISH_ENCODE();
	}




	// These are shared op codes from client <--> server that need bidirectional treatment. By default we map client-->server to data bindings
	ENCODE(OP_ApproveName) {
		SETUP_DIRECT_ENCODE(structs::Int_Struct, structs::Int_Struct);
		eq->value = (int)__emu_buffer[0];
		__packet->SetOpcode((EmuOpcode)OP_ApproveName_Server);
		FINISH_ENCODE();
	}

	// DECODE methods
	DECODE(OP_SendLoginInfo)
	{
		// DECODE_LENGTH_EXACT(structs::LoginInfo_Struct);
		SETUP_DIRECT_DECODE(LoginInfo_Struct, structs::LoginInfo_Struct);
		std::string name(eq->name);
		std::string pw(eq->password);
		int idx = 0;
		for (int i = 0; i < name.length(); i++, idx++)
		{
			emu->login_info[idx] = name[i];
		}
		emu->login_info[idx++] = '\0';
		for (int i = 0; i < pw.length(); i++, idx++)
		{
			emu->login_info[idx] = pw[i];
		}
		IN(zoning);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DeleteCharacter)
	{
		SETUP_DIRECT_DECODE(char*, structs::String_Struct);
		__packet->pBuffer = (unsigned char*)eq->value;
	}

	DECODE(OP_AdventureMerchantSell)
	{
		DECODE_LENGTH_EXACT(structs::Adventure_Sell_Struct);
		SETUP_DIRECT_DECODE(Adventure_Sell_Struct, structs::Adventure_Sell_Struct);

		IN(npcid);
		emu->slot = WebToServerSlot(eq->slot);
		IN(charges);
		IN(sell_price);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_ApplyPoison)
	{
		DECODE_LENGTH_EXACT(structs::ApplyPoison_Struct);
		SETUP_DIRECT_DECODE(ApplyPoison_Struct, structs::ApplyPoison_Struct);

		emu->inventorySlot = WebToServerSlot(eq->inventory_slot);
		IN(success);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_AugmentItem)
	{
		DECODE_LENGTH_EXACT(structs::AugmentItem_Struct);
		SETUP_DIRECT_DECODE(AugmentItem_Struct, structs::AugmentItem_Struct);

		emu->container_slot = WebToServerSlot(eq->container_slot);
		emu->augment_slot = eq->augment_slot;

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_Buff)
	{
		DECODE_LENGTH_EXACT(structs::SpellBuffPacket_Struct);
		SETUP_DIRECT_DECODE(SpellBuffPacket_Struct, structs::SpellBuffPacket_Struct);

		IN(entityid);
		IN(buff.effect_type);
		IN(buff.level);
		IN(buff.bard_modifier);
		IN(buff.spellid);
		IN(buff.duration);
		IN(buff.counters);
		IN(buff.player_id);
		emu->slotid = WebToServerBuffSlot(eq->slotid);
		IN(bufffade);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_Bug)
	{
		DECODE_LENGTH_EXACT(structs::BugReport_Struct);
		SETUP_DIRECT_DECODE(BugReport_Struct, structs::BugReport_Struct);

		emu->category_id = EQ::bug::CategoryNameToCategoryID(eq->category_name);
		memcpy(emu->category_name, eq, sizeof(structs::BugReport_Struct));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_CastSpell)
	{
		DECODE_LENGTH_EXACT(structs::CastSpell_Struct);
		SETUP_DIRECT_DECODE(CastSpell_Struct, structs::CastSpell_Struct);

		emu->slot = static_cast<uint32>(WebToServerCastingSlot(static_cast<spells::CastingSlot>(eq->slot), eq->inventoryslot));
		IN(spell_id);
		emu->inventoryslot = WebToServerSlot(eq->inventoryslot);
		IN(target_id);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_ChannelMessage)
	{
		unsigned char *__eq_buffer = __packet->pBuffer;

		std::string old_message = (char *)&__eq_buffer[sizeof(ChannelMessage_Struct)];
		std::string new_message;
		WebToServerSayLink(new_message, old_message);

		__packet->size = sizeof(ChannelMessage_Struct) + new_message.length() + 1;
		__packet->pBuffer = new unsigned char[__packet->size];

		ChannelMessage_Struct *emu = (ChannelMessage_Struct *)__packet->pBuffer;

		memcpy(emu, __eq_buffer, sizeof(ChannelMessage_Struct));
		strcpy(emu->message, new_message.c_str());

		delete[] __eq_buffer;
	}

	DECODE(OP_CharacterCreate)
	{
		DECODE_LENGTH_EXACT(structs::CharCreate_Struct);
		SETUP_DIRECT_DECODE(CharCreate_Struct, structs::CharCreate_Struct);

		emu->class_ = eq->char_class;
		IN(beardcolor);
		IN(beard);
		IN(haircolor);
		IN(gender);	
		IN(race);
		IN(start_zone);
		IN(hairstyle);
		IN(deity);
		IN_DIFF(STR, str);
		IN_DIFF(STA, sta);
		IN_DIFF(AGI, agi);
		IN_DIFF(DEX, dex);
		IN_DIFF(WIS, wis);
		IN_DIFF(INT, intel);
		IN_DIFF(CHA, cha);
		IN(face);
		IN(eyecolor1);
		IN(eyecolor2);
		IN(tutorial);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_ClientUpdate)
	{
		SETUP_DIRECT_DECODE(PlayerPositionUpdateClient_Struct, structs::PlayerPositionUpdateClient_Struct);

		IN(spawn_id);
		IN(sequence);
		IN(x_pos);
		IN(y_pos);
		IN(z_pos);
		IN(heading);
		IN(delta_x);
		IN(delta_y);
		IN(delta_z);
		IN(delta_heading);
		IN(animation);
		emu->vehicle_id = 0;

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_Consume)
	{
		DECODE_LENGTH_EXACT(structs::Consume_Struct);
		SETUP_DIRECT_DECODE(Consume_Struct, structs::Consume_Struct);

		emu->slot = WebToServerSlot(eq->slot);
		IN(auto_consumed);
		IN(type);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DeleteItem)
	{
		DECODE_LENGTH_EXACT(structs::DeleteItem_Struct);
		SETUP_DIRECT_DECODE(DeleteItem_Struct, structs::DeleteItem_Struct);

		emu->from_slot = WebToServerSlot(eq->from_slot);
		emu->to_slot = WebToServerSlot(eq->to_slot);
		IN(number_in_stack);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DzAddPlayer)
	{
		DECODE_LENGTH_EXACT(structs::ExpeditionCommand_Struct);
		SETUP_DIRECT_DECODE(ExpeditionCommand_Struct, structs::ExpeditionCommand_Struct);

		strn0cpy(emu->name, eq->name, sizeof(emu->name));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DzChooseZoneReply)
	{
		DECODE_LENGTH_EXACT(structs::DynamicZoneChooseZoneReply_Struct);
		SETUP_DIRECT_DECODE(DynamicZoneChooseZoneReply_Struct, structs::DynamicZoneChooseZoneReply_Struct);

		IN(dz_zone_id);
		IN(dz_instance_id);
		IN(dz_type);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DzExpeditionInviteResponse)
	{
		DECODE_LENGTH_EXACT(structs::ExpeditionInviteResponse_Struct);
		SETUP_DIRECT_DECODE(ExpeditionInviteResponse_Struct, structs::ExpeditionInviteResponse_Struct);

		IN(dz_zone_id);
		IN(dz_instance_id);
		IN(accepted);
		IN(swapping);
		strn0cpy(emu->swap_name, eq->swap_name, sizeof(emu->swap_name));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DzMakeLeader)
	{
		DECODE_LENGTH_EXACT(structs::ExpeditionCommand_Struct);
		SETUP_DIRECT_DECODE(ExpeditionCommand_Struct, structs::ExpeditionCommand_Struct);

		strn0cpy(emu->name, eq->name, sizeof(emu->name));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DzRemovePlayer)
	{
		DECODE_LENGTH_EXACT(structs::ExpeditionCommand_Struct);
		SETUP_DIRECT_DECODE(ExpeditionCommand_Struct, structs::ExpeditionCommand_Struct);

		strn0cpy(emu->name, eq->name, sizeof(emu->name));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_DzSwapPlayer)
	{
		DECODE_LENGTH_EXACT(structs::ExpeditionCommandSwap_Struct);
		SETUP_DIRECT_DECODE(ExpeditionCommandSwap_Struct, structs::ExpeditionCommandSwap_Struct);

		strn0cpy(emu->add_player_name, eq->add_player_name, sizeof(emu->add_player_name));
		strn0cpy(emu->rem_player_name, eq->rem_player_name, sizeof(emu->rem_player_name));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_Emote)
	{
		unsigned char *__eq_buffer = __packet->pBuffer;

		std::string old_message = (char *)&__eq_buffer[4]; // unknown01 offset
		std::string new_message;
		WebToServerSayLink(new_message, old_message);

		__packet->size = sizeof(Emote_Struct);
		__packet->pBuffer = new unsigned char[__packet->size];

		char *InBuffer = (char *)__packet->pBuffer;

		memcpy(InBuffer, __eq_buffer, 4);
		InBuffer += 4;
		strcpy(InBuffer, new_message.substr(0, 1023).c_str());
		InBuffer[1023] = '\0';

		delete[] __eq_buffer;
	}

	DECODE(OP_FaceChange)
	{
		DECODE_LENGTH_EXACT(structs::FaceChange_Struct);
		SETUP_DIRECT_DECODE(FaceChange_Struct, structs::FaceChange_Struct);
		IN(haircolor);
		IN(beardcolor);
		IN(eyecolor1);
		IN(eyecolor2);
		IN(hairstyle);
		IN(beard);
		IN(face);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_InspectAnswer)
	{
		DECODE_LENGTH_EXACT(structs::InspectResponse_Struct);
		SETUP_DIRECT_DECODE(InspectResponse_Struct, structs::InspectResponse_Struct);

		IN_DIFF(TargetID, target_id);
		IN(playerid);

		for (int i = invslot::slotCharm; i <= invslot::slotWaist; ++i)
		{
			//strn0cpy(emu->itemnames[i], eq->itemnames[i], sizeof(emu->itemnames[i]));
			IN(itemicons[i]);
		}

		// move ammo up to last element in server array
		//strn0cpy(emu->itemnames[EQ::invslot::slotAmmo], eq->itemnames[invslot::slotAmmo], sizeof(emu->itemnames[EQ::invslot::slotAmmo]));
		emu->itemicons[EQ::invslot::slotAmmo] = eq->itemicons[invslot::slotAmmo];

		// nullify power source element in server array
		strn0cpy(emu->itemnames[EQ::invslot::slotPowerSource], "", sizeof(emu->itemnames[EQ::invslot::slotPowerSource]));
		emu->itemicons[EQ::invslot::slotPowerSource] = 0xFFFFFFFF;

		strn0cpy(emu->text, eq->text, sizeof(emu->text));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_InspectRequest)
	{
		DECODE_LENGTH_EXACT(structs::Inspect_Struct);
		SETUP_DIRECT_DECODE(Inspect_Struct, structs::Inspect_Struct);

		IN_DIFF(TargetID, target_id);
		IN_DIFF(PlayerID, player_id);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_ItemLinkClick)
	{
		DECODE_LENGTH_EXACT(structs::ItemViewRequest_Struct);
		SETUP_DIRECT_DECODE(ItemViewRequest_Struct, structs::ItemViewRequest_Struct);
		MEMSET_IN(ItemViewRequest_Struct);

		IN(item_id);
		int r;
		for (r = 0; r < 5; r++)
		{
			IN(augments[r]);
		}
		IN(link_hash);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_LFGuild)
	{
		uint32 Command = __packet->ReadUInt32();

		if (Command != 0)
			return;

		SETUP_DIRECT_DECODE(LFGuild_PlayerToggle_Struct, structs::LFGuild_Playertoggle_Struct);
		memcpy(emu, eq, sizeof(structs::LFGuild_Playertoggle_Struct));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_LoadSpellSet)
	{
		DECODE_LENGTH_EXACT(structs::LoadSpellSet_Struct);
		SETUP_DIRECT_DECODE(LoadSpellSet_Struct, structs::LoadSpellSet_Struct);

		for (int i = 0; i < spells::SPELL_GEM_COUNT; ++i)
			IN(spell[i]);
		for (int i = spells::SPELL_GEM_COUNT; i < EQ::spells::SPELL_GEM_COUNT; ++i)
			emu->spell[i] = 0xFFFFFFFF;

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_LootItem)
	{
		DECODE_LENGTH_EXACT(structs::LootingItem_Struct);
		SETUP_DIRECT_DECODE(LootingItem_Struct, structs::LootingItem_Struct);

		Log(Logs::Detail, Logs::Netcode, "Web::DECODE(OP_LootItem)");

		IN(lootee);
		IN(looter);
		emu->slot_id = WebToServerCorpseSlot(eq->slot_id);
		IN(auto_loot);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_MoveItem)
	{
		DECODE_LENGTH_EXACT(structs::MoveItem_Struct);
		SETUP_DIRECT_DECODE(MoveItem_Struct, structs::MoveItem_Struct);

		Log(Logs::Detail, Logs::Netcode, "Web::DECODE(OP_MoveItem)");

		emu->from_slot = WebToServerSlot(eq->from_slot);
		emu->to_slot = WebToServerSlot(eq->to_slot);
		IN(number_in_stack);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_PetCommands)
	{
		DECODE_LENGTH_EXACT(structs::PetCommand_Struct);
		SETUP_DIRECT_DECODE(PetCommand_Struct, structs::PetCommand_Struct);

		switch (eq->command)
		{
		case 1: // back off
			emu->command = 28;
			break;
		case 2: // get lost
			emu->command = 29;
			break;
		case 3:				  // as you were ???
			emu->command = 4; // fuck it follow
			break;
		case 4: // report HP
			emu->command = 0;
			break;
		case 5: // guard here
			emu->command = 5;
			break;
		case 6:				  // guard me
			emu->command = 4; // fuck it follow
			break;
		case 7: // attack
			emu->command = 2;
			break;
		case 8: // follow
			emu->command = 4;
			break;
		case 9: // sit down
			emu->command = 7;
			break;
		case 10: // stand up
			emu->command = 8;
			break;
		case 11: // taunt toggle
			emu->command = 12;
			break;
		case 12: // hold toggle
			emu->command = 15;
			break;
		case 13: // taunt on
			emu->command = 13;
			break;
		case 14: // no taunt
			emu->command = 14;
			break;
		// 15 is target, doesn't send packet
		case 16: // leader
			emu->command = 1;
			break;
		case 17: // feign
			emu->command = 27;
			break;
		case 18: // no cast toggle
			emu->command = 21;
			break;
		case 19: // focus toggle
			emu->command = 24;
			break;
		default:
			emu->command = eq->command;
		}
		IN(target);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_RaidInvite)
	{
		DECODE_LENGTH_ATLEAST(structs::RaidGeneral_Struct);
		RaidGeneral_Struct *rgs = (RaidGeneral_Struct *)__packet->pBuffer;

		switch (rgs->action)
		{
		case raidSetMotd:
		{
			SETUP_VAR_DECODE(RaidMOTD_Struct, structs::RaidMOTD_Struct, motd);

			IN(general.action);
			IN(general.parameter);
			IN_str(general.leader_name);
			IN_str(general.player_name);

			auto len = 0;
			if (__packet->size < sizeof(structs::RaidMOTD_Struct))
			{
				len = __packet->size - sizeof(structs::RaidGeneral_Struct);
			}
			else
			{
				len = sizeof(eq->motd);
			}

			strn0cpy(emu->motd, eq->motd, len > 1024 ? 1024 : len);
			emu->motd[len - 1] = '\0';

			FINISH_VAR_DECODE();
			break;
		}
		case raidSetNote:
		{
			SETUP_VAR_DECODE(RaidNote_Struct, structs::RaidNote_Struct, note);

			IN(general.action);
			IN(general.parameter);
			IN_str(general.leader_name);
			IN_str(general.player_name);
			IN_str(note);

			FINISH_VAR_DECODE();
			break;
		}
		default:
		{
			SETUP_DIRECT_DECODE(RaidGeneral_Struct, structs::RaidGeneral_Struct);
			IN(action);
			IN(parameter);
			IN_str(leader_name);
			IN_str(player_name);

			FINISH_DIRECT_DECODE();
			break;
		}
		}
	}

	DECODE(OP_ReadBook)
	{
		// no apparent slot translation needed
		DECODE_LENGTH_ATLEAST(structs::BookRequest_Struct);
		SETUP_DIRECT_DECODE(BookRequest_Struct, structs::BookRequest_Struct);

		IN(window);
		IN(type);
		strn0cpy(emu->txtfile, eq->txtfile, sizeof(emu->txtfile));

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_SetServerFilter)
	{
		DECODE_LENGTH_EXACT(structs::SetServerFilter_Struct);
		SETUP_DIRECT_DECODE(SetServerFilter_Struct, structs::SetServerFilter_Struct);

		int r;
		for (r = 0; r < 29; r++)
		{
			IN(filters[r]);
		}

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_ShopPlayerSell)
	{
		DECODE_LENGTH_EXACT(structs::Merchant_Purchase_Struct);
		SETUP_DIRECT_DECODE(Merchant_Purchase_Struct, structs::Merchant_Purchase_Struct);

		IN(npcid);
		emu->itemslot = WebToServerSlot(eq->itemslot);
		IN(quantity);
		IN(price);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_TraderBuy)
	{
		DECODE_LENGTH_EXACT(structs::TraderBuy_Struct);
		SETUP_DIRECT_DECODE(TraderBuy_Struct, structs::TraderBuy_Struct);
		MEMSET_IN(TraderBuy_Struct);

		IN_DIFF(Action, action);
		IN_DIFF(Price, price);
		IN_DIFF(TraderID, trader_id);
		memcpy(emu->ItemName, eq->item_name, sizeof(emu->ItemName));
		IN_DIFF(ItemID, item_id);
		IN_DIFF(Quantity, quantity);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_TradeSkillCombine)
	{
		DECODE_LENGTH_EXACT(structs::NewCombine_Struct);
		SETUP_DIRECT_DECODE(NewCombine_Struct, structs::NewCombine_Struct);

		emu->container_slot = WebToServerSlot(eq->container_slot);
		IN(guildtribute_slot);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_TributeItem)
	{
		DECODE_LENGTH_EXACT(structs::TributeItem_Struct);
		SETUP_DIRECT_DECODE(TributeItem_Struct, structs::TributeItem_Struct);

		emu->slot = WebToServerSlot(eq->slot);
		IN(quantity);
		IN(tribute_master_id);
		IN(tribute_points);

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_WearChange)
	{
		DECODE_LENGTH_EXACT(structs::WearChange_Struct);
		SETUP_DIRECT_DECODE(WearChange_Struct, structs::WearChange_Struct);

		IN(spawn_id);
		IN(material);
		// IN(color.Color);
		IN(wear_slot_id);
		emu->elite_material = 0;
		emu->hero_forge_model = 0;

		FINISH_DIRECT_DECODE();
	}

	DECODE(OP_WhoAllRequest)
	{
		DECODE_LENGTH_EXACT(structs::Who_All_Struct);
		SETUP_DIRECT_DECODE(Who_All_Struct, structs::Who_All_Struct);

		memcpy(emu->whom, eq->whom, sizeof(emu->whom));
		IN(wrace);
		IN(wclass);
		IN(lvllow);
		IN(lvlhigh);
		IN(gmlookup);
		emu->type = 3;

		FINISH_DIRECT_DECODE();
	}

	// file scope helper methods
	void SerializeItem(EQ::OutBuffer &ob, const EQ::ItemInstance *inst, int16 slot_id_in, uint8 depth)
	{
		const char *protection = "\\\\\\\\\\";
		const EQ::ItemData *item = inst->GetUnscaledItem();

		ob << StringFormat(
			"%.*s%s",
			(depth ? (depth - 1) : 0),
			protection,
			(depth ? "\"" : "")); // For leading quotes (and protection) if a subitem;

		// Instance data
		ob << itoa((inst->IsStackable() ? inst->GetCharges() : 0));																			 // stack count
		ob << '|' << itoa((!inst->GetMerchantSlot() ? slot_id_in : inst->GetMerchantSlot()));												 // inst slot/merchant slot
		ob << '|' << itoa(inst->GetPrice());																								 // merchant price
		ob << '|' << itoa((!inst->GetMerchantSlot() ? 1 : inst->GetMerchantCount()));														 // inst count/merchant count
		ob << '|' << itoa((inst->IsScaling() ? (inst->GetExp() / 100) : 0));																 // inst experience
		ob << '|' << itoa((!inst->GetMerchantSlot() ? inst->GetSerialNumber() : inst->GetMerchantSlot()));									 // merchant serial number
		ob << '|' << itoa(inst->GetRecastTimestamp());																						 // recast timestamp
		ob << '|' << itoa(((inst->IsStackable() ? ((inst->GetItem()->ItemType == EQ::item::ItemTypePotion) ? 1 : 0) : inst->GetCharges()))); // charge count
		ob << '|' << itoa((inst->IsAttuned() ? 1 : 0));																						 // inst attuned
		ob << '|';

		ob << StringFormat("%.*s\"", depth, protection); // Quotes (and protection, if needed) around static data

		// Item data
		ob << itoa(item->ItemClass);
		ob << '|' << item->Name;
		ob << '|' << item->Lore;
		ob << '|' << item->IDFile;
		ob << '|' << itoa(item->ID);
		ob << '|' << itoa(((item->Weight > 255) ? 255 : item->Weight));

		ob << '|' << itoa(item->NoRent);
		ob << '|' << itoa(item->NoDrop);
		ob << '|' << itoa(item->Size);
		ob << '|' << itoa(Catch22(SwapBits21And22(item->Slots)));
		ob << '|' << itoa(item->Price);
		ob << '|' << itoa(item->Icon);
		ob << '|' << "0";
		ob << '|' << "0";
		ob << '|' << itoa(item->BenefitFlag);
		ob << '|' << itoa(item->Tradeskills);

		ob << '|' << itoa(item->CR);
		ob << '|' << itoa(item->DR);
		ob << '|' << itoa(item->PR);
		ob << '|' << itoa(item->MR);
		ob << '|' << itoa(item->FR);

		ob << '|' << itoa(item->AStr);
		ob << '|' << itoa(item->ASta);
		ob << '|' << itoa(item->AAgi);
		ob << '|' << itoa(item->ADex);
		ob << '|' << itoa(item->ACha);
		ob << '|' << itoa(item->AInt);
		ob << '|' << itoa(item->AWis);

		ob << '|' << itoa(item->HP);
		ob << '|' << itoa(item->Mana);
		ob << '|' << itoa(item->AC);
		ob << '|' << itoa(item->Deity);

		ob << '|' << itoa(item->SkillModValue);
		ob << '|' << itoa(item->SkillModMax);
		ob << '|' << itoa(item->SkillModType);

		ob << '|' << itoa(item->BaneDmgRace);
		if (item->BaneDmgAmt > 255)
			ob << '|' << "255";
		else
			ob << '|' << itoa(item->BaneDmgAmt);
		ob << '|' << itoa(item->BaneDmgBody);

		ob << '|' << itoa(item->Magic);
		ob << '|' << itoa(item->CastTime_);
		ob << '|' << itoa(item->ReqLevel);
		ob << '|' << itoa(item->BardType);
		ob << '|' << itoa(item->BardValue);
		ob << '|' << itoa(item->Light);
		ob << '|' << itoa(item->Delay);

		ob << '|' << itoa(item->RecLevel);
		ob << '|' << itoa(item->RecSkill);

		ob << '|' << itoa(item->ElemDmgType);
		ob << '|' << itoa(item->ElemDmgAmt);

		ob << '|' << itoa(item->Range);
		ob << '|' << itoa(item->Damage);

		ob << '|' << itoa(item->Color);
		ob << '|' << itoa(item->Classes);
		ob << '|' << itoa(item->Races);
		ob << '|' << "0";

		ob << '|' << itoa(item->MaxCharges);
		ob << '|' << itoa(item->ItemType);
		ob << '|' << itoa(item->Material);
		ob << '|' << StringFormat("%f", item->SellRate);

		ob << '|' << "0";
		ob << '|' << itoa(item->CastTime_);
		ob << '|' << "0";

		ob << '|' << itoa(item->ProcRate);
		ob << '|' << itoa(item->CombatEffects);
		ob << '|' << itoa(item->Shielding);
		ob << '|' << itoa(item->StunResist);
		ob << '|' << itoa(item->StrikeThrough);
		ob << '|' << itoa(item->ExtraDmgSkill);
		ob << '|' << itoa(item->ExtraDmgAmt);
		ob << '|' << itoa(item->SpellShield);
		ob << '|' << itoa(item->Avoidance);
		ob << '|' << itoa(item->Accuracy);

		ob << '|' << itoa(item->CharmFileID);

		ob << '|' << itoa(item->FactionMod1);
		ob << '|' << itoa(item->FactionMod2);
		ob << '|' << itoa(item->FactionMod3);
		ob << '|' << itoa(item->FactionMod4);

		ob << '|' << itoa(item->FactionAmt1);
		ob << '|' << itoa(item->FactionAmt2);
		ob << '|' << itoa(item->FactionAmt3);
		ob << '|' << itoa(item->FactionAmt4);

		ob << '|' << item->CharmFile;

		ob << '|' << itoa(item->AugType);

		ob << '|' << itoa(item->AugSlotType[0]);
		ob << '|' << itoa(item->AugSlotVisible[0]);
		ob << '|' << itoa(item->AugSlotType[1]);
		ob << '|' << itoa(item->AugSlotVisible[1]);
		ob << '|' << itoa(item->AugSlotType[2]);
		ob << '|' << itoa(item->AugSlotVisible[2]);
		ob << '|' << itoa(item->AugSlotType[3]);
		ob << '|' << itoa(item->AugSlotVisible[3]);
		ob << '|' << itoa(item->AugSlotType[4]);
		ob << '|' << itoa(item->AugSlotVisible[4]);

		ob << '|' << itoa(item->LDoNTheme);
		ob << '|' << itoa(item->LDoNPrice);
		ob << '|' << itoa(item->LDoNSold);

		ob << '|' << itoa(item->BagType);
		ob << '|' << itoa(item->BagSlots);
		ob << '|' << itoa(item->BagSize);
		ob << '|' << itoa(item->BagWR);

		ob << '|' << itoa(item->Book);
		ob << '|' << itoa(item->BookType);

		ob << '|' << item->Filename;

		ob << '|' << itoa(item->BaneDmgRaceAmt);
		ob << '|' << itoa(item->AugRestrict);
		ob << '|' << itoa(item->LoreGroup);
		ob << '|' << itoa(item->PendingLoreFlag);
		ob << '|' << itoa(item->ArtifactFlag);
		ob << '|' << itoa(item->SummonedFlag);

		ob << '|' << itoa(item->Favor);
		ob << '|' << itoa(item->FVNoDrop);
		ob << '|' << itoa(item->Endur);
		ob << '|' << itoa(item->DotShielding);
		ob << '|' << itoa(item->Attack);
		ob << '|' << itoa(item->Regen);
		ob << '|' << itoa(item->ManaRegen);
		ob << '|' << itoa(item->EnduranceRegen);
		ob << '|' << itoa(item->Haste);
		ob << '|' << itoa(item->DamageShield);
		ob << '|' << itoa(item->RecastDelay);
		ob << '|' << itoa(item->RecastType);
		ob << '|' << itoa(item->GuildFavor);

		ob << '|' << itoa(item->AugDistiller);

		ob << '|' << itoa(item->Attuneable);
		ob << '|' << itoa(item->NoPet);
		ob << '|' << itoa(item->PointType);

		ob << '|' << itoa(item->PotionBelt);
		ob << '|' << itoa(item->PotionBeltSlots);
		ob << '|' << itoa(item->StackSize);
		ob << '|' << itoa(item->NoTransfer);
		ob << '|' << itoa(item->Stackable);

		ob << '|' << itoa(item->Click.Effect);
		ob << '|' << itoa(item->Click.Type);
		ob << '|' << itoa(item->Click.Level2);
		ob << '|' << itoa(item->Click.Level);
		ob << '|' << "0"; // Click name

		ob << '|' << itoa(item->Proc.Effect);
		ob << '|' << itoa(item->Proc.Type);
		ob << '|' << itoa(item->Proc.Level2);
		ob << '|' << itoa(item->Proc.Level);
		ob << '|' << "0"; // Proc name

		ob << '|' << itoa(item->Worn.Effect);
		ob << '|' << itoa(item->Worn.Type);
		ob << '|' << itoa(item->Worn.Level2);
		ob << '|' << itoa(item->Worn.Level);
		ob << '|' << "0"; // Worn name

		ob << '|' << itoa(item->Focus.Effect);
		ob << '|' << itoa(item->Focus.Type);
		ob << '|' << itoa(item->Focus.Level2);
		ob << '|' << itoa(item->Focus.Level);
		ob << '|' << "0"; // Focus name

		ob << '|' << itoa(item->Scroll.Effect);
		ob << '|' << itoa(item->Scroll.Type);
		ob << '|' << itoa(item->Scroll.Level2);
		ob << '|' << itoa(item->Scroll.Level);
		ob << '|' << "0"; // Scroll name

		ob << StringFormat("%.*s\"", depth, protection); // Quotes (and protection, if needed) around static data

		// Sub data
		for (int index = EQ::invbag::SLOT_BEGIN; index <= invbag::SLOT_END; ++index)
		{
			ob << '|';

			EQ::ItemInstance *sub = inst->GetItem(index);
			if (!sub)
				continue;

			SerializeItem(ob, sub, 0, (depth + 1));
		}

		ob << StringFormat(
			"%.*s%s",
			(depth ? (depth - 1) : 0),
			protection,
			(depth ? "\"" : "")); // For trailing quotes (and protection) if a subitem;

		if (!depth)
			ob.write("\0", 1);
	}

	static inline int16 ServerToWebSlot(uint32 server_slot)
	{
		int16 web_slot = invslot::SLOT_INVALID;

		if (server_slot <= EQ::invslot::slotWaist)
		{
			web_slot = server_slot;
		}
		else if (server_slot == EQ::invslot::slotAmmo)
		{
			web_slot = server_slot - 1;
		}
		else if (server_slot <= EQ::invslot::slotGeneral8 && server_slot >= EQ::invslot::slotGeneral1)
		{
			web_slot = server_slot - 1;
		}
		else if (server_slot <= (EQ::invslot::POSSESSIONS_COUNT + EQ::invslot::slotWaist) &&
				 server_slot >= EQ::invslot::slotCursor)
		{
			web_slot = server_slot - 3;
		}
		else if (server_slot == (EQ::invslot::POSSESSIONS_COUNT + EQ::invslot::slotAmmo))
		{
			web_slot = server_slot - 4;
		}
		else if (server_slot <= EQ::invbag::GENERAL_BAGS_8_END &&
				 server_slot >= EQ::invbag::GENERAL_BAGS_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invbag::CURSOR_BAG_END && server_slot >= EQ::invbag::CURSOR_BAG_BEGIN)
		{
			web_slot = server_slot - 20;
		}
		else if (server_slot <= EQ::invslot::TRIBUTE_END && server_slot >= EQ::invslot::TRIBUTE_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invslot::GUILD_TRIBUTE_END &&
				 server_slot >= EQ::invslot::GUILD_TRIBUTE_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot == EQ::invslot::SLOT_TRADESKILL_EXPERIMENT_COMBINE)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invslot::BANK_END && server_slot >= EQ::invslot::BANK_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invbag::BANK_BAGS_16_END && server_slot >= EQ::invbag::BANK_BAGS_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invslot::SHARED_BANK_END && server_slot >= EQ::invslot::SHARED_BANK_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invbag::SHARED_BANK_BAGS_END &&
				 server_slot >= EQ::invbag::SHARED_BANK_BAGS_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invslot::TRADE_END && server_slot >= EQ::invslot::TRADE_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invbag::TRADE_BAGS_END && server_slot >= EQ::invbag::TRADE_BAGS_BEGIN)
		{
			web_slot = server_slot;
		}
		else if (server_slot <= EQ::invslot::WORLD_END && server_slot >= EQ::invslot::WORLD_BEGIN)
		{
			web_slot = server_slot;
		}

		LogNetcode("Convert Server Slot [{}] to Web Slot [{}]", server_slot, web_slot);

		return web_slot;
	}

	static inline int16 ServerToWebCorpseSlot(uint32 server_corpse_slot)
	{
		int16 web_slot = invslot::SLOT_INVALID;

		if (server_corpse_slot <= EQ::invslot::slotGeneral8 && server_corpse_slot >= EQ::invslot::slotGeneral1)
		{
			web_slot = server_corpse_slot - 1;
		}

		else if (server_corpse_slot <= (EQ::invslot::POSSESSIONS_COUNT + EQ::invslot::slotWaist) &&
				 server_corpse_slot >= EQ::invslot::slotCursor)
		{
			web_slot = server_corpse_slot - 3;
		}

		else if (server_corpse_slot == (EQ::invslot::POSSESSIONS_COUNT + EQ::invslot::slotAmmo))
		{
			web_slot = server_corpse_slot - 4;
		}

		Log(Logs::Detail,
			Logs::Netcode,
			"Convert Server Corpse Slot %i to Web Corpse Slot %i",
			server_corpse_slot,
			web_slot);

		return web_slot;
	}

	static inline uint32 WebToServerSlot(int16 web_slot)
	{
		uint32 server_slot = EQ::invslot::SLOT_INVALID;

		if (web_slot <= invslot::slotWaist)
		{
			server_slot = web_slot;
		}
		else if (web_slot == invslot::slotAmmo)
		{
			server_slot = web_slot + 1;
		}
		else if (web_slot <= invslot::slotGeneral8 && web_slot >= invslot::slotGeneral1)
		{
			server_slot = web_slot + 1;
		}
		else if (web_slot <= (invslot::POSSESSIONS_COUNT + invslot::slotWaist) &&
				 web_slot >= invslot::slotCursor)
		{
			server_slot = web_slot + 3;
		}
		else if (web_slot == (invslot::POSSESSIONS_COUNT + invslot::slotAmmo))
		{
			server_slot = web_slot + 4;
		}
		else if (web_slot <= invbag::GENERAL_BAGS_END && web_slot >= invbag::GENERAL_BAGS_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invbag::CURSOR_BAG_END && web_slot >= invbag::CURSOR_BAG_BEGIN)
		{
			server_slot = web_slot + 20;
		}
		else if (web_slot <= invslot::TRIBUTE_END && web_slot >= invslot::TRIBUTE_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot == invslot::SLOT_TRADESKILL_EXPERIMENT_COMBINE)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invslot::GUILD_TRIBUTE_END && web_slot >= invslot::GUILD_TRIBUTE_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invslot::BANK_END && web_slot >= invslot::BANK_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invbag::BANK_BAGS_END && web_slot >= invbag::BANK_BAGS_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invslot::SHARED_BANK_END && web_slot >= invslot::SHARED_BANK_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invbag::SHARED_BANK_BAGS_END && web_slot >= invbag::SHARED_BANK_BAGS_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invslot::TRADE_END && web_slot >= invslot::TRADE_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invbag::TRADE_BAGS_END && web_slot >= invbag::TRADE_BAGS_BEGIN)
		{
			server_slot = web_slot;
		}
		else if (web_slot <= invslot::WORLD_END && web_slot >= invslot::WORLD_BEGIN)
		{
			server_slot = web_slot;
		}

		LogNetcode("Convert Web Slot [{}] to Server Slot [{}]", web_slot, server_slot);

		return server_slot;
	}

	static inline uint32 WebToServerCorpseSlot(int16 web_corpse_slot)
	{
		uint32 server_slot = EQ::invslot::SLOT_INVALID;

		if (web_corpse_slot <= invslot::slotGeneral8 && web_corpse_slot >= invslot::slotGeneral1)
		{
			server_slot = web_corpse_slot + 1;
		}

		else if (web_corpse_slot <= (invslot::POSSESSIONS_COUNT + invslot::slotWaist) && web_corpse_slot >= invslot::slotCursor)
		{
			server_slot = web_corpse_slot + 3;
		}

		else if (web_corpse_slot == (invslot::POSSESSIONS_COUNT + invslot::slotAmmo))
		{
			server_slot = web_corpse_slot + 4;
		}

		LogNetcode("Convert Web Corpse Slot [{}] to Server Corpse Slot [{}]", web_corpse_slot, server_slot);

		return server_slot;
	}

	static inline void ServerToWebSayLink(std::string &web_saylink, const std::string &server_saylink)
	{
		if ((constants::SAY_LINK_BODY_SIZE == EQ::constants::SAY_LINK_BODY_SIZE) || (server_saylink.find('\x12') == std::string::npos))
		{
			web_saylink = server_saylink;
			return;
		}

		auto segments = Strings::Split(server_saylink, '\x12');

		for (size_t segment_iter = 0; segment_iter < segments.size(); ++segment_iter)
		{
			if (segment_iter & 1)
			{
				if (segments[segment_iter].length() <= EQ::constants::SAY_LINK_BODY_SIZE)
				{
					web_saylink.append(segments[segment_iter]);
					// TODO: log size mismatch error
					continue;
				}

				// Idx:  0 1     6     11    16    21    26    31    36 37   41 43    48       (Source)
				// RoF2: X XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX X  XXXX XX XXXXX XXXXXXXX (56)
				// 6.2:  X XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX       X  XXXX  X       XXXXXXXX (45)
				// Diff:                                       ^^^^^         ^  ^^^^^

				web_saylink.push_back('\x12');
				web_saylink.append(segments[segment_iter].substr(0, 31));
				web_saylink.append(segments[segment_iter].substr(36, 5));

				if (segments[segment_iter][41] == '0')
					web_saylink.push_back(segments[segment_iter][42]);
				else
					web_saylink.push_back('F');

				web_saylink.append(segments[segment_iter].substr(48));
				web_saylink.push_back('\x12');
			}
			else
			{
				web_saylink.append(segments[segment_iter]);
			}
		}
	}

	static inline void WebToServerSayLink(std::string &server_saylink, const std::string &web_saylink)
	{
		if ((EQ::constants::SAY_LINK_BODY_SIZE == constants::SAY_LINK_BODY_SIZE) || (web_saylink.find('\x12') == std::string::npos))
		{
			server_saylink = web_saylink;
			return;
		}

		auto segments = Strings::Split(web_saylink, '\x12');

		for (size_t segment_iter = 0; segment_iter < segments.size(); ++segment_iter)
		{
			if (segment_iter & 1)
			{
				if (segments[segment_iter].length() <= constants::SAY_LINK_BODY_SIZE)
				{
					server_saylink.append(segments[segment_iter]);
					// TODO: log size mismatch error
					continue;
				}

				// Idx:  0 1     6     11    16    21    26          31 32    36       37       (Source)
				// 6.2:  X XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX       X  XXXX  X        XXXXXXXX (45)
				// RoF2: X XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX X  XXXX XX  XXXXX XXXXXXXX (56)
				// Diff:                                       ^^^^^         ^   ^^^^^

				server_saylink.push_back('\x12');
				server_saylink.append(segments[segment_iter].substr(0, 31));
				server_saylink.append("00000");
				server_saylink.append(segments[segment_iter].substr(31, 5));
				server_saylink.push_back('0');
				server_saylink.push_back(segments[segment_iter][36]);
				server_saylink.append("00000");
				server_saylink.append(segments[segment_iter].substr(37));
				server_saylink.push_back('\x12');
			}
			else
			{
				server_saylink.append(segments[segment_iter]);
			}
		}
	}

	static inline spells::CastingSlot ServerToWebCastingSlot(EQ::spells::CastingSlot slot)
	{
		switch (slot)
		{
		case EQ::spells::CastingSlot::Gem1:
			return spells::CastingSlot::Gem1;
		case EQ::spells::CastingSlot::Gem2:
			return spells::CastingSlot::Gem2;
		case EQ::spells::CastingSlot::Gem3:
			return spells::CastingSlot::Gem3;
		case EQ::spells::CastingSlot::Gem4:
			return spells::CastingSlot::Gem4;
		case EQ::spells::CastingSlot::Gem5:
			return spells::CastingSlot::Gem5;
		case EQ::spells::CastingSlot::Gem6:
			return spells::CastingSlot::Gem6;
		case EQ::spells::CastingSlot::Gem7:
			return spells::CastingSlot::Gem7;
		case EQ::spells::CastingSlot::Gem8:
			return spells::CastingSlot::Gem8;
		case EQ::spells::CastingSlot::Gem9:
			return spells::CastingSlot::Gem9;
		case EQ::spells::CastingSlot::Item:
			return spells::CastingSlot::Item;
		case EQ::spells::CastingSlot::PotionBelt:
			return spells::CastingSlot::PotionBelt;
		case EQ::spells::CastingSlot::Discipline:
			return spells::CastingSlot::Discipline;
		case EQ::spells::CastingSlot::AltAbility:
			return spells::CastingSlot::AltAbility;
		default: // we shouldn't have any issues with other slots ... just return something
			return spells::CastingSlot::Discipline;
		}
	}

	static inline EQ::spells::CastingSlot WebToServerCastingSlot(spells::CastingSlot slot, uint32 item_location)
	{
		switch (slot)
		{
		case spells::CastingSlot::Gem1:
			return EQ::spells::CastingSlot::Gem1;
		case spells::CastingSlot::Gem2:
			return EQ::spells::CastingSlot::Gem2;
		case spells::CastingSlot::Gem3:
			return EQ::spells::CastingSlot::Gem3;
		case spells::CastingSlot::Gem4:
			return EQ::spells::CastingSlot::Gem4;
		case spells::CastingSlot::Gem5:
			return EQ::spells::CastingSlot::Gem5;
		case spells::CastingSlot::Gem6:
			return EQ::spells::CastingSlot::Gem6;
		case spells::CastingSlot::Gem7:
			return EQ::spells::CastingSlot::Gem7;
		case spells::CastingSlot::Gem8:
			return EQ::spells::CastingSlot::Gem8;
		case spells::CastingSlot::Gem9:
			return EQ::spells::CastingSlot::Gem9;
		case spells::CastingSlot::Ability:
			return EQ::spells::CastingSlot::Ability;
			// Tit uses 10 for item and discipline casting, but items have a valid location
		case spells::CastingSlot::Item:
			if (item_location == INVALID_INDEX)
				return EQ::spells::CastingSlot::Discipline;
			else
				return EQ::spells::CastingSlot::Item;
		case spells::CastingSlot::PotionBelt:
			return EQ::spells::CastingSlot::PotionBelt;
		case spells::CastingSlot::AltAbility:
			return EQ::spells::CastingSlot::AltAbility;
		default: // we shouldn't have any issues with other slots ... just return something
			return EQ::spells::CastingSlot::Discipline;
		}
	}

	static inline int ServerToWebBuffSlot(int index)
	{
		// we're a disc
		if (index >= EQ::spells::LONG_BUFFS + EQ::spells::SHORT_BUFFS)
			return index - EQ::spells::LONG_BUFFS - EQ::spells::SHORT_BUFFS +
				   spells::LONG_BUFFS + spells::SHORT_BUFFS;
		// we're a song
		if (index >= EQ::spells::LONG_BUFFS)
			return index - EQ::spells::LONG_BUFFS + spells::LONG_BUFFS;
		// we're a normal buff
		return index; // as long as we guard against bad slots server side, we should be fine
	}

	static inline int WebToServerBuffSlot(int index)
	{
		// we're a disc
		if (index >= spells::LONG_BUFFS + spells::SHORT_BUFFS)
			return index - spells::LONG_BUFFS - spells::SHORT_BUFFS + EQ::spells::LONG_BUFFS +
				   EQ::spells::SHORT_BUFFS;
		// we're a song
		if (index >= spells::LONG_BUFFS)
			return index - spells::LONG_BUFFS + EQ::spells::LONG_BUFFS;
		// we're a normal buff
		return index; // as long as we guard against bad slots server side, we should be fine
	}
} /*Web*/

// todo structs with count structs needing to be mapped
// var names to count
/**
 * DoorSpawns_Struct - done
 * ZonePoints
 * Track_Struct
 * WhoAllReturnStruct
 * GuildMembers_Struct
 * FindPersonResult_Struct
 * Titles_Struct
 * TitleList_Struct
 * TaskHistory_Struct
 * SendAA_Struct
 * DynamicZoneMemberList_Struct
 * ExpeditionLockoutTimers_Struct
 * DynamicZoneCompass_Struct
 * 
 */
