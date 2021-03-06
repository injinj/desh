# defines a directory for build, for example, RH6_x86_64
lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else uname -s ; fi)
lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHatEnterprise,rh,\
                   $(patsubst RedHat,rh,\
                     $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
                       $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist))))))))
short_dist    := $(shell echo $(short_dist_lc) | tr a-z A-Z)
pwd           := $(shell pwd)
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
cc          := $(CC) -std=c11
cpp         := $(CXX)
# if not linking libstdc++
ifdef NO_STL
cppflags    := -std=c++11 -fno-rtti -fno-exceptions
cpplink     := $(CC)
else
cppflags    := -std=c++11
cpplink     := $(CXX)
endif
arch_cflags := -fno-omit-frame-pointer
gcc_wflags  := -Wall -Werror
fpicflags   := -fPIC
soflag      := -shared
rpath1      := -Wl,-rpath,$(pwd)/$(libd)

ifeq (Darwin,$(lsb_dist))
dll         := dylib
else
dll         := so
endif

ifdef DEBUG
default_cflags := -ggdb
else
default_cflags := -O3 -ggdb
endif
# rpmbuild uses RPM_OPT_FLAGS
ifeq ($(RPM_OPT_FLAGS),)
CFLAGS ?= $(default_cflags)
else
CFLAGS ?= $(RPM_OPT_FLAGS)
endif
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)

INCLUDES    ?= -Iinclude
includes    := $(INCLUDES)
DEFINES     ?=
defines     := $(DEFINES)
cpp_lnk     :=
sock_lib    :=
math_lib    := -lm
#thread_lib  := -pthread -lrt

# test submodules exist (they don't exist for dist_rpm, dist_dpkg targets)
have_lc_submodule  := $(shell if [ -f ./linecook/GNUmakefile ]; then echo yes; else echo no; fi )
have_dec_submodule := $(shell if [ -f ./libdecnumber/GNUmakefile ]; then echo yes; else echo no; fi )

lnk_lib     :=
dlnk_lib    :=

# if building submodules, reference them rather than the libs installed
ifeq (yes,$(have_lc_submodule))
lc_lib      := linecook/$(libd)/liblinecook.a
lnk_lib     += $(lc_lib)
dlnk_lib    += -Llinecook/$(libd) -llinecook
rpath2       = ,-rpath,$(pwd)/linecook/$(libd)
else
lnk_lib     += -llinecook
dlnk_lib    += -llinecook
endif

ifeq (yes,$(have_dec_submodule))
dec_lib     := libdecnumber/$(libd)/libdecnumber.a
lnk_lib     += $(dec_lib)
dlnk_lib    += -Llibdecnumber/$(libd) -ldecnumber
rpath3       = ,-rpath,$(pwd)/libdecnumber/$(libd)
else
lnk_lib     += -ldecnumber
dlnk_lib    += -ldecnumber
endif

rpath        = $(rpath1)$(rpath2)$(rpath3)
dlnk_lib    += -lpcre2-32
lnk_lib     += -lpcre2-32
malloc_lib  :=

# before include, that has srpm target
.PHONY: everything
everything: $(dec_lib) $(lc_lib) all

# build submodules if have them
ifeq (yes,$(have_dec_submodule))
$(dec_lib):
	$(MAKE) -C libdecnumber
endif
ifeq (yes,$(have_lc_submodule))
$(lc_lib):
	$(MAKE) -C linecook
endif

# copr/fedora build (with version env vars)
# copr uses this to generate a source rpm with the srpm target
-include .copr/Makefile

# debian build (debuild)
# target for building installable deb: dist_dpkg
-include deb/Makefile

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
# submodule includes
input_includes   := -Ilinecook/include
decimal_includes := -Ilibdecnumber/include

common_objs := $(addprefix $(objd)/, $(addsuffix .o, $(common_files)))
common_dbjs := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(common_files)))
all_depends += $(addprefix $(dependd)/, $(addsuffix .d, $(all_files)))
all_depends += $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(all_files)))

libdesh_objs  := $(objd)/initial.o $(common_objs)
libdesh_dbjs  := $(objd)/initial.fpic.o $(common_dbjs)
libdesh_dlnk  := $(dlnk_lib)
libdesh_spec  := $(version)-$(build_num)
libdesh_dylib := $(version).$(build_num)
libdesh_ver   := $(major_num).$(minor_num)

$(libd)/libdesh.a: $(libdesh_objs)
$(libd)/libdesh.$(dll): $(libdesh_dbjs)

all_libs += $(libd)/libdesh.a $(libd)/libdesh.$(dll)

desh_objs := $(objd)/main.fpic.o
desh_libs := $(libd)/libdesh.$(dll)
desh_lnk  := -ldesh $(dlnk_lib)
$(bind)/desh: $(desh_objs) $(desh_libs)

src/y.tab.c include/desh/token.h: src/parse.y
	yacc -vd src/parse.y
	mv y.tab.c src/y.tab.c
	mv y.tab.h include/desh/token.h

src/initial.c: $(bind)/esdump script/initial.es
	$(bind)/esdump < script/initial.es > src/initial.c

src/sigmsgs.c: $(bind)/mksignal
	$(bind)/mksignal > src/sigmsgs.c

esdump_objs := $(objd)/dump.o $(objd)/main.o $(common_objs) 
esdump_lnk  := $(dlnk_lib)
$(bind)/esdump: $(esdump_objs) $(lc_lib) $(dec_lib)
mksignal_objs := $(objd)/mksignal.o
$(bind)/mksignal: $(mksignal_objs)

all_exes += $(bind)/desh

all_dirs := $(bind) $(libd) $(objd) $(dependd)

gen_files += src/initial.c src/sigmsgs.c src/y.tab.c include/desh/token.h

# the default targets
.PHONY: all
all: $(gen_files) $(all_libs) $(all_exes)

.PHONY: make_dirs
make_dirs:
	@mkdir -p $(all_dirs)

# create directories
$(dependd): make_dirs include/desh/token.h

# remove target bins, objs, depends
.PHONY: clean
clean:
	rm -rf $(bind) $(libd) $(objd) $(dependd)
	if [ "$(build_dir)" != "." ] ; then rmdir $(build_dir) ; fi
	rm -f $(gen_files) y.output

.PHONY: clean_dist
clean_dist:
	rm -rf dpkgbuild rpmbuild

.PHONY: clean_all
clean_all: clean clean_dist

# force a remake of depend using 'make -B depend'
.PHONY: depend
depend: $(dependd)/depend.make

$(dependd)/depend.make: $(dependd) $(all_depends)
	@echo "# depend file" > $(dependd)/depend.make
	@cat $(all_depends) >> $(dependd)/depend.make

ifeq (SunOS,$(lsb_dist))
remove_rpath = rpath -r
else
ifeq (Darwin,$(lsb_dist))
remove_rpath = true
else
remove_rpath = chrpath -d
endif
endif
# remove the relative link run paths
.PHONY: dist_bins
dist_bins: all
	$(remove_rpath) $(libd)/libdesh.$(dll)
	$(remove_rpath) $(bind)/desh

.PHONY: dist_rpm
dist_rpm: srpm
	( cd rpmbuild && rpmbuild --define "-topdir `pwd`" -ba SPECS/desh.spec )

# dependencies made by 'make depend'
-include $(dependd)/depend.make

ifeq ($(DESTDIR),)
# 'sudo make install' puts things in /usr/local/lib, /usr/local/include
install_prefix ?= /usr/local
else
# debuild uses DESTDIR to put things into debian/libdecnumber/usr
install_prefix = $(DESTDIR)/usr
endif
# this should be 64 for rpm based, /64 for SunOS
install_lib_suffix ?=

install: dist_bins
	install -d $(install_prefix)/lib$(install_lib_suffix) $(install_prefix)/bin
	install -d $(install_prefix)/include/desh
	install -d $(install_prefix)/share/man/man1
	install -d $(install_prefix)/share/doc/desh
	for f in $(libd)/libdesh.* ; do \
	if [ -h $$f ] ; then \
	cp -a $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	else \
	install $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	fi ; \
	done
	install $(bind)/desh $(install_prefix)/bin
	install -m 644 script/deshrc script/esrc.haahr $(install_prefix)/share/doc/desh/
	install -m 644 README.md doc/CHANGES doc/es.1 $(install_prefix)/share/doc/desh/
	install -m 644 include/desh/*.h $(install_prefix)/include/desh
	install -m 644 doc/es.1 $(install_prefix)/share/man/man1/desh.1

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

$(libd)/%.dylib:
	$(cpplink) -dynamiclib $(cflags) -o $@.$($(*)_dylib).dylib -current_version $($(*)_dylib) -compatibility_version $($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(cpp_dll_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_dylib).dylib $(@F).$($(*)_ver).dylib && ln -f -s $(@F).$($(*)_ver).dylib $(@F)

$(bind)/%:
	$(cpplink) $(cflags) $(rpath) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

$(dependd)/%.d: src/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.fpic.d: src/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.d: test/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: test/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

