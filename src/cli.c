#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <json.h>

#include "clint.h"


static enum {OK, IMPERFECT, MINOR_ERR, MAJOR_ERR} retval = OK;
static enum {TOKENIZE, PARSE, CHECK} action = CHECK;
static const char *config = ".clintrc";


enum cmd_e {
    CMD_LIMIT,
    CMD_SHORTLY,
    CMD_CONFIG,
    CMD_NO_COLORS,
    CMD_VERBOSE,
    CMD_TOKENIZE,
    CMD_SHOW_TREE,
    CMD_UNSORTED,
    CMD_HELP,
    CMD_VERSION
};


struct option_s {
    enum cmd_e id;
    const char *command;
    const char abbrev;
    const char *brief;
    const char *argname;
};


static struct option_s options[] = {
    {CMD_LIMIT,      "limit",     'l',  "The maximum number of errors", "NUM"},
    {CMD_SHORTLY,    "shortly",   's',  "One-line output",               NULL},
    {CMD_CONFIG,     "config",    'c',  "Use FILE instead .clintrc",   "FILE"},
    {CMD_NO_COLORS,  "no-colors",  0,   "Disable colors for output",     NULL},
    {CMD_VERBOSE,    "verbose",   'v',  "Output errors during parsing",  NULL},
    {CMD_TOKENIZE,   "tokenize",   0,   "Tokenize file and exit",        NULL},
    {CMD_SHOW_TREE,  "show-tree",  0,   "Parse file and exit",           NULL},
    {CMD_UNSORTED,   "unsorted",   0,   "Disable output sorting",        NULL},
    {CMD_HELP,       "help",      'h',  "Display this help and exit",    NULL},
    {CMD_VERSION,    "version",   'V',  "Output version and exit",       NULL}
};

static int num_options = sizeof(options)/sizeof(*options);


static void display_help(void)
{
    static int brief_offset = 25;

    printf("Usage:\n"
           "  clint [OPTION]... [FILE]...\n\n"
           "Check style for the FILEs (the current directory by default).\n\n"
           "Options:\n");

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
           "  0  if OK,\n"
           "  1  if style warnings or (when --verbose) errors,\n"
           "  2  if minor problems (e.g., cannot read file),\n"
           "  3  if serious trouble (e.g., bad argument).\n");
}


static void display_version(void)
{
    printf("clint %s\n", VERSION);
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


static void process_option(struct option_s *opt, const char *arg)
{
    assert(opt);

    switch (opt->id)
    {
        case CMD_LIMIT:
        {
            int limit;
            if (sscanf(arg, "%d", &limit) < 1 || limit < 0)
            {
                fprintf(stderr, "Invalid argument of --%s.\n", opt->command);
                exit(MAJOR_ERR);
            }

            g_log_limit = limit;
            break;
        }

        case CMD_SHORTLY:
            g_log_mode |= LOG_SHORTLY;
            break;

        case CMD_CONFIG:
            config = arg;
            break;

        case CMD_NO_COLORS:
            g_log_mode &= ~LOG_COLOR;
            break;

        case CMD_VERBOSE:
            g_log_mode |= LOG_VERBOSE;
            break;

        case CMD_TOKENIZE:
            action = TOKENIZE;
            break;

        case CMD_SHOW_TREE:
            action = PARSE;
            break;

        case CMD_UNSORTED:
            g_log_mode &= ~LOG_SORTED;
            break;

        case CMD_HELP:
            display_help();
            exit(OK);

        case CMD_VERSION:
            display_version();
            exit(OK);

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

static int process_file(const char *fpath)
{
    FILE *fp;
    int size;

    if (!accept(fpath))
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

    if (g_log_mode & LOG_SORTED)
        print_errors_in_order();

    if (g_errors && retval == OK)
        retval = IMPERFECT;

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


static void tree_walk(const char *path)
{
    struct stat fstat;

    if (stat(path, &fstat))
        goto error;

    if (S_ISREG(fstat.st_mode))
    {
        process_file(path);
        return;
    }

    if (!S_ISDIR(fstat.st_mode))
        return;

    DIR *dir = opendir(path);

    if (!dir)
        goto error;

    for (;;)
    {
        struct dirent *entry = readdir(dir);

        if (!entry)
            break;

        // Skip hidden (".", "..", ".svn", ".git", ".hg" etc).
        if (entry->d_name[0] == '.')
            continue;

        char fpath[strlen(path) + strlen(entry->d_name) + 2];
        sprintf(fpath, "%s/%s", path, entry->d_name);
        tree_walk(fpath);
    }

    if (closedir(dir))
        goto error;

    return;

error:
    fprintf(stderr, "%s: %s.\n", path, strerror(errno));
    retval = MINOR_ERR;
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
        tree_walk(files[i]);

    return retval;
}
