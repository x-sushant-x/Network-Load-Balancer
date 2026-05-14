#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../types.h"

BackendPool* load_backend() {
    FILE *f = fopen("/Users/sushantdhiman/C/Load-Balancer/servers.json", "r");
    if (!f) {
        perror("fopen failed");
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek failed");
        fclose(f);
        return NULL;
    }

    long size = ftell(f);
    if (size < 0) {
        perror("ftell failed");
        fclose(f);
        return NULL;
    }

    rewind(f);

    char *data = malloc(size + 1);
    if (!data) {
        perror("malloc failed");
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(data, 1, size, f);
    if (read_bytes != (size_t)size) {
        fprintf(stderr, "fread failed\n");
        free(data);
        fclose(f);
        return NULL;
    }

    data[size] = '\0';
    fclose(f);

    cJSON *json = cJSON_Parse(data);
    if (!json) {
        fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
        free(data);
        return NULL;
    }

    if (!cJSON_IsArray(json)) {
        fprintf(stderr, "Expected JSON array\n");
        cJSON_Delete(json);
        free(data);
        return NULL;
    }

    int count = cJSON_GetArraySize(json);

    BackendPool *pool = malloc(sizeof(BackendPool));
    if (!pool) {
        perror("malloc failed");
        cJSON_Delete(json);
        free(data);
        return NULL;
    }

    pool->backends = calloc(count, sizeof(Backend));
    if (!pool->backends) {
        perror("calloc failed");
        free(pool);
        cJSON_Delete(json);
        free(data);
        return NULL;
    }

    pool->count = count;
    pool->current = 0;

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(json, i);
        if (!cJSON_IsObject(item)) {
            fprintf(stderr, "Invalid backend entry at index %d\n", i);
            goto error;
        }

        cJSON *ip = cJSON_GetObjectItem(item, "ip");
        cJSON *port = cJSON_GetObjectItem(item, "port");
        cJSON *hc_port = cJSON_GetObjectItem(item, "health_check_port");

        if (!cJSON_IsString(ip) || !cJSON_IsNumber(port) || !cJSON_IsNumber(hc_port)) {
            fprintf(stderr, "Invalid fields in backend %d\n", i);
            goto error;
        }

        pool->backends[i].IP = strdup(ip->valuestring);
        if (!pool->backends[i].IP) {
            perror("strdup failed");
            goto error;
        }

        pool->backends[i].port = port->valueint;
        pool->backends[i].health_check_port = hc_port->valueint;
        pool->backends[i].is_healthy = 1;
    }

    cJSON_Delete(json);
    free(data);
    return pool;

error:
    // cleanup partially allocated resources
    for (int j = 0; j < count; j++) {
        free(pool->backends[j].IP);
    }
    free(pool->backends);
    free(pool);
    cJSON_Delete(json);
    free(data);
    return NULL;
}
