#include "goofy_token.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

typedef enum {
    GoofyTokenStateCommand,
    GoofyTokenStateString,
    GoofyTokenStateKeys,
    GoofyTokenStateExpression,
} GoofyTokenState;

struct GoofyToken {
    File* file;
    char current;
    GoofyTokenState state;
    uint32_t line_cnt;
};

static inline void goofy_token_advance(GoofyToken* token) {
    char next;
    if(storage_file_read(token->file, &next, 1) == 1) {
        token->current = next;
    }
}
static inline char goofy_token_current(GoofyToken* token) {
    return token->current;
}
static inline bool goofy_token_end(GoofyToken* token) {
    return storage_file_eof(token->file);
}
static inline void goofy_token_skip_spaces(GoofyToken* token) {
    while(!goofy_token_end(token) && isspace((uint8_t)goofy_token_current(token)) &&
          (goofy_token_current(token) != '\n'))
        goofy_token_advance(token);
}
static GoofyTokenKey goofy_token_key(string_t content) {
    if((string_cmp_str(content, "UP") == 0) || string_cmp_str(content, "UPARROW") == 0) {
        return GoofyTokenKeyUp;
    } else if((string_cmp_str(content, "DOWN") == 0) || (string_cmp_str(content, "DOWNARROW") == 0)) {
        return GoofyTokenKeyDown;
    } else if((string_cmp_str(content, "LEFT") == 0) || (string_cmp_str(content, "LEFTARROW") == 0)) {
        return GoofyTokenKeyLeft;
    } else if(
        (string_cmp_str(content, "RIGHT") == 0) || (string_cmp_str(content, "RIGHTARROW") == 0)) {
        return GoofyTokenKeyRight;
    } else if(string_cmp_str(content, "PAGEUP") == 0) {
        return GoofyTokenKeyPageUp;
    } else if(string_cmp_str(content, "PAGEDOWN") == 0) {
        return GoofyTokenKeyPageDown;
    } else if(string_cmp_str(content, "HOME") == 0) {
        return GoofyTokenKeyHome;
    } else if(string_cmp_str(content, "END") == 0) {
        return GoofyTokenKeyEnd;
    } else if((string_cmp_str(content, "INSERT") == 0) || (string_cmp_str(content, "INS") == 0)) {
        return GoofyTokenKeyInsert;
    } else if((string_cmp_str(content, "DELETE") == 0) || (string_cmp_str(content, "DEL") == 0)) {
        return GoofyTokenKeyDelete;
    } else if((string_cmp_str(content, "BACKSPACE") == 0) || (string_cmp_str(content, "BACK") == 0)) {
        return GoofyTokenKeyBackspace;
    } else if(string_cmp_str(content, "TAB") == 0) {
        return GoofyTokenKeyTab;
    } else if(string_cmp_str(content, "SPACE") == 0) {
        return GoofyTokenKeySpace;
    } else if(string_cmp_str(content, "ENTER") == 0) {
        return GoofyTokenKeyEnter;
    } else if((string_cmp_str(content, "ESCAPE") == 0) || (string_cmp_str(content, "ESC") == 0)) {
        return GoofyTokenKeyEscape;
    } else if((string_cmp_str(content, "PAUSE") == 0) || (string_cmp_str(content, "BREAK") == 0)) {
        return GoofyTokenKeyBreak;
    } else if(string_cmp_str(content, "PRINTSCREEN") == 0) {
        return GoofyTokenKeyPrintscreen;
    } else if((string_cmp_str(content, "MENU") == 0) || (string_cmp_str(content, "APP") == 0)) {
        return GoofyTokenKeyApp;
    } else if(string_cmp_str(content, "F1") == 0) {
        return GoofyTokenKeyF1;
    } else if(string_cmp_str(content, "F2") == 0) {
        return GoofyTokenKeyF2;
    } else if(string_cmp_str(content, "F3") == 0) {
        return GoofyTokenKeyF3;
    } else if(string_cmp_str(content, "F4") == 0) {
        return GoofyTokenKeyF4;
    } else if(string_cmp_str(content, "F5") == 0) {
        return GoofyTokenKeyF5;
    } else if(string_cmp_str(content, "F6") == 0) {
        return GoofyTokenKeyF6;
    } else if(string_cmp_str(content, "F7") == 0) {
        return GoofyTokenKeyF7;
    } else if(string_cmp_str(content, "F8") == 0) {
        return GoofyTokenKeyF8;
    } else if(string_cmp_str(content, "F9") == 0) {
        return GoofyTokenKeyF9;
    } else if(string_cmp_str(content, "F10") == 0) {
        return GoofyTokenKeyF10;
    } else if(string_cmp_str(content, "F11") == 0) {
        return GoofyTokenKeyF11;
    } else if(string_cmp_str(content, "F12") == 0) {
        return GoofyTokenKeyF12;
    } else if(
        (string_cmp_str(content, "CONTROL") == 0) || (string_cmp_str(content, "CTRL") == 0) ||
        (string_cmp_str(content, "LCTRL") == 0)) {
        return GoofyTokenKeyLeftCtrl;
    } else if(string_cmp_str(content, "RCTRL") == 0) {
        return GoofyTokenKeyRightCtrl;
    } else if((string_cmp_str(content, "SHIFT") == 0) || (string_cmp_str(content, "LSHIFT") == 0)) {
        return GoofyTokenKeyLeftShift;
    } else if(string_cmp_str(content, "RSHIFT") == 0) {
        return GoofyTokenKeyRightShift;
    } else if(
        (string_cmp_str(content, "ALT") == 0) || (string_cmp_str(content, "LALT") == 0) ||
        (string_cmp_str(content, "OPTION") == 0)) {
        return GoofyTokenKeyLeftAlt;
    } else if(string_cmp_str(content, "RALT") == 0) {
        return GoofyTokenKeyRightAlt;
    } else if(
        (string_cmp_str(content, "WINDOWS") == 0) || (string_cmp_str(content, "GUI") == 0) ||
        (string_cmp_str(content, "LGUI") == 0) || (string_cmp_str(content, "COMMAND") == 0)) {
        return GoofyTokenKeyLeftGui;
    } else if(string_cmp_str(content, "RGUI") == 0) {
        return GoofyTokenKeyRightGui;
    } else if(string_cmp_str(content, "CAPSLOCK") == 0) {
        return GoofyTokenKeyCapsLock;
    } else if(string_cmp_str(content, "NUMLOCK") == 0) {
        return GoofyTokenKeyNumLock;
    } else if(
        (string_cmp_str(content, "SCROLLLOCK") == 0) ||
        (string_cmp_str(content, "SCROLLOCK") == 0)) {
        return GoofyTokenKeyScrollLock;
    } else {
        return GoofyTokenKeyNone;
    };
}

/*GoofyToken* goofy_token_alloc(string_t file_name) {
    GoofyToken* token = malloc(sizeof(GoofyToken));

    token->file = storage_file_alloc(furi_record_open(RECORD_STORAGE));
    storage_file_open(token->file, string_get_cstr(file_name), FSAM_READ, FSOM_OPEN_EXISTING);

    token->current = '\0';
    token->state = GoofyTokenStateCommand;

    goofy_token_advance(token);
    return token;
}*/
GoofyToken* goofy_token_alloc(File* file) {
    GoofyToken* token = malloc(sizeof(GoofyToken));

    token->file = file;
    token->current = '\0';
    token->state = GoofyTokenStateCommand;
    token->line_cnt = 1;

    goofy_token_advance(token);
    return token;
}

void goofy_token_free(GoofyToken* token) {
    storage_file_close(token->file);
    storage_file_free(token->file);
    free(token);
}

uint32_t goofy_token_line(GoofyToken* token) {
    return token->line_cnt;
}

goofy_pos_t goofy_token_pos(GoofyToken* token) {
    return storage_file_tell(token->file);
}

void goofy_token_jmp(GoofyToken* token, goofy_pos_t pos) {
    storage_file_seek(token->file, pos, true);
}

GoofyTokenSymbol goofy_token_next(GoofyToken* token, string_t content) {
    // end of file, no more token to read
    if(goofy_token_end(token)) return GoofyTokenSymbolEof;

    if(token->state == GoofyTokenStateCommand) {
        GoofyTokenSymbol symbol;

        goofy_token_skip_spaces(token);
        string_reset(content);
        while(isalnum((uint8_t)goofy_token_current(token))) {
            string_push_back(content, goofy_token_current(token));
        }

        if(string_cmp_str(content, "REM") == 0) {
            token->state = GoofyTokenStateString;
            symbol = GoofyTokenSymbolCmdRem;
        } else if(string_cmp_str(content, "STRING") == 0) {
            token->state = GoofyTokenStateString;
            symbol = GoofyTokenSymbolCmdString;
        } else if(string_cmp_str(content, "STRINGLN") == 0) {
            token->state = GoofyTokenStateString;
            symbol = GoofyTokenSymbolCmdStringLn;
        } else if(string_cmp_str(content, "DELAY") == 0) {
            token->state = GoofyTokenStateExpression;
            symbol = GoofyTokenSymbolCmdDelay;
        } else if(string_cmp_str(content, "VAR") == 0) {
            token->state = GoofyTokenStateExpression;
            symbol = GoofyTokenSymbolCmdVar;
        } else if(string_cmp_str(content, "HOLD") == 0) {
            token->state = GoofyTokenStateKeys;
            symbol = GoofyTokenSymbolCmdHold;
        } else if(string_cmp_str(content, "RELEASE") == 0) {
            token->state = GoofyTokenStateKeys;
            symbol = GoofyTokenSymbolCmdRelease;
        } else if(string_cmp_str(content, "IF") == 0) {
            token->state = GoofyTokenStateExpression;
            symbol = GoofyTokenSymbolCmdIf;
        } else if(string_cmp_str(content, "END_IF") == 0) {
            token->state = GoofyTokenStateCommand;
            symbol = GoofyTokenSymbolCmdEndIf;
        } else if(string_cmp_str(content, "ELSE") == 0) {
            token->state = GoofyTokenStateCommand;
            symbol = GoofyTokenSymbolCmdElse;
        } else if(string_cmp_str(content, "ELSE_IF") == 0) { // should be 'else if'
            token->state = GoofyTokenStateCommand;
            symbol = GoofyTokenSymbolCmdElseIf;
        } else if(string_cmp_str(content, "WHILE") == 0) {
            token->state = GoofyTokenStateExpression;
            symbol = GoofyTokenSymbolCmdWhile;
        } else if(string_cmp_str(content, "END_WHILE") == 0) {
            token->state = GoofyTokenStateCommand;
            symbol = GoofyTokenSymbolCmdEndWhile;
        } else if(string_cmp_str(content, "FUNCTION") == 0) {
            token->state = GoofyTokenStateExpression;
            symbol = GoofyTokenSymbolCmdFunction;
        } else if(string_cmp_str(content, "END_FUNCTION") == 0) {
            token->state = GoofyTokenStateCommand;
            symbol = GoofyTokenSymbolCmdEndFunction;
        } else if(string_cmp_str(content, "RETURN") == 0) {
            token->state = GoofyTokenStateExpression; // fuck it... return always with a value.
            symbol = GoofyTokenSymbolCmdReturn;
        } else {
            GoofyTokenKey key = goofy_token_key(content);
            if(key != GoofyTokenKeyNone) {
                token->state = GoofyTokenStateKeys;
                symbol = GoofyTokenSymbolCmdKey & (key << 16);
            } else {
                symbol = GoofyTokenSymbolError;
            }
        }

        goofy_token_skip_spaces(token);
        if(goofy_token_current(token) == '\n') {
            goofy_token_advance(token);
            token->line_cnt++;
            token->state = GoofyTokenStateCommand;
        }
        return symbol;
    } else if(token->state == GoofyTokenStateString) {
        while(!goofy_token_end(token) && (goofy_token_current(token) != '\n')) {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
        }
        goofy_token_advance(token);
        token->line_cnt++;
        token->state = GoofyTokenStateCommand;
        return GoofyTokenSymbolString;
    } else if(token->state == GoofyTokenStateKeys) {
        GoofyTokenSymbol symbol;
        while(!goofy_token_end(token) && (isalnum((uint8_t)goofy_token_current(token)) ||
                                          (goofy_token_current(token) == '_'))) {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
        }
        goofy_token_skip_spaces(token);

        GoofyTokenKey key = goofy_token_key(content);
        if(key != GoofyTokenKeyNone) {
            symbol = GoofyTokenSymbolKey & (key << 16);
        } else if((string_size(content) == 1) && isalnum((uint8_t)string_get_char(content, 0))) {
            symbol = GoofyTokenSymbolKey & (GoofyTokenKeyChar << 16);
        } else {
            symbol = GoofyTokenSymbolError;
        }

        if((goofy_token_current(token) == '+') || (goofy_token_current(token) == '-')) {
            goofy_token_advance(token);
            goofy_token_skip_spaces(token);
        } else if(goofy_token_current(token) == '\n') {
            goofy_token_advance(token);
            token->line_cnt++;
            token->state = GoofyTokenStateCommand;
        }
        return symbol;
    } else if(token->state == GoofyTokenStateExpression) {
        GoofyTokenSymbol symbol;

        if(isalpha((uint8_t)goofy_token_current(token))) {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            while(!goofy_token_end(token) && isalpha((uint8_t)goofy_token_current(token))) {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
            }
            if(string_cmp_str(content, "THEN") == 0) {
                token->state = GoofyTokenStateCommand;
                symbol = GoofyTokenSymbolThen;
            } else if(string_cmp_str(content, "TRUE") == 0) {
                symbol = GoofyTokenSymbolTrue;
            } else if(string_cmp_str(content, "FALSE") == 0) {
                symbol = GoofyTokenSymbolFalse;
            } else {
                symbol = GoofyTokenSymbolName;
            }
        } else if(goofy_token_current(token) == '$') {
            goofy_token_advance(token);
            while(!goofy_token_end(token) && (isalnum((uint8_t)goofy_token_current(token)) ||
                                              (goofy_token_current(token) == '_'))) {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
            }
            symbol = GoofyTokenSymbolName;
        } else if(isdigit((uint8_t)goofy_token_current(token))) {
            string_push_back(content, goofy_token_current(token));
            while(!goofy_token_end(token) && isdigit((uint8_t)goofy_token_current(token))) {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
            }
            symbol = GoofyTokenSymbolNumber;
        } else if(goofy_token_current(token) == '=') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            if(goofy_token_current(token) == '=') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolEqu;
            } else {
                symbol = GoofyTokenSymbolAssign;
            }
        } else if(goofy_token_current(token) == '&') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            if(goofy_token_current(token) == '&') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolAnd;
            } else {
                symbol = GoofyTokenSymbolBitAnd;
            }
        } else if(goofy_token_current(token) == '|') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            if(goofy_token_current(token) == '|') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolOr;
            } else {
                symbol = GoofyTokenSymbolBitOr;
            }
        } else if(goofy_token_current(token) == '<') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            if(goofy_token_current(token) == '<') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolBitLeft;
            } else if(goofy_token_current(token) == '=') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolLeq;
            } else {
                symbol = GoofyTokenSymbolLt;
            }
        } else if(goofy_token_current(token) == '>') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            if(goofy_token_current(token) == '>') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolBitRight;
            } else if(goofy_token_current(token) == '=') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolGeq;
            } else {
                symbol = GoofyTokenSymbolGt;
            }
        } else if(goofy_token_current(token) == '!') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            if(goofy_token_current(token) == '=') {
                string_push_back(content, goofy_token_current(token));
                goofy_token_advance(token);
                symbol = GoofyTokenSymbolNeq;
            } else {
                symbol = GoofyTokenSymbolNot;
            }
        } else if(goofy_token_current(token) == '+') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolAdd;
        } else if(goofy_token_current(token) == '-') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolSub;
        } else if(goofy_token_current(token) == '*') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolMul;
        } else if(goofy_token_current(token) == '/') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolDiv;
        } else if(goofy_token_current(token) == '%') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolMod;
        } else if(goofy_token_current(token) == '^') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolExp;
        } else if(goofy_token_current(token) == '+') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolAdd;
        } else if(goofy_token_current(token) == '~') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolBitNot;
        } else if(goofy_token_current(token) == '(') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolOpenPar;
        } else if(goofy_token_current(token) == ')') {
            string_push_back(content, goofy_token_current(token));
            goofy_token_advance(token);
            symbol = GoofyTokenSymbolClosePar;
        } else {
            return GoofyTokenSymbolError;
        }

        goofy_token_skip_spaces(token);
        if(goofy_token_current(token) == '\n') {
            goofy_token_advance(token);
            token->line_cnt++;
            token->state = GoofyTokenStateCommand;
        }
        return symbol;
    } else {
        return GoofyTokenSymbolError;
    }
}