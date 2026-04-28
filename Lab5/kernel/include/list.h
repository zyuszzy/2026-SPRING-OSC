#ifndef LIST_H
#define LIST_H

# include "type.h"
# include "mm.h"

struct list_head{
    struct list_head *next;
    struct list_head *prev;
};

// init 空串列
static inline void INIT_LIST_HEAD(struct list_head *list){
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *new_node, struct list_head *prev, struct list_head *next){
    next->prev = new_node;
    new_node->next = next;
    new_node->prev = prev;
    prev->next = new_node;
}
// insert 到 最前面
static inline void list_add(struct list_head *new_node, struct list_head *head){
    __list_add(new_node, head, head->next);
}
// insert 到 最後
static inline void list_add_tail(struct list_head *new_node, struct list_head *head){
    __list_add(new_node, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next){
    next->prev = prev;
    prev->next = next;
}
// 刪除指定node
static inline void list_del(struct list_head *entry){
    __list_del(entry->prev, entry->next);
    // 清空 pointer
    entry->next = (struct list_head *)0;
    entry->prev = (struct list_head *)0;
}

// empty檢查
static inline int list_empty(const struct list_head *head){
    return head->next == head;
}

/**
 * list_entry - 透過member address找回結構體開頭地址
 * @ptr:    指向 struct list_head 的指標
 * @type:   結構體的類型
 * @member: list_head 在該結構體中的變數名稱
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/**
 * list_for_each - 遍歷整個鏈結串列
 * @pos:  用來iterate的 struct list_head pointer
 * @head: 串列頭部
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_safe - 安全的遍歷（允許在遍歷時刪除節點）
 * @pos:  用來iterate的 struct list_head pointer
 * @n:    暫存下一個節點的指標
 * @head: 串列頭部
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#endif