//
// syck.h
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#ifndef SYCK_H
#define SYCK_H

#if defined(__cplusplus)
extern "C" {
#endif

//
// Memory Allocation
// 
#if defined(HAVE_ALLOCA_H) && !defined(__GNUC__)
#include <alloca.h>
#endif

#define ALLOC_N(type,n) (type*)malloc(sizeof(type)*(n))
#define ALLOC(type) (type*)malloc(sizeof(type))
#define REALLOC_N(var,type,n) (var)=(type*)realloc((char*)(var),sizeof(type)*(n))

#define ALLOCA_N(type,n) (type*)alloca(sizeof(type)*(n))

#define MEMZERO(p,type,n) memset((p), 0, sizeof(type)*(n))
#define MEMCPY(p1,p2,type,n) memcpy((p1), (p2), sizeof(type)*(n))
#define MEMMOVE(p1,p2,type,n) memmove((p1), (p2), sizeof(type)*(n))
#define MEMCMP(p1,p2,type,n) memcmp((p1), (p2), sizeof(type)*(n))

#ifndef NULL
# define NULL (void *)0
#endif

//
// Node definitions
//
enum syck_kind_tag {
    map_kind,
    seq_kind,
    str_kind
};

struct SyckNode {
    enum syck_kind_tag kind;
    union {
        struct SyckMap {
            struct SyckNode **keys;
            struct SyckNode **values;
            long capa;
        } *pairs;
        struct SyckSeq {
            struct SyckNode **values;
            long capa;
        } *list;
        char *str;
    } data;
};


//
// Parser definition
//
//struct SyckParser {
//    FILE *fp;
//    char *ptr, *end;
//    void (*hdlr) (struct SyckNode*);
//};

//
// API prototypes
//
struct SyckNode *new_str_node( char *str );
struct SyckNode *new_map_node( struct SyckNode *key, struct SyckNode *value );
void add_map_pair( struct SyckNode *map, struct SyckNode *key, struct SyckNode *value );

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* ifndef SYCK_H */
