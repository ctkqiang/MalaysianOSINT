#pragma once

#include <stddef.h>
#include "memory.h"

#ifndef MULE_H
#define MULE_H

struct semak_mule_response {
    char *data;
    size_t size;
};

int pdrm_semak_mule(const char *url, const char *json_payload, struct semak_mule_response *resp);

void pdrm_semak_mule_response_free(struct semak_mule_response *resp);

#endif