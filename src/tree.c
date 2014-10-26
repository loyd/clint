/*!
 * @brief It implements interface of the tree.
 */

#include "clint.h"


list_t new_list(void)
{
    list_t list = xmalloc(sizeof(*list));
    list->first = list->last = NULL;
    return list;
}


void add_to_list(list_t list, tree_t data)
{
    assert(list && entry);

    struct list_entry_s *entry = xmalloc(sizeof(*entry));
    entry->next = NULL;
    entry->data = data;

    if (list->last)
        list->last->next = entry;
    else
        list->first = list->last = entry;
}


tree_t new_transl_unit(list_t entities)
{
    struct transl_unit_s *res = xmalloc(sizeof(*res));
    res->entities = entities;
    return res;
}

//#TODO: add the rest constructors.
