#include "json/parser.h"

const char *parserErrorToString(ParserError err) {
    switch (err) {
        case PARSER_OK:
            return "Success";
        case PARSER_ERR_MISSING_FIELD:
            return "Missing required field";
        case PARSER_ERR_STRING_TOO_SHORT:
            return "String is too short";
        case PARSER_ERR_STRING_TOO_LONG:
            return "String is too long";
        case PARSER_ERR_VALUE_TOO_SMALL:
            return "Value is too small";
        case PARSER_ERR_VALUE_TOO_LARGE:
            return "Value is too large";
        case PARSER_ERR_ARRAY_TOO_SHORT:
            return "Array has too few items";
        case PARSER_ERR_INVALID_VARIANT:
            return "Invalid variant type";
        case PARSER_ERR_VALIDATION_FAILED:
            return "Validation failed";
        default:
            return "Unknown error";
    }
}
