#pragma once

#include <stddef.h>

#ifndef BNM_H
#define BNM_H

#ifndef BNM_HEADER_VERSION
#define BNM_HEADER_VERSION 1
#endif

#ifndef BNM_URL
#define BNM_URL ""
#endif

struct buf { 
    char *data; 
    size_t size; 
};

typedef struct {
    char *name;
    char *website;
    char *date;
} BNMAlertEntry;

int bnm_fetch_alerts(BNMAlertEntry **entries, size_t *count);

void bnm_free_alerts(BNMAlertEntry *entries, size_t count);

#endif
