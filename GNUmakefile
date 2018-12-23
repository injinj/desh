# defines a directory for build, for example, RH6_x86_64
lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHatEnterprise,rh,\
                   $(patsubst RedHat,rh,\
                     $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
                       $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist))))))))
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
rpath1      := -Wl,-rpath,$(libd)

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
thread_lib  := -pthread -lrt

# test submodules exist (they don't exist for dist_rpm, dist_dpkg targets)
have_lc_submodule  := $(shell if [ -d ./linecook ]; then echo yes; else echo no; fi )
have_dec_submodule := $(shell if [ -d ./libdecnumber ]; then echo yes; else echo no; fi )

lnk_lib     :=
dlnk_lib    :=

# if building submodules, reference them rather than the libs installed
ifeq (yes,$(have_lc_submodule))
lc_lib      := linecook/$(libd)/liblinecook.a
lnk_lib     += $(lc_lib)
dlnk_lib    += -Llinecook/$(libd) -llinecook
rpath2       = ,-rpath,linecook/$(libd)
else
lnk_lib     += -llinecook
dlnk_lib    += -llinecook
endif

ifeq (yes,$(have_dec_submodule))
dec_lib     := libdecnumber/$(libd)/libdecnumber.a
lnk_lib     += $(dec_lib)
dlnk_lib    += -Llibdecnumber/$(libd) -ldecnumber
rpath3       = ,-rpath,libdecnumber/$(libd)
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

src/sigmsgs.c: $(bind)/mksignal
	$(bind)/mksignal > src/sigmsgs.c

esdump_objs := $(objd)/dump.o $(objd)/main.o $(common_objs) 
esdump_lnk  := $(lnk_lib)
$(bind)/esdump: $(esdump_objs) $(lc_lib) $(dec_lib)
mksignal_objs := $(objd)/mksignal.o
$(bind)/mksignal: $(mksignal_objs)

all_exes += $(bind)/desh

all_dirs := $(bind) $(libd) $(objd) $(dependd)

gen_files += src/initial.c src/sigmsgs.c src/y.tab.c include/desh/token.h

# the default targets
.PHONY: all
all: $(gen_files) $(all_libs) $(all_exes)

# create directories
$(dependd):
	@mkdir -p $(all_dirs)

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

# remove the relative link run paths
.PHONY: dist_bins
dist_bins: all
	chrpath -d $(libd)/libdesh.so
	chrpath -d $(bind)/desh

.PHONY: dist_rpm
dist_rpm: srpm
	( cd rpmbuild && rpmbuild --define "-topdir `pwd`" -ba SPECS/desh.spec )

# dependencies made by 'make depend'
-include $(dependd)/depend.make

ifeq ($(DESTDIR),)
# 'sudo make install' puts things in /usr/local/lib, /usr/local/include
install_prefix = /usr/local
else
# debuild uses DESTDIR to put things into debian/libdecnumber/usr
install_prefix = $(DESTDIR)/usr
endif

install: dist_bins
	install -d $(install_prefix)/lib $(install_prefix)/bin
	install -d $(install_prefix)/include/desh $(install_prefix)/share/man/man1
	install -d $(install_prefix)/etc
	for f in $(libd)/libdesh.* ; do \
	if [ -h $$f ] ; then \
	cp -a $$f $(install_prefix)/lib ; \
	else \
	install $$f $(install_prefix)/lib ; \
	fi ; \
	done
	install $(bind)/desh $(install_prefix)/bin
	install -m 644 script/deshrc $(install_prefix)/etc/deshrc
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

$(bind)/%:
	$(cpplink) $(cflags) $(rpath) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

$(dependd)/%.d: src/%.cpp
	$(cc) -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.fpic.d: src/%.cpp
	$(cc) -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.d: test/%.cpp
	$(cc) -x c++ $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: test/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

