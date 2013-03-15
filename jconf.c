#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "jconf.h"
#include "json.h"
#include "string.h"

#define INT_DIGITS 19		/* enough for 64 bit integer */

static char *itoa(int i)
{
    /* Room for INT_DIGITS digits, - and '\0' */
    static char buf[INT_DIGITS + 2];
    char *p = buf + INT_DIGITS + 1;	/* points to terminating '\0' */
    if (i >= 0) {
        do {
            *--p = '0' + (i % 10);
            i /= 10;
        } while (i != 0);
        return p;
    }
    else {			/* i < 0 */
        do {
            *--p = '0' - (i % 10);
            i /= 10;
        } while (i != 0);
        *--p = '-';
    }
    return p;
}

static char *to_string(const json_value *value) {
    if (value->type == json_string) {
        return strndup(value->u.string.ptr, value->u.string.length);
    } else if (value->type == json_integer) {
        return strdup(itoa(value->u.integer));
    } else if (value->type == json_null) {
        return "null";
    } else {
        LOGE("%d\n", value->type);
        FATAL("Invalid config format.\n");
    }
    return 0;
}

static int to_int(const json_value *value) {
    if (value->type == json_string) {
        return atoi(value->u.string.ptr);
    } else if (value->type == json_integer) {
        return value->u.integer;
    } else {
        FATAL("Invalid config format.\n");
    }
    return 0;
}

jconf_t *read_jconf(const char* file) {

    static jconf_t conf;

    char *buf;
    json_value *obj;

    FILE *f = fopen(file, "r");
    if (f == NULL) FATAL("Invalid config path.\n");

    fseek(f, 0, SEEK_END);
    long pos = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (pos >= MAX_CONF_SIZE) FATAL("Too large config file.\n");

    buf = malloc(pos);
    if (buf == NULL) FATAL("No enough memory.\n");

    fread(buf, pos, 1, f);
    fclose(f);

    obj = json_parse(buf);
    if (obj->type == json_object) {
        int i, j;
        for (i = 0; i < obj->u.object.length; i++) {
            char *name = obj->u.object.values[i].name;
            json_value *value = obj->u.object.values[i].value;
            if (strcmp(name, "server") == 0) {
                if (value->type == json_array) {
                    for (j = 0; j < value->u.array.length; j++) {
                        if (j >= MAX_REMOTE_NUM) break;
                        json_value *v = value->u.array.values[j];
                        conf.remote_host[j] = to_string(v);
                        conf.remote_num = j + 1;
                    }
                } else if (value->type == json_string) {
                    conf.remote_host[0] = to_string(value);
                    conf.remote_num = 1;
                }
            } else if (strcmp(name, "server_port") == 0) {
                conf.remote_port = to_string(value);
            } else if (strcmp(name, "local_port") == 0) {
                conf.local_port = to_string(value);
            } else if (strcmp(name, "password") == 0) {
                conf.password = to_string(value);
            } else if (strcmp(name, "method") == 0) {
                conf.method = to_string(value);
            } else if (strcmp(name, "timeout") == 0) {
                conf.timeout = to_string(value);
            }
        }
    } else {
        FATAL("Invalid config file\n");
    }

    free(buf);
    json_value_free(obj);
    return &conf;

}
