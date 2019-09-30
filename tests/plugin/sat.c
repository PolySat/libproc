#include <stdio.h>
#include <polysat3/polysat.h>
#include <stdint.h>
#include "cube.h"

static void *make_sat(void *cfg, void *opaque, void *extra);

struct L2
{
   char *foo;
};

CFG_NEWOBJ(L2_CFG,
   CFG_MALLOC(struct L2),
   CFG_NULL,
   CFG_STRDUP("foo", struct L2, foo)
)

struct Plugin Sat = {
   .cfg = &CFG_OBJNAME(L2_CFG),
   .name = "sat",
   .factory = &make_sat,
};

void initialize_plugin(void)
{
   printf("Initialize Sat!!\n");
   PL_register(&Cube_plugins, &Sat, NULL);
}

static void *make_sat(void *cfg, void *opaque, void *extra)
{
   struct L2 *c;

   printf("Sat:\n");
   if (cfg) {
      c = (struct L2*)cfg;
      if (c->foo)
         printf("   %s\n", c->foo);
   }

   return NULL;
}
