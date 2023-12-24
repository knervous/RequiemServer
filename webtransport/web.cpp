#include <iostream>
#include <functional>
#include <string.h>
#include <unordered_map>
#include <memory>

#include "go/web_go.h"
#include "web.h"
#include "go/c/c_bridge.h"

#include <string>
#include <iomanip>
#include <vector>
#include <algorithm>

#ifdef _WINDOWS
#include <time.h>
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#endif

#include "../../common/eqemu_logsys.h"
#include "../../common/patches/web_structs.h"
#include "../../common/emu_opcodes.h"
#include "../../common/net/packet.h"
#include "../../common/global_define.h"
#include "../../common/eqemu_logsys.h"
#include "../../common/eq_packet.h"
#include "../../common/op_codes.h"
#include "../../common/platform.h"
#include "../../loginserver/login_types.h"
#include "../../common/strings.h"

using namespace Web::structs;

typedef void (*Go_CloseConnection)(GoInt sessionId);
typedef void (*Go_SendPacket)(GoInt sessionId, GoInt opcode, void* structPtr, GoInt structSize);
typedef void (*Go_StartServer)(int port, void* webstreamManager, OnNewConnection onNewConnection, OnConnectionClosed onConnectionClosed, OnClientPacket onClientPacket, OnError onError, OnLogMessage logFunc);
typedef void (*Go_StopServer)();


static Go_CloseConnection Loaded_CloseConnection;
static Go_SendPacket Loaded_SendPacket;
static Go_StartServer Loaded_StartServer;
static Go_StopServer Loaded_StopServer;

extern "C"
{
	void Go_OnNewConnection(void *webstream_manager, int session_id, void *web_session_ptr)
	{
		auto *manager = reinterpret_cast<EQ::Net::EQWebStreamManager *>(webstream_manager);
		auto *web_session = reinterpret_cast<WebSession_Struct *>(web_session_ptr);
		manager->WebNewConnection(session_id, web_session);
	}
	void Go_OnConnectionClosed(void *webstream_manager, int session_id)
	{
		auto *manager = reinterpret_cast<EQ::Net::EQWebStreamManager *>(webstream_manager);
		manager->WebConnectionStateChange(session_id, EQ::Net::DbProtocolStatus::StatusConnected, EQ::Net::DbProtocolStatus::StatusDisconnected);
	}
	void Go_OnClientPacket(void *webstream_manager, int session_id, uint16 opcode, void *struct_ptr, int size)
	{
		auto *manager = reinterpret_cast<EQ::Net::EQWebStreamManager *>(webstream_manager);
		manager->WebPacketRecv(session_id, opcode, struct_ptr, size);
	}

	void Go_OnError(void *webstream_manager, char *bytes)
	{
		// Handle startup error
	}
	void Go_LogMessage(char* msg) {
		LogInfo("{}", msg);
	}
}


EQ::Net::EQWebStreamManager::EQWebStreamManager(const EQStreamManagerInterfaceOptions &options) : EQStreamManagerInterface(options)
{
	#ifdef _WINDOWS
	HINSTANCE web_go_dll = LoadLibraryA("web_go.dll");
	auto err = GetLastError();
	if (!web_go_dll) {
		LogWarning("Could not load dll for webtransport. Error {0}", err);
		return;
	}

	Loaded_StartServer = reinterpret_cast<Go_StartServer>(GetProcAddress(web_go_dll, "StartServer"));
	Loaded_SendPacket = reinterpret_cast<Go_SendPacket>(GetProcAddress(web_go_dll, "SendPacket"));
	Loaded_CloseConnection = reinterpret_cast<Go_CloseConnection>(GetProcAddress(web_go_dll, "CloseConnection"));
	Loaded_StopServer = reinterpret_cast<Go_StopServer>(GetProcAddress(web_go_dll, "StopServer"));

	#else
		auto go_webtransport_lib = dlopen("./web_go.so", RTLD_NOW);
    	Loaded_StartServer = reinterpret_cast<Go_StartServer>(dlsym(go_webtransport_lib, "StartServer"));
    	Loaded_SendPacket = reinterpret_cast<Go_SendPacket>(dlsym(go_webtransport_lib, "SendPacket"));
    	Loaded_CloseConnection = reinterpret_cast<Go_CloseConnection>(dlsym(go_webtransport_lib, "CloseConnection"));
    	Loaded_StopServer = reinterpret_cast<Go_StopServer>(dlsym(go_webtransport_lib, "StopServer"));
	#endif
	if (Loaded_StartServer != NULL) {
		Loaded_StartServer(options.daybreak_options.port, this, &Go_OnNewConnection, &Go_OnConnectionClosed, &Go_OnClientPacket, &Go_OnError, &Go_LogMessage);
	}
}

EQ::Net::EQWebStreamManager::~EQWebStreamManager()
{
	if (Loaded_StopServer != NULL) {
		Loaded_StopServer();
	}
	
}

void EQ::Net::EQWebStreamManager::SetOptions(const EQStreamManagerInterfaceOptions &options)
{
	m_options = options;
}

void EQ::Net::EQWebStreamManager::WebNewConnection(int connection, WebSession_Struct *web_session)
{
	std::shared_ptr<EQWebStream> stream(new EQWebStream(this, connection, web_session));
	m_streams.emplace(std::make_pair(connection, stream));
	if (m_on_new_connection)
	{
		m_on_new_connection(stream);
	}
}

void EQ::Net::EQWebStreamManager::WebConnectionStateChange(int connection, DbProtocolStatus from, DbProtocolStatus to)
{
	auto iter = m_streams.find(connection);
	if (iter != m_streams.end())
	{
		if (m_on_connection_state_change)
		{
			m_on_connection_state_change(iter->second, from, to);
		}

		if (to == EQ::Net::StatusDisconnected)
		{
			m_streams.erase(iter);
		}
	}
}

void EQ::Net::EQWebStreamManager::WebPacketRecv(int connection, uint16 opcode, void *struct_ptr, int size)
{
	auto iter = m_streams.find(connection);
	if (iter != m_streams.end())
	{
		auto &stream = iter->second;
		auto dyn_packet = EQ::Net::DynamicPacket();
		dyn_packet.Reserve(2 + size);
		dyn_packet.PutUInt16(0, opcode);
		dyn_packet.PutData(2, struct_ptr, size);
		std::unique_ptr<EQ::Net::Packet> t(new EQ::Net::DynamicPacket());
		t->PutPacket(0, dyn_packet);
		stream->m_packet_queue.push_back(std::move(t));
	}
}

void EQ::Net::EQWebStream::QueuePacket(const EQApplicationPacket *p, bool ack_req)
{
	if (p == nullptr)
		return;

	EQApplicationPacket *newp = p->Copy();

	if (newp != nullptr)
		FastQueuePacket(&newp, ack_req);
}
void EQ::Net::EQWebStream::FastQueuePacket(EQApplicationPacket **p, bool ack_req)
{
	EQApplicationPacket *pack = *p;
	*p = nullptr;

	if (pack == nullptr)
		return;

	SendDatagram(pack->GetOpcode(), pack);
}

void EQ::Net::EQWebStream::SendDatagram(uint16 opcode, EQApplicationPacket *p)
{
	Loaded_SendPacket(m_connection, opcode, p->pBuffer, p->size); 
	delete p;
	p = nullptr;
}

EQApplicationPacket *EQ::Net::EQWebStream::PopPacket()
{
	if (m_packet_queue.empty())
	{
		return nullptr;
	}

	if (m_opcode_manager != nullptr && *m_opcode_manager != nullptr)
	{
		auto &p = m_packet_queue.front();

		uint16 opcode = p->GetUInt16(0);
		EmuOpcode emu_op = (EmuOpcode)opcode;
		m_packet_recv_count[static_cast<int>(emu_op)]++;

		EQApplicationPacket *ret = new EQApplicationPacket(emu_op, (unsigned char *)p->Data() + m_owner->GetOptions().opcode_size, p->Length() - m_owner->GetOptions().opcode_size);
		ret->SetProtocolOpcode(opcode);
		m_packet_queue.pop_front();
		return ret;
	}

	return nullptr;
};
void EQ::Net::EQWebStream::Close()
{
	Loaded_CloseConnection(m_connection);
	SetState(DISCONNECTING);
}

std::string EQ::Net::EQWebStream::GetRemoteAddr() const
{
	return m_web_session.remote_addr;
};
uint32 EQ::Net::EQWebStream::GetRemoteIP() const
{
	return m_web_session.remote_ip;
};
uint16 EQ::Net::EQWebStream::GetRemotePort() const
{
	return m_web_session.remote_port;
};
EQStreamInterface::MatchState EQ::Net::EQWebStream::CheckSignature(const Signature *sig)
{
	return sig->first_eq_opcode == OP_WebInitiateConnection ? MatchSuccessful : MatchNotReady;
}
EQStreamState EQ::Net::EQWebStream::GetState()
{
	return m_state;
}

EQStreamInterface::Stats EQ::Net::EQWebStream::GetStats() const
{
	Stats ret;
	return ret;
}

void EQ::Net::EQWebStream::ResetStats()
{
}

EQStreamManagerInterface *EQ::Net::EQWebStream::GetManager() const
{
	return this->m_owner;
}