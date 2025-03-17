CC=gcc
OPTS=-O2 -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-function
DEBUG=-g

SRCDIR=src
OBJDIR=obj
INCDIR=inc

INCLUDE=$(addprefix -I,$(INCDIR))
CFLAGS=$(OPTS) $(INCLUDE) $(DEBUG)

TARGET=ffmpeg-gui

SRCS=$(TARGET).c
ifeq ($(OS),Windows_NT)
	# SRCS+=
	RM=del /q
	RMDIR=del /s /q
	EXEC=$(TARGET).exe
	MKDIR=-mkdir $(OBJDIR) > nul 2>&1
else
	# SRCS+=
	RM=rm -f
	RMDIR=rm -rf
	EXEC=$(TARGET)
	MKDIR=mkdir -p $(OBJDIR)
endif
SOURCES=$(addprefix $(SRCDIR)/,$(SRCS))
OBJECTS=$(addprefix $(OBJDIR)/,$(SRCS:.c=.o))

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	$(MKDIR)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ -lm

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RMDIR) $(OBJDIR)
	$(RM) $(EXEC)