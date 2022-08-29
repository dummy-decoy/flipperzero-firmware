#include "goofy_lexer.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

typedef enum {
    GoofyLexerStateCommand,
    GoofyLexerStateString,
    GoofyLexerStateKeys,
    GoofyLexerStateExpression,
    GoofyLexerStateEol,
} GoofyLexerState;

struct GoofyLexer {
    File* file;
    char current;
    GoofyLexerState state;
    uint32_t line_cnt;
    goofy_pos_t pos;
};


static inline void goofy_lexer_update_pos(GoofyLexer* lexer) {
    lexer->pos = (goofy_pos_t){
        .file_pos = storage_file_tell(lexer->file), 
        .line = lexer->line_cnt, 
        .current = lexer->current, 
        .state = lexer->state
    };
}
static inline void goofy_lexer_advance(GoofyLexer* lexer) {
    char next;
    if(storage_file_read(lexer->file, &next, 1) == 1) {
        lexer->current = next;
    }
}
static inline char goofy_lexer_current(GoofyLexer* lexer) {
    return lexer->current;
}
static inline bool goofy_lexer_end(GoofyLexer* lexer) {
    return storage_file_eof(lexer->file);
}
static inline void goofy_lexer_skip_spaces(GoofyLexer* lexer) {
    while(!goofy_lexer_end(lexer) && isspace((uint8_t)goofy_lexer_current(lexer)) &&
          (goofy_lexer_current(lexer) != '\n'))
        goofy_lexer_advance(lexer);
}
static GoofyLexerKey goofy_lexer_key(string_t content) {
    if((string_cmpi_str(content, "up") == 0) || string_cmpi_str(content, "uparrow") == 0) {
        return GoofyLexerKeyUp;
    } else if(
        (string_cmpi_str(content, "down") == 0) || (string_cmpi_str(content, "downarrow") == 0)) {
        return GoofyLexerKeyDown;
    } else if(
        (string_cmpi_str(content, "left") == 0) || (string_cmpi_str(content, "leftarrow") == 0)) {
        return GoofyLexerKeyLeft;
    } else if(
        (string_cmpi_str(content, "right") == 0) ||
        (string_cmpi_str(content, "rightarrow") == 0)) {
        return GoofyLexerKeyRight;
    } else if(string_cmpi_str(content, "pageup") == 0) {
        return GoofyLexerKeyPageUp;
    } else if(string_cmpi_str(content, "pagedown") == 0) {
        return GoofyLexerKeyPageDown;
    } else if(string_cmpi_str(content, "home") == 0) {
        return GoofyLexerKeyHome;
    } else if(string_cmpi_str(content, "end") == 0) {
        return GoofyLexerKeyEnd;
    } else if((string_cmpi_str(content, "insert") == 0) || (string_cmpi_str(content, "ins") == 0)) {
        return GoofyLexerKeyInsert;
    } else if((string_cmpi_str(content, "delete") == 0) || (string_cmpi_str(content, "del") == 0)) {
        return GoofyLexerKeyDelete;
    } else if(
        (string_cmpi_str(content, "backspace") == 0) || (string_cmpi_str(content, "back") == 0)) {
        return GoofyLexerKeyBackspace;
    } else if(string_cmpi_str(content, "tab") == 0) {
        return GoofyLexerKeyTab;
    } else if(string_cmpi_str(content, "space") == 0) {
        return GoofyLexerKeySpace;
    } else if(string_cmpi_str(content, "enter") == 0) {
        return GoofyLexerKeyEnter;
    } else if((string_cmpi_str(content, "escape") == 0) || (string_cmpi_str(content, "esc") == 0)) {
        return GoofyLexerKeyEscape;
    } else if((string_cmpi_str(content, "pause") == 0) || (string_cmpi_str(content, "break") == 0)) {
        return GoofyLexerKeyBreak;
    } else if(string_cmpi_str(content, "printscreen") == 0) {
        return GoofyLexerKeyPrintscreen;
    } else if((string_cmpi_str(content, "menu") == 0) || (string_cmpi_str(content, "app") == 0)) {
        return GoofyLexerKeyApp;
    } else if(string_cmpi_str(content, "f1") == 0) {
        return GoofyLexerKeyF1;
    } else if(string_cmpi_str(content, "f2") == 0) {
        return GoofyLexerKeyF2;
    } else if(string_cmpi_str(content, "f3") == 0) {
        return GoofyLexerKeyF3;
    } else if(string_cmpi_str(content, "f4") == 0) {
        return GoofyLexerKeyF4;
    } else if(string_cmpi_str(content, "f5") == 0) {
        return GoofyLexerKeyF5;
    } else if(string_cmpi_str(content, "f6") == 0) {
        return GoofyLexerKeyF6;
    } else if(string_cmpi_str(content, "f7") == 0) {
        return GoofyLexerKeyF7;
    } else if(string_cmpi_str(content, "f8") == 0) {
        return GoofyLexerKeyF8;
    } else if(string_cmpi_str(content, "f9") == 0) {
        return GoofyLexerKeyF9;
    } else if(string_cmpi_str(content, "f10") == 0) {
        return GoofyLexerKeyF10;
    } else if(string_cmpi_str(content, "f11") == 0) {
        return GoofyLexerKeyF11;
    } else if(string_cmpi_str(content, "f12") == 0) {
        return GoofyLexerKeyF12;
    } else if((string_cmpi_str(content, "numpad0") == 0) || (string_cmpi_str(content, "np0") == 0)) {
        return GoofyLexerKeyNumpad0;
    } else if((string_cmpi_str(content, "numpad1") == 0) || (string_cmpi_str(content, "np1") == 0)) {
        return GoofyLexerKeyNumpad1;
    } else if((string_cmpi_str(content, "numpad2") == 0) || (string_cmpi_str(content, "np2") == 0)) {
        return GoofyLexerKeyNumpad2;
    } else if((string_cmpi_str(content, "numpad3") == 0) || (string_cmpi_str(content, "np3") == 0)) {
        return GoofyLexerKeyNumpad3;
    } else if((string_cmpi_str(content, "numpad4") == 0) || (string_cmpi_str(content, "np4") == 0)) {
        return GoofyLexerKeyNumpad4;
    } else if((string_cmpi_str(content, "numpad5") == 0) || (string_cmpi_str(content, "np5") == 0)) {
        return GoofyLexerKeyNumpad5;
    } else if((string_cmpi_str(content, "numpad6") == 0) || (string_cmpi_str(content, "np6") == 0)) {
        return GoofyLexerKeyNumpad6;
    } else if((string_cmpi_str(content, "numpad7") == 0) || (string_cmpi_str(content, "np7") == 0)) {
        return GoofyLexerKeyNumpad7;
    } else if((string_cmpi_str(content, "numpad8") == 0) || (string_cmpi_str(content, "np8") == 0)) {
        return GoofyLexerKeyNumpad8;
    } else if((string_cmpi_str(content, "numpad9") == 0) || (string_cmpi_str(content, "np9") == 0)) {
        return GoofyLexerKeyNumpad9;
    } else if(
        (string_cmpi_str(content, "control") == 0) || (string_cmpi_str(content, "ctrl") == 0) ||
        (string_cmpi_str(content, "lctrl") == 0)) {
        return GoofyLexerKeyLeftCtrl;
    } else if(string_cmpi_str(content, "rctrl") == 0) {
        return GoofyLexerKeyRightCtrl;
    } else if((string_cmpi_str(content, "shift") == 0) || (string_cmpi_str(content, "lshift") == 0)) {
        return GoofyLexerKeyLeftShift;
    } else if(string_cmpi_str(content, "rshift") == 0) {
        return GoofyLexerKeyRightShift;
    } else if(
        (string_cmpi_str(content, "alt") == 0) || (string_cmpi_str(content, "lalt") == 0) ||
        (string_cmpi_str(content, "option") == 0)) {
        return GoofyLexerKeyLeftAlt;
    } else if(string_cmpi_str(content, "ralt") == 0) {
        return GoofyLexerKeyRightAlt;
    } else if(
        (string_cmpi_str(content, "windows") == 0) || (string_cmpi_str(content, "gui") == 0) ||
        (string_cmpi_str(content, "lgui") == 0) || (string_cmpi_str(content, "command") == 0)) {
        return GoofyLexerKeyLeftGui;
    } else if(string_cmpi_str(content, "rgui") == 0) {
        return GoofyLexerKeyRightGui;
    } else if(string_cmpi_str(content, "capslock") == 0) {
        return GoofyLexerKeyCapsLock;
    } else if(string_cmpi_str(content, "numlock") == 0) {
        return GoofyLexerKeyNumLock;
    } else if(
        (string_cmpi_str(content, "scrolllock") == 0) ||
        (string_cmpi_str(content, "scrollock") == 0)) {
        return GoofyLexerKeyScrollLock;
    } else {
        return GoofyLexerKeyNone;
    };
}

GoofyLexer* goofy_lexer_alloc(File* file) {
    GoofyLexer* lexer = malloc(sizeof(GoofyLexer));

    lexer->file = file;
    lexer->current = '\0';
    lexer->state = GoofyLexerStateCommand;
    lexer->line_cnt = 1;

    goofy_lexer_update_pos(lexer);
    goofy_lexer_advance(lexer);
    return lexer;
}

void goofy_lexer_free(GoofyLexer* lexer) {
    storage_file_close(lexer->file);
    storage_file_free(lexer->file);
    free(lexer);
}

uint32_t goofy_lexer_line(GoofyLexer* lexer) {
    return lexer->line_cnt;
}

goofy_pos_t goofy_lexer_pos(GoofyLexer* lexer) {
    return lexer->pos;
}

void goofy_lexer_jmp(GoofyLexer* lexer, goofy_pos_t pos) {
    storage_file_seek(lexer->file, pos.file_pos, true);
    lexer->line_cnt = pos.line;
    lexer->current = pos.current;
    lexer->state = pos.state;

    lexer->pos = pos;
}

GoofyLexerSymbol goofy_lexer_next(GoofyLexer* lexer, string_t content) {
    string_reset(content);

    // end of file, no more lexer to read
    if(goofy_lexer_end(lexer)) 
        return GoofyLexerSymbolEof;

    if(lexer->state == GoofyLexerStateEol) {
        if(goofy_lexer_current(lexer) == '\n') {
            goofy_lexer_advance(lexer);
            lexer->line_cnt++;
            lexer->state = GoofyLexerStateCommand;
            goofy_lexer_update_pos(lexer);
            return GoofyLexerSymbolEol;
        } else {
            return GoofyLexerSymbolError;
        }
    } else if(lexer->state == GoofyLexerStateCommand) {
        GoofyLexerSymbol symbol;

        // ignore leading spaces
        goofy_lexer_skip_spaces(lexer);
        // skip blank lines
        while(!goofy_lexer_end(lexer) && goofy_lexer_current(lexer) == '\n') {
            goofy_lexer_advance(lexer);
            lexer->line_cnt++;
            goofy_lexer_skip_spaces(lexer);
        }
        
        if (goofy_lexer_end(lexer)) {
            return GoofyLexerSymbolEof;
        } else if(goofy_lexer_current(lexer) == '#') {
            // comment
            goofy_lexer_advance(lexer);
            while(goofy_lexer_current(lexer) != '\n') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
            }
            lexer->state = GoofyLexerStateEol;
            symbol = GoofyLexerSymbolComment;
        } else if(goofy_lexer_current(lexer) == '$') {
            // assignement
            goofy_lexer_advance(lexer);
            while(!goofy_lexer_end(lexer) && (isalnum((uint8_t)goofy_lexer_current(lexer)) ||
                                              (goofy_lexer_current(lexer) == '_'))) {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
            }
            lexer->state = GoofyLexerStateExpression;
            symbol = GoofyLexerSymbolVariable;
        } else {
            // command
            while(isalnum((uint8_t)goofy_lexer_current(lexer)) || (goofy_lexer_current(lexer) == '_')) {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
            }

            if(string_cmpi_str(content, "string") == 0) {
                lexer->state = GoofyLexerStateString;
                symbol = GoofyLexerSymbolCmdString;
            } else if(string_cmpi_str(content, "stringln") == 0) {
                lexer->state = GoofyLexerStateString;
                symbol = GoofyLexerSymbolCmdStringLn;
            } else if(string_cmpi_str(content, "delay") == 0) {
                lexer->state = GoofyLexerStateExpression;
                symbol = GoofyLexerSymbolCmdDelay;
            } else if(string_cmpi_str(content, "var") == 0) {
                lexer->state = GoofyLexerStateExpression;
                symbol = GoofyLexerSymbolCmdVar;
            } else if(string_cmpi_str(content, "hold") == 0) {
                lexer->state = GoofyLexerStateKeys;
                symbol = GoofyLexerSymbolCmdHold;
            } else if(string_cmpi_str(content, "release") == 0) {
                lexer->state = GoofyLexerStateKeys;
                symbol = GoofyLexerSymbolCmdRelease;
            } else if(string_cmpi_str(content, "if") == 0) {
                lexer->state = GoofyLexerStateExpression;
                symbol = GoofyLexerSymbolCmdIf;
            } else if(string_cmpi_str(content, "end_if") == 0) {
                lexer->state = GoofyLexerStateEol;
                symbol = GoofyLexerSymbolCmdEndIf;
            } else if(string_cmpi_str(content, "else") == 0) {
                lexer->state = GoofyLexerStateEol;
                symbol = GoofyLexerSymbolCmdElse;
            } else if(string_cmpi_str(content, "else_if") == 0) {
                lexer->state = GoofyLexerStateExpression;
                symbol = GoofyLexerSymbolCmdElseIf;
            } else if(string_cmpi_str(content, "while") == 0) {
                lexer->state = GoofyLexerStateExpression;
                symbol = GoofyLexerSymbolCmdWhile;
            } else if(string_cmpi_str(content, "end_while") == 0) {
                lexer->state = GoofyLexerStateEol;
                symbol = GoofyLexerSymbolCmdEndWhile;
            } else if(string_cmpi_str(content, "function") == 0) {
                lexer->state = GoofyLexerStateExpression;
                symbol = GoofyLexerSymbolCmdFunction;
            } else if(string_cmpi_str(content, "end_function") == 0) {
                lexer->state = GoofyLexerStateEol;
                symbol = GoofyLexerSymbolCmdEndFunction;
            } else if(string_cmpi_str(content, "return") == 0) {
                lexer->state = GoofyLexerStateExpression; // fuck it... return always with a value.
                symbol = GoofyLexerSymbolCmdReturn;
            } else {
                GoofyLexerKey key = goofy_lexer_key(content);
                if(key != GoofyLexerKeyNone) {
                    lexer->state = GoofyLexerStateKeys;
                    symbol = GoofyLexerSymbolCmdKey & (key << 16);
                } else {
                    symbol = GoofyLexerSymbolError;
                }
            }
        }

        goofy_lexer_skip_spaces(lexer);
        goofy_lexer_update_pos(lexer);
        return symbol;
    } else if(lexer->state == GoofyLexerStateString) {
        while(!goofy_lexer_end(lexer) && (goofy_lexer_current(lexer) != '\n')) {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
        }
        lexer->state = GoofyLexerStateEol;
        goofy_lexer_update_pos(lexer);
        return GoofyLexerSymbolString;
    } else if(lexer->state == GoofyLexerStateKeys) {
        // keys
        GoofyLexerSymbol symbol;
        while(!goofy_lexer_end(lexer) && (isalnum((uint8_t)goofy_lexer_current(lexer)) ||
                                          (goofy_lexer_current(lexer) == '_'))) {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
        }
        goofy_lexer_skip_spaces(lexer);

        GoofyLexerKey key = goofy_lexer_key(content);
        if(key != GoofyLexerKeyNone) {
            symbol = GoofyLexerSymbolKey & (key << 16);
        } else if((string_size(content) == 1) && isalnum((uint8_t)string_get_char(content, 0))) {
            symbol = GoofyLexerSymbolKey & (GoofyLexerKeyChar << 16);
        } else {
            symbol = GoofyLexerSymbolError;
        }

        if(goofy_lexer_current(lexer) == '\n') {
            lexer->state = GoofyLexerStateEol;
        }
        goofy_lexer_update_pos(lexer);
        return symbol;
    } else if(lexer->state == GoofyLexerStateExpression) {
        // expression
        GoofyLexerSymbol symbol;

        if(isalpha((uint8_t)goofy_lexer_current(lexer))) {
            // function name or true/false
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            while(!goofy_lexer_end(lexer) && isalpha((uint8_t)goofy_lexer_current(lexer))) {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
            }
            if(string_cmpi_str(content, "true") == 0) {
                symbol = GoofyLexerSymbolTrue;
            } else if(string_cmpi_str(content, "false") == 0) {
                symbol = GoofyLexerSymbolFalse;
            } else {
                symbol = GoofyLexerSymbolName;
            }
        } else if(goofy_lexer_current(lexer) == '$') {
            // variable name
            goofy_lexer_advance(lexer);
            while(!goofy_lexer_end(lexer) && (isalnum((uint8_t)goofy_lexer_current(lexer)) ||
                                              (goofy_lexer_current(lexer) == '_'))) {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
            }
            symbol = GoofyLexerSymbolVariable;
        } else if(isdigit((uint8_t)goofy_lexer_current(lexer))) {
            // number
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            while(!goofy_lexer_end(lexer) && isdigit((uint8_t)goofy_lexer_current(lexer))) {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
            }
            symbol = GoofyLexerSymbolNumber;
        } else if(goofy_lexer_current(lexer) == '=') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            if(goofy_lexer_current(lexer) == '=') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolEqu;
            } else {
                symbol = GoofyLexerSymbolAssign;
            }
        } else if(goofy_lexer_current(lexer) == '&') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            if(goofy_lexer_current(lexer) == '&') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolAnd;
            } else {
                symbol = GoofyLexerSymbolBitAnd;
            }
        } else if(goofy_lexer_current(lexer) == '|') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            if(goofy_lexer_current(lexer) == '|') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolOr;
            } else {
                symbol = GoofyLexerSymbolBitOr;
            }
        } else if(goofy_lexer_current(lexer) == '<') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            if(goofy_lexer_current(lexer) == '<') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolBitLeft;
            } else if(goofy_lexer_current(lexer) == '=') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolLeq;
            } else {
                symbol = GoofyLexerSymbolLt;
            }
        } else if(goofy_lexer_current(lexer) == '>') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            if(goofy_lexer_current(lexer) == '>') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolBitRight;
            } else if(goofy_lexer_current(lexer) == '=') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolGeq;
            } else {
                symbol = GoofyLexerSymbolGt;
            }
        } else if(goofy_lexer_current(lexer) == '!') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            if(goofy_lexer_current(lexer) == '=') {
                string_push_back(content, goofy_lexer_current(lexer));
                goofy_lexer_advance(lexer);
                symbol = GoofyLexerSymbolNeq;
            } else {
                symbol = GoofyLexerSymbolNot;
            }
        } else if(goofy_lexer_current(lexer) == '+') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolAdd;
        } else if(goofy_lexer_current(lexer) == '-') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolSub;
        } else if(goofy_lexer_current(lexer) == '*') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolMul;
        } else if(goofy_lexer_current(lexer) == '/') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolDiv;
        } else if(goofy_lexer_current(lexer) == '%') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolMod;
        } else if(goofy_lexer_current(lexer) == '^') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolExp;
        } else if(goofy_lexer_current(lexer) == '+') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolAdd;
        } else if(goofy_lexer_current(lexer) == '~') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolBitNot;
        } else if(goofy_lexer_current(lexer) == '(') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolOpenPar;
        } else if(goofy_lexer_current(lexer) == ')') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolClosePar;
        } else if(goofy_lexer_current(lexer) == ',') {
            string_push_back(content, goofy_lexer_current(lexer));
            goofy_lexer_advance(lexer);
            symbol = GoofyLexerSymbolComma;
        } else {
            return GoofyLexerSymbolError;
        }

        goofy_lexer_skip_spaces(lexer);
        if(goofy_lexer_current(lexer) == '\n') {
            lexer->state = GoofyLexerStateEol;
        }
        goofy_lexer_update_pos(lexer);
        return symbol;
    } else {
        return GoofyLexerSymbolError;
    }
}