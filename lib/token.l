%{
//
// token.l
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"
#include "gram.h"
%}

%option stack
%option noyywrap

COLON   :( |$)
DASH    -( |$)

%%

[a-z0-9]+   {
            }

%%

void syck_perror( char *msg )
{
}
