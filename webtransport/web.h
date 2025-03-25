#pragma once

#include "../common/eq_packet.h"
#include "../common/eq_stream_intf.h"
#include "../common/opcodemgr.h"
#include "../common/patches/web_structs.h"
#include <vector>
#include <deque>
#include <unordered_map>

namespace EQ
{
	namespace Net
	{
		class EQWebStream;
		class EQWebStreamManager : public EQStreamManagerInterface
		{
		public:
			EQWebStreamManager(const EQStreamManagerInterfaceOptions &options);
			~EQWebStreamManager();

			virtual void SetOptions(const EQStreamManagerInterfaceOptions &options);
			void OnNewConnection(std::function<void(std::shared_ptr<EQWebStream>)> func) { m_on_new_connection = func; }
			void OnConnectionStateChange(std::function<void(std::shared_ptr<EQWebStream>, DbProtocolStatus, DbProtocolStatus)> func) { m_on_connection_state_change = func; }
			void WebNewConnection(int connection, Web::structs::WebSession_Struct* web_session);
			void WebConnectionStateChange(int connection, DbProtocolStatus from, DbProtocolStatus to);
			void WebPacketRecv(int connection, uint16 opcode, void *struct_ptr, int size);

		private:
			std::function<void(std::shared_ptr<EQWebStream>)> m_on_new_connection;
			std::function<void(std::shared_ptr<EQWebStream>, DbProtocolStatus, DbProtocolStatus)> m_on_connection_state_change;
			std::map<int, std::shared_ptr<EQWebStream>> m_streams = {};

			friend class EQWebStream;
		};

		struct EQWebSession {
			std::string remote_addr;
			uint32_t remote_ip;
			uint32_t remote_port;
		};

		class EQWebStream : public EQStreamInterface
		{
		public:
			EQWebStream(EQStreamManagerInterface *parent, int connection, Web::structs::WebSession_Struct* web_session) : m_owner(parent), m_connection(connection){
				m_web_session.remote_addr = std::string(web_session->remote_addr);
				m_web_session.remote_ip = web_session->remote_ip;
				m_web_session.remote_port = web_session->remote_port;
			};
			~EQWebStream(){};

			virtual void QueuePacket(const EQApplicationPacket *p, bool ack_req = true);
			virtual void FastQueuePacket(EQApplicationPacket **p, bool ack_req = true);
			virtual EQApplicationPacket *PopPacket();
			virtual void Close();
			virtual void ReleaseFromUse(){};
			virtual void RemoveData(){};
			virtual std::string GetRemoteAddr() const;
			virtual uint32 GetRemoteIP() const;
			virtual uint16 GetRemotePort() const;
			virtual bool CheckState(EQStreamState state) { return GetState() == state; }
			virtual std::string Describe() const { return "EQStream over WebTransport"; }
			virtual void SetActive(bool val) {}
			virtual MatchState CheckSignature(const Signature *sig);
			virtual EQStreamState GetState();
			virtual void SetState(EQStreamState s) { m_state = s; };
			virtual void SetOpcodeManager(OpcodeManager **opm)
			{
				m_opcode_manager = opm;
			}
			virtual OpcodeManager *GetOpcodeManager() const
			{
				return (*m_opcode_manager);
			};

			virtual Stats GetStats() const;
			virtual void ResetStats();
			virtual EQStreamManagerInterface *GetManager() const;
			virtual const bool IsWebstream() const { return true; }

		private:
			void SendDatagram(uint16 opcode, EQApplicationPacket *p);
			EQStreamManagerInterface *m_owner;
			EQStreamState m_state = ESTABLISHED;
			int m_connection;
			OpcodeManager **m_opcode_manager;
			std::deque<std::unique_ptr<EQ::Net::Packet>> m_packet_queue;
			std::unordered_map<int, int> m_packet_recv_count;
			std::unordered_map<int, int> m_packet_sent_count;
			EQWebSession m_web_session;
			friend class EQWebStreamManager;
		};
	}
}