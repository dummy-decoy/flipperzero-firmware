#pragma once

#include <stdint.h>
#include <m-string.h>
#include <storage/storage.h>

typedef struct GoofyLexer GoofyLexer;

typedef struct goofy_pos_t {
    uint64_t file_pos;
    uint32_t line;
    char current;
    int state;
} goofy_pos_t;


typedef enum {
    GoofyLexerSymbolError,
    GoofyLexerSymbolEol,
    GoofyLexerSymbolEof,
    GoofyLexerSymbolBullshit,
    // commands
    GoofyLexerSymbolComment,
    GoofyLexerSymbolCmdString,
    GoofyLexerSymbolCmdStringLn,
    GoofyLexerSymbolCmdKey,
    GoofyLexerSymbolCmdDelay,
    GoofyLexerSymbolCmdVar,
    GoofyLexerSymbolCmdHold,
    GoofyLexerSymbolCmdRelease,
    // control flow
    GoofyLexerSymbolCmdIf,
    GoofyLexerSymbolCmdElseIf,
    GoofyLexerSymbolCmdElse,
    GoofyLexerSymbolCmdEndIf,
    GoofyLexerSymbolCmdWhile,
    GoofyLexerSymbolCmdEndWhile,
    GoofyLexerSymbolCmdFunction,
    GoofyLexerSymbolCmdEndFunction,
    GoofyLexerSymbolCmdReturn,
    GoofyLexerSymbolThen,
    // rest of lines
    GoofyLexerSymbolKey,
    GoofyLexerSymbolString,
    // expressions
    GoofyLexerSymbolVariable,
    GoofyLexerSymbolName,
    GoofyLexerSymbolNumber,
    GoofyLexerSymbolTrue,
    GoofyLexerSymbolFalse,
    GoofyLexerSymbolAssign,
    GoofyLexerSymbolAdd,
    GoofyLexerSymbolSub,
    GoofyLexerSymbolMul,
    GoofyLexerSymbolDiv,
    GoofyLexerSymbolMod,
    GoofyLexerSymbolExp,
    GoofyLexerSymbolEqu,
    GoofyLexerSymbolNeq,
    GoofyLexerSymbolLt,
    GoofyLexerSymbolGt,
    GoofyLexerSymbolLeq,
    GoofyLexerSymbolGeq,
    GoofyLexerSymbolAnd,
    GoofyLexerSymbolOr,
    GoofyLexerSymbolNot,
    GoofyLexerSymbolBitAnd,
    GoofyLexerSymbolBitOr,
    GoofyLexerSymbolBitNot,
    GoofyLexerSymbolBitLeft,
    GoofyLexerSymbolBitRight,
    GoofyLexerSymbolOpenPar,
    GoofyLexerSymbolClosePar,
    GoofyLexerSymbolComma,
} GoofyLexerSymbol;

typedef enum {
    GoofyLexerKeyNone = 0,
    GoofyLexerKeyChar,
    GoofyLexerKeyUp,
    GoofyLexerKeyDown,
    GoofyLexerKeyLeft,
    GoofyLexerKeyRight,
    GoofyLexerKeyPageUp,
    GoofyLexerKeyPageDown,
    GoofyLexerKeyHome,
    GoofyLexerKeyEnd,
    GoofyLexerKeyInsert,
    GoofyLexerKeyDelete,
    GoofyLexerKeyBackspace,
    GoofyLexerKeyTab,
    GoofyLexerKeySpace,
    GoofyLexerKeyEnter,
    GoofyLexerKeyEscape,
    GoofyLexerKeyBreak,
    GoofyLexerKeyPrintscreen,
    GoofyLexerKeyApp,
    GoofyLexerKeyF1,
    GoofyLexerKeyF2,
    GoofyLexerKeyF3,
    GoofyLexerKeyF4,
    GoofyLexerKeyF5,
    GoofyLexerKeyF6,
    GoofyLexerKeyF7,
    GoofyLexerKeyF8,
    GoofyLexerKeyF9,
    GoofyLexerKeyF10,
    GoofyLexerKeyF11,
    GoofyLexerKeyF12,
    GoofyLexerKeyNumpad0,
    GoofyLexerKeyNumpad1,
    GoofyLexerKeyNumpad2,
    GoofyLexerKeyNumpad3,
    GoofyLexerKeyNumpad4,
    GoofyLexerKeyNumpad5,
    GoofyLexerKeyNumpad6,
    GoofyLexerKeyNumpad7,
    GoofyLexerKeyNumpad8,
    GoofyLexerKeyNumpad9,
    GoofyLexerKeyLeftCtrl,
    GoofyLexerKeyLeftShift,
    GoofyLexerKeyLeftAlt,
    GoofyLexerKeyLeftGui,
    GoofyLexerKeyRightCtrl,
    GoofyLexerKeyRightShift,
    GoofyLexerKeyRightAlt,
    GoofyLexerKeyRightGui,
    GoofyLexerKeyCapsLock,
    GoofyLexerKeyNumLock,
    GoofyLexerKeyScrollLock,
} GoofyLexerKey;

//GoofyLexer* goofy_lexer_alloc(string_t file_name);
GoofyLexer* goofy_lexer_alloc(File* file);
void goofy_lexer_free(GoofyLexer* lexer);

uint32_t goofy_lexer_line(GoofyLexer* lexer);
goofy_pos_t goofy_lexer_pos(GoofyLexer* lexer);
void goofy_lexer_jmp(GoofyLexer* lexer, goofy_pos_t pos);
GoofyLexerSymbol goofy_lexer_next(GoofyLexer* lexer, string_t content);
