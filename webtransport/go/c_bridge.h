#include "../../common/patches/web_structs.h"

typedef void (*OnNewConnection)(void* webstream_manager, int session_id, void* web_session_ptr);
typedef void (*OnConnectionClosed)(void* webstream_manager, int session_id);
typedef void (*OnClientPacket)(void* webstream_manager, int session_id, uint16_t opcode, void* struct_ptr);
typedef void (*OnError)(void* webstream_manager, char* message);

void bridge_new_connection(void* webstream_manager, int session_id, void* web_session_ptr, OnNewConnection f);
void bridge_connection_closed(void* webstream_manager, int session_id, OnConnectionClosed f);
void bridge_client_packet(void* webstream_manager, int session_id, uint16_t opcode, void* struct_ptr, OnClientPacket f);
void bridge_error(void* webstream_manager, char* bytes, OnError f);
