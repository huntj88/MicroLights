/*
 * cli_model.c
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#include "model/cli_model.h"

/* Global CLI input state.
 * Note: This variable is not thread-safe and is intended for single-threaded use only.
 * If used from multiple threads or ISRs, external synchronization must be provided.
 */
CliInput cliInput;
