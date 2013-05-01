
/**
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section description
 * for now this file will be used to hold misc operations
 */



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include "block.h"

#include "bt.h"
#include "bt_local.h"
#include "bt_util.h"

#include "sha1.h"

char *bt_generate_peer_id(
)
{
    char *str;
    int rand_num;

    rand_num = 0;
//    rand_num = random();
    asprintf(&str, "-AA-%d-%011d", VERSION_NUM, rand_num);
    assert(strlen(str) == PEER_ID_LEN);
    return str;
}

/*----------------------------------------------------------------------------*/
