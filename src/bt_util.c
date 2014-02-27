
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include "bt.h"
#include "bt_string.h"
#include "bt_local.h"
#include "bt_util.h"

#include "sha1.h"

char *bt_generate_peer_id(
)
{
    char *str;
    int rand_num;

    rand_num = rand();
    asprintf(&str, "AAAAA%d%011d", VERSION_NUM, rand_num);
    assert(strlen(str) == PEER_ID_LEN);
    return str;
}

