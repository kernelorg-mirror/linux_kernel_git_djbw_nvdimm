#ifndef __UTIL_LIST_H__
#define __UTIL_LIST_H__
#include <util/kernel.h>
#include <linux/list.h>

#define list_next(head, pos, member) \
({ \
	typeof(pos) _pos = (pos); \
	struct list_head *_head = (head); \
	\
	_pos = list_next_entry(_pos, member); \
	if (&_pos->member == _head) \
		_pos = NULL; \
	else \
		; \
	_pos; \
})

/* TODO: add a debug mode that checks @pos is on @head */
static inline void list_del_from(struct list_head *head, struct list_head *node)
{
	list_del(node);
}
#endif /* __UTIL_LIST_H__ */
