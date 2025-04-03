.PHONY: all run clean
.DEFAULT_GOAL=all

# Converts "src/foo/bar/mycoolfile.cc" to "bin/obj/foo--bar--mycoolfile.o"
cc_to_o=$(addprefix $(DIR_BIN_OBJ)/,$(subst .cc,.o,$(subst /,--,$(subst src/,,$(1)))))

# Defines a rule formatted as such:
#
# bin/obj/foobar.o: src/foobar.cc
# 	...
#
# OR, if the .cc file has an accompanying .h file:
#
# bin/obj/foobar.o: src/foobar.cc src/foobar.h
# 	...
#
# Similary, it only requires $(DIR_BIN_OBJ) if it doesn't exist - we want to create the dir if it doesn't exist, but we don't want to recompile all files each time its "Last updated" changes
define o_rule
$(call cc_to_o,$(1)): $(1) $(if $(wildcard $(subst .cc,.h,$(1))),$(subst .cc,.h,$(1)),) $(if $(wildcard $(DIR_BIN_OBJ)),,$(DIR_BIN_OBJ))
	$(Q) echo "Compiling $$<..."
	$(Q) $$(CC) -MMD -MP -c $(1) -o $$@ $(FLAGS)
endef

ifeq "$(shell uname)" "Linux"
  OS:="Linux"
else
  OS:="Windows"
endif

ifndef V
  Q:=@
else
  Q:=
endif

CC:=x86_64-w64-mingw32-gcc
WINDRES:=x86_64-w64-mingw32-windres

DIR_SRC:=src
DIR_RES:=res
DIR_BIN:=bin
DIR_BIN_OBJ:=$(DIR_BIN)/obj
DIR_BIN_RES:=$(DIR_BIN)/res

EXE:=$(DIR_BIN)/traybot.exe

RES_FILE:=$(DIR_BIN_RES)/traybot.res
RC_FILE:=$(DIR_RES)/traybot.rc

CC_FILES:=$(wildcard $(DIR_SRC)/*.cc) $(wildcard $(DIR_SRC)/*/*.cc)
H_FILES:=$(wildcard $(DIR_SRC)/*.h) $(wildcard $(DIR_SRC)/*/*.h)
O_FILES:=$(foreach f,$(CC_FILES),$(call cc_to_o,$(f)))
D_FILES:=$(foreach f,$(O_FILES),$(subst .o,.d,$(f)))

FLAGS:=-mwindows #-static-libgcc -static-libstdc++

-include $(D_FILES)

$(foreach f,$(CC_FILES),$(eval $(call o_rule,$(f))))

$(DIR_BIN):
	$(Q) mkdir -p $(DIR_BIN)

$(DIR_BIN_OBJ): $(DIR_BIN)
	$(Q) mkdir -p $(DIR_BIN_OBJ)

$(DIR_BIN_RES): $(DIR_BIN)
	$(Q) mkdir -p $(DIR_BIN_RES)

$(RES_FILE): $(DIR_BIN_RES)
	$(Q) echo "Creating $(RES_FILE)..."
	$(Q) $(WINDRES) $(RC_FILE) -O coff -o $(RES_FILE)

$(EXE): $(O_FILES) $(RES_FILE)
	$(Q) echo "Compiling $(EXE)..."
	$(Q) $(CC) $(FLAGS) $(O_FILES) $(RES_FILE) -o $(EXE)

all: $(EXE)

clean:
	$(Q) rm -rf $(DIR_BIN)

run: $(EXE)
	$(Q) echo "Running $(EXE)..."
	$(Q) $(EXE)