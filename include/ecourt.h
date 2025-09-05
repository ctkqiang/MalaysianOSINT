#pragma once

#ifndef  ECOURT_H
#define ECOURT_H

#define MAX_SEARCH_LEN 256
#define MAX_RESULT_LEN 8192

typedef struct {
    char *raw_json;     
    int total_results;  
} EJudgmentResponse;

EJudgmentResponse* ejudgment_search(
    const char* search,
    const char* jurisdictionType,
    const char* courtCategory,
    const char* court,
    const char* judgeName,
    const char* caseType,
    const char* dateOfAPFrom,
    const char* dateOfAPTo,
    const char* dateOfResultFrom,
    const char* dateOfResultTo,
    int currPage,
    const char* ordering,
    int maxRetries,
    int delayBetweenRetries
);

void ejudgment_response_free(EJudgmentResponse* resp);

char* ecourt_open_document(const char* documentId);

#endif
