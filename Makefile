VERSION = 0.1

CXX = g++
CC = gcc
CXXFLAGS = -std=c++11 
CFLAGS = -std=gnu11
COMMONFLAGS = -Wall -Werror -Wfatal-errors -g -O0 -fPIC -DXJOY_VERSION=\"$(VERSION)\"
MKDIR_P = mkdir -p

OUTDIR = ./bin
OBJDIR = ./obj
SRCDIR = ./src
INCDIRS = . ./includes $(BOOST_INCLUDES)
LIBDIRS = $(BOOST_LIBS)

LIBS = boost_thread boost_system pthread

ifneq (,$(LIBS))
    LIBRARIES = $(addprefix -l,$(LIBS))
    $(info $(LIBRARIES))
else
    LIBRARIES =
endif

ifneq (,$(INCDIRS))
    INCLUDES = $(addprefix -I,$(INCDIRS))
    $(info $(INCLUDES))
else
    INCLUDES =
endif

ifneq (,$(LIBDIRS))
    LFLAGS = $(addprefix -L,$(LIBDIRS))
    $(info $(LFLAGS))
else
    LFLAGS =
endif



TARGET = libxjoy.so.$(VERSION)

SOURCES = $(wildcard $(addprefix $(SRCDIR)/,*.cpp))
OBJECTS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(notdir $(SOURCES)))))

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(COMMONFLAGS) -c -o $@ $< $(INCLUDES)
	
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(COMMONFLAGS) -Wl,-soname,$@ -shared -o $(OUTDIR)/$@ $^ $(LIBRARIES) $(LFLAGS)

all: directories $(TARGET)

.PHONY: clean
clean:
	rm -f ./obj/*.o ./obj/*.d
	rm $(OUTDIR)/$(TARGET)
	
.PHONY: directories
directories: $(OUTDIR) $(OBJDIR) # Only the directores that might not exist yet

$(OBJDIR):
	$(MKDIR_P) $(OBJDIR)
	
$(OUTDIR):
	$(MKDIR_P) $(OUTDIR)

	
