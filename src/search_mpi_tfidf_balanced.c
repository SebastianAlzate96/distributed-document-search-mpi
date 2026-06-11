#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <mpi.h>

#define TOP_K 10
#define MAX_TITLE 256
#define MAX_TOPIC 64
#define MAX_QUERY 512
#define MAX_QUERY_TERMS 20
#define MAX_TERM 64

typedef struct {
    int doc_id;
    double score;
    int word_count;
    char title[MAX_TITLE];
    char topic[MAX_TOPIC];
} SearchResult;

int is_word_boundary(char c) {
    return !isalnum((unsigned char)c);
}

int count_documents(const char *filename) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    int count = 0;

    if (getline(&line, &len, file) == -1) {
        free(line);
        fclose(file);
        return 0;
    }

    while (getline(&line, &len, file) != -1) {
        count++;
    }

    free(line);
    fclose(file);

    return count;
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

void initialize_top_results(SearchResult top_results[TOP_K]) {
    for (int i = 0; i < TOP_K; i++) {
        top_results[i].doc_id = -1;
        top_results[i].score = 0.0;
        top_results[i].word_count = 0;
        strcpy(top_results[i].title, "");
        strcpy(top_results[i].topic, "");
    }
}

void insert_top_result(SearchResult top_results[TOP_K], SearchResult candidate) {
    if (candidate.score <= 0.0) {
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

void get_range(int total_docs, int size, int rank, int *start, int *end) {
    int base = total_docs / size;
    int rem = total_docs % size;

    int local_n = base + (rank < rem ? 1 : 0);

    *start = rank * base + (rank < rem ? rank : rem);
    *end = *start + local_n;
}

void compute_weighted_ranges(const char *filename, int total_docs, int size, int *starts, int *ends) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int *word_counts = malloc(total_docs * sizeof(int));

    if (word_counts == NULL) {
        printf("Memory allocation error.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char *line = NULL;
    size_t len = 0;

    if (getline(&line, &len, file) == -1) {
        printf("Empty file.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    long long total_words = 0;
    int index = 0;

    while (getline(&line, &len, file) != -1 && index < total_docs) {
        char *doc_id_str = strtok(line, ",");
        char *title = strtok(NULL, ",");
        char *topic = strtok(NULL, ",");
        char *word_count_str = strtok(NULL, ",");

        if (doc_id_str != NULL && title != NULL && topic != NULL && word_count_str != NULL) {
            word_counts[index] = atoi(word_count_str);
        } else {
            word_counts[index] = 0;
        }

        total_words += word_counts[index];
        index++;
    }

    free(line);
    fclose(file);

    long long target_words = total_words / size;

    int current_process = 0;
    long long current_words = 0;

    starts[0] = 0;

    for (int doc = 0; doc < total_docs; doc++) {
        if (
            current_process < size - 1 &&
            current_words > 0 &&
            current_words + word_counts[doc] > target_words
        ) {
            ends[current_process] = doc;
            current_process++;
            starts[current_process] = doc;
            current_words = 0;
        }

        current_words += word_counts[doc];
    }

    ends[current_process] = total_docs;

    for (int p = current_process + 1; p < size; p++) {
        starts[p] = total_docs;
        ends[p] = total_docs;
    }

    free(word_counts);
}

void compute_local_document_frequency(
    const char *input_file,
    int start_doc,
    int end_doc,
    char terms[MAX_QUERY_TERMS][MAX_TERM],
    int term_count,
    int local_df[MAX_QUERY_TERMS]
) {
    FILE *file = fopen(input_file, "r");

    if (file == NULL) {
        printf("Error opening file: %s\n", input_file);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char *line = NULL;
    size_t len = 0;

    if (getline(&line, &len, file) == -1) {
        printf("Empty file.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int current_doc = 0;

    while (getline(&line, &len, file) != -1) {
        if (current_doc >= start_doc && current_doc < end_doc) {
            char *doc_id_str = strtok(line, ",");
            char *title = strtok(NULL, ",");
            char *topic = strtok(NULL, ",");
            char *word_count_str = strtok(NULL, ",");
            char *text = strtok(NULL, "\n");

            if (doc_id_str != NULL && title != NULL && topic != NULL && word_count_str != NULL && text != NULL) {
                for (int t = 0; t < term_count; t++) {
                    int title_count = count_term_occurrences(title, terms[t]);
                    int text_count = count_term_occurrences(text, terms[t]);

                    if (title_count + text_count > 0) {
                        local_df[t]++;
                    }
                }
            }
        }

        current_doc++;

        if (current_doc >= end_doc) {
            break;
        }
    }

    free(line);
    fclose(file);
}

void search_local_documents_tfidf(
    const char *input_file,
    int start_doc,
    int end_doc,
    char terms[MAX_QUERY_TERMS][MAX_TERM],
    int term_count,
    double idf[MAX_QUERY_TERMS],
    SearchResult local_top[TOP_K],
    int *local_documents,
    long long *local_words
) {
    FILE *file = fopen(input_file, "r");

    if (file == NULL) {
        printf("Error opening file: %s\n", input_file);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char *line = NULL;
    size_t len = 0;

    if (getline(&line, &len, file) == -1) {
        printf("Empty file.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int current_doc = 0;

    while (getline(&line, &len, file) != -1) {
        if (current_doc >= start_doc && current_doc < end_doc) {
            char *doc_id_str = strtok(line, ",");
            char *title = strtok(NULL, ",");
            char *topic = strtok(NULL, ",");
            char *word_count_str = strtok(NULL, ",");
            char *text = strtok(NULL, "\n");

            if (doc_id_str != NULL && title != NULL && topic != NULL && word_count_str != NULL && text != NULL) {
                double score = 0.0;

                for (int t = 0; t < term_count; t++) {
                    int title_tf = count_term_occurrences(title, terms[t]);
                    int text_tf = count_term_occurrences(text, terms[t]);

                    score += (3.0 * title_tf + text_tf) * idf[t];
                }

                SearchResult candidate;
                candidate.doc_id = atoi(doc_id_str);
                candidate.score = score;
                candidate.word_count = atoi(word_count_str);

                strncpy(candidate.title, title, MAX_TITLE - 1);
                candidate.title[MAX_TITLE - 1] = '\0';

                strncpy(candidate.topic, topic, MAX_TOPIC - 1);
                candidate.topic[MAX_TOPIC - 1] = '\0';

                insert_top_result(local_top, candidate);

                (*local_documents)++;
                (*local_words) += candidate.word_count;
            }
        }

        current_doc++;

        if (current_doc >= end_doc) {
            break;
        }
    }

    free(line);
    fclose(file);
}

int main(int argc, char *argv[]) {
    int rank;
    int size;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        if (rank == 0) {
            printf("Usage: %s <documents_csv> <query>\n", argv[0]);
        }

        MPI_Finalize();
        return 1;
    }

    const char *input_file = argv[1];

    char query[MAX_QUERY];

    if (rank == 0) {
        strncpy(query, argv[2], MAX_QUERY - 1);
        query[MAX_QUERY - 1] = '\0';
    }

    MPI_Bcast(query, MAX_QUERY, MPI_CHAR, 0, MPI_COMM_WORLD);

    char query_copy[MAX_QUERY];
    strncpy(query_copy, query, MAX_QUERY - 1);
    query_copy[MAX_QUERY - 1] = '\0';

    char terms[MAX_QUERY_TERMS][MAX_TERM];
    int term_count = split_query(query_copy, terms);

    if (term_count == 0) {
        if (rank == 0) {
            printf("Empty query.\n");
        }

        MPI_Finalize();
        return 1;
    }

    int total_docs = 0;

    if (rank == 0) {
        total_docs = count_documents(input_file);
    }

    MPI_Bcast(&total_docs, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int *starts = malloc(size * sizeof(int));
    int *ends = malloc(size * sizeof(int));

    if (starts == NULL || ends == NULL) {
        printf("Memory allocation error.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        compute_weighted_ranges(input_file, total_docs, size, starts, ends);
    }

    MPI_Bcast(starts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(ends, size, MPI_INT, 0, MPI_COMM_WORLD);

    int start_doc = starts[rank];
    int end_doc = ends[rank];

    int local_df[MAX_QUERY_TERMS];
    int global_df[MAX_QUERY_TERMS];
    double idf[MAX_QUERY_TERMS];

    for (int i = 0; i < MAX_QUERY_TERMS; i++) {
        local_df[i] = 0;
        global_df[i] = 0;
        idf[i] = 0.0;
    }

    SearchResult local_top[TOP_K];
    initialize_top_results(local_top);

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    compute_local_document_frequency(
        input_file,
        start_doc,
        end_doc,
        terms,
        term_count,
        local_df
    );

    MPI_Allreduce(
        local_df,
        global_df,
        term_count,
        MPI_INT,
        MPI_SUM,
        MPI_COMM_WORLD
    );

    for (int t = 0; t < term_count; t++) {
        idf[t] = log(((double)total_docs + 1.0) / ((double)global_df[t] + 1.0)) + 1.0;
    }

    int local_documents = 0;
    long long local_words = 0;

    search_local_documents_tfidf(
        input_file,
        start_doc,
        end_doc,
        terms,
        term_count,
        idf,
        local_top,
        &local_documents,
        &local_words
    );

    double local_elapsed = MPI_Wtime() - start_time;
    double global_elapsed = 0.0;

    MPI_Reduce(
        &local_elapsed,
        &global_elapsed,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD
    );

    SearchResult *all_results = NULL;

    if (rank == 0) {
        all_results = malloc(size * TOP_K * sizeof(SearchResult));

        if (all_results == NULL) {
            printf("Memory allocation error.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Gather(
        local_top,
        TOP_K * sizeof(SearchResult),
        MPI_BYTE,
        all_results,
        TOP_K * sizeof(SearchResult),
        MPI_BYTE,
        0,
        MPI_COMM_WORLD
    );

    int *documents_by_process = NULL;
    long long *words_by_process = NULL;
    double *time_by_process = NULL;

    if (rank == 0) {
        documents_by_process = malloc(size * sizeof(int));
        words_by_process = malloc(size * sizeof(long long));
        time_by_process = malloc(size * sizeof(double));

        if (documents_by_process == NULL || words_by_process == NULL || time_by_process == NULL) {
            printf("Memory allocation error.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Gather(
        &local_documents,
        1,
        MPI_INT,
        documents_by_process,
        1,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    MPI_Gather(
        &local_words,
        1,
        MPI_LONG_LONG,
        words_by_process,
        1,
        MPI_LONG_LONG,
        0,
        MPI_COMM_WORLD
    );

    MPI_Gather(
        &local_elapsed,
        1,
        MPI_DOUBLE,
        time_by_process,
        1,
        MPI_DOUBLE,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0) {
        SearchResult global_top[TOP_K];
        initialize_top_results(global_top);

        for (int i = 0; i < size * TOP_K; i++) {
            insert_top_result(global_top, all_results[i]);
        }

        long long min_words = words_by_process[0];
        long long max_words = words_by_process[0];
        double min_time = time_by_process[0];
        double max_time = time_by_process[0];

        for (int p = 1; p < size; p++) {
            if (words_by_process[p] < min_words) {
                min_words = words_by_process[p];
            }

            if (words_by_process[p] > max_words) {
                max_words = words_by_process[p];
            }

            if (time_by_process[p] < min_time) {
                min_time = time_by_process[p];
            }

            if (time_by_process[p] > max_time) {
                max_time = time_by_process[p];
            }
        }

        double word_imbalance = (min_words > 0) ? (double)max_words / (double)min_words : 0.0;
        double time_imbalance = (min_time > 0.0) ? max_time / min_time : 0.0;

        printf("MPI TF-IDF Balanced Distributed Search completed.\n");
        printf("Input file    : %s\n", input_file);
        printf("Documents     : %d\n", total_docs);
        printf("Query         : %s\n", query);
        printf("Processes     : %d\n", size);
        printf("Time          : %.6f seconds\n", global_elapsed);

        printf("\nTF-IDF global term statistics:\n");

        for (int t = 0; t < term_count; t++) {
            printf(
                "Term: %-20s | DF: %d | IDF: %.6f\n",
                terms[t],
                global_df[t],
                idf[t]
            );
        }

        printf("\nLoad balance by process:\n");

        for (int p = 0; p < size; p++) {
            printf(
                "Process %d | Documents: %d | Words: %lld | Local time: %.6f seconds\n",
                p,
                documents_by_process[p],
                words_by_process[p],
                time_by_process[p]
            );
        }

        printf("\nImbalance metrics:\n");
        printf("Word imbalance ratio: %.4f\n", word_imbalance);
        printf("Time imbalance ratio: %.4f\n", time_imbalance);

        printf("\nTop %d results:\n", TOP_K);

        for (int i = 0; i < TOP_K; i++) {
            if (global_top[i].doc_id != -1) {
                printf(
                    "%2d. DocID: %d | Score: %.4f | Topic: %s | Words: %d | Title: %s\n",
                    i + 1,
                    global_top[i].doc_id,
                    global_top[i].score,
                    global_top[i].topic,
                    global_top[i].word_count,
                    global_top[i].title
                );
            }
        }

        free(all_results);
        free(documents_by_process);
        free(words_by_process);
        free(time_by_process);
    }

    free(starts);
    free(ends);

    MPI_Finalize();

    return 0;
}
