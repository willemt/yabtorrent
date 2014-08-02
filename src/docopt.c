#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#endif


typedef struct {
    /* arguments */
    char *new_torrent_file;
    char *torrent_file;
    /* options without arguments */
    int blocklist;
    int encryption_preferred;
    int encryption_required;
    int encryption_tolerated;
    int help;
    int no_blocklist;
    int no_portmap;
    int no_uplimit;
    int portmap;
    int private;
    int scrape;
    int tos;
    int verbose;
    /* options with arguments */
    char *announce;
    char *comment;
    char *config_directory;
    char *downlimit;
    char *download_dir;
    char *finish;
    char *info;
    char *new;
    char *port;
    char *uplimit;
    /* special */
    const char *usage_pattern;
    const char *help_message;
} DocoptArgs;

const char help_message[] =
"yabtorrent - a bittorrent client\n"
"\n"
"Usage:\n"
"  yabtorrent --help\n"
"  yabtorrent --info <torrent-file>\n"
//"  yabtorrent --scrape <torrent-file>\n"
//"  yabtorrent <new-torrent-file> --new=<sourcefile>\n"
//"      [--announce=<url>]\n"
//"      [--comment=<comment-text>]\n"
//"      [--private]\n"
"  yabtorrent <torrent-file>\n"
//"      [--blocklist | --no-blocklist]\n"
//"      [--downlimit=<downlimit-number> | -no-downlimit]\n"
//"      [--encryption-required | --encryption-preferred | --encryption-tolerated]\n"
//"      [--finish=<script>]\n"
//"      [--config-directory=<config-directory>]\n"
"      [--help]\n"
//"      [--portmap | --no-portmap]\n"
"      [--port=<listen-port>]\n"
//"      [--tos]\n"
//"      [--uplimit=<uplimit-number> | --no-uplimit]\n"
"      [--verbose]\n"
"      [--download-dir=<download-dir>] \n"
"\n"
"Options:\n"
//"  -a --announce=<announce-url>  When creating a new torrent, set its announce URL.\n"
//"  -b --blocklist  Enable peer blocklists.\n"
//"  -B --no-blocklist  Disable blocklists.\n"
//"  -c --comment=<comment-text>  When creating a new torrent, set its comment field.\n"
//"  -d --downlimit=<downlimit-number>  Set the maximum download speed in KB/s.\n"
//"  -D --no-downlimit  Don’t limit the download speed.\n"
//"  -er --encryption-required  Encrypt all peer connections.\n"
//"  -ep --encryption-preferred  Prefer encrypted peer connetions.\n"
//"  -et --encryption-tolerated  Prefer unencrypted peer connections.\n"
//"  -f --finish=<script>  Set a script to run when the torrent finishes.\n"
//"  -g --config-dir=<directory>  Where to look for configuration files.\n"
"  -h --help  Prints a short usage summary.\n"
"  -i --info=<torrent-file>  Shows torrent details and exit.\n"
//"  -m --portmap  Enable portmapping via NAT-PMP or UPnP.\n"
//"  -M --no-portmap  Disable portmapping.\n"
//"  -n --new=<sourcefile>  Create torrent from the specified file or directory.\n"
"  -p --port=<listen-port>  Set the port to listen for incoming peers. (Default: 51413).\n"
//"  -r --private  When creating a new torrent, set its ’private’ flag.\n"
//"  -s --scrape  Print the current number of seeders and leechers for the specified torrent.\n"
//"  -t --tos  Set the peer socket TOS for local router-based traffic shaping.\n"
//"  -u --uplimit=<uplimit-number>  Set the maximum upload speed in KB/s.\n"
//"  -U --no-uplimit  Don’t limit the upload speed.\n"
"  -v --verify  Verify the downloaded torrent data.\n"
"  -w --download-dir=<download-dir>  Where to save downloaded data.\n"
"";

const char usage_pattern[] =
"Usage:\n"
"  yabtorrent --help\n"
"  yabtorrent --info <torrent-file>\n"
//"  yabtorrent --scrape <torrent-file>\n"
//"  yabtorrent <new-torrent-file> --new=<sourcefile>\n"
//"      [--announce=<url>]\n"
//"      [--comment=<comment-text>]\n"
//"      [--private]\n"
"  yabtorrent <torrent-file>\n"
//"      [--blocklist | --no-blocklist]\n"
//"      [--downlimit=<downlimit-number> | -no-downlimit]\n"
//"      [--encryption-required | --encryption-preferred | --encryption-tolerated]\n"
//"      [--finish=<script>]\n"
//"      [--config-directory=<config-directory>]\n"
"      [--help]\n"
//"      [--portmap | --no-portmap]\n"
"      [--port=<listen-port>]\n"
//"      [--tos]\n"
//"      [--uplimit=<uplimit-number> | --no-uplimit]\n"
"      [--verbose]\n"
"      [--download-dir=<download-dir>]";

typedef struct {
    const char *name;
    bool value;
} Command;

typedef struct {
    const char *name;
    char *value;
    char **array;
} Argument;

typedef struct {
    const char *oshort;
    const char *olong;
    bool argcount;
    bool value;
    char *argument;
} Option;

typedef struct {
    int n_commands;
    int n_arguments;
    int n_options;
    Command *commands;
    Argument *arguments;
    Option *options;
} Elements;


/*
 * Tokens object
 */

typedef struct Tokens {
    int argc;
    char **argv;
    int i;
    char *current;
} Tokens;

Tokens tokens_new(int argc, char **argv) {
    Tokens ts = {argc, argv, 0, argv[0]};
    return ts;
}

Tokens* tokens_move(Tokens *ts) {
    if (ts->i < ts->argc) {
        ts->current = ts->argv[++ts->i];
    }
    if (ts->i == ts->argc) {
        ts->current = NULL;
    }
    return ts;
}


/*
 * ARGV parsing functions
 */

int parse_doubledash(Tokens *ts, Elements *elements) {
    //int n_commands = elements->n_commands;
    //int n_arguments = elements->n_arguments;
    //Command *commands = elements->commands;
    //Argument *arguments = elements->arguments;

    // not implemented yet
    // return parsed + [Argument(None, v) for v in tokens]
    return 0;
}

int parse_long(Tokens *ts, Elements *elements) {
    int i;
    int len_prefix;
    int n_options = elements->n_options;
    char *eq = strchr(ts->current, '=');
    Option *option;
    Option *options = elements->options;

    len_prefix = (eq-(ts->current))/sizeof(char);
    for (i=0; i < n_options; i++) {
        option = &options[i];
        if (!strncmp(ts->current, option->olong, len_prefix))
            break;
    }
    if (i == n_options) {
        // TODO '%s is not a unique prefix
        fprintf(stderr, "%s is not recognized\n", ts->current);
        return 1;
    }
    tokens_move(ts);
    if (option->argcount) {
        if (eq == NULL) {
            if (ts->current == NULL) {
                fprintf(stderr, "%s requires argument\n", option->olong);
                return 1;
            }
            option->argument = ts->current;
            tokens_move(ts);
        } else {
            option->argument = eq + 1;
        }
    } else {
        if (eq != NULL) {
            fprintf(stderr, "%s must not have an argument\n", option->olong);
            return 1;
        }
        option->value = true;
    }
    return 0;
}

int parse_shorts(Tokens *ts, Elements *elements) {
    char *raw;
    int i;
    int n_options = elements->n_options;
    Option *option;
    Option *options = elements->options;

    raw = &ts->current[1];
    tokens_move(ts);
    while (raw[0] != '\0') {
        for (i=0; i < n_options; i++) {
            option = &options[i];
            if (option->oshort != NULL && option->oshort[1] == raw[0])
                break;
        }
        if (i == n_options) {
            // TODO -%s is specified ambiguously %d times
            fprintf(stderr, "-%c is not recognized\n", raw[0]);
            return 1;
        }
        raw++;
        if (!option->argcount) {
            option->value = true;
        } else {
            if (raw[0] == '\0') {
                if (ts->current == NULL) {
                    fprintf(stderr, "%s requires argument\n", option->oshort);
                    return 1;
                }
                raw = ts->current;
                tokens_move(ts);
            }
            option->argument = raw;
            break;
        }
    }
    return 0;
}

int parse_argcmd(Tokens *ts, Elements *elements) {
    int i;
    int n_commands = elements->n_commands;
    //int n_arguments = elements->n_arguments;
    Command *command;
    Command *commands = elements->commands;
    //Argument *arguments = elements->arguments;

    for (i=0; i < n_commands; i++) {
        command = &commands[i];
        if (!strcmp(command->name, ts->current)){
            command->value = true;
            tokens_move(ts);
            return 0;
        }
    }
    // not implemented yet, just skip for now
    // parsed.append(Argument(None, tokens.move()))
    /*fprintf(stderr, "! argument '%s' has been ignored\n", ts->current);
    fprintf(stderr, "  '");
    for (i=0; i<ts->argc ; i++)
        fprintf(stderr, "%s ", ts->argv[i]);
    fprintf(stderr, "'\n");*/
    tokens_move(ts);
    return 0;
}

int parse_args(Tokens *ts, Elements *elements) {
    int ret;

    while (ts->current != NULL) {
        if (strcmp(ts->current, "--") == 0) {
            ret = parse_doubledash(ts, elements);
            if (!ret) break;
        } else if (ts->current[0] == '-' && ts->current[1] == '-') {
            ret = parse_long(ts, elements);
        } else if (ts->current[0] == '-' && ts->current[1] != '\0') {
            ret = parse_shorts(ts, elements);
        } else
            ret = parse_argcmd(ts, elements);
        if (ret) return ret;
    }
    return 0;
}

int elems_to_args(Elements *elements, DocoptArgs *args, bool help,
                  const char *version){
    Command *command;
    Argument *argument;
    Option *option;
    int i;

    /* options */
    for (i=0; i < elements->n_options; i++) {
        option = &elements->options[i];
        if (help && option->value && !strcmp(option->olong, "--help")) {
            printf("%s", args->help_message);
            return 1;
        } else if (version && option->value &&
                   !strcmp(option->olong, "--version")) {
            printf("%s\n", version);
            return 1;
        } else if (!strcmp(option->olong, "--blocklist")) {
            args->blocklist = option->value;
        } else if (!strcmp(option->olong, "--encryption-preferred")) {
            args->encryption_preferred = option->value;
        } else if (!strcmp(option->olong, "--encryption-required")) {
            args->encryption_required = option->value;
        } else if (!strcmp(option->olong, "--encryption-tolerated")) {
            args->encryption_tolerated = option->value;
        } else if (!strcmp(option->olong, "--help")) {
            args->help = option->value;
        } else if (!strcmp(option->olong, "--no-blocklist")) {
            args->no_blocklist = option->value;
        } else if (!strcmp(option->olong, "--no-portmap")) {
            args->no_portmap = option->value;
        } else if (!strcmp(option->olong, "--no-uplimit")) {
            args->no_uplimit = option->value;
        } else if (!strcmp(option->olong, "--portmap")) {
            args->portmap = option->value;
        } else if (!strcmp(option->olong, "--private")) {
            args->private = option->value;
        } else if (!strcmp(option->olong, "--scrape")) {
            args->scrape = option->value;
        } else if (!strcmp(option->olong, "--tos")) {
            args->tos = option->value;
        } else if (!strcmp(option->olong, "--verbose")) {
            args->verbose = option->value;
        } else if (!strcmp(option->olong, "--announce")) {
            args->announce = option->argument;
        } else if (!strcmp(option->olong, "--comment")) {
            args->comment = option->argument;
        } else if (!strcmp(option->olong, "--config-directory")) {
            args->config_directory = option->argument;
        } else if (!strcmp(option->olong, "--downlimit")) {
            args->downlimit = option->argument;
        } else if (!strcmp(option->olong, "--download-dir")) {
            args->download_dir = option->argument;
        } else if (!strcmp(option->olong, "--finish")) {
            args->finish = option->argument;
        } else if (!strcmp(option->olong, "--info")) {
            args->info = option->argument;
        } else if (!strcmp(option->olong, "--new")) {
            args->new = option->argument;
        } else if (!strcmp(option->olong, "--port")) {
            args->port = option->argument;
        } else if (!strcmp(option->olong, "--uplimit")) {
            args->uplimit = option->argument;
        }
    }
    /* commands */
    for (i=0; i < elements->n_commands; i++) {
        command = &elements->commands[i];
    }
    /* arguments */
    for (i=0; i < elements->n_arguments; i++) {
        argument = &elements->arguments[i];
        if (!strcmp(argument->name, "<new-torrent-file>")) {
            args->new_torrent_file = argument->value;
        } else if (!strcmp(argument->name, "<torrent-file>")) {
            args->torrent_file = argument->value;
        }
    }
    return 0;
}


/*
 * Main docopt function
 */

DocoptArgs docopt(int argc, char *argv[], bool help, const char *version) {
    DocoptArgs args = {
        NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        usage_pattern, help_message
    };
    Tokens ts;
    Command commands[] = {
    };
    Argument arguments[] = {
        {"<new-torrent-file>", NULL, NULL},
        {"<torrent-file>", NULL, NULL}
    };
    Option options[] = {
        {"-b", "--blocklist", 0, 0, NULL},
        {"-ep", "--encryption-preferred", 0, 0, NULL},
        {"-er", "--encryption-required", 0, 0, NULL},
        {"-et", "--encryption-tolerated", 0, 0, NULL},
        {"-h", "--help", 0, 0, NULL},
        {"-B", "--no-blocklist", 0, 0, NULL},
        {"-M", "--no-portmap", 0, 0, NULL},
        {"-U", "--no-uplimit", 0, 0, NULL},
        {"-m", "--portmap", 0, 0, NULL},
        {"-r", "--private", 0, 0, NULL},
        {"-s", "--scrape", 0, 0, NULL},
        {"-t", "--tos", 0, 0, NULL},
        {NULL, "--verbose", 0, 0, NULL},
        {"-a", "--announce", 1, 0, NULL},
        {"-c", "--comment", 1, 0, NULL},
        {NULL, "--config-directory", 1, 0, NULL},
        {"-d", "--downlimit", 1, 0, NULL},
        {"-w<download-dir>", "--download-dir", 1, 0, NULL},
        {"-f", "--finish", 1, 0, NULL},
        {"-i", "--info", 1, 0, NULL},
        {"-n", "--new", 1, 0, NULL},
        {"-p", "--port", 1, 0, NULL},
        {"-u", "--uplimit", 1, 0, NULL}
    };
    Elements elements = {0, 2, 23, commands, arguments, options};

    ts = tokens_new(argc, argv);
    if (parse_args(&ts, &elements))
        exit(EXIT_FAILURE);
    if (elems_to_args(&elements, &args, help, version))
        exit(EXIT_SUCCESS);
    return args;
}
