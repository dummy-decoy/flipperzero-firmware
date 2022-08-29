#pragma once

#include <storage/storage.h>

typedef struct GoofyParser GoofyParser;

typedef enum {
    GoofyParserStatusOK,
    GoofyParserStatusError,
    GoofyParserStatusEof,
    GoofyParserStatusReturn,
} GoofyParserStatus;


GoofyParser* goofy_parser_alloc(File* file_name);
void goofy_parser_free(GoofyParser* parser);

GoofyParserStatus goofy_parser_next(GoofyParser* parser);
