#include <assert.h>
#include <errno.h>
#include <ftw.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json.h>

#include "clint.h"


static enum {OK, IMPERFECT, MINOR_ERR, MAJOR_ERR} retval = OK;


enum cmd_e {
    CMD_HELP,
    CMD_CONFIG,
    CMD_SHORTLY,
    CMD_VERBOSE,
    CMD_NO_COLORS,
    CMD_TOKENIZE,
    CMD_SHOW_TREE
};


struct option_s {
    enum cmd_e id;
    const char *command;
    const char abbrev;
    const char *brief;
    const char *argname;
};


static struct option_s options[] = {
    {CMD_HELP,       "help",      'h',  "This help information",         NULL},
    {CMD_CONFIG,     "config",    'c',  "Use FILE instead .clintrc",   "FILE"},
    {CMD_SHORTLY,    "shortly",   's',  "One-line output",               NULL},
    {CMD_VERBOSE,    "verbose",   'v',  "Output errors during run",      NULL},
    {CMD_NO_COLORS,  "no-colors",  0,   "Disable colors for output",     NULL},
    {CMD_TOKENIZE,   "tokenize",   0,   "Tokenize file and show tokens", NULL},
    {CMD_SHOW_TREE,  "show-tree",  0,   "Parse file and show tree",      NULL}
};

static int num_options = sizeof(options)/sizeof(*options);


static void display_help(void)
{
    static int brief_offset = 25;

    printf("Usage: clint [OPTION]... [FILE]...\n"
           "Check style for the FILEs (the current directory by default).\n\n"
           "Mandatory argument to long option is mandatory for short too.\n");

    for (int i = 0; i < num_options; ++i)
    {
        struct option_s *opt = &options[i];

        if (opt->abbrev)
            printf("  -%c, ", opt->abbrev);
        else
            printf("      ");

        if (opt->argname)
            printf("--%s %-*s %s.\n", opt->command, brief_offset - 6 -
                   (int)strlen(opt->command), opt->argname, opt->brief);
        else
            printf("--%-*s %s.\n", brief_offset - 5, opt->command, opt->brief);
    }

    printf("\nExit status:\n"
           " 0  if OK,\n"
           " 1  if style warnings or (when --verbose) errors,\n"
           " 2  if minor problems (e.g., cannot read file),\n"
           " 3  if serious trouble (e.g., bad argument).\n");
}


static struct option_s *find_command(const char *command)
{
    for (int i = 0; i < num_options; ++i)
        if (!strcmp(command, options[i].command))
            return &options[i];

    return NULL;
}


static struct option_s *find_abbrev(char abbrev)
{
    for (int i = 0; i < num_options; ++i)
        if (abbrev == options[i].abbrev)
            return &options[i];

    return NULL;
}


static const char *config = ".clintrc";

static enum {TOKENIZE, PARSE, CHECK} action = CHECK;

static void process_option(struct option_s *opt, const char *arg)
{
    assert(opt);

    switch (opt->id)
    {
        case CMD_HELP:
            display_help();
            exit(OK);

        case CMD_CONFIG:
            config = arg;
            break;

        case CMD_VERBOSE:
            set_log_level(LOG_ERROR);
            break;

        case CMD_TOKENIZE:
            action = TOKENIZE;
            break;

        case CMD_SHOW_TREE:
            action = PARSE;
            break;

        default:
            fprintf(stderr, "Option --%s isn't yet implemented, sorry.\n",
                    opt->command);
    }
}


static bool accept(const char *fpath)
{
    //#TODO: add custom pattern for `fpath`.
    char *ext = strrchr(fpath, '.');
    return ext && (!strcmp(ext, ".c") || !strcmp(ext, ".h"));
}


#define CHECK(x) if (!(x)) goto error

static int process_file(const char *fpath, const struct stat *sb, int type)
{
    //#TODO: what about skipping `.svn`, `.git` etc.?
    FILE *fp;
    int size;

    if (!(type == FTW_F && accept(fpath)))
        return 0;

    g_filename = xstrdup(fpath);

    CHECK(fp = fopen(fpath, "r"));

    // Determine the size.
    CHECK(!fseek(fp, 0, SEEK_END));
    CHECK((size = ftell(fp)) >= 0);
    CHECK(!fseek(fp, 0, SEEK_SET));

    // Read all content from the file.
    g_data = xmalloc(size + 1);
    CHECK(fread(g_data, 1, size, fp) == (size_t)size);
    g_data[size] = '\0';

    CHECK(fclose(fp) != EOF);

    // Do something.
    if (action == TOKENIZE)
    {
        init_lexer();
        tokenize();
        char *str = stringify_tokens();
        printf("%s: (%lu tokens)\n%s\n", fpath, vec_len(g_tokens), str);
        free(str);
    }
    else if (action == PARSE)
    {
        init_parser();
        parse();
        char *str = stringify_tree();
        printf("%s:\n%s\n", fpath, str);
        free(str);
    }
    else
    {
        assert(action == CHECK);
        init_parser();
        parse();
        if (!check_rules())
            retval = IMPERFECT;
    }

    printf("Done processing %s.\n", fpath);
    reset_state();
    errno = 0;
    return 0;

error:
    fprintf(stderr, "%s: %s.\n", fpath, strerror(errno));
    retval = MINOR_ERR;
    reset_state();
    errno = 0;
    return 0;
}


static void load_config(void)
{
    FILE *fp;
    int size;
    char *data;
    char errbuf[512];

    CHECK(fp = fopen(config, "r"));

    // Determine the size.
    CHECK(!fseek(fp, 0, SEEK_END));
    CHECK((size = ftell(fp)) >= 0);
    CHECK(!fseek(fp, 0, SEEK_SET));

    // Read all content from the file.
    data = xmalloc(size + 1);
    data[size] = '\0';

    CHECK(fread(data, 1, size, fp) == (size_t)size);
    CHECK(fclose(fp) != EOF);

    // Parse file as json.
    g_config = json_parse_ex(&(json_settings){
        .settings = json_enable_comments
    }, data, size, errbuf);

    if (!g_config)
    {
        printf("Error while parsing config while: %s.\n", errbuf);
        exit(MAJOR_ERR);
    }

    // Configure all rules.
    if (!configure_rules())
        exit(MAJOR_ERR);

    free(data);
    return;

error:
    fprintf(stderr, "%s: %s.\n", config, strerror(errno));
    exit(MAJOR_ERR);
}


int main(int argc, const char *argv[])
{
    const char **files = new_vec(char *, 10);

    // Parse arguments.
    for (int i = 1; i < argc; ++i)
    {
        struct option_s *opt;

        // File (or directory).
        if (argv[i][0] != '-')
        {
            vec_push(files, argv[i]);
            continue;
        }

        // Long option.
        if (argv[i][1] == '-')
        {
            opt = find_command(&argv[i][2]);

            if (!opt)
            {
                fprintf(stderr, "Unknown option %s.\n", argv[i]);
                return MAJOR_ERR;
            }

            if (opt->argname && i + 1 == argc)
            {
                fprintf(stderr, "Option --%s requires argument.\n",
                        opt->command);
                return MAJOR_ERR;
            }

            process_option(opt, opt->argname ? argv[++i] : NULL);
            continue;
        }

        // Short option, otherwise.
        for (int j = 1; argv[i][j]; ++j)
        {
            opt = find_abbrev(argv[i][j]);

            if (!opt)
            {
                fprintf(stderr, "Unknown option -%c.\n", argv[i][j]);
                return MAJOR_ERR;
            }

            if (opt->argname && (argv[i][j+1] || i + 1 == argc))
            {
                fprintf(stderr, "Option -%c requires argument.\n",
                        opt->abbrev);
                return MAJOR_ERR;
            }

            process_option(opt, opt->argname ? argv[++i] : NULL);

            if (opt->argname)
                break;
        }
    }

    if (action == CHECK)
        load_config();

    // Process files.
    if (vec_len(files) == 0)
        vec_push(files, ".");

    for (unsigned i = 0; i < vec_len(files); ++i)
    {
        errno = 0;
        ftw(files[i], process_file, 20);
        if (errno)
        {
            printf("%s: %s.\n", files[i], strerror(errno));
            retval = MINOR_ERR;
        }
    }

    return retval;
}
