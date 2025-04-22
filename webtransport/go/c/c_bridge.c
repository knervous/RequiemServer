#include "c_bridge.h"
#include <string.h>
#include <stdlib.h>

// #define MAX_CALLBACKS 64
// static struct CallbackEntry callbacks[MAX_CALLBACKS];
// static int callback_count = 0;

// void register_callback(const char* name, GenericCallback callback) {
//     if (callback_count >= MAX_CALLBACKS) return;
//     callbacks[callback_count].name = strdup(name);
//     callbacks[callback_count].callback = callback;
//     callback_count++;
// }

// void invoke_callback(const char* name, void* webstream_manager, int session_id, void* data, int size) {
//     for (int i = 0; i < callback_count; i++) {
//         if (strcmp(callbacks[i].name, name) == 0) {
//             callbacks[i].callback(webstream_manager, session_id, data, size);
//             return;
//         }
//     }
// }

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