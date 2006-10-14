/*
docCopyright("Steve Dekorte", 2002)
docLicense("BSD revised")
*/

#ifndef IOYAML_DEFINED
#define IOYAML_DEFINED 1

#include "IoObject.h"
#include "IoSeq.h"
#include "IoList.h"
#include "syck.h"

#define ISYAML(self) IoObject_hasCloneFunc_(self, (TagCloneFunc *)IoYAML_rawClone)

typedef IoObject IoYAML;

typedef struct
{
    IoSymbol *path;
} IoYAMLData;

IoYAML *IoYAML_rawClone(IoYAML *self);
IoYAML *IoYAML_proto(void *state);
IoYAML *IoYAML_new(void *state);
IoYAML *IoYAML_newWithPath_(void *state, IoSymbol *path);

void IoYAML_free(IoYAML *self);
void IoYAML_mark(IoYAML *self);

IoObject *IoYAML_load(IoYAML *self, IoObject *locals, IoMessage *m);
IoObject *IoYAML_dump(IoYAML *self, IoObject *locals, IoMessage *m);

#endif
