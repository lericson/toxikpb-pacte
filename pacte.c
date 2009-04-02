/**
 * Pacte, a play on the word paste and C. Fucken' funny.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#define PASTEBIN_URL "http://pb.lericson.se/"

typedef struct curl_slist curl_slist_t;

static inline char *pacte_strncat(char *dest, const char *src, size_t n) {
    return strncat(dest, src, n) + n;
}

static inline char *pacte_strcat(char *dest, const char *src) {
    return pacte_strncat(dest, src, strlen(src));
}

static size_t _dispose_data(void *a, size_t b, size_t nmemb, void *d) {
    /* I'm useless. <sulk> */
    return nmemb;
}

static size_t _header_parse(void *line, size_t size, size_t nmemb, void *user) {
    size_t len = size * nmemb;

    if (strncasecmp(line, "location: ", len > 10 ? 10 : len) == 0) {
        char **buf = (char **)user, *ndl;

        *buf = malloc(len - 10);
        memcpy(*buf, line + 10, len - 10);
        *((*buf) + len - 9) = 0;
        *((ndl = strchr(*buf, '\r')) ? ndl : strchr(*buf, '\n')) = 0;
    }

    return nmemb;
}

int main(int argc, char **argv) {
    char *result_url;
    /* General */
    CURL *curl;
    CURLcode res;
    curl_slist_t *headers;
    char *post_data, *pd_pos;
    size_t post_data_size;
    /* Lexer */
    char *lexer;
    char *lexer_esc;
    /* Code */
    size_t code_offset;
    FILE *input = stdin;
    char *code_pos, *code_esc;
    size_t code_esc_len;

    /* General. */

    if ((res = curl_global_init(CURL_GLOBAL_NOTHING))) {
        fprintf(stderr, "%s: curl initialization error\n", *argv);
        return EXIT_FAILURE;
    }

    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "%s: curl_easy_init gave NULL?\n", *argv);
        return EXIT_FAILURE;
    }

    post_data_size = 1 << 12;
    post_data = pd_pos = malloc(post_data_size);
    *pd_pos = 0;

    /* Lexer. */

    if (argc < 2) {
        lexer = "guess";
    } else {
        lexer = argv[1];
    }

    /* Append form key "lexer". */
    pd_pos = pacte_strncat(pd_pos, "lexer=", 6L);

    /* Append lexer name in escaped form. */
    lexer_esc = curl_easy_escape(curl, lexer, 0);
    pd_pos = pacte_strcat(pd_pos, lexer_esc);
    curl_free(lexer_esc);

    /* Append form key "code". */
    pd_pos = pacte_strncat(pd_pos, "&code=", 6L);

    /* Code. */
    code_offset = pd_pos - post_data;

    /* Om nom nom all the data. */
    while (!feof(input)) {
        size_t used = pd_pos - post_data;
        size_t nread = fread((void *)pd_pos, 1, post_data_size - used, input);

        pd_pos += nread;
        used += nread;

        if (used >= (post_data_size >> 1)) {
            char *pd_new = realloc(post_data, post_data_size * 2);

            if (pd_new == NULL) {
                fprintf(stderr, "%s: out of memory?\n", *argv);
                return EXIT_FAILURE;
            }
            post_data = pd_new;
            pd_pos = post_data + used;
            post_data_size += post_data_size;
        }
    }

    code_pos = post_data + code_offset;
    code_esc = curl_easy_escape(curl, code_pos, pd_pos - code_pos);
    code_esc_len = strlen(code_esc);

    if (code_offset + code_esc_len > post_data_size) {
        char *pd_new = realloc(post_data, code_offset + code_esc_len);

        if (pd_new == NULL) {
            fprintf(stderr, "%s: out of memory :-(\n", *argv);
            return EXIT_FAILURE;
        }
        post_data = pd_new;
        code_pos = post_data + code_offset;
    }

    memcpy(code_pos, code_esc, code_esc_len);
    curl_free(code_esc);

    headers = NULL;
    headers = curl_slist_append(headers, "Expect:");
    headers = curl_slist_append(headers, "X-Woo: Yeah");

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
    curl_easy_setopt(curl, CURLOPT_URL, PASTEBIN_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, code_offset + code_esc_len);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _dispose_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _header_parse);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &result_url);

    result_url = NULL;

    
    if ((res = curl_easy_perform(curl))) {
        fprintf(stderr, "%s: error during query :/ %s\n",
                *argv, curl_easy_strerror(res));
        return EXIT_FAILURE;
    } else {
        long code;

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if (code != 302) {
            fprintf(stderr, "%s: unexpected HTTP code: %ld\n", *argv, code);
        }

        if (result_url != NULL) {
            printf("%s\n", result_url);
            fflush(stdout);
        }
    }

    /*curl_slist_free_all(headers);*/
    /*curl_easy_cleanup(curl);*/
    return EXIT_SUCCESS;
}
