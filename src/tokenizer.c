#include "risc_v.h"
#include "common.h"
#include "tokenizer.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

#if 1
static const char * token_strs[] = {
    [TOK_NONE]      = "NONE",
    [',']           = ",",
    [':']           = ":",
    ['(']           = "(",
    [')']           = ")",
    ['\n']          = "\\n",
    [TOK_DIR]       = "DIR",
    [TOK_MNEM]      = "MNEM",
    [TOK_PSEUDO]    = "PSEUDO",
    [TOK_REG]       = "REG",
    [TOK_CSR]       = "CSR",
    [TOK_NUM]       = "NUM",
    [TOK_IDENT]     = "IDENT",
    [TOK_STRING]    = "STRING",
    [TOK_EOF]       = "EOF"
};
#endif

#if 0
#define X CONST_STRING
static String directives[] = {
    X(".text"),
    X(".data"),
    X(".bss"),
    X(".section"),
    X(".align"),
    X(".balign"),
    X(".globl"),
    X(".string"),
    X(".byte"),
    X(".half"),
    X(".word"),
    X(".dword"),
    X(".float"),
    X(".double"),
    X(".option")
};
#undef X

static const size_t num_directives = NELEM(directives);
#endif

#define X CONST_STRING
static String csr_names[] = {
    X("mstatus"),
    X("mhartid")
};
#undef X

static const size_t num_csr_names = NELEM(csr_names);

Tokenizer
init_tokenizer(Buffer buffer)
{
    return (Tokenizer) { .buffer = buffer };
}

static Token
init_token(Tokenizer * tz, TokenType type)
{
    return (Token) {
        .type = type,
        .str = (String) {
            .data = &tz->buffer.p[tz->pos],
            .len = 0
        }
    };
}

static void
tokenizer_advance(Tokenizer * tz)
{
    tz->pos++;
}

static char
tokenizer_get_curr_char(Tokenizer * tz)
{
    if (tz->pos >= tz->buffer.len)
        return '\0';
    return tz->buffer.p[tz->pos];
}

static Token
get_mnemonic_etc(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_NONE);

    bool contains_period = false;
    while (1) {
        char c = tokenizer_get_curr_char(tz);
        if (!isalnum(c) && c != '_' && c != '.')
            break;
        if (c == '.')
            contains_period = true;
        tokenizer_advance(tz);
        tok.str.len++;
    }

#define CHECK_LIST(LIST, TYPE) \
    if (tok.type == TOK_NONE) { \
        for (size_t i = 0; i < num_ ## LIST; i++) { \
            if (string_equal(tok.str, LIST[i])) { \
                tok.type = TYPE; \
                break; \
            } \
        } \
    }
    CHECK_LIST(reg_names,           TOK_REG);
    CHECK_LIST(mnemonics,           TOK_MNEM);
    CHECK_LIST(pseudo_mnemonics,    TOK_PSEUDO);
    CHECK_LIST(csr_names,           TOK_CSR);
#undef CHECK_LIST
    if (tok.type == TOK_NONE) {
        if (contains_period)
            assert(0); // TODO: error-handling
        tok.type = TOK_IDENT;
    }
    
    return tok;
}

// TODO: handle base 8 and 16
static Token
get_number(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_NUM);
    //bool negative = false;
    if (tokenizer_get_curr_char(tz) == '-') {
        //negative = true;
        tokenizer_advance(tz);
        tok.str.len++;
    }
    while (1) {
        char c = tokenizer_get_curr_char(tz);
        if (!isdigit(c))
            break;
        tokenizer_advance(tz);
        tok.str.len++;
    }
    return tok;
}

static Token
get_string(Tokenizer * tz)
{
    tokenizer_advance(tz); // discard "
    Token tok = init_token(tz, TOK_STRING);
    bool prev_esc = 0;
    while (1) {
        char c = tokenizer_get_curr_char(tz);
        tokenizer_advance(tz);
        if ((c == '"' && !prev_esc) || c == '\0')
            break;
        prev_esc = c == '\\';
        tok.str.len++;
    }
    return tok;
}

static Token
get_directive(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_DIR);

    tokenizer_advance(tz); // skip .
    tok.str.len++;

    while (1) {
        char c = tokenizer_get_curr_char(tz);
        if (!isalnum(c))
            break;
        tokenizer_advance(tz);
        tok.str.len++;
    }

    // TODO: check directives list

    return tok;
}

static Token
get_syntax(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_NONE);
    tok.type = tokenizer_get_curr_char(tz);
    tokenizer_advance(tz);
    tok.str.len = 1;
    return tok;
}

void
print_token(Token tok)
{
    fprintf(stderr, "%s '%.*s'\n", token_strs[tok.type], (int) tok.str.len, tok.str.data);
}

// TODO: error-handling
Token
get_token(Tokenizer * tz)
{
    Token tok;

    char c = tokenizer_get_curr_char(tz);
    while (c == ' ' || c == '\t') {
        tokenizer_advance(tz);
        c = tokenizer_get_curr_char(tz);
    }

    if (c == '\0') {
        tz->eof = true;
        tok = (Token) { .type = TOK_EOF, .str = STRING("") };
    } else if (isalpha(c) || c == '_') {
        tok = get_mnemonic_etc(tz);
    } else if (isdigit(c) || c == '-') {
        tok = get_number(tz);
    } else if (c == '"') {
        tok = get_string(tz);
    } else if (c == '.') {
        tok = get_directive(tz);
    } else {
        tok = get_syntax(tz);
        if (tok.type == '\n')
            tz->ln++;
    }

    //print_token(tok);
    return tok;
}
