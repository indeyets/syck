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
%}

%option stack

COLON   :( |$)
DASH    -( |$)

%%

[a-z0-9]+   {
            }

%%

void syck_perror( char *msg )
{
}
