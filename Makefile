BINDIR = bin
OBJDIR = obj

CXX = g++-8 -g -std=c++17 -Wall -Werror -Wfatal-errors
COMPFLAGS = -pthread `sdl2-config --cflags`
INCDIRS = include include/imgui include/imgui/misc/cpp
LIBDIRS =
LIBS = SDL2 GL dl stdc++fs

explorer.out: | $(BINDIR) $(OBJDIR)
	$(CXX) -o $(BINDIR)/explorer.out src/main.cpp $(COMPFLAGS) $(foreach D,$(INCDIRS),-I$D ) $(foreach D,$(LIBDIRS),-L$D ) $(foreach L,$(LIBS),-l$L )

.PHONY: explorer.out

tinflate.out: | $(BINDIR) $(OBJDIR)
	$(CXX) -o $(BINDIR)/tinflate.out tinflate/test.cpp tinflate/tinflate.c -Iinclude

clean:
	rm -f $(BINDIR)/*
	rm -f $(OBJDIR)/*

$(BINDIR):
	mkdir $(BINDIR)

$(OBJDIR):
	mkdir $(OBJDIR)
