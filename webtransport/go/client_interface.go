package main

// #include "c/c_bridge.h"
// #include <stdlib.h>
import "C"

import (
	"encoding/binary"
	"fmt"
	reflect "reflect"
	"strconv"
	"strings"
	"unsafe"

	"google.golang.org/protobuf/proto"
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/reflect/protoregistry"
)

// CLIENT -> SERVER
func opcodeToEQStruct(opcode OpCodes) (reflect.Value, protoreflect.ProtoMessage) {
	tie := func(eqStruct interface{}) (reflect.Value, protoreflect.ProtoMessage) {
		enumOptions := OpCodes.Descriptor(opcode).Values().ByNumber(opcode.Enum().Number()).Options()
		pbtype, err := protoregistry.GlobalTypes.FindMessageByName(protoreflect.FullName(proto.GetExtension(enumOptions, E_MessageType).(string)))
		if err != nil {
			return reflect.ValueOf(""), nil
		}
		return reflect.ValueOf(eqStruct).Elem(), pbtype.New().Interface()
	}

	switch (OpCodes)(opcode) {
	case OpCodes_OP_LoginWeb:
		return tie(&C.struct_WebLogin_Struct{})
	case OpCodes_OP_ServerListRequest:
		return tie(&C.struct_WebLoginServerRequest_Struct{})
	case OpCodes_OP_PlayEverquestRequest:
		return tie(&C.struct_WebPlayEverquestRequest_Struct{})
	case OpCodes_OP_WebInitiateConnection:
		return tie(&C.struct_WebInitiateConnection_Struct{})
	case OpCodes_OP_SendLoginInfo:
		return tie(&C.struct_LoginInfo_Struct{})

	// World
	case OpCodes_OP_EnterWorld:
		return tie(&C.struct_EnterWorld_Struct{})

	// Zone
	case OpCodes_OP_ZoneEntry:
		return tie(&C.struct_ClientZoneEntry_Struct{})

	case OpCodes_OP_SetServerFilter:
		return tie(&C.struct_SetServerFilter_Struct{})

	case OpCodes_OP_SendAATable:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SendTributes:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SendGuildTributes:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SendAAStats:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ReqClientSpawn:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ReqNewZone:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SendExpZonein:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ClientReady:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ClientError:
		return tie(&C.struct_ClientError_Struct{})

	case OpCodes_OP_ApproveZone:
		return tie(&C.struct_ApproveZone_Struct{})

	case OpCodes_OP_TGB:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_AckPacket:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_ClientUpdate:
		return tie(&C.struct_PlayerPositionUpdateClient_Struct{})

	case OpCodes_OP_AutoAttack:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_AutoAttack2:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_Consent:
		return tie(&C.struct_Consent_Struct{})

	case OpCodes_OP_ConsentDeny:
		return tie(&C.struct_Consent_Struct{})

	case OpCodes_OP_TargetMouse:
		return tie(&C.struct_ClientTarget_Struct{})

	case OpCodes_OP_TargetCommand:
		return tie(&C.struct_ClientTarget_Struct{})

	case OpCodes_OP_Shielding:
		return tie(&C.struct_Shielding_Struct{})

	case OpCodes_OP_Jump:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_AdventureInfoRequest:
		return tie(&C.struct_EntityId_Struct{})

	case OpCodes_OP_AdventureRequest:
		return tie(&C.struct_AdventureRequest_Struct{})

	case OpCodes_OP_LDoNButton:
		return tie(&C.struct_Bool_Struct{})

	case OpCodes_OP_LeaveAdventure:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Consume:
		return tie(&C.struct_Consume_Struct{})

	case OpCodes_OP_AdventureMerchantRequest:
		return tie(&C.struct_AdventureMerchant_Struct{})

	case OpCodes_OP_AdventureMerchantPurchase:
		return tie(&C.struct_Adventure_Purchase_Struct{})

	case OpCodes_OP_ConsiderCorpse:
		return tie(&C.struct_Consider_Struct{})

	case OpCodes_OP_Consider:
		return tie(&C.struct_Consider_Struct{})

	case OpCodes_OP_Begging:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_TestBuff:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Surname:
		return tie(&C.struct_Surname_Struct{})

	case OpCodes_OP_YellForHelp:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Assist:
		return tie(&C.struct_EntityId_Struct{})

	case OpCodes_OP_GMTraining:
		return tie(&C.struct_GMTrainee_Struct{})

	case OpCodes_OP_GMEndTraining:
		return tie(&C.struct_GMTrainEnd_Struct{})

	case OpCodes_OP_GMTrainSkill:
		return tie(&C.struct_GMSkillChange_Struct{})

	case OpCodes_OP_RequestDuel:
		return tie(&C.struct_Duel_Struct{})

	case OpCodes_OP_DuelDecline:
		return tie(&C.struct_DuelResponse_Struct{})

	case OpCodes_OP_DuelAccept:
		return tie(&C.struct_Duel_Struct{})

	case OpCodes_OP_SpawnAppearance:
		return tie(&C.struct_SpawnAppearance_Struct{})

	case OpCodes_OP_BazaarInspect:
		return tie(&C.struct_BazaarInspect_Struct{})

	case OpCodes_OP_Death:
		return tie(&C.struct_Death_Struct{})

	case OpCodes_OP_MoveCoin:
		return tie(&C.struct_MoveCoin_Struct{})

	case OpCodes_OP_ItemLinkClick:
		return tie(&C.struct_ItemViewRequest_Struct{})

	case OpCodes_OP_MoveItem:
		return tie(&C.struct_MoveItem_Struct{})

	case OpCodes_OP_Camp:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Logout:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SenseHeading:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_FeignDeath:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Sneak:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Hide:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ChannelMessage:
		return tie(&C.struct_ChannelMessage_Struct{})

	case OpCodes_OP_WearChange:
		return tie(&C.struct_WearChange_Struct{})

	case OpCodes_OP_DeleteSpawn:
		return tie(&C.struct_EntityId_Struct{})

	case OpCodes_OP_SaveOnZoneReq:
		return tie(&C.struct_Save_Struct{})

	case OpCodes_OP_Save:
		return tie(&C.struct_Save_Struct{})

	case OpCodes_OP_WhoAllRequest:
		return tie(&C.struct_Who_All_Struct{})

	case OpCodes_OP_GMZoneRequest:
		return tie(&C.struct_GMZoneRequest_Struct{})

	case OpCodes_OP_GMZoneRequest2:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_EndLootRequest:
		return tie(&C.struct_EntityId_Struct{})

	case OpCodes_OP_LootRequest:
		return tie(&C.struct_EntityId_Struct{})

	case OpCodes_OP_ConfirmDelete:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_LootItem:
		return tie(&C.struct_LootingItem_Struct{})

	case OpCodes_OP_GuildDelete:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_GuildPublicNote:
		return tie(&C.struct_GuildUpdate_PublicNote{})

	case OpCodes_OP_GetGuildsList:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SetGuildMOTD:
		return tie(&C.struct_GuildMOTD_Struct{})

	case OpCodes_OP_SetRunMode:
		return tie(&C.struct_SetRunMode_Struct{})

	case OpCodes_OP_GuildPeace:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_GuildWar:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_GuildLeader:
		return tie(&C.struct_GuildMakeLeader{})

	case OpCodes_OP_GuildDemote:
		return tie(&C.struct_GuildDemote_Struct{})

	case OpCodes_OP_GuildInvite:
		return tie(&C.struct_GuildCommand_Struct{})

	case OpCodes_OP_GuildRemove:
		return tie(&C.struct_GuildCommand_Struct{})

	case OpCodes_OP_GuildInviteAccept:
		return tie(&C.struct_GuildInviteAccept_Struct{})

	case OpCodes_OP_ManaChange:
		return tie(&C.struct_ManaChange_Struct{})

	case OpCodes_OP_MemorizeSpell:
		return tie(&C.struct_MemorizeSpell_Struct{})

	case OpCodes_OP_SwapSpell:
		return tie(&C.struct_SwapSpell_Struct{})

	case OpCodes_OP_CastSpell:
		return tie(&C.struct_CastSpell_Struct{})

	case OpCodes_OP_DeleteItem:
		return tie(&C.struct_DeleteItem_Struct{})

	case OpCodes_OP_CombatAbility:
		return tie(&C.struct_CombatAbility_Struct{})

	case OpCodes_OP_Taunt:
		return tie(&C.struct_ClientTarget_Struct{})

	case OpCodes_OP_InstillDoubt:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_RezzAnswer:
		return tie(&C.struct_Resurrect_Struct{})

	case OpCodes_OP_GMSummon:
		return tie(&C.struct_GMSummon_Struct{})

	case OpCodes_OP_TradeBusy:
		return tie(&C.struct_TradeBusy_Struct{})

	case OpCodes_OP_TradeRequest:
		return tie(&C.struct_TradeRequest_Struct{})

	case OpCodes_OP_TradeRequestAck:
		return tie(&C.struct_TradeRequest_Struct{})

	case OpCodes_OP_CancelTrade:
		return tie(&C.struct_CancelTrade_Struct{})

	case OpCodes_OP_TradeAcceptClick:
		return tie(&C.struct_TradeAccept_Struct{})

	case OpCodes_OP_BoardBoat:
		return tie(&C.struct_EntityId_Struct{})

	case OpCodes_OP_LeaveBoat:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_RandomReq:
		return tie(&C.struct_RandomReq_Struct{})

	case OpCodes_OP_Buff:
		return tie(&C.struct_SpellBuffPacket_Struct{})

	case OpCodes_OP_GMHideMe:
		return tie(&C.struct_SpawnAppearance_Struct{})

	case OpCodes_OP_GMNameChange:
		return tie(&C.struct_GMName_Struct{})

	case OpCodes_OP_GMKill:
		return tie(&C.struct_GMKill_Struct{})

	case OpCodes_OP_GMLastName:
		return tie(&C.struct_GMLastName_Struct{})

	case OpCodes_OP_GMToggle:
		return tie(&C.struct_GMToggle_Struct{})

	case OpCodes_OP_LFGCommand:
		return tie(&C.struct_LFG_Struct{})

	case OpCodes_OP_GMGoto:
		return tie(&C.struct_GMSummon_Struct{})

	case OpCodes_OP_TraderShop:
		return tie(&C.struct_TraderClick_Struct{})

	case OpCodes_OP_ShopRequest:
		return tie(&C.struct_Merchant_Click_Struct{})

	case OpCodes_OP_Bazaar:
		return tie(&C.struct_BazaarSearch_Struct{})

	case OpCodes_OP_ShopPlayerBuy:
		return tie(&C.struct_Merchant_Sell_Struct{})

	case OpCodes_OP_ShopPlayerSell:
		return tie(&C.struct_Merchant_Purchase_Struct{})

	case OpCodes_OP_ShopEnd:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_CloseContainer:
		return tie(&C.struct_ClickObjectAction_Struct{})

	case OpCodes_OP_ClickObjectAction:
		return tie(&C.struct_ClickObjectAction_Struct{})

	case OpCodes_OP_ClickObject:
		return tie(&C.struct_ClickObject_Struct{})

	case OpCodes_OP_RecipesFavorite:
		return tie(&C.struct_TradeskillFavorites_Struct{})

	case OpCodes_OP_RecipesSearch:
		return tie(&C.struct_RecipesSearch_Struct{})

	case OpCodes_OP_RecipeDetails:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_RecipeAutoCombine:
		return tie(&C.struct_RecipeAutoCombine_Struct{})

	case OpCodes_OP_TradeSkillCombine:
		return tie(&C.struct_NewCombine_Struct{})

	case OpCodes_OP_ItemName:
		return tie(&C.struct_ItemNamePacket_Struct{})

	case OpCodes_OP_AugmentItem:
		return tie(&C.struct_AugmentItem_Struct{})

	case OpCodes_OP_ClickDoor:
		return tie(&C.struct_ClickDoor_Struct{})

	case OpCodes_OP_FaceChange:
		return tie(&C.struct_FaceChange_Struct{})

	case OpCodes_OP_GroupInvite:
		return tie(&C.struct_GroupInvite_Struct{})

	case OpCodes_OP_GroupInvite2:
		return tie(&C.struct_GroupInvite_Struct{})

	case OpCodes_OP_GroupFollow:
		return tie(&C.struct_GroupGeneric_Struct{})

	case OpCodes_OP_GroupFollow2:
		return tie(&C.struct_GroupGeneric_Struct{})

	case OpCodes_OP_GroupAcknowledge:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_GroupCancelInvite:
		return tie(&C.struct_GroupGeneric_Struct{})

	case OpCodes_OP_GroupDisband:
		return tie(&C.struct_GroupGeneric_Struct{})

	case OpCodes_OP_GroupDelete:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_GMEmoteZone:
		return tie(&C.struct_GMEmoteZone_Struct{})

	case OpCodes_OP_InspectRequest:
		return tie(&C.struct_Inspect_Struct{})

	case OpCodes_OP_InspectAnswer:
		return tie(&C.struct_Inspect_Struct{})

	case OpCodes_OP_DeleteSpell:
		return tie(&C.struct_DeleteSpell_Struct{})

	case OpCodes_OP_Petition:
		return tie(&C.struct_String_Struct{})

	case OpCodes_OP_PetitionCheckIn:
		return tie(&C.struct_Petition_Struct{})

	case OpCodes_OP_PetitionResolve:
		return tie(&C.struct_PetitionUpdate_Struct{})

	case OpCodes_OP_PetitionDelete:
		return tie(&C.struct_PetitionUpdate_Struct{})

	case OpCodes_OP_PetitionUnCheckout:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_PetitionQue:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_PDeletePetition:
		return tie(&C.struct_String_Struct{})

	case OpCodes_OP_PetitionCheckout:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_PetitionRefresh:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_PetCommands:
		return tie(&C.struct_PetCommand_Struct{})

	case OpCodes_OP_ReadBook:
		return tie(&C.struct_BookRequest_Struct{})

	case OpCodes_OP_Emote:
		return tie(&C.struct_Emote_Struct{})

	case OpCodes_OP_GMDelCorpse:
		return tie(&C.struct_GMDelCorpse_Struct{})

	case OpCodes_OP_GMKick:
		return tie(&C.struct_GMKick_Struct{})

	case OpCodes_OP_GMServers:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Illusion:
		return tie(&C.struct_Illusion_Struct{})

	case OpCodes_OP_GMBecomeNPC:
		return tie(&C.struct_BecomeNPC_Struct{})

	case OpCodes_OP_Fishing:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Forage:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Mend:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_EnvDamage:
		return tie(&C.struct_EnvDamage2_Struct{})

	case OpCodes_OP_Damage:
		return tie(&C.struct_CombatDamage_Struct{})

	case OpCodes_OP_AAAction:
		return tie(&C.struct_AA_Action{})

	case OpCodes_OP_TraderBuy:
		return tie(&C.struct_TraderBuy_Struct{})

	case OpCodes_OP_Trader:
		return tie(&C.struct_Trader_ShowItems_Struct{})

	case OpCodes_OP_GMFind:
		return tie(&C.struct_GMSummon_Struct{})

	case OpCodes_OP_PickPocket:
		return tie(&C.struct_PickPocket_Struct{})

	case OpCodes_OP_Bind_Wound:
		return tie(&C.struct_BindWound_Struct{})

	case OpCodes_OP_TrackTarget:
		return tie(&C.struct_TrackTarget_Struct{})

	case OpCodes_OP_Track:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_TrackUnknown:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ReloadUI:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Split:
		return tie(&C.struct_Split_Struct{})

	case OpCodes_OP_SenseTraps:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_DisarmTraps:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_OpenTributeMaster:
		return tie(&C.struct_StartTribute_Struct{})

	case OpCodes_OP_OpenGuildTributeMaster:
		return tie(&C.struct_StartTribute_Struct{})

	case OpCodes_OP_TributeItem:
		return tie(&C.struct_TributeItem_Struct{})

	case OpCodes_OP_TributeMoney:
		return tie(&C.struct_TributeMoney_Struct{})

	case OpCodes_OP_SelectTribute:
		return tie(&C.struct_SelectTributeReq_Struct{})

	case OpCodes_OP_TributeUpdate:
		return tie(&C.struct_TributeInfo_Struct{})

	case OpCodes_OP_TributeToggle:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_TributeNPC:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_CrashDump:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ControlBoat:
		return tie(&C.struct_ControlBoat_Struct{})

	case OpCodes_OP_DumpName:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SafeFallSuccess:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_Heartbeat:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_SafePoint:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_FindPersonRequest:
		return tie(&C.struct_FindPersonRequest_Struct{})

	case OpCodes_OP_LeadershipExpToggle:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_PurchaseLeadershipAA:
		return tie(&C.struct_Int_Struct{})

	case OpCodes_OP_BankerChange:
		return tie(&C.struct_BankerChange_Struct{})

	case OpCodes_OP_SetTitle:
		return tie(&C.struct_SetTitle_Struct{})

	case OpCodes_OP_RequestTitles:
		return tie(&C.struct_Zero_Struct{})

	case OpCodes_OP_ItemVerifyRequest:
		return tie(&C.struct_ItemVerifyRequest_Struct{})
	}
	LogEQInfo("Got unknown op code map %v", opcode)
	return reflect.ValueOf(""), nil
}

// CLIENT -> SERVER
func HandleMessage(msg []byte, webstreamManager unsafe.Pointer, sessionId int, onClientPacket C.OnClientPacket) {
	opcode := binary.LittleEndian.Uint16(msg[0:2])
	restBytes := msg[2:]

	reflectEQStruct, protoMessage := opcodeToEQStruct((OpCodes)(opcode))

	proto.Unmarshal(restBytes, protoMessage)
	cleanupFuncs := []func(){}
	fields := protoMessage.ProtoReflect().Descriptor().Fields()
	size := 0
	for i := 0; i < fields.Len(); i++ {
		fieldName := fields.Get(i).Name()
		originalName := (string)(fieldName)
		rf := reflectEQStruct.FieldByName(originalName)
		if rf.IsValid() {
			rf = reflect.NewAt(rf.Type(), unsafe.Pointer(rf.UnsafeAddr())).Elem()
			val := protoMessage.ProtoReflect().Get(fields.ByName(fieldName))

			cType := fmt.Stringer.String(rf.Type())
			switch cType {
			case "*main._Ctype_char":
				strVal := C.CString(reflect.ValueOf(val.String()).Interface().(string))
				cleanupFuncs = append(cleanupFuncs, func() { C.free(unsafe.Pointer(strVal)) })
				rf.Set(reflect.ValueOf(strVal))
				size += int(C.ptr_size())
				break
			case "main._Ctype_uint":
				rf.Set(reflect.ValueOf(C.uint(reflect.ValueOf(val.Int()).Interface().(int64))))
				size += 4
				break
			case "main._Ctype__Bool":
				rf.Set(reflect.ValueOf(C.bool(reflect.ValueOf(val.Bool()).Interface().(bool))))
				size += 1
				break
			case "main._Ctype_uchar":
				rf.Set(reflect.ValueOf(C.uchar(reflect.ValueOf(val.Int()).Interface().(int64))))
				size += 1
				break
			default:
				// Fixed char array
				if strings.Contains(cType, "]main._Ctype_char") {
					str := FixedArraySizePattern.FindStringSubmatch(cType)[1]
					length, err := strconv.Atoi(str)
					if err != nil {
						break
					}
					strVal := reflect.ValueOf(val.String()).Interface().(string)
					slice := (*[1 << 28]C.char)(unsafe.Pointer(rf.UnsafeAddr()))[:length:length]
					for i := 0; i < len(strVal) && i < length; i++ { // leave element 256 at zero
						slice[i] = C.char(strVal[i])
					}
					size += length
					break
				}
				LogEQInfo("Unhandled c type client -> server %s", cType)
			}

		}
	}

	C.bridge_client_packet(webstreamManager, C.int(sessionId), C.ushort(opcode), reflectEQStruct.Addr().UnsafePointer(), C.int(size), onClientPacket)

	// for _, fn := range cleanupFuncs {
	// 	fn()
	// }
}
