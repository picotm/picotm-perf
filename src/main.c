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

#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "tm.h"
#include "opts.h"

static const struct test_func*
find_test(enum opt_io_pattern io_pattern)
{
    if (io_pattern >= number_of_tm_tests()) {
        fprintf(stderr, "no test for given I/O pattern\n");
        return NULL;
    }
    return tm_test + io_pattern;
}

int
main(int argc, char* argv[])
{
    switch (parse_opts(argc, argv)) {
        case PARSE_OPTS_EXIT:
            return EXIT_SUCCESS;
        case PARSE_OPTS_ERROR:
            return EXIT_FAILURE;
        default:
            break;
    }

    const struct test_func* test = find_test(g_io_pattern);
    if (!test) {
        return EXIT_FAILURE;
    }

    int res = run_test(g_nthreads, g_nmsecs, test, g_nloads, g_nstores);
    if (res < 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
