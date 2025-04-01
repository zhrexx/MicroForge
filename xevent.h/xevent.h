#ifndef EVENT_H_
#define EVENT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define EVENT_PLATFORM_WINDOWS
    #include <windows.h>
#else
    #define EVENT_PLATFORM_POSIX
    #include <pthread.h>
#endif

typedef enum {
    EVENT_SUCCESS = 0,
    EVENT_ERROR_INVALID_ARGUMENT = -1,
    EVENT_ERROR_OUT_OF_MEMORY = -2,
    EVENT_ERROR_SYSTEM = -3,
    EVENT_ERROR_TIMEOUT = -4,
    EVENT_ERROR_NOT_FOUND = -5,
    EVENT_ERROR_ALREADY_EXISTS = -6
} event_result_t;

typedef struct event_context event_context_t;
typedef struct event_subscription event_subscription_t;

typedef struct {
    const char* name;
    void* data;
    size_t data_size;
    uint64_t timestamp;
    uint32_t event_id;
} event_t;

typedef void (*event_callback_t)(const event_t* event, void* user_data);
event_result_t event_create_context(event_context_t** context, size_t max_events);
event_result_t event_destroy_context(event_context_t* context);
event_result_t event_register(event_context_t* context, const char* name, uint32_t* event_id);

event_result_t event_subscribe(
    event_context_t* context,
    uint32_t event_id,
    event_callback_t callback,
    void* user_data,
    event_subscription_t** subscription
);

event_result_t event_unsubscribe(
    event_context_t* context,
    event_subscription_t* subscription
);

event_result_t event_dispatch(
    event_context_t* context,
    uint32_t event_id,
    const void* data,
    size_t data_size
);
const char* event_error_string(event_result_t result);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_H_ */
