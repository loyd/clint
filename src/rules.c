#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "clint.h"

#define RULES(XX)                                                             \
    XX(naming)                                                                \
    XX(lines)                                                                 \
    XX(indentation)                                                           \
    XX(whitespace)                                                            \
    XX(block)                                                                 \
    XX(runtime)

#define XX(name) extern struct rule_s name ## _rule;
RULES(XX)
#undef XX


static jmp_buf cfgbuf;
static struct rule_s *current;


static json_value *json_get(json_value *obj, const char *prop)
{
    assert(obj && obj->type == json_object);

    for (unsigned i = 0; i < obj->u.object.length; ++i)
        if (!strcmp(prop, obj->u.object.values[i].name))
            return obj->u.object.values[i].value;

    return NULL;
}


void cfg_fatal(const char *prop, const char *message)
{
    fprintf(stderr, "\"%s\" %s.\n", prop, message);
    longjmp(cfgbuf, 1);
}


json_type cfg_typeof(const char *prop)
{
    json_value *value = json_get(current->config, prop);
    return value ? value->type : json_none;
}


bool cfg_boolean(const char *prop)
{
    json_value *value = json_get(current->config, prop);

    if (!value)
        return false;

    if (value->type != json_boolean)
        cfg_fatal(prop, "must be a boolean");

    return value->u.boolean;
}


char *cfg_string(const char *prop)
{
    json_value *value = json_get(current->config, prop);

    if (!value)
        return NULL;

    if (value->type != json_string)
        cfg_fatal(prop, "must be a string");

    return value->u.string.ptr;
}


unsigned cfg_natural(const char *prop)
{
    json_value *value = json_get(current->config, prop);

    if (!value)
        return 0;

    if (value->type != json_integer || value->u.integer < 0)
        cfg_fatal(prop, "must be a natural number");

    return value->u.boolean;
}


char **cfg_strings(const char *prop)
{
    json_value *value = json_get(current->config, prop);
    unsigned len;
    char **res;

    if (!value)
        return NULL;

    if (value->type != json_array)
        cfg_fatal(prop, "must be an array of strings");

    len = value->u.array.length;
    res = new_vec(char *, len);

    for (unsigned i = 0; i < len; ++i)
    {
        json_value *item = value->u.array.values[i];

        if (item->type != json_string)
        {
            free_vec(res);
            cfg_fatal(prop, "must contain only strings");
        }

        vec_push(res, item->u.string.ptr);
    }

    return res;
}


bool configure_rules(void)
{
#define XX(name)                                                              \
    current = &name ## _rule;                                                 \
    if ((current->config = json_get(g_config, #name)))                        \
    {                                                                         \
        if (current->config->type != json_object)                             \
        {                                                                     \
            current->config = NULL;                                           \
            cfg_fatal(#name, "must be an object");                            \
        }                                                                     \
        current->configure();                                                 \
    }

    if (!setjmp(cfgbuf))
    {
        RULES(XX)
        return true;
    }
    else
        return false;
#undef XX
}


void check_rules(void)
{
#define XX(name)                                                              \
    if ((name ## _rule).config)                                               \
        (name ## _rule).check();

    RULES(XX)
#undef XX
}
