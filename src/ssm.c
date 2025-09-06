#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/ssm.h"

char* get_ssm_format(const char *ssm_number) {
    if (ssm_number == NULL) return NULL;

    char *normalized = malloc(13);
    int j = 0;

    for (int i = 0; ssm_number[i] != '\0'; i++) {
        if (isdigit((unsigned char) ssm_number[i])) {
            if (j < 12) normalized[j++] = ssm_number[i];
        }
    }
    normalized[j] = '\0';

    if (j != 12) {
        free(normalized);
        return NULL;
    }

    return normalized;
}

char* ssm_get_entity_code(const char *ssm_number) {
    if (ssm_number == NULL || strlen(ssm_number) != 12) {
        return strdup("Invalid SSM number");
    }

    char entity_code[3];
    
    entity_code[0] = ssm_number[4];
    entity_code[1] = ssm_number[5];
    entity_code[2] = '\0';

    struct { const char *code; const char *desc; } mapping[] = {
        {"01", "Local Company"},
        {"02", "Foreign Company"},
        {"03", "Business Entity"},
        {"04", "Local Limited Liability Partnership (LLP)"},
        {"05", "Foreign Limited Liability Partnership (LLP)"},
        {"06", "Local Liability Partnership for Professional Practice"}
    };

    for (int i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++) {
        if (strcmp(entity_code, mapping[i].code) == 0) {
            return strdup(mapping[i].desc); 
        }
    }

    return strdup("Unknown Entity");
}
