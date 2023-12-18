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
