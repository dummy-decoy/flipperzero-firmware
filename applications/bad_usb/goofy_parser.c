#include "goofy_parser.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <m-string.h>
#include <m-array.h>

#include "goofy_lexer.h"
#include "goofy_actions.h"

typedef int32_t goofy_value_t;

ARRAY_DEF(strarray, string_t);
ARRAY_DEF(intarray, goofy_value_t);

typedef struct GoofyParserVars {
    string_t name;
    goofy_value_t value;
    struct GoofyParserVars* next;
} GoofyParserVars;

typedef struct GoofyParserFuncs {
    string_t name;
    goofy_pos_t pos;
    strarray_t parameters;
    intarray_t values;
    struct GoofyParserFuncs* next;
} GoofyParserFuncs;

typedef struct GoofyParserScope {
    GoofyParserVars* vars;
    GoofyParserFuncs* funcs;
    struct GoofyParserScope* next;
} GoofyParserScope;

struct GoofyParser {
    GoofyLexer* lexer;
    struct {
        goofy_pos_t pos;
        GoofyLexerSymbol symbol;
        string_t content;
    } current;
    GoofyParserVars* vars;
    GoofyParserFuncs* funcs;
    GoofyParserScope* scope;
};

// raise base to the power of exp, for integer
int ipow(int base, int exp) {
    int result = 1;
    for(;;) {
        if(exp & 1) result *= base;
        exp >>= 1;
        if(!exp) break;
        base *= base;
    }

    return result;
}

static void goofy_parser_advance(GoofyParser* parser) {
    parser->current.pos = goofy_lexer_pos(parser->lexer);
    parser->current.symbol = goofy_lexer_next(parser->lexer, parser->current.content);
}
static goofy_pos_t goofy_parser_pos(GoofyParser* parser) {
    return parser->current.pos;
}
static void goofy_parser_jmp(GoofyParser* parser, goofy_pos_t pos) {
    goofy_lexer_jmp(parser->lexer, pos);
    goofy_parser_advance(parser);
}
static GoofyLexerSymbol goofy_parser_current_symbol(GoofyParser* parser) {
    return parser->current.symbol;
}

static GoofyParserStatus goofy_parser_error(GoofyParser* parser, char* message) {
    //printf("ERROR: line %l: %s\n", goofy_lexer_line(parser->lexer), message);
    UNUSED(parser);
    UNUSED(message);
    return GoofyParserStatusError;
}

static GoofyParserVars* goofy_parser_var_alloc(string_t name, goofy_value_t value) {
    GoofyParserVars* vars = malloc(sizeof(GoofyParserVars));
    string_init_set(vars->name, name);
    vars->value = value;
    vars->next = NULL;
    return vars;
}
static void goofy_parser_var_free(GoofyParserVars* stack) {
    string_clear(stack->name);
    free(stack);
}
static GoofyParserVars*
    goofy_parser_var_push(GoofyParser* parser, string_t name, goofy_value_t value) {
    GoofyParserVars* vars = goofy_parser_var_alloc(name, value);
    vars->next = parser->vars;
    parser->vars = vars;
    return vars;
}
static void goofy_parser_var_pop(GoofyParser* parser) {
    GoofyParserVars* vars = parser->vars;
    if(vars) {
        parser->vars = vars->next;
        goofy_parser_var_free(vars);
    }
}
static GoofyParserVars* goofy_parser_var_ref(GoofyParser* parser, string_t name) {
    GoofyParserVars* vars = parser->vars;
    while(vars) {
        if(string_cmpi(vars->name, name) == 0) break;
        vars = vars->next;
    }
    return vars;
}
static goofy_value_t goofy_parser_var_get(GoofyParserVars* var) {
    return var->value;
}
static void goofy_parser_var_set(GoofyParserVars* var, goofy_value_t value) {
    var->value = value;
}

static GoofyParserFuncs* goofy_parser_func_alloc(string_t name, goofy_pos_t pos) {
    GoofyParserFuncs* funcs = malloc(sizeof(GoofyParserFuncs));
    string_init_set(funcs->name, name);
    funcs->pos = pos;
    funcs->next = NULL;
    strarray_init(funcs->parameters);
    intarray_init(funcs->values);
    return funcs;
}
static void goofy_parser_func_free(GoofyParserFuncs* func) {
    string_clear(func->name);
    strarray_clear(func->parameters);
    intarray_clear(func->values);
    free(func);
}
static void goofy_parser_func_add_param(GoofyParserFuncs* func, string_t name) {
    strarray_push_back(func->parameters, name);
}
static void goofy_parser_func_add_value(GoofyParserFuncs* func, goofy_value_t value) {
    intarray_push_back(func->values, value);
}
static size_t goofy_parser_func_param_count(GoofyParserFuncs* func) {
    return strarray_size(func->parameters);
}
static GoofyParserFuncs*
    goofy_parser_func_push(GoofyParser* parser, string_t name, goofy_pos_t pos) {
    GoofyParserFuncs* funcs = goofy_parser_func_alloc(name, pos);
    funcs->next = parser->funcs;
    parser->funcs = funcs;
    return funcs;
}
static void goofy_parser_func_pop(GoofyParser* parser) {
    GoofyParserFuncs* funcs = parser->funcs;
    if(funcs) {
        parser->funcs = funcs->next;
        goofy_parser_func_free(funcs);
    }
}
static GoofyParserFuncs* goofy_parser_func_ref(GoofyParser* parser, string_t name) {
    GoofyParserFuncs* funcs = parser->funcs;
    while(funcs) {
        if(string_cmpi(funcs->name, name) == 0) break;
        funcs = funcs->next;
    }
    return funcs;
}
static void goofy_parser_prepare_func_call(GoofyParser* parser, GoofyParserFuncs* func) {
    for(size_t index = 0; index < strarray_size(func->parameters); index++) {
        string_t* name = strarray_get(func->parameters, index);
        goofy_value_t* value = intarray_get(func->values, index);
        goofy_parser_var_push(parser, *name, *value);
    }
    intarray_reset(func->values);
}

static GoofyParserScope* goofy_parser_scope_alloc(GoofyParserVars* vars, GoofyParserFuncs* funcs) {
    GoofyParserScope* scope = malloc(sizeof(GoofyParserScope));
    scope->vars = vars;
    scope->funcs = funcs;
    scope->next = NULL;
    return scope;
}
static void goofy_parser_scope_free(GoofyParserScope* scope) {
    free(scope);
}
static void goofy_parser_begin_scope(GoofyParser* parser) {
    GoofyParserScope* scope = goofy_parser_scope_alloc(parser->vars, parser->funcs);
    scope->next = parser->scope;
    parser->scope = scope;
}
static void goofy_parser_end_scope(GoofyParser* parser) {
    GoofyParserScope* scope = parser->scope;
    if(scope) {
        while(parser->vars != scope->vars) goofy_parser_var_pop(parser);
        while(parser->funcs != scope->funcs) goofy_parser_func_pop(parser);
        parser->scope = scope->next;
        goofy_parser_scope_free(scope);
    }
}

/* skip production */

static GoofyParserStatus goofy_skip_statement(GoofyParser* parser);

static GoofyParserStatus goofy_skip_line(GoofyParser* parser) {
    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
        goofy_parser_advance(parser);
    }
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_skip_if(GoofyParser* parser) {
    GoofyParserStatus status;

    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
        goofy_parser_advance(parser);
    }
    goofy_parser_advance(parser);

    while((goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElseIf) &&
          (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElse) &&
          (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndIf)) {
        status = goofy_skip_statement(parser);
        if(status != GoofyParserStatusOK) return status;
    }
    while(goofy_parser_current_symbol(parser) == GoofyLexerSymbolCmdElseIf) {
        while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
            goofy_parser_advance(parser);
        }
        goofy_parser_advance(parser);

        while((goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElseIf) &&
              (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElse) &&
              (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndIf)) {
            status = goofy_skip_statement(parser);
            if(status != GoofyParserStatusOK) return status;
        }
    }
    if(goofy_parser_current_symbol(parser) == GoofyLexerSymbolCmdElse) {
        while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
            goofy_parser_advance(parser);
        }
        goofy_parser_advance(parser);

        while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndIf) {
            status = goofy_skip_statement(parser);
            if(status != GoofyParserStatusOK) return status;
        }
    }

    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
        goofy_parser_advance(parser);
    }
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_skip_while(GoofyParser* parser) {
    GoofyParserStatus status;

    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
        goofy_parser_advance(parser);
    }
    goofy_parser_advance(parser);

    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndWhile) {
        status = goofy_skip_statement(parser);
        if(status != GoofyParserStatusOK) return status;
    }

    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
        goofy_parser_advance(parser);
    }
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_skip_function_body(GoofyParser* parser) {
    GoofyParserStatus status;
    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndFunction) {
        status = goofy_skip_statement(parser);
        if(status != GoofyParserStatusOK) return status;
    }

    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
        goofy_parser_advance(parser);
    }
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_skip_function(GoofyParser* parser) {
    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol) {
        goofy_parser_advance(parser);
    }
    goofy_parser_advance(parser);

    return goofy_skip_function_body(parser);
}
static GoofyParserStatus goofy_skip_statement(GoofyParser* parser) {
    GoofyLexerSymbol symbol;
    GoofyParserStatus status;

    symbol = goofy_parser_current_symbol(parser);
    status = GoofyParserStatusOK;
    if(symbol == GoofyLexerSymbolComment) {
        // ignore comment
        goofy_parser_advance(parser);
    } else if(symbol == GoofyLexerSymbolVariable) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdString) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdStringLn) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdDelay) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdVar) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdHold) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdRelease) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdIf) {
        status = goofy_skip_if(parser);
    } else if(symbol == GoofyLexerSymbolCmdWhile) {
        status = goofy_skip_while(parser);
    } else if(symbol == GoofyLexerSymbolCmdFunction) {
        status = goofy_skip_function(parser);
    } else if(symbol == GoofyLexerSymbolCmdReturn) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolCmdKey) {
        status = goofy_skip_line(parser);
    } else if(symbol == GoofyLexerSymbolError) {
        status = goofy_parser_error(parser, "unexpected token");
    }

    if(status != GoofyParserStatusOK) return status;

    symbol = goofy_parser_current_symbol(parser);
    if(symbol != GoofyLexerSymbolEol) return goofy_parser_error(parser, "expected eol");
    goofy_parser_advance(parser);
    return GoofyParserStatusOK;
}

/* parse production */

static GoofyParserStatus goofy_parse_expression(GoofyParser* parser, goofy_value_t* value);
static GoofyParserStatus goofy_parse_function(GoofyParser* parser, goofy_value_t* value);
static GoofyParserStatus goofy_parse_statement(GoofyParser* parser, goofy_value_t* value);

static GoofyParserStatus goofy_parse_primary(GoofyParser* parser, goofy_value_t* value) {
    // primary ::= number | variable | name '(' [expr {',' expr}] ')' | '(' expr ')' | true | false
    GoofyParserStatus status;
    GoofyLexerSymbol symbol;

    symbol = goofy_parser_current_symbol(parser);
    if(symbol == GoofyLexerSymbolNumber) {
        // convert content to number
        *value = atoi(string_get_cstr(parser->current.content));
        goofy_parser_advance(parser);
    } else if(symbol == GoofyLexerSymbolOpenPar) {
        goofy_parser_advance(parser);

        status = goofy_parse_expression(parser, value);
        if(status != GoofyParserStatusOK) return status;

        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolClosePar)
            return goofy_parser_error(parser, "expected right parenthesis");
        goofy_parser_advance(parser);
    } else if(symbol == GoofyLexerSymbolTrue) {
        *value = 1;
        goofy_parser_advance(parser);
    } else if(symbol == GoofyLexerSymbolFalse) {
        *value = 0;
        goofy_parser_advance(parser);
    } else if(symbol == GoofyLexerSymbolVariable) {
        GoofyParserVars* var = goofy_parser_var_ref(parser, parser->current.content);
        if(var == NULL) return goofy_parser_error(parser, "undefined variable");
        *value = goofy_parser_var_get(var);
        goofy_parser_advance(parser);
    } else if(symbol == GoofyLexerSymbolName) {
        GoofyParserFuncs* func = goofy_parser_func_ref(parser, parser->current.content);
        if(func == NULL) return goofy_parser_error(parser, "undefined function");

        goofy_parser_advance(parser);
        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolOpenPar)
            return goofy_parser_error(parser, "expected left parenthesis for function call");
        goofy_parser_advance(parser);

        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolClosePar) {
            goofy_value_t parameter;
            size_t count = 0;

            goofy_parse_expression(parser, &parameter);
            goofy_parser_func_add_value(func, parameter);
            count++;

            while(goofy_parser_current_symbol(parser) == GoofyLexerSymbolComma) {
                goofy_parser_advance(parser);

                goofy_parse_expression(parser, &parameter);
                goofy_parser_func_add_value(func, parameter);
                count++;
            }

            if(goofy_parser_func_param_count(func) != count)
                return goofy_parser_error(parser, "parameter mismatch for function call");
        }

        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolClosePar)
            return goofy_parser_error(parser, "expected right parenthesis for function call");

        goofy_pos_t pos = goofy_lexer_pos(parser->lexer);
        goofy_parser_begin_scope(parser);
        goofy_parser_prepare_func_call(parser, func);
        goofy_parser_jmp(parser, func->pos);
        status = goofy_parse_function(parser, value);
        goofy_parser_jmp(parser, pos);
        goofy_parser_end_scope(parser);

        if(status != GoofyParserStatusOK) return status;
    } else {
        return goofy_parser_error(parser, "invalid token");
    }
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_parse_factor(GoofyParser* parser, goofy_value_t* value) {
    // factor ::= primary ['^' primary] | ('+'|'-'|'!'|'~') primary
    goofy_value_t right;
    GoofyParserStatus status;
    GoofyLexerSymbol operator;

    operator= goofy_parser_current_symbol(parser);
    if((operator== GoofyLexerSymbolAdd) || (operator== GoofyLexerSymbolSub) ||
       (operator== GoofyLexerSymbolNot) || (operator== GoofyLexerSymbolBitNot)) {
        goofy_parser_advance(parser);

        status = goofy_parse_primary(parser, &right);
        if(status != GoofyParserStatusOK) return status;

        if(operator== GoofyLexerSymbolAdd)
            *value = right;
        else if(operator== GoofyLexerSymbolSub)
            *value = -right;
        else if(operator== GoofyLexerSymbolNot)
            *value = !right;
        else if(operator== GoofyLexerSymbolBitNot)
            *value = ~(uint32_t)right;
    } else {
        status = goofy_parse_primary(parser, value);
        if(status != GoofyParserStatusOK) return status;

        operator= goofy_parser_current_symbol(parser);
        if(operator== GoofyLexerSymbolExp) {
            goofy_parser_advance(parser);

            status = goofy_parse_primary(parser, &right);
            if(status != GoofyParserStatusOK) return status;

            *value = ipow(*value, right);
        }
    }
    return status;
}
static GoofyParserStatus goofy_parse_term(GoofyParser* parser, goofy_value_t* value) {
    // term ::= factor {('*'|'/'|'%') factor}
    goofy_value_t right;
    GoofyParserStatus status;
    GoofyLexerSymbol operator;

    status = goofy_parse_factor(parser, value);
    if(status != GoofyParserStatusOK) return status;

    operator= goofy_parser_current_symbol(parser);
    while((operator== GoofyLexerSymbolMul) || (operator== GoofyLexerSymbolDiv) ||
          (operator== GoofyLexerSymbolMod)) {
        goofy_parser_advance(parser);

        status = goofy_parse_factor(parser, &right);
        if(status != GoofyParserStatusOK) return status;

        if(operator== GoofyLexerSymbolMul)
            *value = (*value) * right;
        else if(operator== GoofyLexerSymbolDiv) {
            if(right == 0)
                return goofy_parser_error(parser, "division by zero");
            else
                *value = (*value) / right;
        } else if(operator== GoofyLexerSymbolMod) {
            if(right == 0)
                return goofy_parser_error(parser, "modulo by zero");
            else
                *value = (*value) % right;
        }

        operator= goofy_parser_current_symbol(parser);
    }
    return status;
}
static GoofyParserStatus goofy_parse_bitwise_primary(GoofyParser* parser, goofy_value_t* value) {
    // bitwise_primary ::= term {('+'|'-') term}
    goofy_value_t right;
    GoofyParserStatus status;
    GoofyLexerSymbol operator;

    status = goofy_parse_term(parser, value);
    if(status != GoofyParserStatusOK) return status;

    operator= goofy_parser_current_symbol(parser);
    while((operator== GoofyLexerSymbolAdd) || (operator== GoofyLexerSymbolSub)) {
        goofy_parser_advance(parser);

        status = goofy_parse_term(parser, &right);
        if(status != GoofyParserStatusOK) return status;

        if(operator== GoofyLexerSymbolAdd)
            *value = (*value) + right;
        else if(operator== GoofyLexerSymbolSub)
            *value = (*value) - right;

        operator= goofy_parser_current_symbol(parser);
    }
    return status;
}
static GoofyParserStatus goofy_parse_bitwise_term(GoofyParser* parser, goofy_value_t* value) {
    // bitwise_term ::= bitwise_primary {('<<'|'>>') bitwise_primary}
    goofy_value_t right;
    GoofyParserStatus status;
    GoofyLexerSymbol operator;

    status = goofy_parse_bitwise_primary(parser, value);
    if(status != GoofyParserStatusOK) return status;

    operator= goofy_parser_current_symbol(parser);
    while((operator== GoofyLexerSymbolBitLeft) || (operator== GoofyLexerSymbolBitRight)) {
        goofy_parser_advance(parser);

        status = goofy_parse_bitwise_primary(parser, &right);
        if(status != GoofyParserStatusOK) return status;

        if(operator== GoofyLexerSymbolBitLeft)
            *value = ((*value) << right);
        else if(operator== GoofyLexerSymbolBitRight)
            *value = ((*value) >> right);

        operator= goofy_parser_current_symbol(parser);
    }
    return status;
}
static GoofyParserStatus goofy_parse_simple_expr(GoofyParser* parser, goofy_value_t* value) {
    // simple_exp ::= bitwise_term {('&'|'|') bitwise_term}
    goofy_value_t right;
    GoofyParserStatus status;
    GoofyLexerSymbol operator;

    status = goofy_parse_bitwise_term(parser, value);
    if(status != GoofyParserStatusOK) return status;

    operator= goofy_parser_current_symbol(parser);
    while((operator== GoofyLexerSymbolBitAnd) || (operator== GoofyLexerSymbolBitOr)) {
        goofy_parser_advance(parser);

        status = goofy_parse_bitwise_term(parser, &right);
        if(status != GoofyParserStatusOK) return status;

        if(operator== GoofyLexerSymbolBitAnd)
            *value = ((uint32_t)(*value) & (uint32_t)right);
        else if(operator== GoofyLexerSymbolBitOr)
            *value = ((uint32_t)(*value) | (uint32_t)right);

        operator= goofy_parser_current_symbol(parser);
    }
    return status;
}
static GoofyParserStatus goofy_parse_relation(GoofyParser* parser, goofy_value_t* value) {
    // relation ::= simple_exp [('=='|'!='|'<'|'<='|'>'|'>=') simple_exp]
    goofy_value_t right;
    GoofyParserStatus status;
    GoofyLexerSymbol operator;

    status = goofy_parse_simple_expr(parser, value);
    if(status != GoofyParserStatusOK) return status;

    operator= goofy_parser_current_symbol(parser);
    if((operator== GoofyLexerSymbolEqu) || (operator== GoofyLexerSymbolNeq) ||
       (operator== GoofyLexerSymbolLt) || (operator== GoofyLexerSymbolGt) ||
       (operator== GoofyLexerSymbolLeq) || (operator== GoofyLexerSymbolGeq)) {
        goofy_parser_advance(parser);

        status = goofy_parse_simple_expr(parser, &right);
        if(status != GoofyParserStatusOK) return status;

        if(operator== GoofyLexerSymbolEqu)
            *value = ((*value) == right);
        else if(operator== GoofyLexerSymbolNeq)
            *value = ((*value) != right);
        else if(operator== GoofyLexerSymbolLt)
            *value = ((*value) < right);
        else if(operator== GoofyLexerSymbolGt)
            *value = ((*value) > right);
        else if(operator== GoofyLexerSymbolLeq)
            *value = ((*value) <= right);
        else if(operator== GoofyLexerSymbolGeq)
            *value = ((*value) >= right);
    }
    return status;
}
static GoofyParserStatus goofy_parse_expression(GoofyParser* parser, goofy_value_t* value) {
    // expr ::= relation {'&&' relation} | relation {'||' relation}
    goofy_value_t right;
    GoofyParserStatus status;

    status = goofy_parse_relation(parser, value);
    if(status != GoofyParserStatusOK) return status;

    if(goofy_parser_current_symbol(parser) == GoofyLexerSymbolOr) {
        while(goofy_parser_current_symbol(parser) == GoofyLexerSymbolOr) {
            goofy_parser_advance(parser);
            status = goofy_parse_relation(parser, &right);
            if(status != GoofyParserStatusOK) return status;
            *value = (*value) || right;
        }
    } else if(goofy_parser_current_symbol(parser) == GoofyLexerSymbolAnd) {
        while(goofy_parser_current_symbol(parser) == GoofyLexerSymbolAnd) {
            goofy_parser_advance(parser);
            status = goofy_parse_relation(parser, &right);
            if(status != GoofyParserStatusOK) return status;
            *value = (*value) && right;
        }
    }
    return status;
}

static GoofyParserStatus goofy_parse_assignement(GoofyParser* parser) {
    // assignment ::= variable '=' expr
    goofy_value_t value;
    GoofyParserVars* var = NULL;

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolVariable)
        return goofy_parser_error(parser, "expected variable");
    var = goofy_parser_var_ref(parser, parser->current.content);
    if(var == NULL) return goofy_parser_error(parser, "undefined variable");

    goofy_parser_advance(parser);
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolAssign)
        return goofy_parser_error(parser, "expected =");

    goofy_parser_advance(parser);
    GoofyParserStatus status = goofy_parse_expression(parser, &value);
    if(status != GoofyParserStatusOK) return status;

    goofy_parser_var_set(var, value);

    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_parse_cmd_var(GoofyParser* parser) {
    // cmd_var ::= 'var' assignment eol
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdVar)
        return goofy_parser_error(parser, "expected keyword var");
    goofy_parser_advance(parser);

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolVariable)
        return goofy_parser_error(parser, "expected variable");
    goofy_parser_var_push(parser, parser->current.content, 0);

    // do not skip the current symbol, it is used while parsing assignment
    return goofy_parse_assignement(parser);
}
static GoofyParserStatus goofy_parse_if(GoofyParser* parser, goofy_value_t* value) {
    // cmd_if ::= 'if' expr eol {statement} {'else_if' expr eol {statement}} ['else' eol {statement}] 'end_if' eol
    GoofyParserStatus status, result;
    goofy_value_t test;
    bool skip = false;

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdIf)
        return goofy_parser_error(parser, "expected keyword if");
    goofy_parser_advance(parser);
    status = goofy_parse_expression(parser, &test);
    if(status != GoofyParserStatusOK) return status;
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol)
        return goofy_parser_error(parser, "expected eol");
    goofy_parser_advance(parser);

    result = GoofyParserStatusOK;
    goofy_parser_begin_scope(parser);
    while((goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElseIf) &&
          (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElse) &&
          (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndIf)) {
        if((result != GoofyParserStatusReturn) && test)
            status = goofy_parse_statement(parser, value);
        else
            status = goofy_skip_statement(parser);
        if(status == GoofyParserStatusReturn)
            result = status;
        else if(status != GoofyParserStatusOK)
            return status;
    }
    goofy_parser_end_scope(parser);
    skip = test;

    while(goofy_parser_current_symbol(parser) == GoofyLexerSymbolCmdElseIf) {
        goofy_parser_advance(parser);
        status = goofy_parse_expression(parser, &test);
        if(status != GoofyParserStatusOK) return status;
        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol)
            return goofy_parser_error(parser, "expected eol");
        goofy_parser_advance(parser);

        goofy_parser_begin_scope(parser);
        while((goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElseIf) &&
              (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdElse) &&
              (goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndIf)) {
            if((result != GoofyParserStatusReturn) && !skip && test)
                status = goofy_parse_statement(parser, value);
            else
                status = goofy_skip_statement(parser);
            if(status == GoofyParserStatusReturn)
                result = status;
            else if(status != GoofyParserStatusOK)
                return status;
        }
        goofy_parser_end_scope(parser);
        skip = skip | test;
    }

    if(goofy_parser_current_symbol(parser) == GoofyLexerSymbolCmdElse) {
        goofy_parser_advance(parser);
        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol)
            return goofy_parser_error(parser, "expected eol");
        goofy_parser_advance(parser);

        goofy_parser_begin_scope(parser);
        while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndIf) {
            if((result != GoofyParserStatusReturn) && !skip)
                status = goofy_parse_statement(parser, value);
            else
                status = goofy_skip_statement(parser);
            if(status == GoofyParserStatusReturn)
                result = status;
            else if(status != GoofyParserStatusOK)
                return status;
        }
        goofy_parser_end_scope(parser);
    }

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndIf)
        return goofy_parser_error(parser, "expected keyword end_if");
    goofy_parser_advance(parser);

    return result;
}
static GoofyParserStatus goofy_parse_while(GoofyParser* parser, goofy_value_t* value) {
    // cmd_while ::= 'while' expr eol {statement} 'end_while' eol
    GoofyParserStatus status, result;
    goofy_value_t test = true;

    result = GoofyParserStatusOK;
    goofy_pos_t pos = goofy_parser_pos(parser);
    while((result == GoofyParserStatusOK) && test) {
        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdWhile)
            return goofy_parser_error(parser, "expected keyword while");
        goofy_parser_advance(parser);
        status = goofy_parse_expression(parser, &test);
        if(status != GoofyParserStatusOK) return status;
        if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol)
            return goofy_parser_error(parser, "expected eol");
        goofy_parser_advance(parser);

        goofy_parser_begin_scope(parser);
        while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndWhile) {
            if((result != GoofyParserStatusReturn) && test)
                status = goofy_parse_statement(parser, value);
            else
                status = goofy_skip_statement(parser);
            if(status == GoofyParserStatusReturn)
                result = status;
            else if(status != GoofyParserStatusOK)
                return status;
        }
        goofy_parser_end_scope(parser);

        if((result == GoofyParserStatusOK) && test) {
            goofy_parser_jmp(parser, pos);
        }
    }
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndWhile)
        return goofy_parser_error(parser, "expected keyword end_while");
    goofy_parser_advance(parser);
    return result;
}
static GoofyParserStatus goofy_parse_function(GoofyParser* parser, goofy_value_t* value) {
    // cmd_function ::= 'function' name '(' [variable {',' variable}] ')' eol {statement} 'end_function' eol
    GoofyParserStatus status, result;

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdFunction)
        return goofy_parser_error(parser, "expected keyword function");
    goofy_parser_advance(parser);

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolName)
        return goofy_parser_error(parser, "expected name at function declaration");
    goofy_parser_advance(parser);
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolOpenPar)
        return goofy_parser_error(parser, "expected left parenthesis at function declaration");
    goofy_parser_advance(parser);

    if(goofy_parser_current_symbol(parser) == GoofyLexerSymbolVariable) {
        goofy_parser_advance(parser);

        while(goofy_parser_current_symbol(parser) == GoofyLexerSymbolComma) {
            goofy_parser_advance(parser);

            if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolVariable)
                return goofy_parser_error(
                    parser, "expected variable name at function declaration");
            goofy_parser_advance(parser);
        }
    }

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolClosePar)
        return goofy_parser_error(parser, "expected right parenthesis at function declaration");
    goofy_parser_advance(parser);
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol)
        return goofy_parser_error(parser, "expected eol");

    result = GoofyParserStatusOK;
    goofy_parser_begin_scope(parser);
    while(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndFunction) {
        if(result == GoofyParserStatusReturn)
            status = goofy_skip_statement(parser);
        else
            status = goofy_parse_statement(parser, value);
        if(status == GoofyParserStatusReturn)
            result = status;
        else if(status != GoofyParserStatusOK)
            return status;
    }
    goofy_parser_end_scope(parser);

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdEndFunction)
        return goofy_parser_error(parser, "expected keyword end_function");
    goofy_parser_advance(parser);
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_parse_return(GoofyParser* parser, goofy_value_t* value) {
    // cmd_return ::= 'return' expr eol
    GoofyParserStatus status;

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdReturn)
        return goofy_parser_error(parser, "expected keyword return");
    goofy_parser_advance(parser);

    status = goofy_parse_expression(parser, value);
    if(status != GoofyParserStatusOK) return status;

    return GoofyParserStatusReturn;
}

static GoofyParserStatus goofy_parse_cmd_string(GoofyParser* parser) {
    // cmd_string ::= 'string' ... eol
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdString)
        return goofy_parser_error(parser, "expected keyword string");
    goofy_parser_advance(parser);

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolString)
        return goofy_parser_error(parser, "expected character string");

    goofy_action_string(parser->current.content);

    goofy_parser_advance(parser);
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_parse_cmd_stringln(GoofyParser* parser) {
    // cmd_stringln ::= 'stringln' ... eol
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdStringLn)
        return goofy_parser_error(parser, "expected keyword stringln");
    goofy_parser_advance(parser);

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolString)
        return goofy_parser_error(parser, "expected character string");

    goofy_action_stringln(parser->current.content);

    goofy_parser_advance(parser);
    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_parse_cmd_delay(GoofyParser* parser) {
    // cmd_delay ::= 'delay' expr eol
    GoofyParserStatus status;
    goofy_value_t value;

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdDelay)
        return goofy_parser_error(parser, "expected keyword delay");
    goofy_parser_advance(parser);

    status = goofy_parse_expression(parser, &value);
    if(status != GoofyParserStatusOK) return status;

    goofy_action_delay(value);

    return GoofyParserStatusOK;
}
static GoofyParserStatus goofy_parse_cmd_hold(GoofyParser* parser) {
    // cmd_hold ::= 'hold' any_key {any_key}  eol
    UNUSED(parser);
    return GoofyParserStatusError;
}
static GoofyParserStatus goofy_parse_cmd_release(GoofyParser* parser) {
    // cmd_release ::= 'release' any_key {any_key} eol
    UNUSED(parser);
    return GoofyParserStatusError;
}
static GoofyParserStatus goofy_parse_cmd_key(GoofyParser* parser) {
    // cmd_key ::= key {any_key} eol
    UNUSED(parser);

    return GoofyParserStatusError;
}

static GoofyParserStatus goofy_declare_function(GoofyParser* parser) {
    goofy_pos_t pos = goofy_parser_pos(parser);

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolCmdFunction)
        return goofy_parser_error(parser, "expected keyword function");
    goofy_parser_advance(parser);

    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolName)
        return goofy_parser_error(parser, "expected name at function declaration");

    GoofyParserFuncs* func = goofy_parser_func_push(parser, parser->current.content, pos);

    goofy_parser_advance(parser);
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolOpenPar)
        return goofy_parser_error(parser, "expected left parenthesis at function declaration");
    goofy_parser_advance(parser);
    if(goofy_parser_current_symbol(parser) == GoofyLexerSymbolVariable) {
        goofy_parser_func_add_param(func, parser->current.content);
        goofy_parser_advance(parser);

        while(goofy_parser_current_symbol(parser) == GoofyLexerSymbolComma) {
            goofy_parser_advance(parser);
            if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolVariable)
                return goofy_parser_error(
                    parser, "expected variable name at function declaration");
            goofy_parser_func_add_param(func, parser->current.content);
            goofy_parser_advance(parser);
        }
    }
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolClosePar)
        return goofy_parser_error(parser, "expected right parenthesis at function declaration");
    goofy_parser_advance(parser);
    if(goofy_parser_current_symbol(parser) != GoofyLexerSymbolEol)
        return goofy_parser_error(parser, "expected eol");

    return goofy_skip_function_body(parser);
}

static GoofyParserStatus goofy_parse_statement(GoofyParser* parser, goofy_value_t* value) {
    // statement ::= comment | assignment | cmd_string | cmd_stringln | cmd_delay | cmd_var | cmd_hold | cmd_release | cmd_if | cmd_while | cmd_function | cmd_return | cmd_key
    GoofyLexerSymbol symbol;
    GoofyParserStatus status;

    *value = 0;

    symbol = goofy_parser_current_symbol(parser);
    status = GoofyParserStatusOK;
    if(symbol == GoofyLexerSymbolEof) {
        status = GoofyParserStatusEof;
    } else if(symbol == GoofyLexerSymbolComment) {
        // ignore comment
        goofy_parser_advance(parser);
    } else if(symbol == GoofyLexerSymbolVariable) {
        status = goofy_parse_assignement(parser);
    } else if(symbol == GoofyLexerSymbolCmdString) {
        status = goofy_parse_cmd_string(parser);
    } else if(symbol == GoofyLexerSymbolCmdStringLn) {
        status = goofy_parse_cmd_stringln(parser);
    } else if(symbol == GoofyLexerSymbolCmdDelay) {
        status = goofy_parse_cmd_delay(parser);
    } else if(symbol == GoofyLexerSymbolCmdVar) {
        status = goofy_parse_cmd_var(parser);
    } else if(symbol == GoofyLexerSymbolCmdHold) {
        status = goofy_parse_cmd_hold(parser);
    } else if(symbol == GoofyLexerSymbolCmdRelease) {
        status = goofy_parse_cmd_release(parser);
    } else if(symbol == GoofyLexerSymbolCmdIf) {
        status = goofy_parse_if(parser, value);
    } else if(symbol == GoofyLexerSymbolCmdWhile) {
        status = goofy_parse_while(parser, value);
    } else if(symbol == GoofyLexerSymbolCmdFunction) {
        status = goofy_declare_function(parser);
    } else if(symbol == GoofyLexerSymbolCmdReturn) {
        status = goofy_parse_return(parser, value);
    } else if(symbol == GoofyLexerSymbolCmdKey) {
        status = goofy_parse_cmd_key(parser);
    } else if(symbol == GoofyLexerSymbolError) {
        status = goofy_parser_error(parser, "unexpected token");
    }

    if((status != GoofyParserStatusOK) && (status != GoofyParserStatusReturn)) return status;

    symbol = goofy_parser_current_symbol(parser);
    if(symbol != GoofyLexerSymbolEol) return goofy_parser_error(parser, "expected eol");
    goofy_parser_advance(parser);
    return status;
}

GoofyParser* goofy_parser_alloc(File* file) {
    GoofyParser* parser = malloc(sizeof(GoofyParser));
    parser->lexer = goofy_lexer_alloc(file);
    parser->current.symbol = GoofyLexerSymbolError;
    string_init(parser->current.content);
    parser->vars = NULL;
    parser->funcs = NULL;
    parser->scope = NULL;

    goofy_parser_advance(parser);
    return parser;
}

void goofy_parser_free(GoofyParser* parser) {
    goofy_lexer_free(parser->lexer);
    string_clear(parser->current.content);
    while(parser->scope != NULL) goofy_parser_end_scope(parser);
    while(parser->vars != NULL) goofy_parser_var_pop(parser);
    while(parser->funcs != NULL) goofy_parser_func_pop(parser);
    free(parser);
}

GoofyParserStatus goofy_parser_next(GoofyParser* parser) {
    GoofyParserStatus status;
    goofy_value_t dummy;

    status = goofy_parse_statement(parser, &dummy);
    while(status == GoofyParserStatusOK) {
        status = goofy_parse_statement(parser, &dummy);
    }
    return status;
}
