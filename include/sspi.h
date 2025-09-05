#pragma once

#include <stddef.h>

#ifndef SSPI_H
#define SSPI_H

struct sspi_response {
    char *raw_html;
    char *status;
};

struct memory {
    char *data;
    size_t size;
};


int sspi_check(const char *ic_no, struct sspi_response *resp);

void sspi_response_free(struct sspi_response *resp);

#endif