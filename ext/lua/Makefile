# change these to reflect your Lua installation
LUA=/usr/
LUAINC= $(LUA)/include/lua50
LUALIB= $(LUA)/lib
LUABIN= $(LUA)/bin

CFLAGS= $(INCS) $(WARN)
WARN= -Wall
INCS= -I$(LUAINC)
LIBS=-lsyck

MYNAME= syck
MYLIB= l$(MYNAME)

OBJS= $(MYLIB).o

T= $(MYLIB).so

all:	$T

test:	$T
	$(LUABIN)/lua test.lua 

$T:	$(OBJS)
	$(CC) -o $@ -shared $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $T core core.* a.out
