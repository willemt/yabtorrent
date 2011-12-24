/*
 * =====================================================================================
 *
 *       Filename:  bt_piece_cache.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/04/11 22:43:27
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include "bt_local.h"
#include "bt.h"


typedef struct
{
    int idx;
} cachepiece_t typedef struct
{
    void *private;
    cachepiece_t *pieces;
} bt_piececache_private_t;

#define priv(x) (bt_piececache_private_t*)(x)

/*  get the data from piece of this idx 
 *  read from the disk if we don't have this piece loaded */
void *bt_piececache_get_data(
    bt_piececache_t * cache,
    int idx
)                               //bt_piece_t* pce)
{
priv(cache)->}
