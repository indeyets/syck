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
#include "st.h"

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
#define S_ALLOC_N(type,n) (type*)malloc(sizeof(type)*(n))
#define S_ALLOC(type) (type*)malloc(sizeof(type))
#define S_REALLOC_N(var,type,n) (var)=(type*)realloc((char*)(var),sizeof(type)*(n))

#define S_ALLOCA_N(type,n) (type*)alloca(sizeof(type)*(n))

#define S_MEMZERO(p,type,n) memset((p), 0, sizeof(type)*(n))
#define S_MEMCPY(p1,p2,type,n) memcpy((p1), (p2), sizeof(type)*(n))
#define S_MEMMOVE(p1,p2,type,n) memmove((p1), (p2), sizeof(type)*(n))
#define S_MEMCMP(p1,p2,type,n) memcmp((p1), (p2), sizeof(type)*(n))

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
typedef struct _syck_file SyckIoFile;
typedef struct _syck_str SyckIoStr;
typedef struct _syck_node SyckNode;

enum syck_kind_tag {
    syck_map_kind,
    syck_seq_kind,
    syck_str_kind
};

enum map_part {
    map_key,
    map_value
};

struct _syck_node {
    // Symbol table ID
    SYMID id;
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
        struct SyckStr {
            char *ptr;
            long len;
        } *str;
    } data;
};

//
// Parser definitions
//
typedef SYMID (*SyckNodeHandler)(SyckNode *);
typedef int (*SyckIoFileRead)(char *, SyckIoFile *, int); 
typedef int (*SyckIoStrRead)(char *, SyckIoStr *, int);

enum syck_io_type {
    syck_io_str,
    syck_io_file
};

struct _syck_parser {
    // Root node
    SYMID root;
    // Scripting language function to handle nodes
    SyckNodeHandler handler;
    // IO type
    enum syck_io_type io_type;
    union {
        struct _syck_file {
            FILE *ptr;
            SyckIoFileRead read;
        } *file;
        struct _syck_str {
            char *ptr, *end;
            SyckIoStrRead read;
        } *str;
    } io;
    // Symbol table
    st_table *anchors;
};

//
// Handler prototypes
//
SYMID syck_hdlr_add_node( SyckParser *, SyckNode * );
SyckNode *syck_hdlr_add_anchor( SyckParser *, const char *, SyckNode * );
SyckNode *syck_hdlr_add_alias( SyckParser *, const char * );
SyckNode *syck_add_transfer( char *, SyckNode * );
int syck_try_implicit( SyckNode * );
void syck_fold_format( char *, SyckNode * );

//
// API prototypes
//
char *syck_strndup( char *, long );
int syck_io_file_read( char *, SyckIoFile *, int );
int syck_io_str_read( char *, SyckIoStr *, int );
SyckParser *syck_new_parser();
void syck_parser_handler( SyckParser *, SyckNodeHandler );
void syck_parser_file( SyckParser *, FILE *, SyckIoFileRead );
void syck_parser_str( SyckParser *, char *, long, SyckIoStrRead );
void syck_parser_str_auto( SyckParser *, char *, SyckIoStrRead );
void free_any_io( SyckParser * );
int syck_parser_readline( char *, SyckParser * );
int syck_parser_read( char *, SyckParser *, int );
void syck_parser_init( SyckParser *, int );
SYMID syck_parse( SyckParser * );

//
// Allocation prototypes
//
SyckNode *syck_alloc_map();
SyckNode *syck_alloc_seq();
SyckNode *syck_alloc_str();
SyckNode *syck_new_str( char * );
SyckNode *syck_new_str2( char *, long );
char *syck_str_read( SyckNode * );
SyckNode *syck_new_map( SYMID, SYMID );
void syck_map_add( SyckNode *, SYMID, SYMID );
SYMID syck_map_read( SyckNode *, enum map_part, long );
long syck_map_count( SyckNode * );
void syck_map_update( SyckNode *, SyckNode * );
SyckNode *syck_new_seq( SYMID );
void syck_seq_add( SyckNode *, SYMID );
SYMID syck_seq_read( SyckNode *, long );
long syck_seq_count( SyckNode * );

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* ifndef SYCK_H */
