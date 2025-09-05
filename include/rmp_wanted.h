#pragma once

#include <stddef.h>
#include "memory.h"

#ifndef RMP_WANTED_H
#define RMP_WANTED_H

#define MAX_NAME_LEN 128
#define MAX_AGE_LEN 16
#define MAX_PHOTO_LEN 256

#ifndef PDRM_WANTED__LIST
#define PDRM_WANTED__LIST "https://www.rmp.gov.my/orang-dikehendaki"
#endif


struct wanted_person {
    char *name;
    char *ic_number;
    char *offense;
};

struct wanted_list {
    struct wanted_person *persons;
    size_t count;
};

typedef struct {
    char name[MAX_NAME_LEN];
    char age[MAX_AGE_LEN];
    char photo_url[MAX_PHOTO_LEN];
} WantedPerson;

char* rmp_fetch_wanted_html(const char* url);

int rmp_parse_wanted_list(const char* html, WantedPerson** list);

void rmp_free_html(char* html);

#endif
