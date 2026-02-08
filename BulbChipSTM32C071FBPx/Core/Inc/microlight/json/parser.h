#ifndef INC_JSON_PARSER_H_
#define INC_JSON_PARSER_H_

typedef enum {
    PARSER_OK = 0,
    PARSER_ERR_MISSING_FIELD,
    PARSER_ERR_STRING_TOO_SHORT,
    PARSER_ERR_STRING_TOO_LONG,
    PARSER_ERR_VALUE_TOO_SMALL,
    PARSER_ERR_VALUE_TOO_LARGE,
    PARSER_ERR_ARRAY_TOO_SHORT,
    PARSER_ERR_INVALID_VARIANT,
    PARSER_ERR_VALIDATION_FAILED
} ParserError;

typedef struct {
    ParserError error;
    char path[128];
} ParserErrorContext;

const char *parserErrorToString(ParserError err);

#endif /* INC_JSON_PARSER_H_ */
