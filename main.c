#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <json-c/json.h>
#include <limits.h>

#define MAX_MATCHES 50
#define MAX_SIMILARITY 85

void pretty_print_item(struct json_object *item);
int levenshtein_distance(char *restrict s, char *restrict t);
int levenshtein_partial(const char *query, const char *candidate);
int partial_ratio(const char *query, const char *candidate);

typedef struct {
    char name[256];
    int id;
} MatchResult;

static inline int min2 (const int a, const int b) {
    return a < b ? a : b;
}

static inline int min3 (const int a, const int b, const int c) {
    return min2 (a, min2 (b, c));
}

int main(int argc, char *argv[]) { 
    if (argc == 1) {
        printf("Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    struct json_object *items = json_object_from_file("/usr/share/itemsarz/items.json");
    if (items == NULL) {
        perror("Failed to parse JSON from file");
        return 1;
    }

    int opt;
    static struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "id", required_argument, 0, 'i' },
        { "name", required_argument, 0, 'n' },
        { 0, 0, 0, 0 }
    };
    
    while ((opt = getopt_long(argc, argv, "hi:n:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: <%s>\n\
    -h, --help      Show this screen\n\
    -i, --id        Search by itemid\n\
    -n, --name      Search by approximate item name\n", argv[0]);
                break;
            case 'i': {
                int needed_id = atoi(optarg);
				if (needed_id < 1) {
					printf("Input correct ID!\n");
					break;
				}
                
				size_t json_array_length = json_object_array_length(items);
				
                for (size_t i = 0; i < json_array_length; i++) {
                    struct json_object *item = json_object_array_get_idx(items, i);
                    struct json_object *id_obj;

                    if (json_object_object_get_ex(item, "id", &id_obj)) {                       
                        int item_id = json_object_get_int(id_obj);

                        if (item_id == needed_id) {
                            pretty_print_item(item);
                        }
                    }
                }
                break;
            }
            case 'n': {
                char *approximate_name = optarg;
				if (strlen(approximate_name) <= 2) {
					printf("Enter at least 3 characters.\n");
					break;
				}

                size_t json_array_length = json_object_array_length(items);
                MatchResult match_array[MAX_MATCHES];
                int match_count = 0;
                    
                for (size_t i = 0; i < json_array_length; i++) {
					if (match_count >= MAX_MATCHES) {
						printf("More than %d result found. %d result will be displayed.\nRefine your search.\n", MAX_MATCHES, MAX_MATCHES);
						break;
					}
                    struct json_object *item = json_object_array_get_idx(items, i);
                    struct json_object *name_obj;
                    struct json_object *id_obj;

                    if (json_object_object_get_ex(item, "name", &name_obj)) {
                        char *name = json_object_get_string(name_obj);
                        int similarity = partial_ratio(approximate_name, name);

                        if (similarity >= MAX_SIMILARITY) {
                            strncpy(match_array[match_count].name, name, sizeof(match_array[match_count].name) - 1);
                            match_array[match_count].name[sizeof(match_array[match_count].name) - 1] = '\0';

                            if (json_object_object_get_ex(item, "id", &id_obj)) {
                                match_array[match_count].id = json_object_get_int(id_obj);
                            } else {
                                match_array[match_count].id = -1;
                            }                            

                            match_count++;                            
                        }
                    }
                }

                if (match_count) {
                    for (int i = 0; i < match_count; i++) {
                        printf("%s[%d]\n", match_array[i].name, match_array[i].id);
                    }
                } else {
                    printf("Nothing :(\n");
                }
                break;
            }
            case '?':
				break;
            default:
                break;
        }
    }

    json_object_put(items);
    return 0;
}

void pretty_print_item(struct json_object *item) {
    struct json_object *id_obj;
    struct json_object *name_obj;
    struct json_object *desc_obj;
    struct json_object *bonuses_obj;
    int item_id = 0;
    
    if (json_object_object_get_ex(item, "id", &id_obj)) {
        item_id = json_object_get_int(id_obj);
    }

    if (json_object_object_get_ex(item, "name", &name_obj)) {
        printf("Name[ID]: %s[%d]\n", json_object_get_string(name_obj), item_id);
    }

    printf("========================\n");
    if (json_object_object_get_ex(item, "description", &desc_obj)) {
        printf("Description:\n%s\n", json_object_get_string(desc_obj));
    }

    printf("========================\n");
    if (json_object_object_get_ex(item, "bonuses", &bonuses_obj)) {
        printf("Bonuses:\n%s\n", json_object_get_string(bonuses_obj));
    }
}

// https://github.com/dkhaldi/levenshtein-distance/blob/master/levenshtein-distance.c
int levenshtein_distance(char *restrict s, char *restrict t) {
    int len_s = strlen(s);
    int len_t = strlen(t);

    if (strcmp(s, t) == 0) return 0;
    if (len_s == 0) return len_t;
    if (len_t == 0) return len_s;
    
    int *v0 = malloc(((len_t+1) * sizeof(int)));
    int *v1 = malloc(((len_t+1) * sizeof(int)));
    
    for (int i = 0; i < len_t+1; i++)
        v0[i] = i;

    for (int i = 0; i < len_s; i++)
    {
        v1[0] = i + 1;

        for (int j = 0; j <  len_t; j++)
        {
            int cost = (s[i] == t[j]) ? 0 : 1;
            v1[j + 1] = min3(v1[j] + 1, v0[j + 1] + 1, v0[j] + cost);
        }

        for (int j = 0; j < len_t+1; j++)
            v0[j] = v1[j];
    }
    int distance =  v1[len_t];
    free(v0);
    free(v1);
    return distance;
}

int levenshtein_partial(const char *query, const char *candidate) {
    size_t query_len = strlen(query);
    size_t cand_len = strlen(candidate);

    if (query_len > cand_len) {
        const char *temp = query;
        query = candidate;
        candidate = temp;

        size_t temp_len = query_len;
        query_len = cand_len;
        cand_len = temp_len;
    }

    if (query_len == cand_len || query_len == 0) {
        return levenshtein_distance((char*)query, (char*)candidate);
    }

    int min_dist = INT_MAX;

    char *substr = malloc(query_len + 1);
    if (!substr) {
        return INT_MAX;
    }

    for (size_t i = 0; i <= cand_len - query_len; i++) {
        strncpy(substr, candidate + i, query_len);
        substr[query_len] = '\0';

        int dist = levenshtein_distance((char*)query, substr);

        if (dist < min_dist) {
            min_dist = dist;
        }

        if (min_dist == 0) {
            break;
        }
    }

    free(substr);
    return min_dist;
}

int partial_ratio(const char *query, const char *candidate) {
    int dist = levenshtein_partial(query, candidate);
    size_t query_len = strlen(query);
    size_t cand_len = strlen(candidate);
    
    size_t shorter_len = query_len < cand_len ? query_len : cand_len;
    
    if (shorter_len == 0) {
        return 100; 
    }
    
    int ratio = (int)(100.0 - (dist * 100.0 / shorter_len));
    return ratio > 0 ? ratio : 0;
}
