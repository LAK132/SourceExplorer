BINDIR = bin
OBJDIR = obj

TCC_SRCDIR = include/tcc
TCC_LNAME = tinycc
TCC_FNAME = lib$(TCC_LNAME).a
TINYCC = $(OBJDIR)/$(TCC_FNAME)

LUA_SRCDIR = include/lua-5.3.5/src
LUA_SRCFILES = lapi lauxlib lbaselib lbitlib lcode lcorolib lctype ldblib \
ldebug ldo ldump lfunc lgc linit liolib llex lmathlib lmem loadlib lobject \
lopcodes loslib lparser lstate lstring lstrlib ltable ltablib ltm lundump \
lutf8lib lvm lzio
LUA_LNAME = lua
LUA_FNAME = lib$(LUA_LNAME).a
LUA = $(OBJDIR)/$(LUA_FNAME)

LUA_API_SRCDIR = include/lua-api-pp/luapp
LUA_API_LNAME = luaapi
LUA_API_FNAME = lib$(LUA_API_LNAME).o
LUA_API = $(OBJDIR)/$(LUA_API_FNAME)

CXX = g++-8
CC  = gcc-8
AR  = ar
RL  = ranlib

release: OPTIMISATION := -g0 -O3
release: explorer.elf

debug: OPTIMISATION := -g3 -O0
debug: explorer.elf

CXXFLAGS = $(OPTIMISATION) -no-pie -std=c++17 -Wall -Werror -Wfatal-errors -pthread `sdl2-config --cflags`
CCFLAGS  = $(OPTIMISATION) -no-pie -std=c99 -Wall -Werror -Wfatal-errors -Wno-unused-variable -Wno-unused-result -Wno-unused-function -pthread -ldl
ARFLAGS  = rcs

INCDIRS = include include/glm include/imgui include/imgui/misc/cpp include/lua-api-pp
LIBDIRS = $(OBJDIR)
LIBS = SDL2 GL dl stdc++fs lua

# explorer.elf: $(TINYCC) | $(BINDIR) $(OBJDIR)
# 	$(CXX) $(CXXFLAGS) -o $(BINDIR)/explorer.elf src/main.cpp $(TINYCC) $(foreach D,$(INCDIRS),-I$D ) $(foreach D,$(LIBDIRS),-L$D ) $(foreach L,$(LIBS),-l$L )
explorer.elf: $(LUA) $(LUA_API) | $(BINDIR) $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/explorer.elf src/main.cpp $(LUA) $(LUA_API) $(foreach D,$(INCDIRS),-I$D ) $(foreach D,$(LIBDIRS),-L$D ) $(foreach L,$(LIBS),-l$L )

.PHONY: explorer.elf

$(LUA): | $(OBJDIR)
	$(foreach S,$(LUA_SRCFILES),$(CC) $(CCFLAGS) -o $(OBJDIR)/$S.o -c $(LUA_SRCDIR)/$S.c && )\
	$(AR) $(ARFLAGS) $(LUA) $(foreach S,$(LUA_SRCFILES),$(OBJDIR)/$S.o ) && \
	$(RL) $(LUA)

$(LUA_API): $(LUA) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $(LUA_API) -c $(LUA_API_SRCDIR)/impl.cpp -I$(LUA_SRCDIR)

libtcc $(TINYCC): $(TCC_SRCDIR)/libtcc.c $(TCC_SRCDIR)/tcctools.c | $(OBJDIR)
	$(CC) $(CCFLAGS) -o $(OBJDIR)/libtcc.o -c $(TCC_SRCDIR)/libtcc.c && \
	$(CC) $(CCFLAGS) -o $(OBJDIR)/tcctools.o -c $(TCC_SRCDIR)/tcctools.c && \
	$(AR) $(ARFLAGS) $(TINYCC) $(OBJDIR)/libtcc.o $(OBJDIR)/tcctools.o && \
	$(RL) $(TINYCC)

$(TCC_SRCDIR)/libtcc.c $(TCC_SRCDIR)/tcctools.c: | $(TCC_SRCDIR)
	git submodule init && git submodule update

tinflate.elf: | $(BINDIR) $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/tinflate.elf tinflate/test.cpp tinflate/tinflate.c -Iinclude

clean:
	rm -f $(BINDIR)/*
	rm -f $(OBJDIR)/*

$(BINDIR):
	mkdir $(BINDIR)

$(OBJDIR):
	mkdir $(OBJDIR)
