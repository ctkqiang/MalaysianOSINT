#pragma once

#ifndef COMPANY_H
#define COMPANY_H

#include "memory.h"

struct company_entry {
    char *name;       
    char *category;   
    char *address;    
    char *website;    
    char *source;
};

int company_search(const char *keyword, struct company_entry **results, size_t *count);


void company_free_results(struct company_entry *results, size_t count);


#endif