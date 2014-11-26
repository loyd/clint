#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "clint.h"

#define RULES(XX)                                                             \
    XX(naming)                                                                \
    XX(lines)

#define XX(name) extern struct rule_s name ## _rule;
RULES(XX)
#undef XX


static jmp_buf cnfbuf;
static struct rule_s *current;


static const char *stringify_json_type(json_type type)
{
    switch (type)
    {
        case json_none:    return "none";
        case json_object:  return "object";
        case json_array:   return "array";
        case json_integer: return "integer";
        case json_double:  return "double";
        case json_string:  return "string";
        case json_boolean: return "boolean";
        case json_null:    return "null";
    }
}


json_value *json_get(json_value *obj, const char *key, json_type type)
{
    assert(obj);

    if (obj->type != json_object)
    {
        fprintf(stderr, "\"%s\" must be object.\n", current->name);
        longjmp(cnfbuf, 1);
    }

    for (unsigned i = 0; i < obj->u.object.length; ++i)
        if (!strcmp(key, obj->u.object.values[i].name))
            if (!type || obj->u.object.values[i].value->type == type)
                return obj->u.object.values[i].value;
            else
            {
                fprintf(stderr, "\"%s:%s\" must be %s.\n", current->name,
                        key, stringify_json_type(type));
                longjmp(cnfbuf, 1);
            }

    return NULL;
}


bool configure_rules(void)
{
#define XX(name)                                                              \
    (current = &name ## _rule)->configure(json_get(g_config, #name, json_none));

    if (!setjmp(cnfbuf))
    {
        RULES(XX)
        return true;
    }
    else
        return false;
#undef XX
}


bool check_rules(void)
{
#define XX(name) (current = &name ## _rule)->check();
    RULES(XX)
#undef XX
    return true;
}
