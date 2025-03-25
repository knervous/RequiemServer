#include "c_bridge.h"

// Bridge functions

void bridge_new_connection(void *webstream_manager, int session_id, void *web_session_ptr, OnNewConnection f)
{
   f(webstream_manager, session_id, web_session_ptr);
}
void bridge_connection_closed(void *webstream_manager, int session_id, OnConnectionClosed f)
{
   f(webstream_manager, session_id);
}
void bridge_client_packet(void *webstream_manager, int session_id, uint16_t opcode, void *struct_ptr, int size, OnClientPacket f)
{
   f(webstream_manager, session_id, opcode, struct_ptr, size);
}

void bridge_error(void *webstream_manager, char *bytes, OnError f)
{
   f(webstream_manager, bytes);
}
void bridge_log_message(char* message, OnLogMessage f)
{
   f(message);
}

// Helper functions
void * ptr_at(void **ptr, int idx) {
    return ptr[idx];
}

int ptr_size() {
   return sizeof(void*);
}