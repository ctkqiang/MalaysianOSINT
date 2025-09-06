#pragma once

#ifndef SOCIAL_H
#define SOCIAL_H

#include <stddef.h>

typedef struct {
    const char *name;
    const char *url_template; 
} SocialTarget;

struct social_state {
    const char *username;
    size_t current_target;
};

void init_curl();

void cleanup_curl();

int check_username(const SocialTarget *target, const char *email);
ssize_t callback_send_chunk(void *cls, uint64_t pos, char *buf, size_t max);

extern SocialTarget targets[];
extern size_t targets_count;

#endif