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
	"math/big"
	"net"
	"net/http"
	reflect "reflect"
	"regexp"
	"strconv"
	"strings"
	"time"
	"unsafe"

	"github.com/adriancable/webtransport-go"
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
var fixedCharPattern *regexp.Regexp = regexp.MustCompile("\\[(\\d+)\\]")

func Ip2Int(ip net.IP) *big.Int {
	i := big.NewInt(0)
	i.SetBytes(ip)
	return i
}

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
	}
	fmt.Println("Got unknown op code map", opcode)
	return reflect.ValueOf(""), nil
}

func mapServerToClientOpCode(opcode OpCodes, structPtr unsafe.Pointer) (protoMessage proto.Message, fieldVal reflect.Value, err error) {
	tie := func(eqStruct interface{}) (protoMessage proto.Message, fieldVal reflect.Value, err error) {
		enumOptions := OpCodes.Descriptor(opcode).Values().ByNumber(opcode.Enum().Number()).Options()
		pbtype, err := protoregistry.GlobalTypes.FindMessageByName(protoreflect.FullName(proto.GetExtension(enumOptions, E_MessageType).(string)))
		if err != nil {
			fmt.Println("Missing map proto message for op code", OpCodes_name[int32(opcode)])
			return nil, reflect.ValueOf(nil), err
		}
		return pbtype.New().Interface(), reflect.ValueOf(eqStruct).Elem(), nil
	}
	switch (OpCodes)(opcode) {
	case OpCodes_OP_LoginAccepted:
		return tie((*C.struct_WebLoginReply_Struct)(structPtr))
	case OpCodes_OP_ServerListResponse:
		return tie((*C.struct_WebLoginServerResponse_Struct)(structPtr))
	// Nested structs
	case OpCodes_Nested_WorldServer:
		return tie((*C.struct_WebLoginWorldServer_Struct)(structPtr))
	}
	return nil, reflect.ValueOf(nil), errors.New("OpCode not found for server to client")
}

//export SendPacket
func SendPacket(sessionId int, opcode int, structPtr unsafe.Pointer) {
	session := sessionMap[sessionId]
	if session.session == nil {
		return
	}
	messageBytes := make([]byte, 2)
	binary.LittleEndian.PutUint16(messageBytes, uint16(opcode))

	protoMessage, reflectEQStruct, err := mapServerToClientOpCode((OpCodes)(opcode), structPtr)
	if err != nil {
		fmt.Println("Got an error or unhandled op code", OpCodes_name[int32(opcode)])
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
							fmt.Println("Err in nested struct", err)
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
				case "main._Ctype_uint":
					protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(int32(rf.Uint())))
					break
				case "main._Ctype__Bool":
					protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(rf.Bool()))
					break

				// Linked list flexible pointers--need concrete types so each need their own case
				case "*main._Ctype_struct_WebLoginWorldServer_Struct":
					mapNestedStructToRepeated(OpCodes_Nested_WorldServer)
					break

				default:
					// Fixed char array
					if strings.Contains(cType, "]main._Ctype_char") {
						str := fixedCharPattern.FindStringSubmatch(cType)[1]
						length, err := strconv.Atoi(str)
						if err != nil {
							break
						}
						slice := (*[1 << 28]C.char)(unsafePtr)[:length:length]
						protoMessage.ProtoReflect().Set(field, protoreflect.ValueOf(C.GoString(&slice[0])))
						break
					}
					fmt.Println("Unhandled c type", cType)
				}

			} else {
				fmt.Println("Invalid field for server to client proto message", originalName)
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

func handleMessage(msg []byte, webstreamManager unsafe.Pointer, sessionId int, onClientPacket C.OnClientPacket) {
	opcode := binary.LittleEndian.Uint16(msg[0:2])
	restBytes := msg[2:len(msg)]

	reflectEQStruct, protoMessage := mapClientToServerOpCode((OpCodes)(opcode))

	proto.Unmarshal(restBytes, protoMessage)
	cleanupFuncs := []func(){}
	fields := protoMessage.ProtoReflect().Descriptor().Fields()
	for i := 0; i < fields.Len(); i++ {
		fieldName := fields.Get(i).Name()
		originalName := (string)(fieldName)
		rf := reflectEQStruct.FieldByName(originalName)
		if rf.IsValid() {
			rf = reflect.NewAt(rf.Type(), unsafe.Pointer(rf.UnsafeAddr())).Elem()
			val := reflect.ValueOf(protoMessage.ProtoReflect().Get(fields.ByName(fieldName)).String())
			fmt.Println("Setting value for original", originalName, val, fmt.Stringer.String(rf.Type()))
			cType := fmt.Stringer.String(rf.Type())
			switch cType {
			case "*main._Ctype_char":
				strVal := C.CString(val.Interface().(string))
				cleanupFuncs = append(cleanupFuncs, func() { C.free(unsafe.Pointer(strVal)) })
				rf.Set(reflect.ValueOf(strVal))
				break
			case "main._Cuint":
				rf.Set(reflect.ValueOf(C.uint(val.Interface().(int))))

				break
			default:
				fmt.Sprintln("Unhandled c type: %s", cType)
			}

		}
	}

	C.bridge_client_packet(webstreamManager, C.int(sessionId), C.ushort(opcode), reflectEQStruct.Addr().UnsafePointer(), onClientPacket)

	// for _, fn := range cleanupFuncs {
	// 	fn()
	// }
}

//export StartServer
func StartServer(port C.int, webstreamManager unsafe.Pointer, onNewConnection C.OnNewConnection, onConnectionClosed C.OnConnectionClosed, onClientPacket C.OnClientPacket, onError C.OnError) {
	http.HandleFunc("/eq", func(rw http.ResponseWriter, r *http.Request) {
		session := r.Body.(*webtransport.Session)
		session.AcceptSession()
		sessionId := int(session.StreamID())
		sessionMap[sessionId] = webSession{ip: r.RemoteAddr, port: r.URL.Port(), session: session, webstreamManager: webstreamManager}
		split := strings.Split(r.RemoteAddr, ":")
		ip := Ip2Int(net.ParseIP(split[0])).Int64()
		port, portErr := strconv.Atoi(split[1])
		if portErr != nil {
			port = 0
		}
		sessionStruct := C.struct_WebSession_Struct{
			remote_addr: C.CString(r.RemoteAddr),
			remote_ip:   C.uint(ip),
			remote_port: C.uint(port),
		}
		defer C.free(unsafe.Pointer(sessionStruct.remote_addr))
		C.bridge_new_connection(webstreamManager, C.int(sessionId), unsafe.Pointer(&sessionStruct), onNewConnection)

		// Handle incoming datagrams
		go func() {
			for {
				msg, err := session.ReceiveMessage(session.Context())
				if err != nil {
					fmt.Println("Session closed, ending datagram listener:", err)
					delete(sessionMap, sessionId)
					C.bridge_connection_closed(webstreamManager, C.int(sessionId), onConnectionClosed)
					break
				}
				handleMessage(msg, webstreamManager, sessionId, onClientPacket)
			}
		}()
		fmt.Println("Accepted incoming WebTransport session")
	})

	go func() {
		server := &webtransport.Server{
			ListenAddr: fmt.Sprintf(":%d", port),
			TLSCert:    webtransport.CertFile{Path: "certificate.pem"},
			TLSKey:     webtransport.CertFile{Path: "certificate.key"},
			QuicConfig: &webtransport.QuicConfig{
				KeepAlive:      true,
				MaxIdleTimeout: 30 * time.Second,
			},
		}

		fmt.Println("Launching WebTransport server at", server.ListenAddr)
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
