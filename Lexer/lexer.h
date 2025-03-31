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
    TOKEN_ID,         // names (identifiers)
    TOKEN_PLUS,       // +
    TOKEN_MINUS,      // -
    TOKEN_MULTIPLY,   // *
    TOKEN_DIVIDE,     // /
    TOKEN_MODULO,     // %
    TOKEN_EQUALS,     // =
    TOKEN_COLON,      // :
    TOKEN_SEMICOLON,  // ;
    TOKEN_LPAREN,     // (
    TOKEN_RPAREN,     // )
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LBRACKET,   // [
    TOKEN_RBRACKET,   // ]
    TOKEN_LT,         // <
    TOKEN_GT,         // >
    TOKEN_LE,         // <=
    TOKEN_GE,         // >=
    TOKEN_EQ,         // ==
    TOKEN_NE,         // !=
    TOKEN_AND,        // &&
    TOKEN_OR,         // ||
    TOKEN_NOT,        // !
    TOKEN_DOT,        // .
    TOKEN_COMMA,      // ,
    TOKEN_STRING,     // string literals
    TOKEN_NUMBER,     // numbers (integers and floating-point)
    TOKEN_KEYWORD,    // keywords like if, for, while, etc.
    TOKEN_COMMENT,    // comments
    TOKEN_EOF,        // end of file
    TOKEN_ERROR,      // error token
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
#define MAX_KEYWORD_COUNT 20

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
    if (token->lexeme && token->type != TOKEN_PLUS && token->type != TOKEN_MINUS &&
        token->type != TOKEN_EQUALS && token->type != TOKEN_COLON && token->type != TOKEN_SEMICOLON &&
        token->type != TOKEN_LPAREN && token->type != TOKEN_RPAREN && token->type != TOKEN_LBRACE &&
        token->type != TOKEN_RBRACE && token->type != TOKEN_LBRACKET && token->type != TOKEN_RBRACKET &&
        token->type != TOKEN_DOT && token->type != TOKEN_COMMA && token->type != TOKEN_MULTIPLY &&
        token->type != TOKEN_DIVIDE && token->type != TOKEN_MODULO && token->type != TOKEN_LT &&
        token->type != TOKEN_GT && token->type != TOKEN_NOT) {
        free(token->lexeme);
    }
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
    char *standard_keywords[] = {
        "if", "else", "while", "for", "return", "break", "continue", 
        "function", "var", "const", "true", "false", "null", "import"
    };
    
    size_t num_standard_keywords = sizeof(standard_keywords) / sizeof(standard_keywords[0]);
    for (size_t i = 0; i < num_standard_keywords && i < MAX_KEYWORD_COUNT; i++) {
        result->keywords[result->keywords_count++] = strdup(standard_keywords[i]);
    }

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
    
    buffer[buffer_index-1] = '\0';
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

Token lexer_get_comment_token(Lexer *lexer) {
    size_t start_pos = lexer->state.current_pos;
    size_t start_line = lexer->state.current_line;
    size_t start_column = lexer->state.current_column;
    
    lexer_advance(lexer);
    
    if (lexer_peek(lexer) == '/') {
        lexer_advance(lexer);
        
        while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') {
            lexer_advance(lexer);
        }
    } else if (lexer_peek(lexer) == '*') {
        lexer_advance(lexer);
        
        while (!(lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') && lexer_peek(lexer) != '\0') {
            lexer_advance(lexer);
        }
        
        if (lexer_peek(lexer) == '\0') {
            return token_create(TOKEN_ERROR, "Unterminated block comment", start_line, start_column);
        }
        
        lexer_advance(lexer);
        lexer_advance(lexer);
    } else {
        lexer->state.current_pos = start_pos;
        lexer->state.current_line = start_line;
        lexer->state.current_column = start_column;
        return token_create(TOKEN_DIVIDE, "/", start_line, start_column);
    }
    
    size_t length = lexer->state.current_pos - start_pos;
    char *lexeme = (char *)malloc(length + 1);
    if (!lexeme) {
        ERROR_HAPPENED("Memory allocation failed for comment lexeme\n", "");
    }
    
    strncpy(lexeme, lexer->state.file_content + start_pos, length);
    lexeme[length] = '\0';
    
    Token token = token_create(TOKEN_COMMENT, lexeme, start_line, start_column);
    free(lexeme);
    
    return token;
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
        
        if (current_char == '/') {
            if (lexer_peek_next(lexer) == '/' || lexer_peek_next(lexer) == '*') {
                return lexer_get_comment_token(lexer);
            }
        }
        
        lexer_advance(lexer);
        
        switch (current_char) {
            case '+': return token_create(TOKEN_PLUS, "+", start_line, start_column);
            case '-': return token_create(TOKEN_MINUS, "-", start_line, start_column);
            case '*': return token_create(TOKEN_MULTIPLY, "*", start_line, start_column);
            case '/': return token_create(TOKEN_DIVIDE, "/", start_line, start_column);
            case '%': return token_create(TOKEN_MODULO, "%", start_line, start_column);
            case '=':
                if (lexer_match(lexer, '=')) {
                    return token_create(TOKEN_EQ, "==", start_line, start_column);
                }
                return token_create(TOKEN_EQUALS, "=", start_line, start_column);
            case ':': return token_create(TOKEN_COLON, ":", start_line, start_column);
            case ';': return token_create(TOKEN_SEMICOLON, ";", start_line, start_column);
            case '(': return token_create(TOKEN_LPAREN, "(", start_line, start_column);
            case ')': return token_create(TOKEN_RPAREN, ")", start_line, start_column);
            case '{': return token_create(TOKEN_LBRACE, "{", start_line, start_column);
            case '}': return token_create(TOKEN_RBRACE, "}", start_line, start_column);
            case '[': return token_create(TOKEN_LBRACKET, "[", start_line, start_column);
            case ']': return token_create(TOKEN_RBRACKET, "]", start_line, start_column);
            case '<':
                if (lexer_match(lexer, '=')) {
                    return token_create(TOKEN_LE, "<=", start_line, start_column);
                }
                return token_create(TOKEN_LT, "<", start_line, start_column);
            case '>':
                if (lexer_match(lexer, '=')) {
                    return token_create(TOKEN_GE, ">=", start_line, start_column);
                }
                return token_create(TOKEN_GT, ">", start_line, start_column);
            case '!':
                if (lexer_match(lexer, '=')) {
                    return token_create(TOKEN_NE, "!=", start_line, start_column);
                }
                return token_create(TOKEN_NOT, "!", start_line, start_column);
            case '&':
                if (lexer_match(lexer, '&')) {
                    return token_create(TOKEN_AND, "&&", start_line, start_column);
                }
                return token_create(TOKEN_ERROR, "Unexpected character '&'", start_line, start_column);
            case '|':
                if (lexer_match(lexer, '|')) {
                    return token_create(TOKEN_OR, "||", start_line, start_column);
                }
                return token_create(TOKEN_ERROR, "Unexpected character '|'", start_line, start_column);
            case '.': return token_create(TOKEN_DOT, ".", start_line, start_column);
            case ',': return token_create(TOKEN_COMMA, ",", start_line, start_column);
            default:
                return token_create(TOKEN_ERROR, "Unexpected character", start_line, start_column);
        }
    }
    
    return token_create(TOKEN_EOF, "EOF", lexer->state.current_line, lexer->state.current_column);
}

void lexer_tokenize_all(Lexer *lexer) {
    Token token;
    do {
        token = lexer_get_next_token(lexer);
        lexer_add_token(lexer, token);
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
}

void lexer_print_token(Token token) {
    printf("Token: %s (Type: %d, Line: %zu, Column: %zu)\n", 
           token.lexeme, token.type, token.line, token.column);
}

void lexer_print_tokens(Lexer *lexer) {
    for (size_t i = 0; i < lexer->tokens_size; i++) {
        lexer_print_token(lexer->tokens[i]);
    }
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_ID: return "IDENTIFIER";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_MULTIPLY: return "MULTIPLY";
        case TOKEN_DIVIDE: return "DIVIDE";
        case TOKEN_MODULO: return "MODULO";
        case TOKEN_EQUALS: return "EQUALS";
        case TOKEN_COLON: return "COLON";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_LT: return "LESS_THAN";
        case TOKEN_GT: return "GREATER_THAN";
        case TOKEN_LE: return "LESS_EQUAL";
        case TOKEN_GE: return "GREATER_EQUAL";
        case TOKEN_EQ: return "EQUAL_EQUAL";
        case TOKEN_NE: return "NOT_EQUAL";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_DOT: return "DOT";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_STRING: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void lexer_print_tokens_detailed(Lexer *lexer) {
    for (size_t i = 0; i < lexer->tokens_size; i++) {
        Token token = lexer->tokens[i];
        printf("Token: %-20s | Type: %-15s | Line: %4zu | Column: %4zu\n", 
               token.lexeme, token_type_to_string(token.type), token.line, token.column);
    }
}

#endif
