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
