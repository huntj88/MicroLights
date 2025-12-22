import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

import { modeSchema } from '../src/app/models/mode';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// TODO: add max name length, array length validation in Zod

/**
 * This script generates a C parser for the Mode model defined in src/app/models/mode.ts.
 *
 * INSTRUCTIONS FOR UPDATING:
 * 1. If src/app/models/mode.ts changes, update the `schema` object below to reflect the new structure.
 * 2. The `schema` object uses a simplified definition format:
 *    - `type`: 'struct' | 'discriminatedUnion'
 *    - `fields`: Object mapping field names to definitions.
 *    - `refine`: C expression string for validation logic (e.g. "out->has_front || out->has_case_comp").
 * 3. Field definitions:
 *    - `type`: 'string' | 'uint32' | 'boolean' | 'array' | [StructName]
 *    - `min`, `max`: Validation constraints.
 *    - `optional`: boolean.
 * 4. Run `npx vite-node scripts/generate_c_parser.ts` to regenerate mode_parser.h and mode_parser.c.
 */

const examplesData = [
  {
    name: 'Simple Pattern',
    data: {
      name: 'Blinky',
      front: {
        pattern: {
          type: 'simple',
          name: 'Flash',
          duration: 1000,
          changeAt: [
            { ms: 0, output: '#FF0000' },
            { ms: 500, output: '#000000' },
          ],
        },
      },
    },
  },
  {
    name: 'Equation Pattern',
    data: {
      name: 'Sine Wave',
      case: {
        pattern: {
          type: 'equation',
          name: 'Red Sine',
          duration: 2000,
          red: {
            sections: [{ equation: 'sin(t * 2 * pi)', duration: 2000 }],
            loopAfterDuration: true,
          },
          green: { sections: [], loopAfterDuration: true },
          blue: { sections: [], loopAfterDuration: true },
        },
      },
    },
  },
  {
    name: 'Accel Trigger',
    data: {
      name: 'Impact',
      front: {
        pattern: {
          type: 'simple',
          name: 'Steady Off',
          duration: 1000,
          changeAt: [{ ms: 0, output: '#000000' }],
        },
      },
      accel: {
        triggers: [
          {
            threshold: 50,
            front: {
              pattern: {
                type: 'simple',
                name: 'White Flash',
                duration: 100,
                changeAt: [{ ms: 0, output: '#FFFFFF' }],
              },
            },
          },
        ],
      },
    },
  },
];

// Validate examples
console.log('Validating examples...');
for (const example of examplesData) {
  try {
    modeSchema.parse(example.data);
    console.log(`✓ Example "${example.name}" is valid.`);
  } catch (error) {
    console.error(`✗ Example "${example.name}" is INVALID:`);
    console.error(JSON.stringify(error, null, 2));
    process.exit(1);
  }
}
console.log('All examples validated successfully.\n');

const examplesComment = `
 * EXAMPLES:
 *
${examplesData
  .map(
    (ex, i) => ` * ${String(i + 1)}. ${ex.name}:
${JSON.stringify(ex.data, null, 2)
  .split('\n')
  .map(line => ` * ${line}`)
  .join('\n')}`,
  )
  .join('\n *\n')}
`;

type FieldType =
  | 'string'
  | 'uint8'
  | 'uint32'
  | 'boolean'
  | 'array'
  | 'ModeComponent'
  | 'ModePattern'
  | 'ModeAccel'
  | 'ModeAccelTrigger'
  | 'SimplePattern'
  | 'EquationPattern'
  | 'PatternChange'
  | 'EquationSection'
  | 'ChannelConfig'
  | 'SimpleOutput';

interface BaseFieldDef {
  type: FieldType;
  optional?: boolean;
}

interface StringFieldDef extends BaseFieldDef {
  type: 'string';
  min?: number;
  max: number;
}

interface Uint8FieldDef extends BaseFieldDef {
  type: 'uint8';
  min?: number;
}

interface Uint32FieldDef extends BaseFieldDef {
  type: 'uint32';
  min?: number;
}

interface BooleanFieldDef extends BaseFieldDef {
  type: 'boolean';
}

interface ArrayFieldDef extends BaseFieldDef {
  type: 'array';
  item: string;
  min?: number;
  max: number;
}

interface StructFieldDef extends BaseFieldDef {
  type:
    | 'ModeComponent'
    | 'ModePattern'
    | 'ModeAccel'
    | 'ModeAccelTrigger'
    | 'SimplePattern'
    | 'EquationPattern'
    | 'PatternChange'
    | 'EquationSection'
    | 'ChannelConfig'
    | 'SimpleOutput'; // Name of another struct
}

type FieldDef = StringFieldDef | Uint8FieldDef | Uint32FieldDef | BooleanFieldDef | ArrayFieldDef | StructFieldDef;

interface RefineDef {
  expr: string;
  field?: string;
}

interface StructDef {
  type: 'struct';
  fields: Record<string, FieldDef>;
  refine?: string | RefineDef;
}

interface DiscriminatedUnionDef {
  type: 'discriminatedUnion';
  discriminator: string;
  variants: Record<string, string>;
}

type SchemaDef = StructDef | DiscriminatedUnionDef;

const schema: Record<string, SchemaDef> = {
  PatternChange: {
    type: 'struct',
    fields: {
      ms: { type: 'uint32', min: 0 },
      output: { type: 'SimpleOutput' },
    },
  },
  SimplePattern: {
    type: 'struct',
    fields: {
      name: { type: 'string', min: 1, max: 31 },
      duration: { type: 'uint32', min: 1 },
      changeAt: { type: 'array', item: 'PatternChange', min: 1, max: 32 },
    },
  },
  EquationSection: {
    type: 'struct',
    fields: {
      equation: { type: 'string', min: 1, max: 63 },
      duration: { type: 'uint32', min: 1 },
    },
  },
  ChannelConfig: {
    type: 'struct',
    fields: {
      sections: { type: 'array', item: 'EquationSection', max: 3 },
      loopAfterDuration: { type: 'boolean' },
    },
  },
  EquationPattern: {
    type: 'struct',
    fields: {
      name: { type: 'string', min: 1, max: 31 },
      duration: { type: 'uint32', min: 0 },
      red: { type: 'ChannelConfig' },
      green: { type: 'ChannelConfig' },
      blue: { type: 'ChannelConfig' },
    },
    refine: {
      expr: 'out->red.sections_count > 0 || out->green.sections_count > 0 || out->blue.sections_count > 0',
      field: 'red',
    },
  },
  ModePattern: {
    type: 'discriminatedUnion',
    discriminator: 'type',
    variants: {
      simple: 'SimplePattern',
      equation: 'EquationPattern',
    },
  },
  ModeComponent: {
    type: 'struct',
    fields: {
      pattern: { type: 'ModePattern' },
    },
  },
  ModeAccelTrigger: {
    type: 'struct',
    fields: {
      threshold: { type: 'uint8', min: 0 },
      front: { type: 'ModeComponent', optional: true },
      case: { type: 'ModeComponent', optional: true },
    },
    refine: { expr: 'out->has_front || out->has_case_comp', field: 'front' },
  },
  ModeAccel: {
    type: 'struct',
    fields: {
      triggers: { type: 'array', item: 'ModeAccelTrigger', min: 1, max: 2 },
    },
  },
  Mode: {
    type: 'struct',
    fields: {
      name: { type: 'string', min: 1, max: 31 },
      front: { type: 'ModeComponent', optional: true },
      case: { type: 'ModeComponent', optional: true },
      accel: { type: 'ModeAccel', optional: true },
    },
    refine: { expr: 'out->has_front || out->has_case_comp', field: 'front' },
  },
};

function generateModelHeader() {
  let out = `/* Generated by scripts/generate_c_parser.ts */
/*
${examplesComment}
 */
#ifndef MODE_MODEL_H
#define MODE_MODEL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PATTERN_TYPE_SIMPLE,
    PATTERN_TYPE_EQUATION
} PatternType;

typedef enum SimpleOutputType {
	BULB,
	RGB
} SimpleOutputType;

typedef enum BulbSimpleOutput {
	low, high
} BulbSimpleOutput;

typedef struct RGBSimpleOutput RGBSimpleOutput;
typedef struct SimpleOutput SimpleOutput;

`;

  // Forward declarations
  for (const name of Object.keys(schema)) {
    out += `typedef struct ${name} ${name};\n`;
  }
  out += '\n';

  out += `struct RGBSimpleOutput {
	uint8_t r; // 0 - 255
	uint8_t g; // 0 - 255
	uint8_t b; // 0 - 255
};

struct SimpleOutput {
	SimpleOutputType type;
	union {
		BulbSimpleOutput bulb;
		RGBSimpleOutput rgb;
	} data;
};

`;

  for (const [name, def] of Object.entries(schema)) {
    out += `struct ${name} {\n`;
    if (def.type === 'struct') {
      for (const [fieldName, fieldDef] of Object.entries(def.fields)) {
        const cName = fieldName === 'case' ? 'case_comp' : fieldName;
        if (fieldDef.type === 'string') {
          out += `    char ${cName}[${String(fieldDef.max + 1)}];\n`;
        } else if (fieldDef.type === 'uint8') {
          out += `    uint8_t ${cName};\n`;
        }  else if (fieldDef.type === 'uint32') {
          out += `    uint32_t ${cName};\n`;
        } else if (fieldDef.type === 'boolean') {
          out += `    bool ${cName};\n`;
        } else if (fieldDef.type === 'array') {
          out += `    ${fieldDef.item} ${cName}[${String(fieldDef.max)}];\n`;
          out += `    uint8_t ${cName}_count;\n`;
        } else {
          // Struct type
          out += `    ${fieldDef.type} ${cName};\n`;
        }

        if (fieldDef.optional) {
          out += `    bool has_${cName};\n`;
        }
      }

      // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
    } else if (def.type === 'discriminatedUnion') {
      out += `    PatternType type;\n`;
      out += `    union {\n`;
      for (const [key, typeName] of Object.entries(def.variants)) {
        out += `        ${typeName} ${key};\n`;
      }
      out += `    } data;\n`;
    }
    out += `};\n\n`;
  }

  out += `#endif // MODE_MODEL_H\n`;
  return out;
}

function generateParserHeader() {
  return `/* Generated by scripts/generate_c_parser.ts */
#ifndef MODE_PARSER_H
#define MODE_PARSER_H

#include "lwjson/lwjson.h"
#include "model/mode.h"

typedef enum {
    MODE_PARSER_OK = 0,
    MODE_PARSER_ERR_MISSING_FIELD,
    MODE_PARSER_ERR_STRING_TOO_SHORT,
    MODE_PARSER_ERR_STRING_TOO_LONG,
    MODE_PARSER_ERR_VALUE_TOO_SMALL,
    MODE_PARSER_ERR_VALUE_TOO_LARGE,
    MODE_PARSER_ERR_ARRAY_TOO_SHORT,
    MODE_PARSER_ERR_INVALID_VARIANT,
    MODE_PARSER_ERR_VALIDATION_FAILED
} ModeParserError;

typedef struct {
    ModeParserError error;
    char path[128];
} ModeErrorContext;

const char* modeParserErrorToString(ModeParserError err);

bool parseMode(lwjson_t *lwjson, lwjson_token_t *token, Mode *out, ModeErrorContext *ctx);

#endif // MODE_PARSER_H
`;
}

function generateParserSource() {
  let out = `/* Generated by scripts/generate_c_parser.ts */
#include <string.h>
#include <stdio.h>
#include "json/mode_parser.h"

// Helper to copy string safely
static void copyString(char *dest, const lwjson_token_t *token, size_t maxLen) {
    size_t len = token->u.str.token_value_len;
    if (len > maxLen) len = maxLen;
    memcpy(dest, token->u.str.token_value, len);
    dest[len] = '\\0';
}

static void prependContext(ModeErrorContext *ctx, const char *prefix, int32_t index) {
    char tmp[256];
    if (ctx->path[0] == '\\0') {
        if (index >= 0) snprintf(tmp, sizeof(tmp), "%s[%d]", prefix, (int)index);
        else snprintf(tmp, sizeof(tmp), "%s", prefix);
    } else {
        if (index >= 0) snprintf(tmp, sizeof(tmp), "%s[%d].%s", prefix, (int)index, ctx->path);
        else snprintf(tmp, sizeof(tmp), "%s.%s", prefix, ctx->path);
    }
    strncpy(ctx->path, tmp, sizeof(ctx->path) - 1);
    ctx->path[sizeof(ctx->path) - 1] = '\\0';
}

const char* modeParserErrorToString(ModeParserError err) {
    switch (err) {
        case MODE_PARSER_OK: return "Success";
        case MODE_PARSER_ERR_MISSING_FIELD: return "Missing required field";
        case MODE_PARSER_ERR_STRING_TOO_SHORT: return "String is too short";
        case MODE_PARSER_ERR_STRING_TOO_LONG: return "String is too long";
        case MODE_PARSER_ERR_VALUE_TOO_SMALL: return "Value is too small";
        case MODE_PARSER_ERR_VALUE_TOO_LARGE: return "Value is too large";
        case MODE_PARSER_ERR_ARRAY_TOO_SHORT: return "Array has too few items";
        case MODE_PARSER_ERR_INVALID_VARIANT: return "Invalid variant type";
        case MODE_PARSER_ERR_VALIDATION_FAILED: return "Validation failed";
        default: return "Unknown error";
    }
}

static uint8_t hexCharToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static bool parseSimpleOutput(lwjson_t *lwjson, lwjson_token_t *token, SimpleOutput *out, ModeErrorContext *ctx) {
    if (token->type != LWJSON_TYPE_STRING) {
        ctx->error = MODE_PARSER_ERR_VALIDATION_FAILED;
        return false;
    }
    
    char tmp[16];
    copyString(tmp, token, 15);
    
    if (strcmp(tmp, "high") == 0) {
        out->type = BULB;
        out->data.bulb = high;
        return true;
    }
    if (strcmp(tmp, "low") == 0) {
        out->type = BULB;
        out->data.bulb = low;
        return true;
    }
    
    if (tmp[0] == '#' && strlen(tmp) == 7) {
        out->type = RGB;
        out->data.rgb.r = (hexCharToInt(tmp[1]) << 4) | hexCharToInt(tmp[2]);
        out->data.rgb.g = (hexCharToInt(tmp[3]) << 4) | hexCharToInt(tmp[4]);
        out->data.rgb.b = (hexCharToInt(tmp[5]) << 4) | hexCharToInt(tmp[6]);
        return true;
    }
    
    ctx->error = MODE_PARSER_ERR_VALIDATION_FAILED;
    return false;
}

`;

  // Forward declarations
  out += `static bool parseSimpleOutput(lwjson_t *lwjson, lwjson_token_t *token, SimpleOutput *out, ModeErrorContext *ctx);\n`;

  for (const name of Object.keys(schema)) {
    if (name === 'Mode') continue;
    out += `static bool parse${name}(lwjson_t *lwjson, lwjson_token_t *token, ${name} *out, ModeErrorContext *ctx);\n`;
  }
  out += '\n';

  for (const [name, def] of Object.entries(schema)) {
    const prefix = name === 'Mode' ? '' : 'static ';
    out += `${prefix}bool parse${name}(lwjson_t *lwjson, lwjson_token_t *token, ${name} *out, ModeErrorContext *ctx) {\n`;
    out += `    const lwjson_token_t *t;\n`;

    if (def.type === 'struct') {
      // Initialize optional flags
      for (const [fieldName, fieldDef] of Object.entries(def.fields)) {
        const cName = fieldName === 'case' ? 'case_comp' : fieldName;
        if (fieldDef.optional) {
          out += `    out->has_${cName} = false;\n`;
        }
        if (fieldDef.type === 'array') {
          out += `    out->${cName}_count = 0;\n`;
        }
      }

      for (const [fieldName, fieldDef] of Object.entries(def.fields)) {
        const cName = fieldName === 'case' ? 'case_comp' : fieldName;

        out += `    if ((t = lwjson_find_ex(lwjson, token, "${fieldName}")) != NULL) {\n`;

        if (fieldDef.type === 'string') {
          out += `        if (t->u.str.token_value_len > ${String(fieldDef.max)}) {\n`;
          out += `            ctx->error = MODE_PARSER_ERR_STRING_TOO_LONG;\n`;
          out += `            strcpy(ctx->path, "${fieldName}");\n`;
          out += `            return false;\n`;
          out += `        }\n`;
          out += `        copyString(out->${cName}, t, ${String(fieldDef.max)});\n`;
          if (fieldDef.min) {
            out += `        if (strlen(out->${cName}) < ${String(fieldDef.min)}) {\n`;
            out += `            ctx->error = MODE_PARSER_ERR_STRING_TOO_SHORT;\n`;
            out += `            strcpy(ctx->path, "${fieldName}");\n`;
            out += `            return false;\n`;
            out += `        }\n`;
          }
        } else if (fieldDef.type === 'uint32') {
          if (fieldDef.min !== undefined) {
            out += `        if (t->u.num_int < ${String(fieldDef.min)}) {\n`;
            out += `            ctx->error = MODE_PARSER_ERR_VALUE_TOO_SMALL;\n`;
            out += `            strcpy(ctx->path, "${fieldName}");\n`;
            out += `            return false;\n`;
            out += `        }\n`;
          }
          out += `        if (t->u.num_int > 4294967295) {\n`;
          out += `            ctx->error = MODE_PARSER_ERR_VALUE_TOO_LARGE;\n`;
          out += `            strcpy(ctx->path, "${fieldName}");\n`;
          out += `            return false;\n`;
          out += `        }\n`;
          out += `        out->${cName} = (uint32_t)t->u.num_int;\n`;
        } else if (fieldDef.type === 'uint8') {
          if (fieldDef.min !== undefined) {
            out += `        if (t->u.num_int < ${String(fieldDef.min)}) {\n`;
            out += `            ctx->error = MODE_PARSER_ERR_VALUE_TOO_SMALL;\n`;
            out += `            strcpy(ctx->path, "${fieldName}");\n`;
            out += `            return false;\n`;
            out += `        }\n`;
          }
          out += `        if (t->u.num_int > 255) {\n`;
          out += `            ctx->error = MODE_PARSER_ERR_VALUE_TOO_LARGE;\n`;
          out += `            strcpy(ctx->path, "${fieldName}");\n`;
          out += `            return false;\n`;
          out += `        }\n`;
          out += `        out->${cName} = (uint8_t)t->u.num_int;\n`;
        } else if (fieldDef.type === 'boolean') {
          out += `        out->${cName} = t->u.num_int != 0; // lwjson parses bools as ints often, or check type\n`;
          out += `        if (t->type == LWJSON_TYPE_TRUE) out->${cName} = true;\n`;
          out += `        else if (t->type == LWJSON_TYPE_FALSE) out->${cName} = false;\n`;
        } else if (fieldDef.type === 'array') {
          out += `        const lwjson_token_t *child = lwjson_get_first_child(t);\n`;
          out += `        while (child != NULL && out->${cName}_count < ${String(fieldDef.max)}) {\n`;
          out += `            if (!parse${fieldDef.item}(lwjson, (lwjson_token_t*)child, &out->${cName}[out->${cName}_count], ctx)) {\n`;
          out += `                prependContext(ctx, "${fieldName}", out->${cName}_count);\n`;
          out += `                return false;\n`;
          out += `            }\n`;
          out += `            out->${cName}_count++;\n`;
          out += `            child = child->next;\n`;
          out += `        }\n`;
          if (fieldDef.min) {
            out += `        if (out->${cName}_count < ${String(fieldDef.min)}) {\n`;
            out += `            ctx->error = MODE_PARSER_ERR_ARRAY_TOO_SHORT;\n`;
            out += `            strcpy(ctx->path, "${fieldName}");\n`;
            out += `            return false;\n`;
            out += `        }\n`;
          }
        } else {
          // Struct type
          out += `        if (!parse${fieldDef.type}(lwjson, (lwjson_token_t*)t, &out->${cName}, ctx)) {\n`;
          out += `            prependContext(ctx, "${fieldName}", -1);\n`;
          out += `            return false;\n`;
          out += `        }\n`;
        }

        if (fieldDef.optional) {
          out += `        out->has_${cName} = true;\n`;
        }

        out += `    }`;
        if (!fieldDef.optional) {
          out += ` else {\n`;
          out += `        ctx->error = MODE_PARSER_ERR_MISSING_FIELD;\n`;
          out += `        strcpy(ctx->path, "${fieldName}");\n`;
          out += `        return false;\n`;
          out += `    }\n`;
        } else {
          out += `\n`;
        }
      }

      if (def.refine) {
        const refineExpr = typeof def.refine === 'string' ? def.refine : def.refine.expr;
        const refineField =
          typeof def.refine === 'object' && def.refine.field ? def.refine.field : null;

        out += `    if (!(${refineExpr})) {\n`;
        out += `        ctx->error = MODE_PARSER_ERR_VALIDATION_FAILED;\n`;
        if (refineField) {
          out += `        strcpy(ctx->path, "${refineField}");\n`;
        } else {
          out += `        ctx->path[0] = '\\0';\n`;
        }
        out += `        return false;\n`;
        out += `    }\n`;
      }

      // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
    } else if (def.type === 'discriminatedUnion') {
      out += `    if ((t = lwjson_find_ex(lwjson, token, "${def.discriminator}")) != NULL) {\n`;
      out += `        char typeStr[32];\n`;
      out += `        copyString(typeStr, t, 31);\n`;

      let first = true;
      for (const [key, typeName] of Object.entries(def.variants)) {
        out += `        ${first ? '' : 'else '}if (strcmp(typeStr, "${key}") == 0) {\n`;
        out += `            out->type = PATTERN_TYPE_${key.toUpperCase()};\n`;
        out += `            if (!parse${typeName}(lwjson, token, &out->data.${key}, ctx)) return false;\n`;
        out += `        }\n`;
        first = false;
      }
      out += `        else {\n`;
      out += `            ctx->error = MODE_PARSER_ERR_INVALID_VARIANT;\n`;
      out += `            strcpy(ctx->path, "${def.discriminator}");\n`;
      out += `            return false;\n`;
      out += `        }\n`;
      out += `    } else {\n`;
      out += `        ctx->error = MODE_PARSER_ERR_MISSING_FIELD;\n`;
      out += `        strcpy(ctx->path, "${def.discriminator}");\n`;
      out += `        return false;\n`;
      out += `    }\n`;
    }

    out += `    return true;\n`;
    out += `}\n\n`;
  }

  return out;
}

const projectRoot = path.resolve(__dirname, '..');
// Assuming the repo structure is MicroLights/App/configure-app-v3 and MicroLights/BulbChipSTM32C071FBPx
const repoRoot = path.resolve(projectRoot, '../..');
const modelIncDir = path.join(repoRoot, 'BulbChipSTM32C071FBPx/Core/Inc/model');
const jsonIncDir = path.join(repoRoot, 'BulbChipSTM32C071FBPx/Core/Inc/json');
const jsonSrcDir = path.join(repoRoot, 'BulbChipSTM32C071FBPx/Core/Src/json');

const modelHeaderPath = path.join(modelIncDir, 'mode.h');
const parserHeaderPath = path.join(jsonIncDir, 'mode_parser.h');
const parserSourcePath = path.join(jsonSrcDir, 'mode_parser.c');

// Ensure directories exist
function ensureDir(dir: string) {
  if (!fs.existsSync(dir)) {
    console.log(`Creating directory ${dir}`);
    fs.mkdirSync(dir, { recursive: true });
  }
}

try {
  ensureDir(modelIncDir);
  fs.writeFileSync(modelHeaderPath, generateModelHeader());
  console.log(`Generated ${modelHeaderPath}`);
} catch (e: unknown) {
  console.error(`Failed to write to ${modelHeaderPath}:`, e);
  console.log('Falling back to local write for mode.h');
  fs.writeFileSync('mode.h', generateModelHeader());
}

try {
  ensureDir(jsonIncDir);
  fs.writeFileSync(parserHeaderPath, generateParserHeader());
  console.log(`Generated ${parserHeaderPath}`);
} catch (e: unknown) {
  console.error(`Failed to write to ${parserHeaderPath}:`, e);
  console.log('Falling back to local write for mode_parser.h');
  fs.writeFileSync('mode_parser.h', generateParserHeader());
}

try {
  ensureDir(jsonSrcDir);
  fs.writeFileSync(parserSourcePath, generateParserSource());
  console.log(`Generated ${parserSourcePath}`);
} catch (e: unknown) {
  console.error(`Failed to write to ${parserSourcePath}:`, e);
  console.log('Falling back to local write for mode_parser.c');
  fs.writeFileSync('mode_parser.c', generateParserSource());
}
