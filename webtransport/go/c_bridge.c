#include "c_bridge.h"

void bridge_new_connection(void* webstream_manager, int session_id, void* web_session_ptr, OnNewConnection f)
{
   f(webstream_manager, session_id, web_session_ptr);
}
void bridge_connection_closed(void* webstream_manager, int session_id, OnConnectionClosed f)
{
   f(webstream_manager, session_id);
}
void bridge_client_packet(void* webstream_manager, int session_id, uint16_t opcode, void* struct_ptr, OnClientPacket f)
{
   f(webstream_manager, session_id, opcode, struct_ptr);
}

void bridge_error(void* webstream_manager, char* bytes, OnError f) {
   f(webstream_manager, bytes);
}
