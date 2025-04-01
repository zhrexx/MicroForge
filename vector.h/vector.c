#include "vector.h"

void vector_init(Vector *vector, size_t initial_capacity, size_t element_size) {
    if (initial_capacity == 0) initial_capacity = 1;
    vector->data = malloc(initial_capacity * element_size);
    if (!vector->data) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    vector->size = 0;
    vector->capacity = initial_capacity;
    vector->element_size = element_size;
}

void vector_resize(Vector *vector, size_t new_capacity) {
    if (new_capacity == 0) new_capacity = 1;
    void *new_data = realloc(vector->data, new_capacity * vector->element_size);
    if (!new_data) {
        fprintf(stderr, "Failed to reallocate memory\n");
        exit(EXIT_FAILURE);
    }
    vector->data = new_data;
    vector->capacity = new_capacity;
}

void vector_push(Vector *vector, const void *value) {
    if (vector->size == vector->capacity) {
        vector_resize(vector, vector->capacity == 0 ? 1 : vector->capacity * 2);
    }
    void *target = (char *)vector->data + (vector->size * vector->element_size);
    memcpy(target, value, vector->element_size);
    vector->size++;
}

void *vector_get(Vector *vector, size_t index) {
    if (index >= vector->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    return (char *)vector->data + (index * vector->element_size);
}

void vector_set(Vector *vector, size_t index, const void *value) {
    if (index >= vector->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    void *target = (char *)vector->data + (index * vector->element_size);
    memcpy(target, value, vector->element_size);
}

bool vector_contains(Vector *vector, const void *value) {
    for (size_t i = 0; i < vector->size; i++) {
        void *current_element = (char *)vector->data + (i * vector->element_size);

        if (vector->element_size == sizeof(char *)) {
            const char *str1 = *(const char **)current_element;
            const char *str2 = (const char *)value;
            if (strcmp(str1, str2) == 0) {
                return true;
            }
        } else {
            if (memcmp(current_element, value, vector->element_size) == 0) {
                return true;
            }
        }
    }
    return false;
}

void vector_remove(Vector *vector, size_t index) {
    if (index >= vector->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    void *target = (char *)vector->data + (index * vector->element_size);
    void *next_element = (char *)vector->data + ((index + 1) * vector->element_size);
    memmove(target, next_element, (vector->size - index - 1) * vector->element_size);
    vector->size--;
}

ssize_t vector_find(Vector *vector, const void *value) {
    for (size_t i = 0; i < vector->size; i++) {
        void *current_element = (char *)vector->data + (i * vector->element_size);
        if (memcmp(current_element, value, vector->element_size) == 0) {
            return i;
        }
    }
    return -1;
}

void vector_compress(Vector *vector) {
    if (vector->capacity > vector->size) {
        vector_resize(vector, vector->size);
    }
}

void vector_copy(Vector *dest, const Vector *src) {
    vector_init(dest, src->capacity, src->element_size);
    memcpy(dest->data, src->data, src->size * src->element_size);
    dest->size = src->size;
}

void vector_free(Vector *vector) {
    if (vector->element_size == sizeof(char *)) {
        for (size_t i = 0; i < vector->size; i++) {
            char **element = (char **)vector_get(vector, i);
            free(*element);
        }
    } else if (vector->element_size == sizeof(Vector)) {
        for (size_t i = 0; i < vector->size; i++) {
            Vector *nested = (Vector *)vector_get(vector, i);
            vector_free(nested);
        }
    }

    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
    vector->element_size = 0;
}

size_t vector_len(Vector *vector) {
    return vector->size;
}

void *vector_pop(Vector *vector) {
    if (vector->size == 0) {
        fprintf(stderr, "Vector is empty\n");
        exit(EXIT_FAILURE);
    }

    void *last_element = vector_get(vector, vector->size - 1);
    vector_remove(vector, vector->size - 1);

    return last_element;
}

Vector parse_pargs(int argc, char **argv) {
    Vector pargs_vector;
    vector_init(&pargs_vector, 3, sizeof(char *));
    for (int i = 0; i < argc; i++) {
        char *arg = strdup(argv[i]);
        vector_push(&pargs_vector, &arg);
    }

    return pargs_vector;
}

Vector split_to_vector(const char* src, const char* delimiter) {
    char* src_copy = strdup(src);
    if (!src_copy) {
        fprintf(stderr, "Failed to duplicate source string\n");
        exit(EXIT_FAILURE);
    }

    char* token;
    Vector result;
    vector_init(&result, 10, sizeof(char *));

    token = strtok(src_copy, delimiter);
    while (token != NULL) {
        char *token_copy = strdup(token);
        if (!token_copy) {
            fprintf(stderr, "Failed to duplicate token\n");
            free(src_copy);
            vector_free(&result);
            exit(EXIT_FAILURE);
        }
        vector_push(&result, &token_copy);
        token = strtok(NULL, delimiter);
    }

    free(src_copy);
    return result;
}

char *vector_get_str(Vector *vector, size_t index) {
    void *pointer = vector_get(vector, index);
    return *(char **)pointer;
}

int vector_get_int(Vector *vector, size_t index) {
    void *pointer = vector_get(vector, index);
    return *(int *)pointer;
}

bool vector_empty(Vector *vector) {
    return vector->size == 0;
}

char *vector_to_string(Vector *vector, const char *separator) {
    if (vector_empty(vector)) {
        return strdup("");
    }

    size_t total_length = 0;
    size_t sep_len = strlen(separator);
    for (size_t i = 0; i < vector->size; i++) {
        char *element = vector_get_str(vector, i);
        total_length += strlen(element);
    }
    total_length += sep_len * (vector->size - 1);

    char *result = malloc(total_length + 1);
    if (!result) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    char *pos = result;
    for (size_t i = 0; i < vector->size; i++) {
        char *element = vector_get_str(vector, i);
        size_t element_len = strlen(element);
        memcpy(pos, element, element_len);
        pos += element_len;
        if (i < vector->size - 1) {
            memcpy(pos, separator, sep_len);
            pos += sep_len;
        }
    }
    *pos = '\0';

    return result;
}
