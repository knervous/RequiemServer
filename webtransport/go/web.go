package main

// #include "c_bridge.h"
// #include <stdlib.h>
import "C"

import (
	"context"
	"encoding/binary"
	"fmt"
	"log"
	"math/big"
	"net"
	"net/http"
	"strconv"
	"strings"
	"time"
	"unsafe"

	"github.com/adriancable/webtransport-go"
	"google.golang.org/protobuf/proto"
)

type webSession struct {
	webstreamManager unsafe.Pointer
	session          *webtransport.Session
	ip               string
	port             string
}

var sessionMap map[int]webSession = map[int]webSession{}
var stopServer context.CancelFunc = nil

//export SendPacket
func SendPacket(sessionId int, opcode int, structPtr unsafe.Pointer) {
	session := sessionMap[sessionId]
	if session.session == nil {
		return
	}
	messageBytes := make([]byte, 2)
	binary.LittleEndian.PutUint16(messageBytes, uint16(opcode))

	switch (OpCodes)(opcode) {
	case OpCodes_OP_LoginAccepted:
		loginReply := (*C.struct_WebLoginReply_Struct)(structPtr)
		loginMessage := LoginReply{
			Key:             C.GoString(loginReply.key),
			ErrorStrId:      int32(loginReply.error_str_id),
			Lsid:            int32(loginReply.lsid),
			Success:         bool(loginReply.success),
			ShowPlayerCount: bool(loginReply.show_player_count),
		}
		bytes, err := proto.Marshal(&loginMessage)
		if err != nil {
			return
		}
		messageBytes = append(messageBytes, bytes...)
		break
	default:
		break
	}
	session.session.SendMessage(messageBytes)
}

func Ip2Int(ip net.IP) *big.Int {
	i := big.NewInt(0)
	i.SetBytes(ip)
	return i
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
				opcode := binary.LittleEndian.Uint16(msg[0:2])
				restBytes := msg[2:len(msg)]
				forward_packet := func(ptr unsafe.Pointer) {
					C.bridge_client_packet(webstreamManager, C.int(sessionId), C.ushort(opcode), ptr, onClientPacket)
				}

				switch (OpCodes)(opcode) {
				case OpCodes_OP_WebLogin:
					loginMessage := &LoginMessage{}
					if err := proto.Unmarshal(restBytes, loginMessage); err != nil {
						log.Fatalln("Failed to parse address book:", err)
					}
					loginStruct := C.struct_WebLogin_Struct{
						username: C.CString(loginMessage.Username),
						password: C.CString(loginMessage.Password),
					}

					defer C.free(unsafe.Pointer(loginStruct.username))
					defer C.free(unsafe.Pointer(loginStruct.password))
					forward_packet(unsafe.Pointer(&loginStruct))

					break
				default:
					break
				}
			}
		}()

		fmt.Println("Accepted incoming WebTransport session")

	})

	go func() {
		server := &webtransport.Server{
			ListenAddr: fmt.Sprintf(":%d", port),
			TLSCert:    webtransport.CertFile{Path: "certificate.pem"},
			TLSKey:     webtransport.CertFile{Path: "certificate.key"},
			// AllowedOrigins: []string{"googlechrome.github.io", "127.0.0.1:8000", "localhost:8000", "new-tab-page", ""},
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
