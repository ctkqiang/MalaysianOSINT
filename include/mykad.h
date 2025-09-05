#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MYKAD_H
#define MYKAD_H

char* mykad_check(const char *ic_no);
char* mykad_get_birth_province(const char *ic_no);

#endif