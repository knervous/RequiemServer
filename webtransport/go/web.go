package main

// #include "c/c_bridge.h"
// #include <stdlib.h>
// #cgo CFLAGS: -g -Wall -O0
import "C"

import (
	"context"
	"fmt"
	"log"
	"math/bits"
	"net"
	"net/http"
	"strconv"
	"strings"
	"time"
	"unsafe"

	"github.com/adriancable/webtransport-go"
	"github.com/praserx/ipconv"
)

type webSession struct {
	webstreamManager unsafe.Pointer
	session          *webtransport.Session
	ip               string
	port             string
}

var sessionMap map[int]webSession = map[int]webSession{}
var stopServer context.CancelFunc = nil

var logInfoFunc C.OnLogMessage = nil
var sessionId int = 0

func LogEQInfo(message string, args ...any) {
	if logInfoFunc == nil {
		return
	}
	str := C.CString(fmt.Sprintf(message, args...))
	C.bridge_log_message(str, logInfoFunc)
	C.free(unsafe.Pointer(str))
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
	SendEQPacket(sessionId, opcode, structPtr, structSize)
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
				HandleMessage(msg, webstreamManager, sessionId, onClientPacket)
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
