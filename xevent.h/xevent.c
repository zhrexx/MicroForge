#include "xevent.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define EVENT_DEFAULT_MAX_EVENTS 256
#define EVENT_DEFAULT_MAX_SUBSCRIBERS 64

typedef struct event_subscriber {
    event_callback_t callback;
    void* user_data;
    struct event_subscriber* next;
} event_subscriber_t;

typedef struct event_type {
    char* name;
    uint32_t id;
    event_subscriber_t* subscribers;
    size_t subscriber_count;
} event_type_t;

struct event_subscription {
    uint32_t event_id;
    event_subscriber_t* subscriber;
};

struct event_context {
#ifdef EVENT_PLATFORM_WINDOWS
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
    event_type_t* events;
    size_t event_count;
    size_t max_events;
    uint32_t next_event_id;
};

static event_result_t event_mutex_init(event_context_t* context) {
#ifdef EVENT_PLATFORM_WINDOWS
    InitializeCriticalSection(&context->lock);
    return EVENT_SUCCESS;
#else
    if (pthread_mutex_init(&context->lock, NULL) != 0) {
        return EVENT_ERROR_SYSTEM;
    }
    return EVENT_SUCCESS;
#endif
}

static void event_mutex_destroy(event_context_t* context) {
#ifdef EVENT_PLATFORM_WINDOWS
    DeleteCriticalSection(&context->lock);
#else
    pthread_mutex_destroy(&context->lock);
#endif
}

static void event_mutex_lock(event_context_t* context) {
#ifdef EVENT_PLATFORM_WINDOWS
    EnterCriticalSection(&context->lock);
#else
    pthread_mutex_lock(&context->lock);
#endif
}

static void event_mutex_unlock(event_context_t* context) {
#ifdef EVENT_PLATFORM_WINDOWS
    LeaveCriticalSection(&context->lock);
#else
    pthread_mutex_unlock(&context->lock);
#endif
}

static uint64_t event_get_timestamp() {
#ifdef EVENT_PLATFORM_WINDOWS
    FILETIME ft;
    ULARGE_INTEGER li;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return (li.QuadPart - 116444736000000000ULL) / 10000;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

static event_type_t* event_find_by_id(event_context_t* context, uint32_t event_id) {
    for (size_t i = 0; i < context->event_count; i++) {
        if (context->events[i].id == event_id) {
            return &context->events[i];
        }
    }
    return NULL;
}


event_result_t event_create_context(event_context_t** context, size_t max_events) {
    if (context == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }
    
    *context = (event_context_t*)malloc(sizeof(event_context_t));
    if (*context == NULL) {
        return EVENT_ERROR_OUT_OF_MEMORY;
    }
    
    (*context)->max_events = max_events > 0 ? max_events : EVENT_DEFAULT_MAX_EVENTS;
    (*context)->events = (event_type_t*)malloc(sizeof(event_type_t) * (*context)->max_events);
    if ((*context)->events == NULL) {
        free(*context);
        *context = NULL;
        return EVENT_ERROR_OUT_OF_MEMORY;
    }
    
    (*context)->event_count = 0;
    (*context)->next_event_id = 1; 
    
    event_result_t result = event_mutex_init(*context);
    if (result != EVENT_SUCCESS) {
        free((*context)->events);
        free(*context);
        *context = NULL;
        return result;
    }
    
    return EVENT_SUCCESS;
}

event_result_t event_destroy_context(event_context_t* context) {
    if (context == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }
    
    for (size_t i = 0; i < context->event_count; i++) {
        event_type_t* event_type = &context->events[i];
        
        if (event_type->name != NULL) {
            free(event_type->name);
        }
        
        event_subscriber_t* subscriber = event_type->subscribers;
        while (subscriber != NULL) {
            event_subscriber_t* next = subscriber->next;
            free(subscriber);
            subscriber = next;
        }
    }
    
    free(context->events);
    
    event_mutex_destroy(context);
    
    free(context);
    
    return EVENT_SUCCESS;
}

event_result_t event_register(event_context_t* context, const char* name, uint32_t* event_id) {
    if (context == NULL || name == NULL || event_id == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }
    
    event_mutex_lock(context);
    
    if (context->event_count >= context->max_events) {
        event_mutex_unlock(context);
        return EVENT_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t i = 0; i < context->event_count; i++) {
        if (strcmp(context->events[i].name, name) == 0) {
            *event_id = context->events[i].id;
            event_mutex_unlock(context);
            return EVENT_ERROR_ALREADY_EXISTS;
        }
    }
    
    event_type_t* event_type = &context->events[context->event_count];
    event_type->name = strdup(name);
    if (event_type->name == NULL) {
        event_mutex_unlock(context);
        return EVENT_ERROR_OUT_OF_MEMORY;
    }
    
    event_type->id = context->next_event_id++;
    event_type->subscribers = NULL;
    event_type->subscriber_count = 0;
    
    *event_id = event_type->id;
    context->event_count++;
    
    event_mutex_unlock(context);
    return EVENT_SUCCESS;
}

event_result_t event_subscribe(
    event_context_t* context,
    uint32_t event_id,
    event_callback_t callback,
    void* user_data,
    event_subscription_t** subscription
) {
    if (context == NULL || callback == NULL || subscription == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }
    
    *subscription = NULL;
    
    event_mutex_lock(context);
    
    event_type_t* event_type = event_find_by_id(context, event_id);
    if (event_type == NULL) {
        event_mutex_unlock(context);
        return EVENT_ERROR_NOT_FOUND;
    }
    
    event_subscriber_t* subscriber = (event_subscriber_t*)malloc(sizeof(event_subscriber_t));
    if (subscriber == NULL) {
        event_mutex_unlock(context);
        return EVENT_ERROR_OUT_OF_MEMORY;
    }
    
    subscriber->callback = callback;
    subscriber->user_data = user_data;
    
    subscriber->next = event_type->subscribers;
    event_type->subscribers = subscriber;
    event_type->subscriber_count++;
    
    *subscription = (event_subscription_t*)malloc(sizeof(event_subscription_t));
    if (*subscription == NULL) {
        event_type->subscribers = subscriber->next;
        event_type->subscriber_count--;
        free(subscriber);
        event_mutex_unlock(context);
        return EVENT_ERROR_OUT_OF_MEMORY;
    }
    
    (*subscription)->event_id = event_id;
    (*subscription)->subscriber = subscriber;
    
    event_mutex_unlock(context);
    return EVENT_SUCCESS;
}

event_result_t event_unsubscribe(
    event_context_t* context,
    event_subscription_t* subscription
) {
    if (context == NULL || subscription == NULL) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }
    
    event_mutex_lock(context);
    
    event_type_t* event_type = event_find_by_id(context, subscription->event_id);
    if (event_type == NULL) {
        event_mutex_unlock(context);
        return EVENT_ERROR_NOT_FOUND;
    }
    
    event_subscriber_t** pp = &event_type->subscribers;
    event_subscriber_t* subscriber = *pp;
    
    while (subscriber != NULL) {
        if (subscriber == subscription->subscriber) {
            *pp = subscriber->next;
            event_type->subscriber_count--;
            free(subscriber);
            
            free(subscription);
            
            event_mutex_unlock(context);
            return EVENT_SUCCESS;
        }
        
        pp = &subscriber->next;
        subscriber = *pp;
    }
    
    event_mutex_unlock(context);
    return EVENT_ERROR_NOT_FOUND;
}

event_result_t event_dispatch(
    event_context_t* context,
    uint32_t event_id,
    const void* data,
    size_t data_size
) {
    if (context == NULL || (data == NULL && data_size > 0)) {
        return EVENT_ERROR_INVALID_ARGUMENT;
    }
    
    event_mutex_lock(context);
    
    event_type_t* event_type = event_find_by_id(context, event_id);
    if (event_type == NULL) {
        event_mutex_unlock(context);
        return EVENT_ERROR_NOT_FOUND;
    }
    
    event_t event;
    event.name = event_type->name;
    event.event_id = event_id;
    event.timestamp = event_get_timestamp();
    
    void* event_data = NULL;
    if (data != NULL && data_size > 0) {
        event_data = malloc(data_size);
        if (event_data == NULL) {
            event_mutex_unlock(context);
            return EVENT_ERROR_OUT_OF_MEMORY;
        }
        memcpy(event_data, data, data_size);
    }
    
    event.data = event_data;
    event.data_size = data_size;
    
    event_subscriber_t** subscribers = 
        (event_subscriber_t**)malloc(sizeof(event_subscriber_t*) * event_type->subscriber_count);
    
    if (subscribers == NULL) {
        free(event_data);
        event_mutex_unlock(context);
        return EVENT_ERROR_OUT_OF_MEMORY;
    }
    
    size_t count = 0;
    event_subscriber_t* subscriber = event_type->subscribers;
    
    while (subscriber != NULL && count < event_type->subscriber_count) {
        subscribers[count++] = subscriber;
        subscriber = subscriber->next;
    }
    
    event_mutex_unlock(context);
    
    for (size_t i = 0; i < count; i++) {
        subscribers[i]->callback(&event, subscribers[i]->user_data);
    }
    
    free(subscribers);
    free(event_data);
    
    return EVENT_SUCCESS;
}

const char* event_error_string(event_result_t result) {
    switch (result) {
        case EVENT_SUCCESS:
            return "Success";
        case EVENT_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case EVENT_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case EVENT_ERROR_SYSTEM:
            return "System error";
        case EVENT_ERROR_TIMEOUT:
            return "Timeout";
        case EVENT_ERROR_NOT_FOUND:
            return "Not found";
        case EVENT_ERROR_ALREADY_EXISTS:
            return "Already exists";
        default:
            return "Unknown error";
    }
}
