/* Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "opts.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ptr.h"

enum opt_io_pattern g_io_pattern = IO_PATTERN_RANDOM;
unsigned long       g_nthreads = 1;
unsigned long       g_nloads = 0;
unsigned long       g_nstores = 0;
unsigned long       g_nmsecs = 0;

static enum parse_opts_result
opt_nthreads(const char* optarg)
{
    errno = 0;

    g_nthreads = strtoul(optarg, NULL, 0);

    if (errno) {
        perror("strtoul()");
        return PARSE_OPTS_ERROR;
    }

    if (!g_nthreads) {
        fprintf(stderr, "at least 1 thread required\n");
    }

    return PARSE_OPTS_OK;
}

static enum parse_opts_result
opt_pattern(const char* optarg)
{
    static const char * const optstr[] = {"random", "sequential"};

    for (size_t i = 0; i < arraylen(optstr); ++i) {
        if (!strcmp(optstr[i], optarg)) {
            g_io_pattern = i;
            return PARSE_OPTS_OK;
        }
    }

    fprintf(stderr, "unknown I/O pattern '%s'\n", optarg);

    return PARSE_OPTS_ERROR;
}

static enum parse_opts_result
opt_nloads(const char* optarg)
{
    errno = 0;

    g_nloads = strtoul(optarg, NULL, 0);

    if (errno) {
        perror("strtoul()");
        return PARSE_OPTS_ERROR;
    }

    return PARSE_OPTS_OK;
}

static enum parse_opts_result
opt_nstores(const char* optarg)
{
    errno = 0;

    g_nstores = strtoul(optarg, NULL, 0);

    if (errno) {
        perror("strtoul()");
        return PARSE_OPTS_ERROR;
    }

    return PARSE_OPTS_OK;
}

static enum parse_opts_result
opt_nmsecs(const char* optarg)
{
    errno = 0;

    g_nmsecs = strtoul(optarg, NULL, 0);

    if (errno) {
        perror("strtoul()");
        return PARSE_OPTS_ERROR;
    }

    return PARSE_OPTS_OK;
}

static enum parse_opts_result
opt_help(const char* optarg)
{
    printf("Usage: picotm-perf [options]\n"
           "Options:\n"
           "  -V                            About this program\n"
           "  -h                            This help\n"
           "  -t <number>                   Number of concurrent threads\n"
           "  -T                            Time of test in milliseconds\n"
           "  -P <pattern>                  I/O pattern, <random|sequential>\n"
           "  -L                            Number of loads per transaction\n"
           "  -S                            Number of stores per transaction\n"
           );

    return PARSE_OPTS_EXIT;
}

static enum parse_opts_result
opt_version(const char *optarg)
{
    printf("picotm Performance Tests\n");
    printf("This software is licensed under the MIT License.\n");

    return PARSE_OPTS_EXIT;
}

enum parse_opts_result
parse_opts(int argc, char *argv[])
{
    static enum parse_opts_result (* const opt[])(const char*) = {
        ['L'] = opt_nloads,
        ['P'] = opt_pattern,
        ['S'] = opt_nstores,
        ['T'] = opt_nmsecs,
        ['V'] = opt_version,
        ['h'] = opt_help,
        ['t'] = opt_nthreads
    };

    if (argc < 2) {
        printf("enter `picotm-perf -h` for a list of command-line options\n");
        return PARSE_OPTS_EXIT;
    }

    int c;

    while ((c = getopt(argc, argv, "L:P:S:T:Vht:")) != -1) {
        if ((c == '?') || (c == ':')) {
            return PARSE_OPTS_ERROR;
        }
        if (c >= arraylen(opt) || !opt[c]) {
            return PARSE_OPTS_ERROR;
        }
        enum parse_opts_result res = opt[c](optarg);
        if (res) {
            return res;
        }
    }

    return PARSE_OPTS_OK;
}
