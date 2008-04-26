# change these to reflect your Lua installation
# Default installation prefix
PREFIX=/usr/local

# System's libraries directory (where binary libraries are installed)
LUA_LIBDIR= $(PREFIX)/lib/lua/5.1

# System's lua directory (where Lua libraries are installed)
LUA_DIR= $(PREFIX)/share/lua/5.1

LUAINC= $(PREFIX)/include
LUALIB= $(PREFIX)/lib
LUABIN= $(PREFIX)/bin

CFLAGS= $(INCS) $(WARN)
WARN= -Wall
INCS= -I$(LUAINC)
LIBS=-lsyck -L$(LUALIB)

MYNAME=syck
OBJS= $(MYNAME).o
T= $(MYNAME).so

all:	$T test

install:
	cp -f ./syck.so $(LUA_LIBDIR)
	cp -f ./yaml.lua $(LUA_DIR)

uninstall:
	rm -f $(LUA_DIR)/yaml.lua
	rm -f $(LUA_LIBDIR)/syck.so 
	
test: $t
	$(LUABIN)/lua test.lua 
	@echo "built and tested successfully. run make install to install, or just move the libs manually"

$T:	$(OBJS)
	$(CC) -o $@ -shared $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $T core core.* a.out test.dump
	
ready:
