#include "test.h"
#include <polysat3/polysat.h>

struct PluginSet RT_plugins = {
   .prefix = "teST1",
   .initial_paths = "plugs,/usr/lib/plugs/,foo/bar//",
};


struct T1
{
   char *t1_type;
   struct CFG_buffered_obj *t1_buff;
};

CFG_NEWOBJ(T1_CFG,
      CFG_MALLOC(struct T1),
      CFG_NULL,
      CFG_STRDUP("__type_l1", struct T1, t1_type),
      CFG_PL_BOBJLIST("l1", &RT_plugins, struct T1, t1_buff)
)

struct PluginSet t2 = {
   .prefix = "T2",
   .initial_paths = "/usr/lib/plugs/,foo/bar",
};

int main(int argc, char **argv)
{
   struct T1 *t;
   void *cfg;

   DBG_init("test");
   DBG_setLevel(DBG_LEVEL_ALL);

   CFG_locateConfigFile("test.cfg");
   t = (struct T1*)CFG_parseFile(&CFG_OBJNAME(T1_CFG));
   if (t && t->t1_buff) {
      printf("T1: %s\n", t->t1_type);
      cfg = CFG_cfg_for_object_buffer(t->t1_buff);
      if (cfg)
         PL_create_object(&RT_plugins, t->t1_type, cfg, NULL);
   }

   return 0;
}
