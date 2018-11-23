# defines a directory for build, for example, RH6_x86_64
lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHat,rh,\
                   $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
                     $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist)))))))
short_dist    := $(shell echo $(short_dist_lc) | tr a-z A-Z)
rpm_os        := $(short_dist_lc)$(lsb_dist_ver).$(uname_m)

# this is where the targets are compiled
build_dir ?= $(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)
bind      := $(build_dir)/bin
libd      := $(build_dir)/lib64
objd      := $(build_dir)/obj
dependd   := $(build_dir)/dep

# use 'make port_extra=-g' for debug build
ifeq (-g,$(findstring -g,$(port_extra)))
  DEBUG = true
endif

CC          ?= gcc
CXX         ?= g++
cc          := $(CC)
cpp         := $(CXX)
cppflags    := -fno-rtti -fno-exceptions
arch_cflags := -fno-omit-frame-pointer
cpplink     := $(CC)
gcc_wflags  := -Wall -Werror
fpicflags   := -fPIC
soflag      := -shared
rpath       := -Wl,-rpath,$(libd),-rpath,linecook/$(libd),-rpath,libdecnumber/$(libd)

ifdef DEBUG
default_cflags := -ggdb
else
default_cflags := -O3 -ggdb
endif
# rpmbuild uses RPM_OPT_FLAGS
CFLAGS ?= $(default_cflags)
#RPM_OPT_FLAGS ?= $(default_cflags)
#CFLAGS ?= $(RPM_OPT_FLAGS)
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)

INCLUDES    ?= -Iinclude
includes    := $(INCLUDES)
DEFINES     ?=
defines     := $(DEFINES)
cpp_lnk     :=
sock_lib    :=
math_lib    := -lm
thread_lib  := -pthread -lrt
lc_lib      := linecook/$(libd)/liblinecook.a
dec_lib     := libdecnumber/$(libd)/libdecnumber.a
lnk_lib     := $(lc_lib) $(dec_lib)
dlnk_lib    := -Llinecook/$(libd) -llinecook -Llibdecnumber/$(libd) -ldecnumber
malloc_lib  :=

# before include, that has srpm target
.PHONY: everything
everything: $(lc_lib) $(dec_lib) all

# version vars
-include .copr/Makefile

# targets filled in below
all_exes    :=
all_libs    :=
all_depends :=
gen_files   :=
version_defines := -DDESH_VER=$(ver_build)

common_files  := access closure conv dict eval except fd gc glob \
                 glom input heredoc list match open opt prim-ctl prim-etc \
                 prim-io prim-sys prim-rel prim-math decimal prim print proc \
		 sigmsgs signal split status str syntax term token tree util \
		 var vec version y.tab
all_files     := $(common_files) main initial dump
input_includes   := -Ilinecook/include
decimal_includes := -Ilibdecnumber/include

common_objs := $(addprefix $(objd)/, $(addsuffix .o, $(common_files)))
common_dbjs := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(common_files)))
all_depends += $(addprefix $(dependd)/, $(addsuffix .d, $(all_files)))
all_depends += $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(all_files)))

libdesh_objs := $(objd)/initial.o $(common_objs)
libdesh_dbjs := $(objd)/initial.fpic.o $(common_dbjs)
libdesh_dlnk := $(dlnk_lib)
libdesh_spec := $(version)-$(build_num)
libdesh_ver  := $(major_num).$(minor_num)

$(libd)/libdesh.a: $(libdesh_objs)
$(libd)/libdesh.so: $(libdesh_dbjs)

all_libs += $(libd)/libdesh.a $(libd)/libdesh.so

desh_objs := $(objd)/main.fpic.o
desh_libs := $(libd)/libdesh.so
desh_lnk  := -ldesh $(dlnk_lib)
$(bind)/desh: $(desh_objs) $(desh_libs)

src/y.tab.c include/desh/token.h: src/parse.y
	yacc -vd src/parse.y
	mv y.tab.c src/y.tab.c
	mv y.tab.h include/desh/token.h

src/initial.c: $(bind)/esdump script/initial.es
	$(bind)/esdump < script/initial.es > src/initial.c

src/sigmsgs.c: /usr/include/bits/signum.h
	cat /usr/include/bits/signum*.h | script/mksignal > src/sigmsgs.c

esdump_objs := $(objd)/dump.o $(objd)/main.o $(common_objs) 
esdump_lnk  := $(lnk_lib)
$(bind)/esdump: $(esdump_objs) $(esdump_lnk)

all_exes += $(bind)/desh

all_dirs := $(bind) $(libd) $(objd) $(dependd)

gen_files += src/sigmsgs.c src/y.tab.c include/desh/token.h

# if sub modules initialized, use those, otherwise use installed
# (git submodule update --init --recursive)
$(lc_lib):
	if [ -d linecook -a -f linecook/GNUmakefile ] ; then \
	  $(MAKE) -C linecook ; \
	else \
	  mkdir -p `dirname $(lc_lib)` ; \
	  ln -s /usr/lib64/liblinecook.* `dirname $(lc_lib)` ; \
	fi

$(dec_lib):
	if [ -d libdecnumber -a -f libdecnumber/GNUmakefile ] ; then \
	  $(MAKE) -C libdecnumber ; \
	else \
	  mkdir -p `dirname $(dec_lib)` ; \
	  ln -s /usr/lib64/libdecnumber.* `dirname $(dec_lib)` ; \
	fi

# the default targets
.PHONY: all
all: $(gen_files) $(all_libs) $(all_exes)

# create directories
$(dependd):
	@mkdir -p $(all_dirs)

# remove target bins, objs, depends
.PHONY: clean
clean:
	rm -r -f $(bind) $(libd) $(objd) $(dependd)
	if [ "$(build_dir)" != "." ] ; then rmdir $(build_dir) ; fi

# force a remake of depend using 'make -B depend'
.PHONY: depend
depend: $(dependd)/depend.make

$(dependd)/depend.make: $(dependd) $(all_depends)
	@echo "# depend file" > $(dependd)/depend.make
	@cat $(all_depends) >> $(dependd)/depend.make

.PHONY: dist_bins
dist_bins: $(gen_files) $(all_libs) $(bind)/desh
	chrpath -d $(libd)/libdesh.so
	chrpath -d $(bind)/desh

.PHONY: dist_rpm
dist_rpm: srpm
	( cd rpmbuild && rpmbuild --define "-topdir `pwd`" -ba SPECS/desh.spec )

# dependencies made by 'make depend'
-include $(dependd)/depend.make

$(objd)/%.o: src/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.c
	$(cc) $(cflags) $(fpicflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(libd)/%.a:
	ar rc $@ $($(*)_objs)

$(libd)/%.so:
	$(cpplink) $(soflag) $(rpath) $(cflags) -o $@.$($(*)_spec) -Wl,-soname=$(@F).$($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(cpp_dll_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_spec) $(@F).$($(*)_ver) && ln -f -s $(@F).$($(*)_ver) $(@F)

$(bind)/%:
	$(cpplink) $(cflags) $(rpath) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

$(dependd)/%.d: src/%.cpp
	gcc -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/%.c
	gcc $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.fpic.d: src/%.cpp
	gcc -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/%.c
	gcc $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.d: test/%.cpp
	gcc -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: test/%.c
	gcc $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

