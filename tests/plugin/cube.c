#include <stdio.h>
#include <polysat3/polysat.h>
#include <stdint.h>
#include "test.h"
#include "cube.h"

struct PluginSet Cube_plugins = {
   .prefix = "cube",
   .initial_paths = "plugs",
};

static void *make_cube(void *cfg, void *opaque, void *extra);

struct L1
{
   char *p;
   int32_t num;
   char *l2_type;
   struct CFG_buffered_obj *l2;
};

CFG_NEWOBJ(L1_CFG,
   CFG_MALLOC(struct L1),
   CFG_NULL,
   CFG_STRDUP("p", struct L1, p),
   CFG_INT32("num", struct L1, num),
   CFG_STRDUP("__type_l2", struct L1, l2_type),
   CFG_PL_BOBJLIST("l2", &Cube_plugins, struct L1, l2)
)

struct Plugin Cube = {
   .cfg = &CFG_OBJNAME(L1_CFG),
   .name = "cube",
   .factory = &make_cube,
};

void initialize_plugin(void)
{
   printf("Initialize Cube!!\n");
   PL_register(&RT_plugins, &Cube, NULL);
}

static void *make_cube(void *cfg, void *opaque, void *extra)
{
   struct L1 *c;

   printf("Cube:\n");
   if (cfg) {
      c = (struct L1*)cfg;
      if (c->p)
         printf("   %s\n", c->p);
      printf("   %d\n", c->num);

      if (c->l2 && c->l2_type) {
         void *cfg2 =  CFG_cfg_for_object_buffer(c->l2);
         if (cfg2)
            PL_create_object(&Cube_plugins, c->l2_type, cfg2, NULL);
      }
   }

   return NULL;
}
