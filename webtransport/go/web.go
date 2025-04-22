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
	"sync"
	"time"
	"unsafe"

	"github.com/praserx/ipconv"
	"github.com/quic-go/quic-go"
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

var (
	sessionMap   = make(map[int]webSession)
	zoneRegistry = make(map[int]int)       // zoneId -> port
	zoneConns    = make(map[int]*zoneConn) // zoneId -> pooled QUIC connection
	zoneMutex    sync.Mutex                // Protects zoneRegistry and zoneConns
	stopServer   context.CancelFunc
	logInfoFunc  C.OnLogMessage
	sessionId    int
	zoneId       = -1
)

// zoneConn holds a pooled QUIC connection and WebTransport dialer for a zone.
type zoneConn struct {
	conn   quic.EarlyConnection
	dialer *webtransport.Dialer
	mutex  sync.Mutex
}

// LogEQInfo relays log messages back to the C side.
func LogEQInfo(message string, args ...any) {
	if logInfoFunc == nil {
		return
	}
	str := C.CString(fmt.Sprintf(message, args...))
	C.bridge_log_message(str, logInfoFunc)
	C.free(unsafe.Pointer(str))
}

//export RegisterZoneId
func RegisterZoneId(id int) {
	zoneId = id
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

// registerWithWorld registers the zone server with the world server over loopback.
func registerWithWorld(zoneId, port int) {
	go func() {
		client := &http.Client{}
		url := "http://127.0.0.1:443/register"
		for {
			resp, err := client.Post(url, "application/json", strings.NewReader(fmt.Sprintf(`{"zoneId": %d, "port": %d}`, zoneId, port)))
			if err != nil {
				LogEQInfo("Failed to register with world server: %v", err)
			} else {
				resp.Body.Close()
				LogEQInfo("Registered zone %d on port %d with world server", zoneId, port)
			}
			time.Sleep(10 * time.Second) // Keep-alive every 10 seconds
		}
	}()
}

// getOrCreateZoneConn gets or creates a pooled QUIC connection for a zone.
func getOrCreateZoneConn(zoneId, zonePort int) (*zoneConn, error) {
	zoneMutex.Lock()
	defer zoneMutex.Unlock()

	if zc, exists := zoneConns[zoneId]; exists {
		return zc, nil
	}

	// Create a new QUIC connection
	udpConn, err := net.ListenUDP("udp", nil)
	if err != nil {
		return nil, err
	}
	transport := &quic.Transport{Conn: udpConn}
	quicConfig := &quic.Config{
		EnableDatagrams: true,
	}
	tlsConfig := &tls.Config{
		InsecureSkipVerify: true, // For testing; replace with proper certs in production
		NextProtos:         []string{"h3"},
	}
	addr := fmt.Sprintf("127.0.0.1:%d", zonePort)
	udpAddr, err := net.ResolveUDPAddr("udp", addr)
	if err != nil {
		return nil, err
	}
	conn, err := transport.DialEarly(context.Background(), udpAddr, tlsConfig, quicConfig)
	if err != nil {
		return nil, fmt.Errorf("failed to dial zone %d at %s: %v", zoneId, addr, err)
	}

	// Create WebTransport dialer with custom DialAddr
	dialer := &webtransport.Dialer{
		TLSClientConfig: tlsConfig,
		QUICConfig:      quicConfig,
		DialAddr: func(ctx context.Context, addr string, tlsCfg *tls.Config, cfg *quic.Config) (quic.EarlyConnection, error) {
			return conn, nil // Reuse the pooled connection
		},
	}

	zc := &zoneConn{
		conn:   conn,
		dialer: dialer,
	}
	zoneConns[zoneId] = zc
	return zc, nil
}

// proxyToZone uses a pooled connection to proxy to the zone server.
func proxyToZone(clientSession *webtransport.Session, zoneId, zonePort int) error {
	zc, err := getOrCreateZoneConn(zoneId, zonePort)
	if err != nil {
		return err
	}

	zc.mutex.Lock()
	defer zc.mutex.Unlock()

	// Dial WebTransport session using the pooled QUIC connection
	url := fmt.Sprintf("https://127.0.0.1:%d/eq", zonePort)
	resp, zoneSession, err := zc.dialer.Dial(context.Background(), url, nil)
	if err != nil {
		return fmt.Errorf("failed to dial zone server at %s: %v", url, err)
	}
	defer resp.Body.Close()

	// Bridge the sessions
	go bridgeSessions(clientSession, zoneSession)

	return nil
}

// bridgeSessions handles streams and datagrams bidirectionally in one goroutine.
func bridgeSessions(client, zone *webtransport.Session) {
	type streamPair struct {
		clientStream webtransport.Stream
		zoneStream   webtransport.Stream
	}
	streamChan := make(chan streamPair, 10) // Buffer for stream pairs
	datagramChan := make(chan []byte, 100)  // Buffer for datagrams

	// Handle client streams
	go func() {
		for {
			clientStream, err := client.AcceptStream(context.Background())
			if err != nil {
				LogEQInfo("Error accepting stream from client: %v", err)
				return
			}
			zoneStream, err := zone.OpenStream()
			if err != nil {
				LogEQInfo("Error opening stream to zone: %v", err)
				clientStream.Close()
				return
			}
			streamChan <- streamPair{clientStream, zoneStream}
		}
	}()

	// Handle zone streams
	go func() {
		for {
			zoneStream, err := zone.AcceptStream(context.Background())
			if err != nil {
				LogEQInfo("Error accepting stream from zone: %v", err)
				return
			}
			clientStream, err := client.OpenStream()
			if err != nil {
				LogEQInfo("Error opening stream to client: %v", err)
				zoneStream.Close()
				return
			}
			streamChan <- streamPair{clientStream, zoneStream}
		}
	}()

	// Handle datagrams with batching
	go func() {
		ticker := time.NewTicker(1 * time.Millisecond) // Batch every 1ms
		defer ticker.Stop()
		var buffer [][]byte
		for {
			select {
			case data := <-datagramChan:
				buffer = append(buffer, data)
			case <-ticker.C:
				if len(buffer) > 0 {
					for _, data := range buffer {
						zone.SendDatagram(data)
						client.SendDatagram(data)
					}
					buffer = nil
				}
			case <-client.Context().Done():
				return
			case <-zone.Context().Done():
				return
			}
		}
	}()

	// Main bridging loop
	for {
		select {
		case pair := <-streamChan:
			go relayStream(pair.clientStream, pair.zoneStream)
		case <-client.Context().Done():
			return
		case <-zone.Context().Done():
			return
		default:
			if data, err := client.ReceiveDatagram(context.Background()); err == nil {
				datagramChan <- data
			}
			if data, err := zone.ReceiveDatagram(context.Background()); err == nil {
				datagramChan <- data
			}
		}
	}
}

// relayStream copies data bidirectionally between two streams.
func relayStream(client, zone webtransport.Stream) {
	defer client.Close()
	defer zone.Close()

	go io.Copy(zone, client) // client -> zone
	io.Copy(client, zone)    // zone -> client
}

// corsMiddleware adds CORS headers to the response and handles OPTIONS preflight requests.
func corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Set CORS headers
		w.Header().Set("Access-Control-Allow-Origin", "*") // Allow all origins; restrict in production if needed
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		w.Header().Set("Access-Control-Max-Age", "86400") // Cache preflight response for 24 hours

		// Handle preflight OPTIONS request
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		// Call the next handler
		next.ServeHTTP(w, r)
	})
}

//export StartServer
func StartServer(worldServer_c C.bool, webstreamManager unsafe.Pointer, onNewConnection C.OnNewConnection, onConnectionClosed C.OnConnectionClosed, onClientPacket C.OnClientPacket, onError C.OnError, logFunc C.OnLogMessage) {
	LogEQInfo("Starting WebTransport server")
	logInfoFunc = logFunc
	worldServer := bool(worldServer_c)

	// Load config
	config, err := NewConfig()
	if err != nil {
		LogEQInfo("Failed to load config: %v", err)
		return
	}
	remoteAddr := config.GetString("server.world.address", "")
	LogEQInfo("Server address from config: %s", remoteAddr)

	// Load TLS config
	tlsConf, err := LoadTLSConfig(worldServer)
	if err != nil {
		LogEQInfo("Failed to load TLS config: %v", err)
		errStr := C.CString(fmt.Sprintf("failed to load TLS config: %v", err))
		C.bridge_error(webstreamManager, errStr, onError)
		C.free(unsafe.Pointer(errStr))
		return
	}

	// Determine bind port
	bindPort := 0
	if worldServer {
		bindPort = 443 // World server uses fixed port 443
	}
	udpAddr, err := net.ResolveUDPAddr("udp", fmt.Sprintf(":%d", bindPort))
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
	port := udpConn.LocalAddr().(*net.UDPAddr).Port
	LogEQInfo("Server bound to UDP port: %d", port)

	// Create WebTransport server
	s := &webtransport.Server{
		H3: http3.Server{
			TLSConfig:       tlsConf,
			EnableDatagrams: true,
		},
		CheckOrigin: func(r *http.Request) bool {
			return true
		},
	}

	// World server: Start HTTPS server for /code and /register
	if worldServer {
		go func() {
			httpMux := http.NewServeMux()
			httpMux.Handle("/code", corsMiddleware(http.HandlerFunc(DiscordAuthHandler)))
			httpMux.HandleFunc("/register", func(w http.ResponseWriter, r *http.Request) {
				if r.RemoteAddr != "127.0.0.1" {
					http.Error(w, "Forbidden", http.StatusForbidden)
					return
				}
				zoneId, _ := strconv.Atoi(r.FormValue("zoneId"))
				port, _ := strconv.Atoi(r.FormValue("port"))
				zoneMutex.Lock()
				zoneRegistry[zoneId] = port
				zoneMutex.Unlock()
				w.Write([]byte("OK"))
			})
			tcpListener, err := net.Listen("tcp", ":443")
			if err != nil {
				LogEQInfo("Failed to create TCP listener for HTTPS: %v", err)
				return
			}
			tlsListener := tls.NewListener(tcpListener, tlsConf)
			LogEQInfo("Starting HTTPS server on TCP port 443")
			if err := http.Serve(tlsListener, httpMux); err != nil {
				LogEQInfo("HTTPS server failed: %v", err)
			}
		}()
	} else if zoneId != -1 {
		// Zone server: Register with world server
		registerWithWorld(zoneId, port)
	}

	// Handle /eq endpoint
	http.HandleFunc("/eq", func(rw http.ResponseWriter, r *http.Request) {
		LogEQInfo("Received request at /eq from %s", r.RemoteAddr)

		if worldServer {
			// Proxy logic for world server
			zoneIdStr := r.URL.Query().Get("zoneId")
			if zoneIdStr != "" {
				zoneId, err := strconv.Atoi(zoneIdStr)
				if err == nil {
					zoneMutex.Lock()
					targetPort, exists := zoneRegistry[zoneId]
					zoneMutex.Unlock()
					if exists {
						LogEQInfo("Proxying request for zone %d to port %d", zoneId, targetPort)
						sess, err := s.Upgrade(rw, r)
						if err != nil {
							LogEQInfo("Failed to upgrade session for proxying: %v", err)
							return
						}
						if err := proxyToZone(sess, zoneId, targetPort); err != nil {
							LogEQInfo("Proxying failed: %v", err)
							sess.CloseWithError(1, "Proxying failed")
						}
						return
					}
					LogEQInfo("Zone %d not found in registry", zoneId)
					http.Error(rw, "Zone not found", http.StatusNotFound)
					return
				}
			}
		}

		// Direct WebTransport handling (world or zone server)
		sess, err := s.Upgrade(rw, r)
		if err != nil {
			LogEQInfo("Failed to upgrade session: %v", err)
			errStr := C.CString(fmt.Sprintf("failed to upgrade session: %v", err))
			C.bridge_error(webstreamManager, errStr, onError)
			C.free(unsafe.Pointer(errStr))
			return
		}
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

		// Handle streams
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

		// Handle datagrams
		go func(sessionID int, sess *webtransport.Session) {
			for {
				data, err := sess.ReceiveDatagram(sess.Context())
				if err != nil {
					LogEQInfo("Error reading datagram: %v", err)
					delete(sessionMap, sessionID)
					C.bridge_connection_closed(webstreamManager, C.int(sessionID), onConnectionClosed)
					break
				}
				LogEQInfo("Received datagram: %d bytes", len(data))
				HandleMessage(data, webstreamManager, sessionID, onClientPacket)
			}
		}(sessionId, sess)

		LogEQInfo("Accepted WebTransport session from IP: %v", r.RemoteAddr)
	})

	// Start WebTransport server
	go func() {
		ctx, cancel := context.WithCancel(context.Background())
		stopServer = cancel
		LogEQInfo("Starting WebTransport UDP server on %s", udpAddr.String())
		if err := s.Serve(udpConn); err != nil {
			LogEQInfo("WebTransport UDP server failed: %v", err)
			errStr := C.CString(fmt.Sprintf("WebTransport UDP server failed: %v", err))
			C.bridge_error(webstreamManager, errStr, onError)
			C.free(unsafe.Pointer(errStr))
			cancel()
		}
		<-ctx.Done()
	}()
}

//export StopServer
func StopServer() {
	if stopServer != nil {
		stopServer()
	}
	zoneMutex.Lock()
	defer zoneMutex.Unlock()
	for _, zc := range zoneConns {
		zc.conn.CloseWithError(0, "")
	}
	zoneConns = make(map[int]*zoneConn)
}

func main() {}
