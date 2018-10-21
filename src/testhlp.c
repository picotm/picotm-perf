/*
 * picotm-perf - Picotm Performance Tests
 * Copyright (c) 2017   Thomas Zimmermann <contact@tzimmermann.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testhlp.h"
#include <picotm/picotm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
abort_transaction_on_error(const char* origin)
{
    switch (picotm_error_status()) {
        case PICOTM_CONFLICTING:
            fprintf(stderr, "%s: Conficting transactions\n", origin);
            break;
        case PICOTM_REVOCABLE:
            fprintf(stderr, "%s: Irrevocability required\n", origin);
            break;
        case PICOTM_ERROR_CODE: {
            enum picotm_error_code error_code =
                picotm_error_as_error_code();
            fprintf(stderr, "%s: Error code %d\n", origin, (int)error_code);
            break;
        }
        case PICOTM_ERRNO: {
            int errno_code = picotm_error_as_errno();
            fprintf(stderr, "%s: Errno code %d (%s)\n", origin, errno_code,
                    strerror(errno_code));
            break;
        }
        default:
            fprintf(stderr, "%s, No error detected.", origin);
            break;
    }

    abort();
}
