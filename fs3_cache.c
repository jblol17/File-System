////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_cache.c
//  Description    : This is the implementation of the cache for the 
//                   FS3 filesystem interface.
//
//  Author         : JOHN B MORILLO 
//  Last Modified  : 6-1-22
//

// Includes


// Project Includes
#include <fs3_cache.h>
#include <fs3_controller.h>
#include <fs3_driver.h>
#include <stdlib.h>
#include <string.h>

//
// Support Macros/Data
//int cache_status = 0; //0 when cache is closed and 1 when it's in use. Will be 0 when close_cache is called. 

typedef struct cache_struct{
    char buf[FS3_SECTOR_SIZE];
    FS3TrackIndex trk_index;
    FS3SectorIndex sec_index;
    int last_time_used;
    int status;
}*cache_s;

//was kinda weird to mess around with the pointers to get the arrows to work, I'd need more practice with that. 

cache_s* cache;
int total_cachelines;
int count = 0;
int hits = 0;
int misses = 0;

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : update_cache_time
// Description  : Initialize the cache with a fixed number of cache lines
//
// Inputs       : a cache that we provide 
// Outputs      : none
void update_cache_time (cache_s updated_cache){

    updated_cache->last_time_used = count;
    count += 1;
}

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_init_cache
// Description  : Initialize the cache with a fixed number of cache lines
//
// Inputs       : cachelines - the number of cache lines to include in cache
// Outputs      : 0 if successful, -1 if failure

int fs3_init_cache(uint16_t cachelines) {

    // initializes the cache
    total_cachelines = cachelines;
    cache = (cache_s*) malloc(sizeof(struct cache_struct*) * cachelines);
    for (int i = 0; i < cachelines; i++)
    {
        cache[i] = (cache_s) malloc(sizeof(struct cache_struct));
        //cache[i]->buf = NULL;
        cache[i]->sec_index = 0;
        cache[i]->trk_index = 0;
        cache[i]->last_time_used = 0;
        cache[i]->status =0; //closed by default
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close_cache
// Description  : Close the cache, freeing any buffers held in it
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_close_cache(void) {

    //loops though and frees any cache
    for (int i = 0; i < total_cachelines; i++)
    {
        free(cache[i]);
    }
    free(cache);
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_put_cache
// Description  : Put an element in the cache
//
// Inputs       : trk - the track number of the sector to put in cache
//                sct - the sector number of the sector to put in cache
// Outputs      : 0 if inserted, -1 if not inserted

int fs3_put_cache(FS3TrackIndex trk, FS3SectorIndex sct, void *buf) {

    // walk thru cache and if the track and index match what we want, return what's in the cache
    for (int i = 0; i < total_cachelines; i++)
    {
        if (cache[i]->trk_index == trk && cache[i]->sec_index == sct)
        {
            memcpy(cache[i]->buf, buf, FS3_SECTOR_SIZE);
            return(0);
        }
    }
    // if that cache line is not being used, then it's a free line to put cache stuff into
    for (int i = 0; i < total_cachelines; i++)
    {
        if (cache[i]->status == 0)
        {
            // cache[i]->buf == malloc(sizeof(FS3_SECTOR_SIZE))
            memcpy(cache[i]->buf, buf, FS3_SECTOR_SIZE);
            cache[i]->trk_index = trk;
            cache[i]->sec_index = sct;
            update_cache_time(cache[i]);
            cache[i]->status = 1; // in use, open
            return(0);
        }
    }
    // this is to check for the least recently used index and update cache values under it
    int lru_index = 0;
    for (int i = 0; i < total_cachelines; i++)
    {
        if (cache[i]->last_time_used < cache[lru_index]->last_time_used)
        {
            lru_index = i;
        }
    }
    memcpy(cache[lru_index]->buf, buf, FS3_SECTOR_SIZE);
    cache[lru_index]->trk_index = trk;
    cache[lru_index]->sec_index = sct;
    update_cache_time(cache[lru_index]);
    cache[lru_index]->status = 1; // in use, open
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_get_cache
// Description  : Get an element from the cache (
//
// Inputs       : trk - the track number of the sector to find
//                sct - the sector number of the sector to find
// Outputs      : returns NULL if not found or failed, pointer to buffer if found

void * fs3_get_cache(FS3TrackIndex trk, FS3SectorIndex sct)  {

    // walk the cache and see if trk and sct are in the cache array. If they are, return the *buf (from struct) if not, return (NULL)
    for (int i = 0; i < total_cachelines; i++)
    {
        if (cache[i]->trk_index == trk && cache[i]->sec_index == sct)
        { 
            update_cache_time(cache[i]);
            hits +=1;
            return(cache[i]->buf); // return the stuff in the cache
        }   
    }
    misses +=1;
    return(NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_log_cache_metrics
// Description  : Log the metrics for the cache 
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_log_cache_metrics(void) { // logging messages for output

    int attempts = hits + misses;
    double ratio = (((double) hits)/attempts) * 100;
    logMessage(LOG_INFO_LEVEL, "Hits: %d \n", hits);
    logMessage(LOG_INFO_LEVEL, "Misses: %d \n", misses);
    logMessage(LOG_INFO_LEVEL, "Ratio: %.2f %% \n", ratio);
    logMessage(LOG_INFO_LEVEL, "Attempts: %d \n", attempts);
    return(0);
}

