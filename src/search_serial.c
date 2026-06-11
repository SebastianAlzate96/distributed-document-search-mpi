#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>

#define TOP_K 10
#define MAX_TITLE 256
#define MAX_TOPIC 64
#define MAX_QUERY_TERMS 20
#define MAX_TERM 64

typedef struct {
    int doc_id;
    int score;
    int word_count;
    char title[MAX_TITLE];
    char topic[MAX_TOPIC];
} SearchResult;

int is_word_boundary(char c) {
    return !isalnum((unsigned char)c);
}

int count_term_occurrences(const char *text, const char *term) {
    int count = 0;
    int term_len = strlen(term);
    int text_len = strlen(text);

    if (term_len == 0 || text_len == 0) {
        return 0;
    }

    for (int i = 0; i <= text_len - term_len; i++) {
        if (strncasecmp(&text[i], term, term_len) == 0) {
            int left_ok = (i == 0) || is_word_boundary(text[i - 1]);
            int right_ok = (i + term_len >= text_len) || is_word_boundary(text[i + term_len]);

            if (left_ok && right_ok) {
                count++;
            }
        }
    }

    return count;
}

int split_query(char *query, char terms[MAX_QUERY_TERMS][MAX_TERM]) {
    int count = 0;
    char *token = strtok(query, " ");

    while (token != NULL && count < MAX_QUERY_TERMS) {
        strncpy(terms[count], token, MAX_TERM - 1);
        terms[count][MAX_TERM - 1] = '\0';
        count++;

        token = strtok(NULL, " ");
    }

    return count;
}

int compute_score(const char *title, const char *text, char terms[MAX_QUERY_TERMS][MAX_TERM], int term_count) {
    int score = 0;

    for (int i = 0; i < term_count; i++) {
        int title_matches = count_term_occurrences(title, terms[i]);
        int text_matches = count_term_occurrences(text, terms[i]);

        score += 3 * title_matches + text_matches;
    }

    return score;
}

void insert_top_result(SearchResult top_results[TOP_K], SearchResult candidate) {
    if (candidate.score <= 0) {
        return;
    }

    for (int i = 0; i < TOP_K; i++) {
        if (candidate.score > top_results[i].score) {
            for (int j = TOP_K - 1; j > i; j--) {
                top_results[j] = top_results[j - 1];
            }

            top_results[i] = candidate;
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <documents_csv> <query>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];

    char query_copy[512];
    strncpy(query_copy, argv[2], sizeof(query_copy) - 1);
    query_copy[sizeof(query_copy) - 1] = '\0';

    char terms[MAX_QUERY_TERMS][MAX_TERM];
    int term_count = split_query(query_copy, terms);

    if (term_count == 0) {
        printf("Empty query.\n");
        return 1;
    }

    FILE *file = fopen(input_file, "r");

    if (file == NULL) {
        printf("Error opening file: %s\n", input_file);
        return 1;
    }

    SearchResult top_results[TOP_K];

    for (int i = 0; i < TOP_K; i++) {
        top_results[i].doc_id = -1;
        top_results[i].score = 0;
        top_results[i].word_count = 0;
        strcpy(top_results[i].title, "");
        strcpy(top_results[i].topic, "");
    }

    char *line = NULL;
    size_t len = 0;

    // Saltar encabezado
    getline(&line, &len, file);

    int documents_read = 0;

    clock_t start = clock();

    while (getline(&line, &len, file) != -1) {
        char *doc_id_str = strtok(line, ",");
        char *title = strtok(NULL, ",");
        char *topic = strtok(NULL, ",");
        char *word_count_str = strtok(NULL, ",");
        char *text = strtok(NULL, "\n");

        if (doc_id_str == NULL || title == NULL || topic == NULL || word_count_str == NULL || text == NULL) {
            continue;
        }

        int score = compute_score(title, text, terms, term_count);

        SearchResult candidate;
        candidate.doc_id = atoi(doc_id_str);
        candidate.score = score;
        candidate.word_count = atoi(word_count_str);

        strncpy(candidate.title, title, MAX_TITLE - 1);
        candidate.title[MAX_TITLE - 1] = '\0';

        strncpy(candidate.topic, topic, MAX_TOPIC - 1);
        candidate.topic[MAX_TOPIC - 1] = '\0';

        insert_top_result(top_results, candidate);

        documents_read++;
    }

    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Serial Search completed.\n");
    printf("Input file    : %s\n", input_file);
    printf("Documents     : %d\n", documents_read);
    printf("Query         : %s\n", argv[2]);
    printf("Time          : %.6f seconds\n", elapsed);
    printf("\nTop %d results:\n", TOP_K);

    for (int i = 0; i < TOP_K; i++) {
        if (top_results[i].doc_id != -1) {
            printf(
                "%2d. DocID: %d | Score: %d | Topic: %s | Words: %d | Title: %s\n",
                i + 1,
                top_results[i].doc_id,
                top_results[i].score,
                top_results[i].topic,
                top_results[i].word_count,
                top_results[i].title
            );
        }
    }

    free(line);
    fclose(file);

    return 0;
}
