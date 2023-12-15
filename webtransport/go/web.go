package main

// #include "c_bridge.h"
// #include <stdlib.h>
import "C"

import (
	"context"
	"encoding/binary"
	"errors"
	"fmt"
	"log"
	"math/bits"
	"net"
	"net/http"
	reflect "reflect"
	"regexp"
	"strconv"
	"strings"
	"time"
	"unsafe"

	"github.com/adriancable/webtransport-go"
	"github.com/praserx/ipconv"
	"google.golang.org/protobuf/proto"
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/reflect/protoregistry"
)

type webSession struct {
	webstreamManager unsafe.Pointer
	session          *webtransport.Session
	ip               string
	port             string
}

var sessionMap map[int]webSession = map[int]webSession{}
var stopServer context.CancelFunc = nil
var fixedArraySizePattern *regexp.Regexp = regexp.MustCompile("\\[(\\d+)\\]")
var logInfoFunc C.OnLogMessage = nil
var sessionId int = 0

var opcodeTypeMap map[string]OpCodes = map[string]OpCodes{
	"_Ctype_struct_WebLoginWorldServer_Struct":  OpCodes_Nested_WorldServer,
	"_Ctype_struct_CharacterSelectEntry_Struct": OpCodes_Nested_CharacterSelectEntry,
	"_Ctype_struct_CharSelectEquip_Struct":      OpCodes_Nested_CharSelectEquip,
	"_Ctype_struct_Tint_Struct":                 OpCodes_Nested_Tint,
}

func LogEQInfo(message string, args ...any) {
	if logInfoFunc == nil {
		return
	}
	C.bridge_log_message(C.CString(fmt.Sprintf(message, args...)), logInfoFunc)
}

func nestedStructToOpCode(cType string) (OpCodes, bool, bool) {
	strSplit := strings.Split(cType, "main.")
	str := strSplit[len(strSplit)-1]
	val, ok := opcodeTypeMap[str]
	return val, ok, strings.Contains(cType, "*")
}

// CLIENT -> SERVER
func mapClientToServerOpCode(opcode OpCodes) (reflect.Value, protoreflect.ProtoMessage) {
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
	}
	LogEQInfo("Got unknown op code map %v", opcode)
	return reflect.ValueOf(""), nil
}

// SERVER -> CLIENT
func mapServerToClientOpCode(opcode OpCodes, structPtr unsafe.Pointer) (protoMessage proto.Message, fieldVal reflect.Value, err error) {
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

//export CloseConnection
func CloseConnection(sessionId int) {
	session := sessionMap[sessionId]
	if session.session == nil {
		return
	}
	delete(sessionMap, sessionId)
	session.session.Close()
}

//export SendPacket
func SendPacket(sessionId int, opcode int, structPtr unsafe.Pointer, structSize int) {
	session := sessionMap[sessionId]
	if session.session == nil {
		return
	}
	messageBytes := make([]byte, 2)
	binary.LittleEndian.PutUint16(messageBytes, uint16(opcode))

	protoMessage, reflectEQStruct, err := mapServerToClientOpCode((OpCodes)(opcode), structPtr)
	if err != nil {
		LogEQInfo("Got an error or unhandled op code: %v", OpCodes_name[int32(opcode)])
		return
	}

	var applyFields func(reflect.Value, protoreflect.ProtoMessage)
	applyFields = func(reflectEQStruct reflect.Value, protoMessage protoreflect.ProtoMessage) {
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
						nestedMessage, reflectEQStruct, err := mapServerToClientOpCode(nestedOpcode, next)
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
					// Fixed char array
					if strings.Contains(cType, "]main._Ctype_char") {
						str := fixedArraySizePattern.FindStringSubmatch(cType)[1]
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
						str := fixedArraySizePattern.FindStringSubmatch(cType)[1]
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
							nestedMessage, _, err := mapServerToClientOpCode(nestedOpCode, reflectEQStruct.Addr().UnsafePointer())
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
						nestedMessage, reflectEQStruct, err := mapServerToClientOpCode(nestedOpCode, unsafePtr)
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

	applyFields(reflectEQStruct, protoMessage)

	bytes, err := proto.Marshal(protoMessage)
	if err != nil {
		return
	}
	messageBytes = append(messageBytes, bytes...)
	session.session.SendMessage(messageBytes)
}

// CLIENT -> SERVER
func handleMessage(msg []byte, webstreamManager unsafe.Pointer, sessionId int, onClientPacket C.OnClientPacket) {
	opcode := binary.LittleEndian.Uint16(msg[0:2])
	restBytes := msg[2:len(msg)]

	reflectEQStruct, protoMessage := mapClientToServerOpCode((OpCodes)(opcode))

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
					str := fixedArraySizePattern.FindStringSubmatch(cType)[1]
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

//export StartServer
func StartServer(port C.int, webstreamManager unsafe.Pointer, onNewConnection C.OnNewConnection, onConnectionClosed C.OnConnectionClosed, onClientPacket C.OnClientPacket, onError C.OnError, logFunc C.OnLogMessage) {
	logInfoFunc = logFunc
	http.HandleFunc("/eq", func(rw http.ResponseWriter, r *http.Request) {
		session := r.Body.(*webtransport.Session)
		session.AcceptSession()
		sessionId++
		sessionMap[sessionId] = webSession{ip: r.RemoteAddr, port: r.URL.Port(), session: session, webstreamManager: webstreamManager}
		split := strings.Split(r.RemoteAddr, ":")
		ip, ipErr := ipconv.IPv4ToInt(net.ParseIP(split[0]))
		if ipErr != nil {
			ip = 0
		}
		port, portErr := strconv.Atoi(split[1])
		if portErr != nil {
			port = 0
		}
		sessionStruct := C.struct_WebSession_Struct{
			remote_addr: C.CString(r.RemoteAddr),
			remote_ip:   C.uint(bits.ReverseBytes32(ip)),
			remote_port: C.uint(port),
		}
		defer C.free(unsafe.Pointer(sessionStruct.remote_addr))
		C.bridge_new_connection(webstreamManager, C.int(sessionId), unsafe.Pointer(&sessionStruct), onNewConnection)

		// Handle incoming datagrams
		go func() {
			for {
				msg, err := session.ReceiveMessage(session.Context())
				if err != nil {
					LogEQInfo("Session closed, ending datagram listener: %v", err)
					delete(sessionMap, sessionId)
					C.bridge_connection_closed(webstreamManager, C.int(sessionId), onConnectionClosed)
					break
				}
				handleMessage(msg, webstreamManager, sessionId, onClientPacket)
			}
		}()

		LogEQInfo("Accepted incoming WebTransport session from IP: %v", r.RemoteAddr)
	})

	// Will want this to be configurable in production
	// This is a TLS cert that can be approved by the client with the associated thumbprint
	// And only lasts for 14 days, i.e. the WebTransport server would need to be restarted every 14 days
	//
	// Alternatively, a dev could supply their own cert that would be good forever
	//
	// This spins up an http server listening on the same port that serves the hash of the generated key and can be
	// accessed by the client by proxying a request since we don't have a real TLS cert in place.
	cert, priv := GenerateCertAndStartServer(int(port))

	go func() {
		server := &webtransport.Server{
			ListenAddr: fmt.Sprintf(":%d", port),
			TLSCert:    webtransport.CertFile{Data: cert}, // webtransport.CertFile{Path: "certificate.pem"},
			TLSKey:     webtransport.CertFile{Data: priv}, // webtransport.CertFile{Path: "certificate.key"},
			QuicConfig: &webtransport.QuicConfig{
				KeepAlive:      true,
				MaxIdleTimeout: 30 * time.Second,
			},
		}
		LogEQInfo("Launching WebTransport server at %s", server.ListenAddr)
		ctx, cancel := context.WithCancel(context.Background())
		stopServer = cancel
		if err := server.Run(ctx); err != nil {
			log.Fatal(err)
			errorString := C.CString(err.Error())
			C.bridge_error(webstreamManager, errorString, onError)
			cancel()
		}

	}()
}

//export StopServer
func StopServer() {
	if stopServer != nil {
		stopServer()
	}
}

func main() {
}
