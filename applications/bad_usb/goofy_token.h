#pragma once

#include <stdint.h>
#include <m-string.h>
#include <storage/storage.h>

typedef struct GoofyToken GoofyToken;

typedef uint64_t goofy_pos_t;

typedef enum {
    GoofyTokenSymbolError,
    GoofyTokenSymbolEof,
    GoofyTokenSymbolBullshit,
    // commands
    GoofyTokenSymbolCmdComment,
    GoofyTokenSymbolCmdString,
    GoofyTokenSymbolCmdStringLn,
    GoofyTokenSymbolCmdKey,
    GoofyTokenSymbolCmdDelay,
    GoofyTokenSymbolCmdVar,
    GoofyTokenSymbolCmdHold,
    GoofyTokenSymbolCmdRelease,
    // control flow
    GoofyTokenSymbolCmdIf,
    GoofyTokenSymbolCmdElseIf,
    GoofyTokenSymbolCmdElse,
    GoofyTokenSymbolCmdEndIf,
    GoofyTokenSymbolCmdWhile,
    GoofyTokenSymbolCmdEndWhile,
    GoofyTokenSymbolCmdFunction,
    GoofyTokenSymbolCmdEndFunction,
    GoofyTokenSymbolCmdReturn,
    GoofyTokenSymbolThen,
    // rest of lines
    GoofyTokenSymbolKey,
    GoofyTokenSymbolString,
    // expressions
    GoofyTokenSymbolName,
    GoofyTokenSymbolNumber,
    GoofyTokenSymbolTrue,
    GoofyTokenSymbolFalse,
    GoofyTokenSymbolAssign,
    GoofyTokenSymbolAdd,
    GoofyTokenSymbolSub,
    GoofyTokenSymbolMul,
    GoofyTokenSymbolDiv,
    GoofyTokenSymbolMod,
    GoofyTokenSymbolExp,
    GoofyTokenSymbolEqu,
    GoofyTokenSymbolNeq,
    GoofyTokenSymbolLt,
    GoofyTokenSymbolGt,
    GoofyTokenSymbolLeq,
    GoofyTokenSymbolGeq,
    GoofyTokenSymbolAnd,
    GoofyTokenSymbolOr,
    GoofyTokenSymbolNot,
    GoofyTokenSymbolBitAnd,
    GoofyTokenSymbolBitOr,
    GoofyTokenSymbolBitNot,
    GoofyTokenSymbolBitLeft,
    GoofyTokenSymbolBitRight,
    GoofyTokenSymbolOpenPar,
    GoofyTokenSymbolClosePar,

} GoofyTokenSymbol;

typedef enum {
    GoofyTokenKeyNone = 0,
    GoofyTokenKeyChar,
    GoofyTokenKeyUp,
    GoofyTokenKeyDown,
    GoofyTokenKeyLeft,
    GoofyTokenKeyRight,
    GoofyTokenKeyPageUp,
    GoofyTokenKeyPageDown,
    GoofyTokenKeyHome,
    GoofyTokenKeyEnd,
    GoofyTokenKeyInsert,
    GoofyTokenKeyDelete,
    GoofyTokenKeyBackspace,
    GoofyTokenKeyTab,
    GoofyTokenKeySpace,
    GoofyTokenKeyEnter,
    GoofyTokenKeyEscape,
    GoofyTokenKeyBreak,
    GoofyTokenKeyPrintscreen,
    GoofyTokenKeyApp,
    GoofyTokenKeyF1,
    GoofyTokenKeyF2,
    GoofyTokenKeyF3,
    GoofyTokenKeyF4,
    GoofyTokenKeyF5,
    GoofyTokenKeyF6,
    GoofyTokenKeyF7,
    GoofyTokenKeyF8,
    GoofyTokenKeyF9,
    GoofyTokenKeyF10,
    GoofyTokenKeyF11,
    GoofyTokenKeyF12,
    GoofyTokenKeyLeftCtrl,
    GoofyTokenKeyLeftShift,
    GoofyTokenKeyLeftAlt,
    GoofyTokenKeyLeftGui,
    GoofyTokenKeyRightCtrl,
    GoofyTokenKeyRightShift,
    GoofyTokenKeyRightAlt,
    GoofyTokenKeyRightGui,
    GoofyTokenKeyCapsLock,
    GoofyTokenKeyNumLock,
    GoofyTokenKeyScrollLock,
} GoofyTokenKey;

//GoofyToken* goofy_token_alloc(string_t file_name);
GoofyToken* goofy_token_alloc(File* file);
void goofy_token_free(GoofyToken* token);

uint32_t goofy_token_line(GoofyToken* token);
goofy_pos_t goofy_token_pos(GoofyToken* token);
void goofy_token_jmp(GoofyToken* token, goofy_pos_t pos);
GoofyTokenSymbol goofy_token_next(GoofyToken* token, string_t content);
