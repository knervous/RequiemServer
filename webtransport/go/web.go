package main

// #include "c/c_bridge.h"
// #include <stdlib.h>
// #cgo CFLAGS: -g -Wall -O0
import "C"

import (
	"context"
	"crypto/tls"
	"fmt"
	"io"
	"math/bits"
	"net"
	"net/http"
	"strconv"
	"strings"
	"unsafe"

	"github.com/praserx/ipconv"
	"github.com/quic-go/quic-go/http3"
	"github.com/quic-go/webtransport-go"
)

// webSession holds session-related information.
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

// LogEQInfo relays log messages back to the C side.
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
	session.session.CloseWithError(0, "")
}

//export SendPacket
func SendPacket(sessionId int, opcode int, structPtr unsafe.Pointer, structSize int) {
	SendEQPacket(sessionId, opcode, structPtr, structSize)
}

// generateTLSConfig creates a tls.Config from certificate and key PEM data.
func generateTLSConfig(certPEM, keyPEM []byte) (*tls.Config, error) {
	cert, err := tls.X509KeyPair(certPEM, keyPEM)
	if err != nil {
		return nil, err
	}
	return &tls.Config{
		Certificates: []tls.Certificate{cert},
		// http3 requires the NextProtos to be set to "h3" (or http3.NextProtoH3)
		NextProtos: []string{http3.NextProtoH3},
	}, nil
}

//export StartServer
func StartServer(port C.int, webstreamManager unsafe.Pointer, onNewConnection C.OnNewConnection, onConnectionClosed C.OnConnectionClosed, onClientPacket C.OnClientPacket, onError C.OnError, logFunc C.OnLogMessage) {
	fmt.Printf("Starting QUIC server on port: %d\n", port)
	LogEQInfo("Starting QUIC server on port: %d", int(port))
	logInfoFunc = logFunc

	certPEM, keyPEM := GenerateCertAndStartServer(int(port + 1))
	LogEQInfo("Generated certPEM length: %d, keyPEM length: %d", len(certPEM), len(keyPEM))

	tlsConf, err := generateTLSConfig(certPEM, keyPEM)
	if err != nil {
		LogEQInfo("Failed to generate TLS config: %v", err)
		errStr := C.CString(fmt.Sprintf("failed to generate TLS config: %v", err))
		C.bridge_error(webstreamManager, errStr, onError)
		C.free(unsafe.Pointer(errStr))
		return
	}

	s := &webtransport.Server{
		H3: http3.Server{
			TLSConfig:       tlsConf,
			EnableDatagrams: true,
		},
	}

	udpAddr, err := net.ResolveUDPAddr("udp", fmt.Sprintf(":%d", int(port)))
	if err != nil {
		LogEQInfo("Failed to resolve UDP address: %v", err)
		errStr := C.CString(fmt.Sprintf("failed to resolve UDP address: %v", err))
		C.bridge_error(webstreamManager, errStr, onError)
		C.free(unsafe.Pointer(errStr))
		return
	}

	udpConn, err := net.ListenUDP("udp", udpAddr)
	if err != nil {
		LogEQInfo("Failed to listen on UDP: %v", err)
		errStr := C.CString(fmt.Sprintf("failed to listen on UDP: %v", err))
		C.bridge_error(webstreamManager, errStr, onError)
		C.free(unsafe.Pointer(errStr))
		return
	}

	http.HandleFunc("/eq", func(rw http.ResponseWriter, r *http.Request) {
		sess, err := s.Upgrade(rw, r)
		if err != nil {
			LogEQInfo("Failed to upgrade session: %v", err)
			errStr := C.CString(fmt.Sprintf("failed to upgrade session: %v", err))
			C.bridge_error(webstreamManager, errStr, onError)
			C.free(unsafe.Pointer(errStr))
			return
		}
		// Rest of the session handling...
		sessionId++
		sessionMap[sessionId] = webSession{
			ip:               r.RemoteAddr,
			port:             r.URL.Port(),
			session:          sess,
			webstreamManager: webstreamManager,
		}
		split := strings.Split(r.RemoteAddr, ":")
		ip, ipErr := ipconv.IPv4ToInt(net.ParseIP(split[0]))
		if ipErr != nil {
			ip = 0
		}
		portNum, portErr := strconv.Atoi(split[1])
		if portErr != nil {
			portNum = 0
		}
		sessionStruct := C.struct_WebSession_Struct{
			remote_addr: C.CString(r.RemoteAddr),
			remote_ip:   C.uint(bits.ReverseBytes32(ip)),
			remote_port: C.uint(portNum),
		}
		defer C.free(unsafe.Pointer(sessionStruct.remote_addr))
		C.bridge_new_connection(webstreamManager, C.int(sessionId), unsafe.Pointer(&sessionStruct), onNewConnection)

		go func(sessionID int, sess *webtransport.Session) {
			for {
				stream, err := sess.AcceptStream(sess.Context())
				if err != nil {
					LogEQInfo("Session closed, ending stream listener: %v", err)
					delete(sessionMap, sessionID)
					C.bridge_connection_closed(webstreamManager, C.int(sessionID), onConnectionClosed)
					break
				}
				data, err := io.ReadAll(stream)
				if err != nil {
					LogEQInfo("Error reading from stream: %v", err)
					continue
				}
				HandleMessage(data, webstreamManager, sessionID, onClientPacket)
			}
		}(sessionId, sess)

		LogEQInfo("Accepted incoming WebTransport session from IP: %v", r.RemoteAddr)
	})

	// Start the WebTransport server only
	go func() {
		_, cancel := context.WithCancel(context.Background())
		stopServer = cancel
		if err := s.Serve(udpConn); err != nil {
			LogEQInfo("WebTransport server failed: %v", err)
			errStr := C.CString(fmt.Sprintf("WebTransport server failed: %v", err))
			C.bridge_error(webstreamManager, errStr, onError)
			C.free(unsafe.Pointer(errStr))
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

func main() {}
