#include <stdlib.h>
#include <string.h>

#include "clint.h"

static bool require_threadsafe_fn = false;
static bool require_safe_fn = false;
static bool require_sized_int = false;
static bool require_sizeof_as_fn = false;


#define MAX_WORD_SZ 15

// Must be sorted.
static char threadunsafe[][MAX_WORD_SZ] = {
    "asctime",
    "ctime",
    "getgrgid",
    "getgrnam",
    "getlogin",
    "getpwnam",
    "getpwuid",
    "gmtime",
    "localtime",
    "rand",
    "readdir",
    "strtok",
    "ttyname"
};

// Must be sorted by first.
static char unsafe[][2][MAX_WORD_SZ] = {
    {"gets", "fgets"},
    {"sprintf", "snprintf"},
    {"strcat", "strncat"},
    {"strcpy", "strncpy"},
    {"vsprintf", "vsnprintf"}
};


static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "require-threadsafe-fn", json_boolean)))
        require_threadsafe_fn = value->u.boolean;

    if ((value = json_get(config, "require-safe-fn", json_boolean)))
        require_safe_fn = value->u.boolean;

    if ((value = json_get(config, "require-sized-int", json_boolean)))
        require_sized_int = value->u.boolean;

    if ((value = json_get(config, "require-sizeof-as-fn", json_boolean)))
        require_sizeof_as_fn = value->u.boolean;
}


static void process_call(struct call_s *tree)
{
    static char key[MAX_WORD_SZ];
    token_t *ident;
    int len;

    if (tree->left->type != IDENTIFIER)
        return;

    ident = &g_tokens[tree->left->start];
    len = ident->end.pos - ident->start.pos + 1;

    memcpy(key, ident->start.pos, len);
    key[len] = '\0';

    if (require_threadsafe_fn &&
        bsearch(key, threadunsafe, sizeof(threadunsafe)/sizeof(*threadunsafe),
            sizeof(*threadunsafe), (int (*)(const void *, const void *))strcmp))
        warn_at(&ident->start, "Consider using %s_r instead of %s", key, key);

    if (require_safe_fn)
    {
        char *res = bsearch(key, unsafe, sizeof(unsafe)/sizeof(*unsafe),
            sizeof(*unsafe), (int (*)(const void *, const void *))strcmp);

        if (res)
            warn_at(&ident->start, "Consider using %s instead of %s",
                    res + MAX_WORD_SZ, key);
    }
}


static void process_id_type(struct id_type_s *tree)
{
    bool ok = true;
    bool is_unsigned = false;

    for (unsigned i = 0; i < vec_len(tree->names); ++i)
        switch (g_tokens[tree->names[i]].kind)
        {
            case KW_LONG:
            case KW_SHORT:
                ok = false;
                break;
            case KW_UNSIGNED:
                is_unsigned = true;
                break;
            case KW_INT:
                break;
            default:
                ok = true;
        }

    if (!ok)
        warn_at(&g_tokens[tree->start].start,
            is_unsigned ? "Use uint16_t/uint64_t/etc, rather than C type"
                        : "Use int16_t/int64_t/etc, rather than C type");
}


static void process_sizeof(struct unary_s *tree)
{
    if (g_tokens[tree->op].kind != KW_SIZEOF)
        return;

    if (g_tokens[tree->op + 1].kind != PN_LPAREN)
        warn_at(&g_tokens[tree->op].end, "Use sizeof as function");
}


static void check(void)
{
    if (require_threadsafe_fn || require_safe_fn)
        iterate_by_type(CALL, process_call);

    if (require_sized_int)
        iterate_by_type(ID_TYPE, process_id_type);

    if (require_sizeof_as_fn)
        iterate_by_type(UNARY, process_sizeof);
}


REGISTER_RULE(runtime, configure, check);
