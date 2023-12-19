package main

// #include "c/c_bridge.h"
import "C"

import (
	"encoding/binary"
	"errors"
	"fmt"
	reflect "reflect"
	"strconv"
	"strings"
	"unsafe"

	"google.golang.org/protobuf/proto"
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/reflect/protoregistry"
)

func nestedStructToOpCode(cType string) (OpCodes, bool, bool) {
	strSplit := strings.Split(cType, "main.")
	str := strSplit[len(strSplit)-1]
	val, ok := OpcodeTypeMap[str]
	return val, ok, strings.Contains(cType, "*")
}

// SERVER -> CLIENT
func eqStructToProtobuf(opcode OpCodes, structPtr unsafe.Pointer) (protoMessage proto.Message, fieldVal reflect.Value, err error) {
	tie := func(eqStruct interface{}) (protoMessage proto.Message, fieldVal reflect.Value, err error) {
		enumOptions := OpCodes.Descriptor(opcode).Values().ByNumber(opcode.Enum().Number()).Options()
		pbtype, err := protoregistry.GlobalTypes.FindMessageByName(protoreflect.FullName(proto.GetExtension(enumOptions, E_MessageType).(string)))
		if err != nil {
			LogEQInfo("Missing map proto message for op code: %v", OpCodes_name[int32(opcode)])
			return nil, reflect.ValueOf(nil), err
		}
		return pbtype.New().Interface(), reflect.ValueOf(eqStruct).Elem(), nil
	}
	switch (OpCodes)(opcode) {
	// Login
	case OpCodes_OP_LoginAccepted:
		return tie((*C.struct_WebLoginReply_Struct)(structPtr))
	case OpCodes_OP_ServerListResponse:
		return tie((*C.struct_WebLoginServerResponse_Struct)(structPtr))
	case OpCodes_OP_PlayEverquestResponse:
		return tie((*C.struct_WebPlayEverquestResponse_Struct)(structPtr))
	// World
	case OpCodes_OP_SendCharInfo:
		return tie((*C.struct_CharacterSelect_Struct)(structPtr))
	case OpCodes_OP_EnterWorld:
		return tie((*C.struct_EnterWorld_Struct)(structPtr))
	case OpCodes_OP_ZoneServerInfo:
		return tie((*C.struct_ZoneServerInfo_Struct)(structPtr))
		// Zone
	case OpCodes_OP_ClearObject:
		return tie((*C.struct_Zero_Struct)(structPtr))

	case OpCodes_OP_FinishTrade:
		return tie((*C.struct_Zero_Struct)(structPtr))

	case OpCodes_OP_GMEndTrainingResponse:
		return tie((*C.struct_Zero_Struct)(structPtr))

	case OpCodes_OP_LootComplete:
		return tie((*C.struct_Zero_Struct)(structPtr))

	case OpCodes_OP_WorldObjectsSent:
		return tie((*C.struct_Zero_Struct)(structPtr))

	case OpCodes_OP_FinishWindow:
		return tie((*C.struct_Zero_Struct)(structPtr))

	case OpCodes_OP_FinishWindow2:
		return tie((*C.struct_Zero_Struct)(structPtr))

	case OpCodes_OP_ItemPacket:
		return tie((*C.struct_ItemPacket_Struct)(structPtr))

	case OpCodes_OP_ColoredText:
		return tie((*C.struct_ColoredText_Struct)(structPtr))

	case OpCodes_OP_ItemRecastDelay:
		return tie((*C.struct_ItemRecastDelay_Struct)(structPtr))

	case OpCodes_OP_FormattedMessage:
		return tie((*C.struct_FormattedMessage_Struct)(structPtr))

	case OpCodes_OP_GuildMemberList:
		return tie((*C.struct_uint32)(structPtr))

	case OpCodes_OP_InterruptCast:
		return tie((*C.struct_InterruptCast_Struct)(structPtr))

	case OpCodes_OP_ItemLinkResponse:
		return tie((*C.struct_ItemPacket_Struct)(structPtr))

	case OpCodes_OP_ZoneSpawns:
		return tie((*C.struct_Spawn_Struct)(structPtr))

	case OpCodes_OP_CompletedTasks:
		return tie((*C.struct_TaskHistory_Struct)(structPtr))

	case OpCodes_OP_CharInventory:
		return tie((*C.struct_ItemPacket_Struct)(structPtr))

	case OpCodes_OP_CustomTitles:
		return tie((*C.struct_Titles_Struct)(structPtr))

	case OpCodes_OP_SpawnDoor:
		return tie((*C.struct_Door_Struct)(structPtr))

	case OpCodes_OP_SendZonepoints:
		return tie((*C.struct_ZonePoints)(structPtr))

	case OpCodes_OP_TributeInfo:
		return tie((*C.struct_TributeAbility_Struct)(structPtr))

	case OpCodes_OP_GuildTributeInfo:
		return tie((*C.struct_GuildTributeAbility_Struct)(structPtr))

	case OpCodes_OP_SendTitleList:
		return tie((*C.struct_TitleList_Struct)(structPtr))

	case OpCodes_OP_SendMaxCharacters:
		return tie((*C.struct_MaxCharacters_Struct)(structPtr))

	case OpCodes_OP_AAExpUpdate:
		return tie((*C.struct_AAExpUpdate_Struct)(structPtr))

	case OpCodes_OP_Action:
		return tie((*C.struct_Action_Struct)(structPtr))

	case OpCodes_OP_AdventureData:
		return tie((*C.struct_AdventureRequestResponse_Struct)(structPtr))

	case OpCodes_OP_AdventureFinish:
		return tie((*C.struct_AdventureFinish_Struct)(structPtr))

	case OpCodes_OP_AdventurePointsUpdate:
		return tie((*C.struct_AdventurePoints_Update_Struct)(structPtr))

	case OpCodes_OP_Animation:
		return tie((*C.struct_Animation_Struct)(structPtr))

	case OpCodes_OP_AnnoyingZoneUnknown:
		return tie((*C.struct_AnnoyingZoneUnknown_Struct)(structPtr))

	case OpCodes_OP_BankerChange:
		return tie((*C.struct_BankerChange_Struct)(structPtr))

	case OpCodes_OP_BecomeTrader:
		return tie((*C.struct_BecomeTrader_Struct)(structPtr))

	case OpCodes_OP_BeginCast:
		return tie((*C.struct_BeginCast_Struct)(structPtr))

	case OpCodes_OP_Charm:
		return tie((*C.struct_Charm_Struct)(structPtr))

	case OpCodes_OP_CameraEffect:
		return tie((*C.struct_Camera_Struct)(structPtr))

	case OpCodes_OP_ClickObjectAction:
		return tie((*C.struct_ClickObjectAction_Struct)(structPtr))

	case OpCodes_OP_ConsentResponse:
		return tie((*C.struct_ConsentResponse_Struct)(structPtr))

	case OpCodes_OP_EnduranceUpdate:
		return tie((*C.struct_EnduranceUpdate_Struct)(structPtr))

	case OpCodes_OP_ExpUpdate:
		return tie((*C.struct_ExpUpdate_Struct)(structPtr))

	case OpCodes_OP_GroundSpawn:
		return tie((*C.struct_Object_Struct)(structPtr))

	case OpCodes_OP_GroupUpdate:
		return tie((*C.struct_GroupJoin_Struct)(structPtr))

	case OpCodes_OP_GuildMOTD:
		return tie((*C.struct_GuildMOTD_Struct)(structPtr))

	case OpCodes_OP_GuildManageAdd:
		return tie((*C.struct_GuildJoin_Struct)(structPtr))

	case OpCodes_OP_GuildManageRemove:
		return tie((*C.struct_GuildManageRemove_Struct)(structPtr))

	case OpCodes_OP_GuildManageStatus:
		return tie((*C.struct_GuildManageStatus_Struct)(structPtr))

	case OpCodes_OP_GuildMemberUpdate:
		return tie((*C.struct_GuildMemberUpdate_Struct)(structPtr))

	case OpCodes_OP_HPUpdate:
		return tie((*C.struct_SpawnHPUpdate_Struct)(structPtr))

	case OpCodes_OP_IncreaseStats:
		return tie((*C.struct_IncreaseStat_Struct)(structPtr))

	case OpCodes_OP_ItemVerifyReply:
		return tie((*C.struct_ItemVerifyReply_Struct)(structPtr))

	case OpCodes_OP_LFGAppearance:
		return tie((*C.struct_LFG_Appearance_Struct)(structPtr))

	case OpCodes_OP_LeadershipExpUpdate:
		return tie((*C.struct_LeadershipExpUpdate_Struct)(structPtr))

	case OpCodes_OP_LevelAppearance:
		return tie((*C.struct_LevelAppearance_Struct)(structPtr))

	case OpCodes_OP_LevelUpdate:
		return tie((*C.struct_LevelUpdate_Struct)(structPtr))

	case OpCodes_OP_ManaUpdate:
		return tie((*C.struct_ManaUpdate_Struct)(structPtr))

	case OpCodes_OP_MobEnduranceUpdate:
		return tie((*C.struct_MobEnduranceUpdate_Struct)(structPtr))

	case OpCodes_OP_MobHealth:
		return tie((*C.struct_MobHealth_Struct)(structPtr))

	case OpCodes_OP_MobManaUpdate:
		return tie((*C.struct_MobManaUpdate_Struct)(structPtr))

	case OpCodes_OP_MobRename:
		return tie((*C.struct_MobRename_Struct)(structPtr))

	case OpCodes_OP_MoneyOnCorpse:
		return tie((*C.struct_moneyOnCorpseStruct)(structPtr))

	case OpCodes_OP_MoneyUpdate:
		return tie((*C.struct_MoneyUpdate_Struct)(structPtr))

	case OpCodes_OP_MoveDoor:
		return tie((*C.struct_MoveDoor_Struct)(structPtr))

	case OpCodes_OP_NewSpawn:
		return tie((*C.struct_NewSpawn_Struct)(structPtr))

	case OpCodes_OP_NewZone:
		return tie((*C.struct_NewZone_Struct)(structPtr))

	case OpCodes_OP_PetitionCheckout:
		return tie((*C.struct_Petition_Struct)(structPtr))

	case OpCodes_OP_PetitionUpdate:
		return tie((*C.struct_PetitionUpdate_Struct)(structPtr))

	case OpCodes_OP_PlayerProfile:
		return tie((*C.struct_PlayerProfile_Struct)(structPtr))

	case OpCodes_OP_RaidUpdate:
		return tie((*C.struct_ZoneInSendName_Struct)(structPtr))

	case OpCodes_OP_RandomReply:
		return tie((*C.struct_RandomReply_Struct)(structPtr))

	case OpCodes_OP_RecipeReply:
		return tie((*C.struct_RecipeReply_Struct)(structPtr))

	case OpCodes_OP_RequestClientZoneChange:
		return tie((*C.struct_RequestClientZoneChange_Struct)(structPtr))

	case OpCodes_OP_RespondAA:
		return tie((*C.struct_AATable_Struct)(structPtr))

	case OpCodes_OP_RezzRequest:
		return tie((*C.struct_Resurrect_Struct)(structPtr))

	case OpCodes_OP_SetTitleReply:
		return tie((*C.struct_SetTitleReply_Struct)(structPtr))

	case OpCodes_OP_ShopDelItem:
		return tie((*C.struct_Merchant_DelItem_Struct)(structPtr))

	case OpCodes_OP_SimpleMessage:
		return tie((*C.struct_SimpleMessage_Struct)(structPtr))

	case OpCodes_OP_SkillUpdate:
		return tie((*C.struct_SkillUpdate_Struct)(structPtr))

	case OpCodes_OP_SomeItemPacketMaybe:
		return tie((*C.struct_Arrow_Struct)(structPtr))

	case OpCodes_OP_SpellEffect:
		return tie((*C.struct_SpellEffect_Struct)(structPtr))

	case OpCodes_OP_Stamina:
		return tie((*C.struct_Stamina_Struct)(structPtr))

	case OpCodes_OP_Stun:
		return tie((*C.struct_Stun_Struct)(structPtr))

	case OpCodes_OP_TargetReject:
		return tie((*C.struct_TargetReject_Struct)(structPtr))

	case OpCodes_OP_TimeOfDay:
		return tie((*C.struct_TimeOfDay_Struct)(structPtr))

	case OpCodes_OP_Track:
		return tie((*C.struct_Track_Struct)(structPtr))

	case OpCodes_OP_TradeCoins:
		return tie((*C.struct_TradeCoin_Struct)(structPtr))

	case OpCodes_OP_TradeMoneyUpdate:
		return tie((*C.struct_TradeMoneyUpdate_Struct)(structPtr))

	case OpCodes_OP_TraderDelItem:
		return tie((*C.struct_TraderDelItem_Struct)(structPtr))

	case OpCodes_OP_TraderItemUpdate:
		return tie((*C.struct_TraderItemUpdate_Struct)(structPtr))

	case OpCodes_OP_TributeTimer:
		return tie((*C.struct_Bool_Struct)(structPtr))

	case OpCodes_OP_UpdateLeadershipAA:
		return tie((*C.struct_UpdateLeadershipAA_Struct)(structPtr))

	case OpCodes_OP_Weather:
		return tie((*C.struct_Weather_Struct)(structPtr))

	case OpCodes_OP_ZoneChange:
		return tie((*C.struct_ZoneChange_Struct)(structPtr))

	case OpCodes_OP_ZoneInUnknown:
		return tie((*C.struct_ZoneInUnknown_Struct)(structPtr))

	case OpCodes_OP_AcceptNewTask:
		return tie((*C.struct_AcceptNewTask_Struct)(structPtr))

	case OpCodes_OP_AdventureInfo:
		return tie((*C.struct_AdventureInfo)(structPtr))

	case OpCodes_OP_ApplyPoison:
		return tie((*C.struct_ApplyPoison_Struct)(structPtr))

	case OpCodes_OP_ApproveWorld:
		return tie((*C.struct_ApproveWorld_Struct)(structPtr))

	case OpCodes_OP_Bandolier:
		return tie((*C.struct_BandolierItem_Struct)(structPtr))

	case OpCodes_OP_BazaarSearch:
		return tie((*C.struct_BazaarSearch_Struct)(structPtr))

	case OpCodes_OP_BecomeCorpse:
		return tie((*C.struct_BecomeCorpse_Struct)(structPtr))

	case OpCodes_OP_CancelTask:
		return tie((*C.struct_CancelTask_Struct)(structPtr))

	case OpCodes_OP_Command:
		return tie((*C.struct_PetCommand_Struct)(structPtr))

	case OpCodes_OP_DynamicWall:
		return tie((*C.struct_DynamicWall_Struct)(structPtr))

	case OpCodes_OP_GuildsList:
		return tie((*C.struct_GuildsListEntry_Struct)(structPtr))

	case OpCodes_OP_LFGuild:
		return tie((*C.struct_LFGuild_SearchPlayer_Struct)(structPtr))

	case OpCodes_OP_LoadSpellSet:
		return tie((*C.struct_LoadSpellSet_Struct)(structPtr))

	case OpCodes_OP_Login:
		return tie((*C.struct_LoginInfo_Struct)(structPtr))

	case OpCodes_OP_LogServer:
		return tie((*C.struct_LogServer_Struct)(structPtr))

	case OpCodes_OP_MOTD:
		return tie((*C.struct_GuildMOTD_Struct)(structPtr))

	case OpCodes_OP_OnLevelMessage:
		return tie((*C.struct_OnLevelMessage_Struct)(structPtr))

	case OpCodes_OP_PlayMP3:
		return tie((*C.struct_PlayMP3_Struct)(structPtr))

	case OpCodes_OP_PotionBelt:
		return tie((*C.struct_PotionBeltItem_Struct)(structPtr))

	case OpCodes_OP_PVPStats:
		return tie((*C.struct_PVPStatsEntry_Struct)(structPtr))

	case OpCodes_OP_Report:
		return tie((*C.struct_BugReport_Struct)(structPtr))

	case OpCodes_OP_SpecialMesg:
		return tie((*C.struct_SpecialMesg_Struct)(structPtr))

	case OpCodes_OP_TaskActivity:
		return tie((*C.struct_TaskActivity_Struct)(structPtr))

	case OpCodes_OP_TaskDescription:
		return tie((*C.struct_TaskDescription_Struct)(structPtr))

	case OpCodes_OP_ZoneUnavail:
		return tie((*C.struct_ZoneUnavail_Struct)(structPtr))
	// Nested structs
	case OpCodes_Nested_WorldServer:
		return tie((*C.struct_WebLoginWorldServer_Struct)(structPtr))
	case OpCodes_Nested_CharacterSelectEntry:
		return tie((*C.struct_CharacterSelectEntry_Struct)(structPtr))
	case OpCodes_Nested_CharSelectEquip:
		return tie((*C.struct_CharSelectEquip_Struct)(structPtr))
	case OpCodes_Nested_Tint:
		return tie((*C.struct_Tint_Struct)(structPtr))
	}
	return nil, reflect.ValueOf(nil), errors.New("OpCode not found for server to client")
}

func applyFields(reflectEQStruct reflect.Value, protoMessage protoreflect.ProtoMessage) {
	fields := protoMessage.ProtoReflect().Descriptor().Fields()
	for i := 0; i < fields.Len(); i++ {
		field := fields.Get(i)
		fieldName := field.Name()
		originalName := (string)(fieldName)
		rf := reflectEQStruct.FieldByName(originalName)
		if rf.IsValid() {
			// Map of flexible pointer in C struct to proto repeated field
			mapNestedStructToRepeated := func(nestedOpcode OpCodes) {
				enumOptions := OpCodes.Descriptor(nestedOpcode).Values().ByNumber(nestedOpcode.Enum().Number()).Options()
				arrFieldName := proto.GetExtension(enumOptions, E_RepeatedField).(string)
				nestedField := protoMessage.ProtoReflect().Descriptor().Fields().ByName(protoreflect.Name(arrFieldName))
				next := rf.UnsafePointer()
				for {
					if next == nil {
						break
					}
					nestedMessage, reflectEQStruct, err := eqStructToProtobuf(nestedOpcode, next)
					if err != nil {
						LogEQInfo("Err in nested struct %v", err)
					}
					applyFields(reflectEQStruct, nestedMessage)
					arr := protoMessage.ProtoReflect().Mutable(nestedField).List()
					arr.Append(protoreflect.ValueOf(nestedMessage.ProtoReflect()))
					next = reflectEQStruct.FieldByName("next").UnsafePointer()
				}
			}
			unsafePtr := unsafe.Pointer(rf.UnsafeAddr())
			rf = reflect.NewAt(rf.Type(), unsafePtr).Elem()
			cType := fmt.Stringer.String(rf.Type())

			switch cType {
			case "*main._Ctype_char":
				protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(C.GoString(rf.Interface().(*C.char))))
				break
			case "main._Ctype_int":
				protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(int32(rf.Int())))
				break
			case "main._Ctype_uint",
				"main._Ctype_uchar",
				"main._Ctype_ushort":
				protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(int32(rf.Uint())))
				break
			case "main._Ctype__Bool":
				protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(rf.Bool()))
				break

			default:
				// 2d Fixed char array
				if strings.Contains(cType, "][") && strings.Contains(cType, "]main._Ctype_char") {
					str := FixedArraySizePattern.FindStringSubmatch(cType)[1]
					length, err := strconv.Atoi(str)
					if err != nil {
						break
					}
					slice := (*[1 << 28]C.char)(unsafePtr)[:length:length]
					protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(C.GoString(&slice[0])))
					break
				}

				// Fixed char array
				if strings.Contains(cType, "]main._Ctype_char") {
					str := FixedArraySizePattern.FindStringSubmatch(cType)[1]
					length, err := strconv.Atoi(str)
					if err != nil {
						break
					}
					slice := (*[1 << 28]C.char)(unsafePtr)[:length:length]
					protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(C.GoString(&slice[0])))
					break
				}

				// Linked list flexible pointers
				flexOpCode, found, isPtr := nestedStructToOpCode(cType)
				if found && isPtr {
					mapNestedStructToRepeated(flexOpCode)
					break
				}

				// Fixed struct array
				if strings.Contains(cType, "]main._Ctype_struct") {
					str := FixedArraySizePattern.FindStringSubmatch(cType)[1]
					length, err := strconv.Atoi(str)
					if err != nil {
						break
					}

					nestedOpCode, found, _ := nestedStructToOpCode(cType)
					if !found {
						LogEQInfo("Not found nested fixed struct array %s :: %s", originalName, cType)
					}

					enumOptions := OpCodes.Descriptor(nestedOpCode).Values().ByNumber(nestedOpCode.Enum().Number()).Options()
					arrFieldName := proto.GetExtension(enumOptions, E_RepeatedField).(string)
					nestedField := protoMessage.ProtoReflect().Descriptor().Fields().ByName(protoreflect.Name(arrFieldName))
					arr := protoMessage.ProtoReflect().Mutable(nestedField).List()

					for i := 0; i < length; i++ {
						reflectEQStruct := rf.Index(i)
						nestedMessage, _, err := eqStructToProtobuf(nestedOpCode, reflectEQStruct.Addr().UnsafePointer())
						if err != nil {
							LogEQInfo("Not found nested fixed struct array %s :: %s", err, originalName)
							return
						}
						applyFields(reflectEQStruct, nestedMessage)
						arr.Append(protoreflect.ValueOf(nestedMessage.ProtoReflect()))
					}
					break
				}

				// Single struct
				if strings.Contains(cType, "main._Ctype_struct") {
					nestedOpCode, found, _ := nestedStructToOpCode(cType)
					if !found {
						LogEQInfo("Not found nested single struct %s :: %s", originalName, cType)
					}
					nestedMessage, reflectEQStruct, err := eqStructToProtobuf(nestedOpCode, unsafePtr)
					if err != nil {
						LogEQInfo("Err in nested struct single %s :: %s", err, originalName)
						return
					}
					applyFields(reflectEQStruct, nestedMessage)
					protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(nestedMessage.ProtoReflect()))
					break
				}
				LogEQInfo("Unhandled c type %s", cType)
			}

		} else {
			LogEQInfo("Invalid field for server to client proto message %s", originalName)
		}
	}
}

func SendEQPacket(sessionId int, opcode int, structPtr unsafe.Pointer, structSize int) {
	session := sessionMap[sessionId]
	if session.session == nil {
		return
	}
	messageBytes := make([]byte, 2)
	binary.LittleEndian.PutUint16(messageBytes, uint16(opcode))

	protoMessage, reflectEQStruct, err := eqStructToProtobuf((OpCodes)(opcode), structPtr)
	if err != nil {
		LogEQInfo("Got an error or unhandled op code: %v", OpCodes_name[int32(opcode)])
		return
	}

	applyFields(reflectEQStruct, protoMessage)

	bytes, err := proto.Marshal(protoMessage)
	if err != nil {
		return
	}
	messageBytes = append(messageBytes, bytes...)
	session.session.SendMessage(messageBytes)
}
