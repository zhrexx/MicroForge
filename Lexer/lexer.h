#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define ERROR_HAPPENED(msg, ...) \
    printf(msg, __VA_ARGS__);    \
    exit(1);

typedef enum {
    TOKEN_ID,
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_KEYWORD,
    TOKEN_EOF,
    TOKEN_ERROR,
} TokenType;

typedef struct {
    char *lexeme;
    TokenType type;
    size_t line;
    size_t column;
} Token;

typedef struct {
    size_t current_pos;
    size_t current_line;
    size_t current_column;
    char *file_name;
    char *file_content;
    size_t file_size;
} LexerState;

typedef struct {
    LexerState state;
    Token *tokens;
    size_t tokens_size;
    size_t tokens_capacity;
    char **keywords;
    size_t keywords_count;
} Lexer;

#define INIT_TOKEN_CAPACITY 100
#define MAX_KEYWORD_COUNT 50

Token token_create(TokenType type, const char *lexeme, size_t line, size_t column) {
    Token token;
    token.type = type;
    if (lexeme) {
        token.lexeme = strdup(lexeme);
    } else {
        token.lexeme = NULL;
    }
    token.line = line;
    token.column = column;
    return token;
}

void token_destroy(Token *token) {
    if (token->lexeme) {
        free(token->lexeme);
    }
}

int lexer_add_keyword(Lexer *lexer, const char *keyword) {
    if (lexer->keywords_count >= MAX_KEYWORD_COUNT) {
        return 0;
    }
    
    for (size_t i = 0; i < lexer->keywords_count; i++) {
        if (strcmp(lexer->keywords[i], keyword) == 0) {
            return 0;
        }
    }
    
    lexer->keywords[lexer->keywords_count] = strdup(keyword);
    if (!lexer->keywords[lexer->keywords_count]) {
        return 0;
    }
    
    lexer->keywords_count++;
    return 1;
}

Lexer *lexer_create(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        ERROR_HAPPENED("Could not open file: %s\n", filename);
    }

    Lexer *result = (Lexer *)malloc(sizeof(Lexer));
    if (!result) {
        ERROR_HAPPENED("Memory allocation failed for lexer\n", "");
    }

    result->state.file_name = filename;
    result->state.current_pos = 0;
    result->state.current_line = 1;
    result->state.current_column = 1;
    result->tokens_capacity = INIT_TOKEN_CAPACITY;
    result->tokens_size = 0;
    result->tokens = (Token *)malloc(result->tokens_capacity * sizeof(Token));
    if (!result->tokens) {
        free(result);
        ERROR_HAPPENED("Memory allocation failed for tokens\n", "");
    }

    fseek(fp, 0, SEEK_END);
    result->state.file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    result->state.file_content = (char *)malloc(result->state.file_size + 1);
    if (!result->state.file_content) {
        free(result->tokens);
        free(result);
        ERROR_HAPPENED("Memory allocation failed for file content\n", "");
    }

    fread(result->state.file_content, 1, result->state.file_size, fp);
    result->state.file_content[result->state.file_size] = '\0';
    fclose(fp);

    result->keywords = (char **)malloc(MAX_KEYWORD_COUNT * sizeof(char *));
    if (!result->keywords) {
        free(result->state.file_content);
        free(result->tokens);
        free(result);
        ERROR_HAPPENED("Memory allocation failed for keywords\n", "");
    }

    result->keywords_count = 0;
    
    return result;
}

void lexer_destroy(Lexer *lexer) {
    for (size_t i = 0; i < lexer->tokens_size; i++) {
        token_destroy(&lexer->tokens[i]);
    }
    
    for (size_t i = 0; i < lexer->keywords_count; i++) {
        free(lexer->keywords[i]);
    }
    
    free(lexer->keywords);
    free(lexer->state.file_content);
    free(lexer->tokens);
    free(lexer);
}

void lexer_add_token(Lexer *lexer, Token token) {
    if (lexer->tokens_size >= lexer->tokens_capacity) {
        lexer->tokens_capacity *= 2;
        Token *new_tokens = (Token *)realloc(lexer->tokens, lexer->tokens_capacity * sizeof(Token));
        if (!new_tokens) {
            ERROR_HAPPENED("Memory reallocation failed for tokens\n", "");
        }
        lexer->tokens = new_tokens;
    }
    lexer->tokens[lexer->tokens_size++] = token;
}

int lexer_is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

int lexer_is_escape_sequence(char ch) {
    return ch == '\\';
}

char handle_escape_sequence(char escape) {
    switch (escape) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '\\': return '\\';
        case '"': return '"';
        case '\'': return '\'';
        case '0': return '\0';
        default: return escape;
    }
}

int lexer_is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

int lexer_is_identifier_start(char ch) {
    return isalpha(ch) || ch == '_';
}

int lexer_is_identifier_char(char ch) {
    return isalnum(ch) || ch == '_';
}

char lexer_peek(Lexer *lexer) {
    if (lexer->state.current_pos >= lexer->state.file_size) {
        return '\0';
    }
    return lexer->state.file_content[lexer->state.current_pos];
}

char lexer_peek_next(Lexer *lexer) {
    if (lexer->state.current_pos + 1 >= lexer->state.file_size) {
        return '\0';
    }
    return lexer->state.file_content[lexer->state.current_pos + 1];
}

char lexer_advance(Lexer *lexer) {
    char current = lexer_peek(lexer);
    lexer->state.current_pos++;
    if (current == '\n') {
        lexer->state.current_line++;
        lexer->state.current_column = 1;
    } else {
        lexer->state.current_column++;
    }
    return current;
}

int lexer_match(Lexer *lexer, char expected) {
    if (lexer_peek(lexer) != expected) {
        return 0;
    }
    lexer_advance(lexer);
    return 1;
}

int lexer_is_keyword(Lexer *lexer, const char *str) {
    for (size_t i = 0; i < lexer->keywords_count; i++) {
        if (strcmp(str, lexer->keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

Token lexer_get_number_token(Lexer *lexer) {
    size_t start_pos = lexer->state.current_pos;
    size_t start_line = lexer->state.current_line;
    size_t start_column = lexer->state.current_column;
    
    while (lexer_is_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    
    if (lexer_peek(lexer) == '.' && lexer_is_digit(lexer_peek_next(lexer))) {
        lexer_advance(lexer);
        while (lexer_is_digit(lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
    }
    
    size_t length = lexer->state.current_pos - start_pos;
    char *lexeme = (char *)malloc(length + 1);
    if (!lexeme) {
        ERROR_HAPPENED("Memory allocation failed for number lexeme\n", "");
    }
    
    strncpy(lexeme, lexer->state.file_content + start_pos, length);
    lexeme[length] = '\0';
    
    Token token = token_create(TOKEN_NUMBER, lexeme, start_line, start_column);
    free(lexeme);
    
    return token;
}

Token lexer_get_string_token(Lexer *lexer) {
    size_t start_line = lexer->state.current_line;
    size_t start_column = lexer->state.current_column;
    
    lexer_advance(lexer);
    
    char *buffer = (char *)malloc(lexer->state.file_size + 1);
    if (!buffer) {
        ERROR_HAPPENED("Memory allocation failed for string buffer\n", "");
    }
    
    size_t buffer_index = 0;
    
    while (lexer_peek(lexer) != '"' && lexer_peek(lexer) != '\0') {
        if (lexer_peek(lexer) == '\\') {
            lexer_advance(lexer);
            if (lexer_peek(lexer) != '\0') {
                buffer[buffer_index++] = handle_escape_sequence(lexer_peek(lexer));
            }
        } else {
            buffer[buffer_index++] = lexer_peek(lexer);
        }
        lexer_advance(lexer);
    }
    
    if (lexer_peek(lexer) == '\0') {
        free(buffer);
        return token_create(TOKEN_ERROR, "Unterminated string", start_line, start_column);
    }
    
    lexer_advance(lexer);
    
    buffer[buffer_index] = '\0';
    Token token = token_create(TOKEN_STRING, buffer, start_line, start_column);
    free(buffer);
    
    return token;
}

Token lexer_get_identifier_or_keyword_token(Lexer *lexer) {
    size_t start_pos = lexer->state.current_pos;
    size_t start_line = lexer->state.current_line;
    size_t start_column = lexer->state.current_column;
    
    while (lexer_is_identifier_char(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    
    size_t length = lexer->state.current_pos - start_pos;
    char *lexeme = (char *)malloc(length + 1);
    if (!lexeme) {
        ERROR_HAPPENED("Memory allocation failed for identifier lexeme\n", "");
    }
    
    strncpy(lexeme, lexer->state.file_content + start_pos, length);
    lexeme[length] = '\0';
    
    TokenType type = lexer_is_keyword(lexer, lexeme) ? TOKEN_KEYWORD : TOKEN_ID;
    Token token = token_create(type, lexeme, start_line, start_column);
    free(lexeme);
    
    return token;
}

Token lexer_get_next_token(Lexer *lexer);

Token lexer_get_symbol_token(Lexer *lexer, char current_char) {
    size_t start_line = lexer->state.current_line;
    size_t start_column = lexer->state.current_column;
    
    char symbol[3] = {current_char, '\0', '\0'};
    int symbol_length = 1;
    
    switch (current_char) {
        case '=':
            if (lexer_match(lexer, '=')) {
                symbol[1] = '=';
                symbol_length = 2;
            }
            break;
        case '<':
            if (lexer_match(lexer, '=')) {
                symbol[1] = '=';
                symbol_length = 2;
            }
            break;
        case '>':
            if (lexer_match(lexer, '=')) {
                symbol[1] = '=';
                symbol_length = 2;
            }
            break;
        case '!':
            if (lexer_match(lexer, '=')) {
                symbol[1] = '=';
                symbol_length = 2;
            }
            break;
        case '&':
            if (lexer_match(lexer, '&')) {
                symbol[1] = '&';
                symbol_length = 2;
            } else {
                return token_create(TOKEN_ERROR, "Unexpected character '&'", start_line, start_column);
            }
            break;
        case '|':
            if (lexer_match(lexer, '|')) {
                symbol[1] = '|';
                symbol_length = 2;
            } else {
                return token_create(TOKEN_ERROR, "Unexpected character '|'", start_line, start_column);
            }
            break;
        case '/':
            if (lexer_peek(lexer) == '/' || lexer_peek(lexer) == '*') {
                while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') {
                    lexer_advance(lexer);
                }
                return lexer_get_next_token(lexer);
            }
            break;
    }
    
    symbol[symbol_length] = '\0';
    return token_create(TOKEN_SYMBOL, symbol, start_line, start_column);
}

Token lexer_get_next_token(Lexer *lexer) {
    while (lexer_peek(lexer) != '\0') {
        size_t start_line = lexer->state.current_line;
        size_t start_column = lexer->state.current_column;
        
        char current_char = lexer_peek(lexer);
        
        if (lexer_is_whitespace(current_char)) {
            lexer_advance(lexer);
            continue;
        }
        
        if (current_char == '"') {
            return lexer_get_string_token(lexer);
        }
        
        if (lexer_is_digit(current_char)) {
            return lexer_get_number_token(lexer);
        }
        
        if (lexer_is_identifier_start(current_char)) {
            return lexer_get_identifier_or_keyword_token(lexer);
        }
        
        if (current_char == '/' && (lexer_peek_next(lexer) == '/' || lexer_peek_next(lexer) == '*')) {
            if (lexer_peek_next(lexer) == '/') {
                lexer_advance(lexer);
                lexer_advance(lexer);
                
                while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') {
                    lexer_advance(lexer);
                }
                continue;
            } else {
                lexer_advance(lexer);
                lexer_advance(lexer);
                
                while (!(lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') && lexer_peek(lexer) != '\0') {
                    lexer_advance(lexer);
                }
                
                if (lexer_peek(lexer) != '\0') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                }
                continue;
            }
        }
        
        lexer_advance(lexer);
        
        return lexer_get_symbol_token(lexer, current_char);
    }
    
    return token_create(TOKEN_EOF, "EOF", lexer->state.current_line, lexer->state.current_column);
}

void lexer_tokenize_all(Lexer *lexer) {
    Token token;
    do {
        token = lexer_get_next_token(lexer);
        if (token.type != TOKEN_ERROR) {
            lexer_add_token(lexer, token);
        } else {
            token_destroy(&token);
        }
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
}

void lexer_print_token(Token token) {
    printf("Token: %s (Type: %d, Line: %zu, Column: %zu)\n", 
           token.lexeme, token.type, token.line, token.column);
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_ID: return "IDENTIFIER";
        case TOKEN_SYMBOL: return "SYMBOL";
        case TOKEN_STRING: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void lexer_print_tokens(Lexer *lexer) {
    for (size_t i = 0; i < lexer->tokens_size; i++) {
        Token token = lexer->tokens[i];
        printf("Token: %-20s | Type: %-15s | Line: %4zu | Column: %4zu\n", 
               token.lexeme, token_type_to_string(token.type), token.line, token.column);
    }
}

Token lexer_expect(Lexer *lexer, TokenType expected_type, const char *expected_lexeme, const char *error_message) {
    Token token = lexer_get_next_token(lexer);
    
    if (token.type != expected_type) {
        token_destroy(&token);
        return token_create(TOKEN_ERROR, error_message, lexer->state.current_line, lexer->state.current_column);
    }
    
    if (expected_lexeme != NULL && strcmp(token.lexeme, expected_lexeme) != 0) {
        token_destroy(&token);
        return token_create(TOKEN_ERROR, error_message, lexer->state.current_line, lexer->state.current_column);
    }
    
    return token;
}

#endif
