export PATH:=/usr/local/bin:$(PATH)
export PKG_CONFIG_PATH:=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig
#calc current
ifeq ($(shell uname),Darwin)
SO_NAME:=install_name
TRIM_SYMBO=cp $@ $(TARGET_SYM_FOLDER)/$(notdir $@)$(SYM_SUBFFIX) > /dev/null && strip -S $@ > /dev/null
XARGS_LINK=-I F ln -sf $(CURR_DIR)/"F"
FRAMEWORK_FLAG:=-framework CoreServices
ifndef SHARE_SUBFFIX
SHARE_SUBFFIX:=.dylib
LD_UNDEF_FLAG:=-Wl,-undefined,error
endif
else
SO_NAME:=soname
TRIM_SYMBO=objcopy --only-keep-debug $@ $(TARGET_SYM_FOLDER)/$(notdir $@)$(SYM_SUBFFIX) > /dev/null && strip --strip-debug --strip-unneeded $@ > /dev/null
XARGS_LINK=-i ln -sf $(CURR_DIR)/{}
endif
CURR_DIR:=$(shell pwd)
PROJECT_NAME:=$(notdir $(PROJECT_HOME))
ifndef SHARE_SUBFFIX
SHARE_SUBFFIX:=.so
LD_UNDEF_FLAG:=-Wl,--no-undefined
FRAMEWORK_FLAG:=
endif
#calc current end

#platform value
ifndef MAKE_NAME
MAKE_NAME:=Makefile
endif
ifndef PKG_CONFIG
PKG_CONFIG:=pkg-config
endif
ifndef BIN_SUBFFIX
BIN_SUBFFIX:=
endif
ifndef STATIC_SUFFIX
STATIC_SUFFIX:=.a
endif
ifndef OBJ_SUBFFIX
OBJ_SUBFFIX:=.o
endif
ifndef DEP_SUBFFIX
DEP_SUBFFIX:=.d
endif
ifndef DEP_TMP_SUBFFIX
DEP_TMP_SUBFFIX:=.td
endif
ifndef C_SUBFFIX
C_SUBFFIX:=.c
endif
ifndef CPP_SUBFFIX
CPP_SUBFFIX:=.cpp
endif
ifndef SYM_SUBFFIX
SYM_SUBFFIX:=.sym
endif
ifndef LIB_PREFIX
LIB_PREFIX:=lib
endif
ifndef LIB_DIR
LIB_DIR:=lib64
endif
ifndef BIN_DIR
BIN_DIR:=bin
endif
ifndef OBJ_DIR
OBJ_DIR:=obj
endif
ifndef SRC_DIR
SRC_DIR:=src
endif
ifndef INC_DIR
INC_DIR:=inc
endif
ifndef SYMBOL_DIR
SYMBOL_DIR:=symbol
endif
SRC_FOLDER:=$(CURR_DIR)/$(SRC_DIR)
INC_FOLDER:=$(CURR_DIR)/$(INC_DIR)
#platform value end

#can set in $(MAKE_NAME), otherwise using default start
ifeq ($(CONFIG),)
	CONFIG:=release
endif
ifeq ($(VERSION),)
	VERSION:=0.0.1
endif
ifeq ($(TYPE),)
	TYPE=NONE
else 
ifneq ($(TYPE),EXE)
ifneq ($(TYPE),STATIC)
ifneq ($(TYPE),SHARE)
ifneq ($(TYPE),REF)
ifneq ($(TYPE),NONE)
$(error Only support target type EXE STATIC SHARE and NONE)
endif
endif
endif
endif
endif
endif
ifeq ($(STD),)
	STD:=c++17
endif
#can set in $(MAKE_NAME), otherwise using default end

#calc version
MAIN_VERSION=$(shell echo $(VERSION)|awk -F'.' '{print $$1}')
SUB_VERSION=$(shell echo $(VERSION)|awk -F'.' '{print $$1"."$$2}')
#calc version end


#calc subfolder
SUB_PATH:=$(shell find . -name $(MAKE_NAME)|grep '^\./[a-zA-Z0-9_\-]*/$(MAKE_NAME)$$'|awk -F'/' "{print \"$(CURR_DIR)\"\"/\"\$$2}")
#calc subfolder end

#calc build path
PROJECT_BUILD:=$(PROJECT_HOME)_build/$(PROJECT_NAME)
ifeq ($(CURR_DIR),$(PROJECT_HOME))
TARGET_BUILD_FOLDER:=$(PROJECT_BUILD)
else
TARGET_BUILD_FOLDER:=$(patsubst $(PROJECT_HOME)/%,$(PROJECT_BUILD)/%,$(CURR_DIR))
endif
#calc build path end

#calc deps
INC_PATHS+=$(INC_FOLDER)
define get_deps
$(strip $1 $(filter-out $1,$(foreach item,$1,$(call get_deps,$(shell if [ -e $(PROJECT_HOME)/$(item)/$(MAKE_NAME) ]; then grep 'DEPS:=' $(PROJECT_HOME)/$(item)/$(MAKE_NAME)|grep -v '^[ \t]*\#';fi|awk -F'=' '{print $$2}')))))
endef
DEP_PATH:=$(shell for dir in $(DEPS);do if [ -e != $(PROJECT_HOME)/$${dir}/$(MAKE_NAME) ]; then if [ "" != "$$(grep 'TYPE:=\(STATIC\)\|\(SHARE\)' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#')" ];then echo $(PROJECT_HOME)/$${dir};fi;fi;done;)
ALL_DEPS:=$(call get_deps,$(DEPS))
DEP_STATIC:=$(shell for dir in $(ALL_DEPS);do if [ -e != $(PROJECT_HOME)/$${dir}/$(MAKE_NAME) ]; then if [ "" != "$$(grep 'TYPE:=STATIC' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#')" ];then echo $(PROJECT_BUILD)/$${dir}/$(LIB_PREFIX)$$(basename $${dir})$(STATIC_SUFFIX);fi;fi;done;)
INC_PATHS+=$(shell for dir in $(ALL_DEPS);do echo $(PROJECT_HOME)/$${dir}/$(INC_DIR); if [ -e != $(PROJECT_HOME)/$${dir}/$(MAKE_NAME) ]; then grep 'INC_PATHS:=' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#'|awk -F'=' '{print $$2}';fi;done;)
LIB_PATHS+=$(shell for dir in $(ALL_DEPS);do if [ -e != $(PROJECT_HOME)/$${dir}/$(MAKE_NAME) ]; then if [ "" != "$$(grep 'TYPE:=\(STATIC\)\|\(SHARE\)' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#')" ];then echo $(PROJECT_BUILD)/$${dir}/$(LIB_DIR);grep 'LIB_PATHS:=' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#'|awk -F'=' '{print $$2}';fi;fi;done;)
LIBS+=$(shell for dir in $(ALL_DEPS);do if [ -e != $(PROJECT_HOME)/$${dir}/$(MAKE_NAME) ]; then if [ "" != "$$(grep 'TYPE:=\(STATIC\)\|\(SHARE\)' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#')" ];then basename $${dir};grep 'LIBS:=' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#'|awk -F'=' '{print $$2}';fi;fi;done;)
DEP_PKGS+=$(shell for dir in $(ALL_DEPS);do if [ -e != $(PROJECT_HOME)/$${dir}/$(MAKE_NAME) ]; then grep 'DEP_PKGS:=' $(PROJECT_HOME)/$${dir}/$(MAKE_NAME)|grep -v '^[ \t]*\#'|awk -F'=' '{print $$2}';fi;done|sort -u;)
INC_PATHS:=-I$(PROJECT_HOME)/$(INC_DIR) $(addprefix -I,$(INC_PATHS))
LIB_PATHS:=$(addprefix -L,$(LIB_PATHS))
LIBS:=$(addprefix -l,$(LIBS))
MACROS:=
#calc deps end

#calc dep_pkgs
DEP_PKGS:=$(strip $(DEP_PKGS))
ifdef DEP_PKGS
PKG_COMPILE_FLAGS:=$(shell $(PKG_CONFIG) --cflags $(DEP_PKGS))
PKG_LINK_FLAGS:=$(shell $(PKG_CONFIG) --static --libs $(DEP_PKGS))
else
PKG_COMPILE_FLAGS:=
PKG_LINK_FLAGS:=
endif
#calc dep_pkgs end

ifeq ($(CONFIG),release)
PROJECT_TARGET_PATH:=$(PROJECT_HOME)_build/$(PROJECT_NAME)_release
BASE_COMPILE_FLAG+:= -O3
else
PROJECT_TARGET_PATH:=$(PROJECT_HOME)_build/$(PROJECT_NAME)_debug
endif
PROJECT_BIN=$(PROJECT_TARGET_PATH)/$(BIN_DIR)
PROJECT_LIB=$(PROJECT_TARGET_PATH)/$(LIB_DIR)

#calc command
TARGET_NAME:=$(notdir $(CURR_DIR))
MACROS+=-DTARGET_NAME=$(TARGET_NAME) -DPYINIT=PyInit_$(TARGET_NAME)
TARGET_OBJ_FOLDER:=$(TARGET_BUILD_FOLDER)/$(OBJ_DIR)
TARGET_SYM_FOLDER:=$(TARGET_BUILD_FOLDER)/$(SYMBOL_DIR)
ifeq ($(TYPE),EXE)
TARGET_FOLDER:=$(TARGET_BUILD_FOLDER)/$(BIN_DIR)
TARGET_LINK_NAME:=$(PROJECT_BIN)/$(TARGET_NAME)$(BIN_SUBFFIX)
TARGET_NAME:=$(TARGET_FOLDER)/$(TARGET_NAME)$(BIN_SUBFFIX)
TARGET_MAIN_VERSION_NAME:=$(TARGET_NAME).$(MAIN_VERSION)
TARGET_SUB_VERSION_NAME:=$(TARGET_NAME).$(SUB_VERSION)
TARGET_VERSION_NAME:=$(TARGET_NAME).$(VERSION)
BUILD_CMD=$(BUILD_EXE)
DEP_STATIC_LIBS:=$(DEP_STATIC)
else
ifeq ($(TYPE),STATIC)
TARGET_FOLDER:=$(TARGET_BUILD_FOLDER)/$(LIB_DIR)
TARGET_LINK_NAME:=$(PROJECT_LIB)/$(LIB_PREFIX)$(TARGET_NAME)$(STATIC_SUFFIX)
TARGET_NAME:=$(TARGET_FOLDER)/$(LIB_PREFIX)$(TARGET_NAME)$(STATIC_SUFFIX)
TARGET_MAIN_VERSION_NAME:=$(TARGET_NAME).$(MAIN_VERSION)
TARGET_SUB_VERSION_NAME:=$(TARGET_NAME).$(SUB_VERSION)
TARGET_VERSION_NAME:=$(TARGET_NAME).$(VERSION)
BUILD_CMD=$(BUILD_STATIC)
DEP_STATIC_LIBS:=
else
ifeq ($(TYPE),SHARE)
TARGET_FOLDER:=$(TARGET_BUILD_FOLDER)/$(LIB_DIR)
TARGET_LINK_NAME:=$(PROJECT_LIB)/$(LIB_PREFIX)$(TARGET_NAME)$(SHARE_SUBFFIX).$(MAIN_VERSION)
TARGET_NAME:=$(TARGET_FOLDER)/$(LIB_PREFIX)$(TARGET_NAME)$(SHARE_SUBFFIX)
TARGET_MAIN_VERSION_NAME:=$(TARGET_NAME).$(MAIN_VERSION)
TARGET_SUB_VERSION_NAME:=$(TARGET_NAME).$(SUB_VERSION)
TARGET_VERSION_NAME:=$(TARGET_NAME).$(VERSION)
BUILD_CMD=$(BUILD_SHARE)
DEP_STATIC_LIBS:=
else
ifeq ($(TYPE),REF)
TARGET_FOLDER:=
TARGET_NAME:=
TARGET_MAIN_VERSION_NAME:=
TARGET_SUB_VERSION_NAME:=
TARGET_VERSION_NAME:=
BUILD_CMD=
DEP_STATIC_LIBS:=
else
TARGET_FOLDER:=
TARGET_NAME:=
TARGET_MAIN_VERSION_NAME:=
TARGET_SUB_VERSION_NAME:=
TARGET_VERSION_NAME:=
BUILD_CMD=
DEP_STATIC_LIBS:=
endif
endif
endif
endif

ifeq ($(CONFIG),release)
TARGET_DEP_FOLDER:=$(TARGET_FOLDER) $(TARGET_SYM_FOLDER)
else
TARGET_DEP_FOLDER:=$(TARGET_FOLDER)
endif

BASE_COMPILE_FLAG:= $(MACROS) -c -fPIC -Werror -Wfatal-errors -Wformat=2 -Winit-self -Wswitch-default -Wall -Wextra -g -std=$(STD)
C_FLAGS+=$(BASE_COMPILE_FLAG)
CPP_FLAGS+=$(BASE_COMPILE_FLAG)
EXE_FLAGS+=
SHARE_FLAGS+=-shared $(LD_UNDEF_FLAG) -Wl,-$(SO_NAME),$(notdir $(TARGET_MAIN_VERSION_NAME))
MKDIR=mkdir -p
LN=ln -sf $^ $@
MAKE=make -s
CC=gcc
CXX=g++
RM=rm -rf
AR=ar rcs
ifeq ($(CONFIG),release)
TRIM_CMD= && $(TRIM_SYMBO)
else
TRIM_CMD=
endif
CCOMPILER=$(CC) -MT $@ -MMD -MP -MF $(TARGET_OBJ_FOLDER)/$*$(DEP_TMP_SUBFFIX) $(C_FLAGS) $(PKG_COMPILE_FLAGS) $(INC_PATHS) -o $@ $<
CXXCOMPILER=$(CXX) -MT $@ -MMD -MP -MF $(TARGET_OBJ_FOLDER)/$*$(DEP_TMP_SUBFFIX) $(CPP_FLAGS) $(PKG_COMPILE_FLAGS) $(INC_PATHS) -o $@ $<
DEP_MV=mv $(TARGET_OBJ_FOLDER)/$*$(DEP_TMP_SUBFFIX) $(TARGET_OBJ_FOLDER)/$*$(DEP_SUBFFIX)
BUILD_STATIC=$(AR) $@ $(filter %$(OBJ_SUBFFIX), $^)
BUILD_SHARE=$(CXX) $(FRAMEWORK_FLAG) $(SHARE_FLAGS) $(LIB_PATHS) -o $@ $(filter %$(OBJ_SUBFFIX), $^) $(LIBS) $(PKG_LINK_FLAGS) $(TRIM_CMD)
BUILD_EXE=$(CXX) $(FRAMEWORK_FLAG) $(EXE_FLAGS) $(LIB_PATHS) -o $@ $(filter %$(OBJ_SUBFFIX), $^) $(LIBS) $(PKG_LINK_FLAGS) $(TRIM_CMD)
CLEAN_SUB=for sub in $(SUB_PATH); do $(MAKE) -C $${sub} $(CLEAN); done
CLEAN_CMD=$(RM) $(TARGET_FOLDER)/* $(TARGET_OBJ_FOLDER)/* $(TARGET_SYM_FOLDER)/*
SUB_CMD=for sub in $(SUB_PATH); do $(MAKE) -C $${sub}; done
DEP_CMD=for dep in $(DEP_PATH); do $(MAKE) -C $${dep} $(SELF); done
#calc subfolder


# rules
ALL:=all
DIR:=dir
DEP:=dep
SUB:=sub
SELF:=self
CLEAN:=clean

.PHONY: $(ALL)
.PHONY: $(DIR)
.PHONY: $(DEP)
.PHONY: $(SUB)
.PHONY: $(SELF)
.PHONY: $(CLEAN)

$(ALL): $(DIR) $(SELF) $(SUB)
$(DIR):
	@$(RM) $(PROJECT_BUILD) && if [ ! -d $(PROJECT_TARGET_PATH) ];then $(MKDIR) $(PROJECT_TARGET_PATH);fi;ln -sf $(PROJECT_TARGET_PATH) $(PROJECT_BUILD) && if [ ! -d $(PROJECT_BIN) ];then $(MKDIR) $(PROJECT_BIN);fi && if [ ! -d $(PROJECT_LIB) ];then $(MKDIR) $(PROJECT_LIB);fi
ifeq ($(TYPE),NONE)
$(SELF):
$(CLEAN):
	@$(CLEAN_SUB)
else
ifeq ($(TYPE),REF)
$(SELF):
	@$(MKDIR) $(TARGET_BUILD_FOLDER) && $(RM) $(TARGET_BUILD_FOLDER)/* && ls -l $(CURR_DIR)/|grep -v '^d'|awk  '{print $$9}'|grep -v '^$(MAKE_NAME)$$'|sed '/^$$/d'|xargs $(XARGS_LINK) $(TARGET_BUILD_FOLDER)/
$(CLEAN):
	@$(RM) $(TARGET_BUILD_FOLDER)/* && $(CLEAN_SUB)
else
$(SELF): $(TARGET_LINK_NAME)
$(CLEAN):
	@$(CLEAN_CMD) && $(CLEAN_SUB)
endif
endif
$(SUB):
	@$(SUB_CMD)
$(DEP):
	@$(DEP_CMD)

SOURCES:=$(wildcard $(SRC_FOLDER)/*$(C_SUBFFIX)) $(wildcard $(SRC_FOLDER)/*$(CPP_SUBFFIX))
OBJECTS:=$(notdir $(SOURCES))
OBJECTS:=$(OBJECTS:%$(C_SUBFFIX)=%$(OBJ_SUBFFIX))
OBJECTS:=$(OBJECTS:%$(CPP_SUBFFIX)=%$(OBJ_SUBFFIX))
OBJECTS:=$(addprefix $(TARGET_OBJ_FOLDER)/, $(OBJECTS))
$(TARGET_LINK_NAME): $(TARGET_NAME)
	@$(LN)
$(TARGET_NAME): $(TARGET_MAIN_VERSION_NAME)
	@$(LN)
$(TARGET_MAIN_VERSION_NAME): $(TARGET_SUB_VERSION_NAME)
	@$(LN)
$(TARGET_SUB_VERSION_NAME): $(TARGET_VERSION_NAME)
	@$(LN)
$(TARGET_VERSION_NAME): $(DEP)
$(TARGET_VERSION_NAME): $(DEP_STATIC_LIBS)
$(TARGET_VERSION_NAME): $(OBJECTS)
	@$(MKDIR) $(TARGET_DEP_FOLDER)
	@$(BUILD_CMD)
$(DEP_STATIC_LIBS):
$(TARGET_OBJ_FOLDER)/%$(OBJ_SUBFFIX): $(SRC_FOLDER)/%$(C_SUBFFIX) $(TARGET_OBJ_FOLDER)/%$(DEP_SUBFFIX)
	@$(MKDIR) $(TARGET_OBJ_FOLDER)
	@$(CCOMPILER)
	@$(DEP_MV)
$(TARGET_OBJ_FOLDER)/%$(OBJ_SUBFFIX): $(SRC_FOLDER)/%$(CPP_SUBFFIX) $(TARGET_OBJ_FOLDER)/%$(DEP_SUBFFIX)
	@$(MKDIR) $(TARGET_OBJ_FOLDER)
	@$(CXXCOMPILER)
	@$(DEP_MV)
$(TARGET_OBJ_FOLDER)/%$(DEP_SUBFFIX): ;
.PRECIOUS: $(TARGET_OBJ_FOLDER)/%$(DEP_SUBFFIX)
include $(wildcard $(TARGET_OBJ_FOLDER)/*$(DEP_SUBFFIX))
# rules end

