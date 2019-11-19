#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bhd_dns.h"
#include "bhd_bl.h"
#include "lib/hmap.h"
#include "lib/strutil.h"
#include "lib/stack.h"
#include "lib/timing.h"

struct bhd_bl
{
        struct hmap* tree;
};

#define MAX_LINE 256

static int bhd_bl_add_labels(struct bhd_bl*, struct stack*);

struct bhd_bl* bhd_bl_create(const char* p)
{
        char line[MAX_LINE];
        struct timing t;
        FILE* f;
        struct bhd_bl* bl;
        int count = 0;
        struct stack* s;

        timing_start(&t);
        f = fopen(p, "r");
        if (!f)
        {
                printf("Failed to open '%s'\n", p);
                perror("fopen");
                return NULL;
        }

        bl = malloc(sizeof(struct bhd_bl));
        if (!bl)
        {
                return NULL;
        }
        bl->tree = hmap_create(NULL, NULL, 16, 0.7f);
        if (!bl->tree)
        {
                free(bl);
                return NULL;
        }
        s = stack_create(10, STACK_AUTO_EXPAND);
        if (!s)
        {
                hmap_destroy(bl->tree);
                free(bl);
                return NULL;
        }

        while (fgets(line, MAX_LINE, f))
        {
                char* pos;
                char* start = line;
                line[MAX_LINE-1] = '\0';
                strrstrip(line);
                strlstrip(line);

                if (line[0] == '#')
                {
                        continue;
                }

                count++;
                pos = strchr(line, '.');
                while (pos)
                {
                        size_t len = pos - start;
                        char* label = malloc(len + 1);

                        if (!label)
                        {
                                printf("WARN: could not add entry block list\n");
                                goto done;
                        }

                        *pos = '\0';
                        memcpy(label, start, len + 1);
                        if (stack_push(s, label))
                        {
                                free(label);
                                printf("WARN: could not add entry block list\n");
                                goto done;
                        }
                        start = pos + 1;
                        pos = strchr(start, '.');
                        if (!pos)
                        {
                                /* Last label */
                                len = strlen(start);
                                label = malloc(len + 1);
                                if (!label)
                                {
                                        printf("WARN: could not add entry block list\n");
                                        goto done;
                                }
                                memcpy(label, start, len + 1);
                                if (stack_push(s, label))
                                {
                                        free(label);
                                        printf("WARN: could not add entry block list\n");
                                        goto done;
                                }
                        }
                }

                bhd_bl_add_labels(bl, s);
        }

done:
        fclose(f);
        stack_destroy(s);
        printf("added %d items in %ldms\n", count, timing_dur_msec(&t));

        return bl;
}

int bhd_bl_match(struct bhd_bl* bl, const struct bhd_dns_q_label* label)
{
        struct stack* s;
        char* l;
        int ret = 1;

        if (!bl)
        {
                return 0;
        }

        s = stack_create(8, STACK_AUTO_EXPAND);
        if (!s)
        {
                return 0;
        }
        while(label)
        {
                if (stack_push(s, label->label))
                {
                        printf("WARN:Failed to push\n");
                        ret = 0;
                        goto done;
                }
                label = label->next;
        }

        struct hmap* hm = bl->tree;
        while ((l = stack_pop(s)))
        {
                hm = hmap_get(hm, l);

                if (hm)
                {
                        size_t children = hmap_size(hm);
                        size_t more = stack_size(s);

                        if (children == 0)
                        {
                                /* The provided domain is an exact match or
                                   a subdomain. Block */
                                break;
                        }
                        if (more == 0)
                        {
                                /* The provided domain is not fully matched */
                                ret = 0;
                                break;
                        }
                }
                else
                {
                        /* no match */
                        ret = 0;
                        break;
                }
        }
done:
        stack_destroy(s);

        return ret;
}

void bhd_bl_free(struct bhd_bl* bl)
{
        if (!bl)
        {
                return;
        }

        struct hmap_entry* d;
        struct hmap* h;
        struct stack* s = stack_create(256, STACK_AUTO_EXPAND);
        size_t cnt;

        if (!s)
        {
                printf("Could not create stack\n");
                return;
        }

        /* each entry is key: label, and data: hmap */
        stack_push(s, bl->tree);

        while ((h = stack_pop(s)))
        {
                d = hmap_iter(h, &cnt);
                if (!d)
                {
                        printf("Failed to purge sub-tree\n");
                        continue;
                }
                for (size_t i = 0; i < cnt; i++)
                {
                        free((void*)d[i].key);
                        if (stack_push(s, d[i].data))
                        {
                                printf("Failed to push\n");
                                break;
                        }

                }
                free(d);
                hmap_destroy(h);
        }
        stack_destroy(s);
        free(bl);
}

static int bhd_bl_add_labels(struct bhd_bl* bl, struct stack* s)
{
        struct hmap* hm = bl->tree;
        char* label;

        while ((label = stack_pop(s)))
        {
                struct hmap* next;

                next = hmap_get(hm, label);

                if (next)
                {
                        free (label);
                        hm = next;
                }
                else
                {
                        next = hmap_create(NULL, NULL, 16, 0.7f);
                        if (!next)
                        {
                                printf("Failed to create subtree\n");
                                free(label);
                                return 1;
                        }
                        if (hmap_set(hm, label, next))
                        {
                                printf("Failed to set label\n");
                                free(label);
                                return 1;
                        }
                        hm = next;
                }
        }

        return 0;
}
