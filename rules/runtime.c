#include <stdlib.h>
#include <string.h>

#include "clint.h"

static bool require_threadsafe_fn;
static bool require_safe_fn;
static bool require_sized_int;
static bool require_sizeof_as_fn;


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


static void configure(void)
{
    require_threadsafe_fn = cfg_boolean("require-threadsafe-fn");
    require_safe_fn = cfg_boolean("require-safe-fn");
    require_sized_int = cfg_boolean("require-sized-int");
    require_sizeof_as_fn = cfg_boolean("require_sizeof_as_fn");
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
        bsearch(key, threadunsafe, sizeof(threadunsafe) / sizeof(*threadunsafe),
            sizeof(*threadunsafe), (int (*)(const void *, const void *))strcmp))
        add_warn_at(ident->start, "Consider using %s_r instead of %s",
                    key, key);

    if (require_safe_fn)
    {
        char *res = bsearch(key, unsafe, sizeof(unsafe) / sizeof(*unsafe),
            sizeof(*unsafe), (int (*)(const void *, const void *))strcmp);

        if (res)
            add_warn_at(ident->start, "Consider using %s instead of %s",
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
        add_warn_at(g_tokens[tree->start].start,
            is_unsigned ? "Use uint16_t/uint64_t/etc, rather than C type"
                        : "Use int16_t/int64_t/etc, rather than C type");
}


static void process_sizeof(struct unary_s *tree)
{
    if (g_tokens[tree->op].kind != KW_SIZEOF)
        return;

    if (g_tokens[tree->op + 1].kind != PN_LPAREN)
        add_warn_at(g_tokens[tree->op].end, "Use sizeof like function");
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
