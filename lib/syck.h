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

#define ALLOC_CT 8
#define ALLOC_N(type,n) (type*)malloc(sizeof(type)*(n))
#define ALLOC(type) (type*)malloc(sizeof(type))
#define REALLOC_N(var,type,n) (var)=(type*)realloc((char*)(var),sizeof(type)*(n))

#define ALLOCA_N(type,n) (type*)alloca(sizeof(type)*(n))

#define MEMZERO(p,type,n) memset((p), 0, sizeof(type)*(n))
#define MEMCPY(p1,p2,type,n) memcpy((p1), (p2), sizeof(type)*(n))
#define MEMMOVE(p1,p2,type,n) memmove((p1), (p2), sizeof(type)*(n))
#define MEMCMP(p1,p2,type,n) memcmp((p1), (p2), sizeof(type)*(n))

#if DEBUG
  void syck_assert( char *, unsigned );
# define ASSERT(f) \
    if ( f ) \
        {}   \
    else     \
        syck_assert( __FILE__, __LINE__ )
#else
# define ASSERT(f)
#endif

#ifndef NULL
# define NULL (void *)0
#endif

//
// Node definitions
//
#define SYMID unsigned long

enum syck_kind_tag {
    map_kind,
    seq_kind,
    str_kind
};

enum map_part {
    map_key,
    map_value
};

struct SyckNode {
    // Underlying kind
    enum syck_kind_tag kind;
    // Fully qualified tag-uri for type
    char *type_id;
    union {
        // Storage for map data
        struct SyckMap {
            SYMID *keys;
            SYMID *values;
            long capa;
            long idx;
        } *pairs;
        // Storage for sequence data
        struct SyckSeq {
            SYMID *items;
            long capa;
            long idx;
        } *list;
        // Storage for string data
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
struct SyckNode *new_str_node( char * );
char *read_str_node( struct SyckNode * );
struct SyckNode *new_map_node( SYMID, SYMID );
void add_map_pair( struct SyckNode *, SYMID, SYMID );
SYMID read_map_node( struct SyckNode *, enum map_part, long );
long read_map_count( struct SyckNode * );
struct SyckNode *new_seq_node( SYMID );
void add_seq_item( struct SyckNode *, SYMID );
SYMID read_seq_node( struct SyckNode *, long );
long read_seq_count( struct SyckNode * );

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* ifndef SYCK_H */
