#ifndef JSON_H
#define JSON_H

extern int json_get_string_prop(const char *json, const char *prop,
      const char **out);
extern int json_get_int_prop(const char *json, const char *prop, int *out);

#endif
