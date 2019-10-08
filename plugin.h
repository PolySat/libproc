#ifndef PLUGIN_H
#define PLUGIN_H

struct CFG_ParseObj;
struct PluginSetData;

typedef void *(*plugin_obj_factory_t)(void *cfg, void *opaque, void *extra);
typedef void (*plugin_init_func)(void);
#define PL_INIT_FUNCTION "initialize_plugin"
#define PL_PRELOAD_PREFIX "preload-"
#define PL_PLUGIN_SUFFIX ".so"

struct Plugin {
   struct CFG_ParseObj *cfg;
   const char *name;
   plugin_obj_factory_t factory;
};

struct PluginSet {
   const char *prefix;
   const char *initial_paths;
   struct PluginSetData *data;
};

extern int PL_register(struct PluginSet *set, struct Plugin *plugin,
      void *opaque);
extern void *PL_create_object(struct PluginSet *set, const char *name,
      void *cfg, void *extra);
extern struct CFG_ParseObj *PL_config_for_plugin(struct PluginSet *set,
      const char *name, int defer_load);
extern struct CFG_ParseObj *PL_objectlookup(const char *name,
      void *arg, int *buffer_obj);
extern struct CFG_ParseObj *PL_objectlookup_w_buffer(const char *name,
      void *arg, int *buffer_obj);
extern void PL_preload_plugins(struct PluginSet *set);

#define CFG_PL_OBJ(n, ps, str, field) { n, &PL_objectlookup, &ps, { &CFG_PtrCpyCB, (void*)offsetof(str,field), NULL } }
#define CFG_PL_OBJLIST(n, ps, str, field) { n, &PL_objectlookup, ps, { &CFG_PtrCpyCB, (void*)offsetof(str,field), NULL } }
#define CFG_PL_OBJARR(n, ps, str, field) { n, &PL_objectlookup, ps, { &CFG_PtrArrayAppendCB, (void*)offsetof(str,field), NULL } }
#define CFG_PL_OBJLISTARR(n, ps, str, field) { n, &PL_objectlookup, ps, { &CFG_PtrArrayAppendCB, (void*)offsetof(str,field), NULL } }

#define CFG_PL_BOBJ(n, ps, str, field) { n, &PL_objectlookup_w_buffer, &ps, { &CFG_PtrCpyCB, (void*)offsetof(str,field), NULL } }
#define CFG_PL_BOBJLIST(n, ps, str, field) { n, &PL_objectlookup_w_buffer, ps, { &CFG_PtrCpyCB, (void*)offsetof(str,field), NULL } }
#define CFG_PL_BOBJARR(n, ps, str, field) { n, &PL_objectlookup_w_buffer, ps, { &CFG_PtrArrayAppendCB, (void*)offsetof(str,field), NULL } }
#define CFG_PL_BOBJLISTARR(n, ps, str, field) { n, &PL_objectlookup_w_buffer, ps, { &CFG_PtrArrayAppendCB, (void*)offsetof(str,field), NULL } }

#endif
