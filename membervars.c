/*
 * membervars - convert member variable naming conventions.
 * Copyright (C) 2026 Lenik <membervars@bodz.net>
 * License: AGPL-3.0-or-later with additional anti-AI statement (see LICENSE).
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <bas/log/uselog.h>
#include <bas/log/deflog.h>

#define TOOL_VERSION "0.1.0"

DEFINE_LOGGER;

typedef enum {
    MODE_NONE = 0,
    MODE_TO_TRAILING_UNDERSCORE, /* m_<name> -> <name>_ */
    MODE_TO_M_PREFIX             /* <name>_ -> m_<name> */
} convert_mode_t;

typedef enum {
    STATE_NORMAL = 0,
    STATE_LINE_COMMENT,
    STATE_BLOCK_COMMENT,
    STATE_STRING,
    STATE_CHAR
} parse_state_t;

static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] files...\n\n", program_name);
    printf("Options:\n");
    printf("    -u      Convert member vars from m_<name> to <name>_\n");
    printf("    -m      Convert member vars from <name>_ to m_<name>\n");
    printf("    -v, --verbose\n");
    printf("    -q, --quiet\n");
    printf("    -h, --help\n");
    printf("    --version\n");
}

static bool is_ident_start(unsigned char c) {
    return (c == '_') || isalpha(c);
}

static bool is_ident_char(unsigned char c) {
    return (c == '_') || isalnum(c);
}

static char *slurp_file(const char *path, size_t *size_out) {
    FILE *fp = fopen(path, "rb");
    char *buf;
    long len;

    if (!fp) {
        logerror_fmt("failed to open '%s': %s", path, strerror(errno));
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        logerror_fmt("failed to seek '%s': %s", path, strerror(errno));
        fclose(fp);
        return NULL;
    }

    len = ftell(fp);
    if (len < 0) {
        logerror_fmt("failed to tell '%s': %s", path, strerror(errno));
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        logerror_fmt("failed to rewind '%s': %s", path, strerror(errno));
        fclose(fp);
        return NULL;
    }

    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        logerror_fmt("out of memory reading '%s'", path);
        fclose(fp);
        return NULL;
    }

    if (len > 0) {
        size_t got = fread(buf, 1, (size_t)len, fp);
        if (got != (size_t)len) {
            logerror_fmt("failed to read '%s'", path);
            free(buf);
            fclose(fp);
            return NULL;
        }
    }

    buf[len] = '\0';
    fclose(fp);

    if (size_out) {
        *size_out = (size_t)len;
    }
    return buf;
}

static bool append_buf(char **out, size_t *len, size_t *cap, const char *src, size_t src_len) {
    size_t needed;
    char *grown;

    if (src_len == 0) {
        return true;
    }

    needed = *len + src_len + 1;
    if (needed > *cap) {
        size_t new_cap = (*cap == 0) ? 1024 : *cap;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        grown = (char *)realloc(*out, new_cap);
        if (!grown) {
            return false;
        }
        *out = grown;
        *cap = new_cap;
    }

    memcpy(*out + *len, src, src_len);
    *len += src_len;
    (*out)[*len] = '\0';
    return true;
}

static bool convert_identifier(const char *token,
                               size_t token_len,
                               convert_mode_t mode,
                               char **converted,
                               size_t *converted_len) {
    if (mode == MODE_TO_TRAILING_UNDERSCORE) {
        if (token_len > 2 && token[0] == 'm' && token[1] == '_' && is_ident_start((unsigned char)token[2])) {
            size_t base_len = token_len - 2;
            char *out = (char *)malloc(base_len + 2);
            if (!out) {
                return false;
            }
            memcpy(out, token + 2, base_len);
            out[base_len] = '_';
            out[base_len + 1] = '\0';
            *converted = out;
            *converted_len = base_len + 1;
            return true;
        }
    } else if (mode == MODE_TO_M_PREFIX) {
        if (token_len > 1 && token[token_len - 1] == '_' && is_ident_start((unsigned char)token[0])) {
            size_t base_len = token_len - 1;
            char *out = (char *)malloc(base_len + 3);
            if (!out) {
                return false;
            }
            out[0] = 'm';
            out[1] = '_';
            memcpy(out + 2, token, base_len);
            out[base_len + 2] = '\0';
            *converted = out;
            *converted_len = base_len + 2;
            return true;
        }
    }

    *converted = NULL;
    *converted_len = 0;
    return true;
}

static bool transform_contents(const char *in,
                               size_t in_len,
                               convert_mode_t mode,
                               char **out,
                               size_t *out_len,
                               size_t *change_count) {
    parse_state_t state = STATE_NORMAL;
    size_t i = 0;
    char *result = NULL;
    size_t len = 0;
    size_t cap = 0;
    size_t changes = 0;

    while (i < in_len) {
        char c = in[i];

        if (state == STATE_NORMAL) {
            if (c == '/' && i + 1 < in_len && in[i + 1] == '/') {
                if (!append_buf(&result, &len, &cap, "//", 2)) {
                    free(result);
                    return false;
                }
                i += 2;
                state = STATE_LINE_COMMENT;
                continue;
            }
            if (c == '/' && i + 1 < in_len && in[i + 1] == '*') {
                if (!append_buf(&result, &len, &cap, "/*", 2)) {
                    free(result);
                    return false;
                }
                i += 2;
                state = STATE_BLOCK_COMMENT;
                continue;
            }
            if (c == '"') {
                if (!append_buf(&result, &len, &cap, &in[i], 1)) {
                    free(result);
                    return false;
                }
                i++;
                state = STATE_STRING;
                continue;
            }
            if (c == '\'') {
                if (!append_buf(&result, &len, &cap, &in[i], 1)) {
                    free(result);
                    return false;
                }
                i++;
                state = STATE_CHAR;
                continue;
            }
            if (is_ident_start((unsigned char)c)) {
                size_t start = i;
                size_t token_len;
                char *converted = NULL;
                size_t converted_len = 0;

                i++;
                while (i < in_len && is_ident_char((unsigned char)in[i])) {
                    i++;
                }
                token_len = i - start;

                if (!convert_identifier(in + start, token_len, mode, &converted, &converted_len)) {
                    free(result);
                    return false;
                }

                if (converted) {
                    if (!append_buf(&result, &len, &cap, converted, converted_len)) {
                        free(converted);
                        free(result);
                        return false;
                    }
                    changes++;
                    free(converted);
                } else {
                    if (!append_buf(&result, &len, &cap, in + start, token_len)) {
                        free(result);
                        return false;
                    }
                }
                continue;
            }

            if (!append_buf(&result, &len, &cap, &in[i], 1)) {
                free(result);
                return false;
            }
            i++;
            continue;
        }

        if (state == STATE_LINE_COMMENT) {
            if (!append_buf(&result, &len, &cap, &in[i], 1)) {
                free(result);
                return false;
            }
            if (in[i] == '\n') {
                state = STATE_NORMAL;
            }
            i++;
            continue;
        }

        if (state == STATE_BLOCK_COMMENT) {
            if (i + 1 < in_len && in[i] == '*' && in[i + 1] == '/') {
                if (!append_buf(&result, &len, &cap, "*/", 2)) {
                    free(result);
                    return false;
                }
                i += 2;
                state = STATE_NORMAL;
            } else {
                if (!append_buf(&result, &len, &cap, &in[i], 1)) {
                    free(result);
                    return false;
                }
                i++;
            }
            continue;
        }

        if (state == STATE_STRING) {
            if (!append_buf(&result, &len, &cap, &in[i], 1)) {
                free(result);
                return false;
            }
            if (in[i] == '\\' && i + 1 < in_len) {
                if (!append_buf(&result, &len, &cap, &in[i + 1], 1)) {
                    free(result);
                    return false;
                }
                i += 2;
                continue;
            }
            if (in[i] == '"') {
                state = STATE_NORMAL;
            }
            i++;
            continue;
        }

        if (state == STATE_CHAR) {
            if (!append_buf(&result, &len, &cap, &in[i], 1)) {
                free(result);
                return false;
            }
            if (in[i] == '\\' && i + 1 < in_len) {
                if (!append_buf(&result, &len, &cap, &in[i + 1], 1)) {
                    free(result);
                    return false;
                }
                i += 2;
                continue;
            }
            if (in[i] == '\'') {
                state = STATE_NORMAL;
            }
            i++;
            continue;
        }
    }

    if (!result) {
        result = (char *)malloc(1);
        if (!result) {
            return false;
        }
        result[0] = '\0';
    }

    *out = result;
    *out_len = len;
    *change_count = changes;
    return true;
}

static int write_file_atomic(const char *path, const char *data, size_t len, mode_t mode_bits) {
    size_t path_len = strlen(path);
    char *tmp_path = (char *)malloc(path_len + 5);
    FILE *fp;
    int rc = -1;

    if (!tmp_path) {
        logerror_fmt("out of memory writing '%s'", path);
        return -1;
    }

    snprintf(tmp_path, path_len + 5, "%s.tmp", path);
    fp = fopen(tmp_path, "wb");
    if (!fp) {
        logerror_fmt("failed to open temp '%s': %s", tmp_path, strerror(errno));
        free(tmp_path);
        return -1;
    }

    if (len > 0 && fwrite(data, 1, len, fp) != len) {
        logerror_fmt("failed to write temp '%s'", tmp_path);
        goto out;
    }
    if (fclose(fp) != 0) {
        fp = NULL;
        logerror_fmt("failed to close temp '%s': %s", tmp_path, strerror(errno));
        goto out;
    }
    fp = NULL;

    if (chmod(tmp_path, mode_bits) != 0) {
        logerror_fmt("failed to set mode on '%s': %s", tmp_path, strerror(errno));
        goto out;
    }

    if (rename(tmp_path, path) != 0) {
        logerror_fmt("failed to replace '%s': %s", path, strerror(errno));
        goto out;
    }

    rc = 0;

out:
    if (fp) {
        fclose(fp);
    }
    if (rc != 0) {
        unlink(tmp_path);
    }
    free(tmp_path);
    return rc;
}

static int process_file(const char *path, convert_mode_t mode, size_t *changes_out) {
    struct stat st;
    char *input = NULL;
    char *output = NULL;
    size_t in_len = 0;
    size_t out_len = 0;
    size_t changes = 0;
    int rc = -1;

    if (stat(path, &st) != 0) {
        logerror_fmt("cannot stat '%s': %s", path, strerror(errno));
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        logerror_fmt("'%s' is not a regular file", path);
        return -1;
    }

    input = slurp_file(path, &in_len);
    if (!input) {
        return -1;
    }

    if (!transform_contents(input, in_len, mode, &output, &out_len, &changes)) {
        logerror_fmt("failed to transform '%s' (out of memory)", path);
        goto out;
    }

    if (changes > 0) {
        logdebug_fmt("%s: %zu replacements", path, changes);
        if (write_file_atomic(path, output, out_len, st.st_mode & 0777) != 0) {
            goto out;
        }
    } else {
        logdebug_fmt("%s: no changes", path);
    }

    if (changes_out) {
        *changes_out = changes;
    }
    rc = 0;

out:
    free(input);
    free(output);
    return rc;
}

int main(int argc, char *argv[]) {
    int opt;
    int option_index = 0;
    convert_mode_t mode = MODE_NONE;
    int failures = 0;
    size_t total_changes = 0;
    int file_count = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 1000},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "umvqh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'u':
                if (mode != MODE_NONE && mode != MODE_TO_TRAILING_UNDERSCORE) {
                    logerror("options -u and -m are mutually exclusive");
                    return EXIT_FAILURE;
                }
                mode = MODE_TO_TRAILING_UNDERSCORE;
                break;
            case 'm':
                if (mode != MODE_NONE && mode != MODE_TO_M_PREFIX) {
                    logerror("options -u and -m are mutually exclusive");
                    return EXIT_FAILURE;
                }
                mode = MODE_TO_M_PREFIX;
                break;
            case 'v':
                log_more();
                break;
            case 'q':
                log_less();
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case 1000:
                printf("membervars %s\n", TOOL_VERSION);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (mode == MODE_NONE) {
        logerror("either -u or -m is required");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (optind >= argc) {
        logerror("at least one file is required");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = optind; i < argc; i++) {
        size_t file_changes = 0;
        file_count++;
        if (process_file(argv[i], mode, &file_changes) != 0) {
            failures++;
            continue;
        }
        total_changes += file_changes;
    }

    logmesg_fmt("processed %d file(s), %zu replacement(s), %d failure(s)",
                file_count,
                total_changes,
                failures);

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
