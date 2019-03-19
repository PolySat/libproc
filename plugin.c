#include "plugin.h"
#include "config.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

struct PluginDirectory {
   char *path;
   int preloaded;
   struct PluginDirectory *next;
};

struct PluginDirectoryList {
   struct PluginDirectory *dir;
   struct PluginDirectoryList *next;
};

struct PluginList {
   struct Plugin *plugin;
   void *opaque;
   struct PluginList *next;
};

struct PluginSetData {
   struct PluginSet *set;
   struct PluginDirectoryList *dirs;
   struct PluginList *plugins;
   char *name_lc;
};

static void load_plugin_file(const char *file)
{
   void *mod;
   plugin_init_func init_func = NULL;

   mod = dlopen(file, RTLD_NOW | RTLD_GLOBAL);
   if (!mod)
      return;

   init_func = (plugin_init_func)dlsym(mod, PL_INIT_FUNCTION);
   if (init_func)
      (*init_func)();
   else
      DBG_print(DBG_LEVEL_WARN, "Failed to find symbol '%s' in %s: %s\n",
            PL_INIT_FUNCTION, file, dlerror());
}

static void preload_directory(struct PluginDirectory *dir)
{
   DIR *ditr;
   struct dirent *dent;
   char path[2049];

   if (!dir)
      return;

   ditr = opendir(dir->path);
   if (!ditr)
      return;

   while ((dent = readdir(ditr))) {
      if (0 == dent->d_name[0] || '.' == dent->d_name[0])
         continue;
      if (strncmp(dent->d_name, PL_PRELOAD_PREFIX, sizeof(PL_PRELOAD_PREFIX)-1))
         continue;
      if (strncmp(&dent->d_name[strlen(dent->d_name) -
               sizeof(PL_PLUGIN_SUFFIX) + 1], PL_PLUGIN_SUFFIX,
               sizeof(PL_PLUGIN_SUFFIX)-1))
         continue;
      path[0] = 0;

      strcpy(path, dir->path);
      strcat(path, "/");
      strcat(path, dent->d_name);

      load_plugin_file(path);
   }

   closedir(ditr);

   dir->preloaded = 1;
}

static struct PluginDirectory *find_directory(const char *name)
{
   static struct PluginDirectory *head = NULL;
   struct PluginDirectory *curr;

   for (curr = head; curr; curr = curr->next) {
      if (!strcmp(name, curr->path))
         return curr;
   }

   curr = (struct PluginDirectory*) malloc(sizeof(*curr));
   memset(curr, 0, sizeof(*curr));
   curr->path = strdup(name);
   curr->next = head;
   head = curr;

   return curr;
}

static struct PluginDirectoryList *configure_directory(const char *name)
{
   struct PluginDirectory *dir;
   struct PluginDirectoryList *result;

   if (!name || !*name)
      return NULL;
   dir = find_directory(name);
   if (!dir)
      return NULL;
   if (!dir->preloaded) {
      preload_directory(dir);
   }
   if (!dir->preloaded)
      return NULL;

   result = (struct PluginDirectoryList*) malloc(sizeof(*result));
   memset(result, 0, sizeof(*result));
   result->dir = dir;

   return result;
}

static void configure_set(struct PluginSet *set)
{
   struct PluginSetData *data;
   char *paths = NULL, *itr, *sep, *tail;
   struct PluginDirectoryList *dir;
   char *c;

   data = (struct PluginSetData*)malloc(sizeof(*data));
   memset(data, 0, sizeof(*data));

   set->data = data;
   data->set = set;
   c = data->name_lc = strdup(set->prefix);
   while ((*c++ = tolower(*c)));

   if (set->initial_paths) {
      paths = strdup(set->initial_paths);
      for (itr = paths; itr && *itr; itr = sep) {
         sep = strchr(itr, ',');
         if (sep)
            *sep++ = 0;
         for (tail = itr + strlen(itr) - 1; tail >= itr && *tail == '/'; tail--)
            *tail = 0;

         dir = configure_directory(itr);
         if (dir) {
            dir->next = data->dirs;
            data->dirs = dir;
         }
      }
   }

   if (paths)
      free(paths);
}

int PL_register(struct PluginSet *set, struct Plugin *plugin, void *opaque)
{
   struct PluginList *node;

   if (!set)
      return -1;
   if (!set->data)
      configure_set(set);
   if (!set->data)
      return -2;

   node = (struct PluginList*)malloc(sizeof(*node));
   memset(node, 0, sizeof(*node));
   node->plugin = plugin;
   node->opaque = opaque;
   node->next = set->data->plugins;
   set->data->plugins = node;

   return 0;
}

static struct PluginList *find_plugin(struct PluginSet *set, const char *name,
      int dont_load)
{
   struct PluginList *curr;
   struct PluginDirectoryList *dirs;
   char _name[256];
   char *name_lc = _name;
   char *c = name_lc;
   int len;
   char path[2048];

   if (!set)
      return NULL;
   if (!set->data)
      configure_set(set);
   if (!set->data)
      return NULL;

   for (curr = set->data->plugins; curr; curr = curr->next) {
      if (!strcasecmp(curr->plugin->name, name))
         return curr;
   }
   if (dont_load)
      return NULL;

   len = strlen(name);
   if (len >= sizeof(_name))
      c = name_lc = malloc(len + 1);
   strcpy(name_lc, name);
   while ((*c++ = tolower(*c)));

   for (dirs = set->data->dirs; dirs && dirs->dir; dirs = dirs->next) {
      snprintf(path, sizeof(path), "%s/%s-%s" PL_PLUGIN_SUFFIX, dirs->dir->path,
            set->data->name_lc, name_lc);
      path[sizeof(path)-1] = 0;
      load_plugin_file(path);
   }

   if (name_lc && name_lc != _name)
      free(name_lc);

   for (curr = set->data->plugins; curr; curr = curr->next) {
      if (!strcasecmp(curr->plugin->name, name))
         return curr;
   }

   return NULL;
}

void *PL_create_object(struct PluginSet *set, const char *name, void *cfg,
      void *extra)
{
   struct PluginList *plug;

   plug = find_plugin(set, name, 0);
   if (!plug || !plug->plugin || !plug->plugin->factory)
      return NULL;

   return plug->plugin->factory(cfg, plug->opaque, extra);
}

struct CFG_ParseObj *PL_config_for_plugin(struct PluginSet *set,
      const char *name, int defer_load)
{
   struct PluginList *plug;

   plug = find_plugin(set, name, defer_load);
   if (!plug || !plug->plugin)
      return NULL;

   return plug->plugin->cfg;
}

struct CFG_ParseObj *PL_objectlookup(
      const char *params, void *arg, int *buffer_obj)
{
   *buffer_obj = 0;
   return PL_config_for_plugin((struct PluginSet*)arg, params, *buffer_obj);
}

struct CFG_ParseObj *PL_objectlookup_w_buffer(
      const char *params, void *arg, int *buffer_obj)
{
   return PL_config_for_plugin((struct PluginSet*)arg, params, *buffer_obj);
}

void PL_preload_plugins(struct PluginSet *set)
{
   if (!set)
      return;
   if (!set->data)
      configure_set(set);
}
