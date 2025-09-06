#pragma once

#include <stddef.h>

#include "memory.h"

#ifndef SPRM_H
#define SPRM_H

#ifndef SPRM_URL
#define SPRM_URL "https://www.sprm.gov.my/index.php?page_id=96"
#endif

typedef struct {
    char name[256];
    char ic[32];
    char state[64];
    char employer[128];
    char position[128];
    char case_no[64];
    char charge[512];
    char law[128];
    char sentence[256];
    char image_url[512];
} Pesalah;

typedef struct {
    Pesalah *list;
    size_t count;
} PesalahList;

char *sprm_fetch_html(const char *url);

PesalahList sprm_parse_html(const char *html);

void sprm_free_list(PesalahList *plist);

void sprm_print_all(const PesalahList *plist);

PesalahList sprm_search(const PesalahList *plist, const char *keyword);

char *sprm_run(void);

#endif