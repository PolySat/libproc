#include <stddef.h>
:: if namespace == 'IPC':
#include "cmd.h"
:: else:
#include <polysat/cmd.h>
#include <polysat/cmd_schema.h>
:: #endif
#include "${header}"

void ${namespace.upper().replace('::','_',400)}_forcelink(void) {}

