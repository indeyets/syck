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

#include <stdio.h>

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

typedef struct _syck_parser SyckParser;
typedef struct _syck_file SyckFile;
typedef struct _syck_str SyckStr;

enum syck_kind_tag {
    syck_map_kind,
    syck_seq_kind,
    syck_str_kind
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
// Parser definitions
//
typedef SYMID (*SyckNodeHandler)(struct SyckNode *);
typedef char *(*SyckFileRead)(SyckFile *); 
typedef char *(*SyckStrRead)(SyckStr *);

enum syck_io_type {
    syck_io_str,
    syck_io_file
};

struct _syck_parser {
    // Scripting language function to handle nodes
    SyckNodeHandler handler;
    // IO type
    enum syck_io_type io_type;
    union {
        struct _syck_file {
            FILE *ptr;
            SyckFileRead read;
        } *file;
        struct _syck_str {
            char *ptr, *end;
            SyckStrRead read;
        } *str;
    } io;
};

//
// API prototypes
//
char *syck_io_file_read( SyckFile * );
char *syck_io_str_read( SyckStr * );
SyckParser *syck_new_parser();
void syck_parser_handler( SyckParser *, SyckNodeHandler );
void syck_parser_file( SyckParser *, FILE *, SyckFileRead );
void syck_parser_str( SyckParser *, char *, long, SyckStrRead );
void syck_parser_str_auto( SyckParser *, char *, SyckStrRead );
void free_any_io( SyckParser * );
SYMID syck_parse( SyckParser * );

//
// Allocation prototypes
//
struct SyckNode *syck_new_str( char * );
char *syck_str_read( struct SyckNode * );
struct SyckNode *syck_new_map( SYMID, SYMID );
void syck_map_add( struct SyckNode *, SYMID, SYMID );
SYMID syck_map_read( struct SyckNode *, enum map_part, long );
long syck_map_count( struct SyckNode * );
struct SyckNode *syck_new_seq( SYMID );
void syck_seq_add( struct SyckNode *, SYMID );
SYMID syck_seq_read( struct SyckNode *, long );
long syck_seq_count( struct SyckNode * );

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* ifndef SYCK_H */
