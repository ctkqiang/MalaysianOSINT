#pragma once

#ifndef SOCIAL_H
#define SOCIAL_H

#include <stddef.h>

typedef struct {
    const char *name;
    const char *url_template; 
} SocialTarget;


void init_curl();

void cleanup_curl();

int check_username(const SocialTarget *target, const char *email);

extern SocialTarget targets[];
extern size_t targets_count;

#endif