DEBUG ?= 1
ifeq ($(DEBUG), 1)
    FLAGS=-g
else
    FLAGS=-O3
endif
#include directories start---
inc_folder_basic := $(dir $(wildcard ../../src/*/))
inc_folder_test := ../

#get list of subdirectories in ../modules/
# inc_folder_module_subdirs := $(sort $(dir $(wildcard ../../src/modules/*/)))
inc_folder_basic := $(sort $(dir $(wildcard ../../src/*/)))

include_dirs = $(inc_folder_basic) $(inc_folder_test)

#include directories end---

include_flags = $(foreach d, $(include_dirs), -I$d)

objects_memory = $(wildcard ../../src/memory/intermediate/*.o)

object_files = $(objects_memory)

dir_build = build
$(shell mkdir -p $(dir_build))

targets := $(patsubst %.cpp, %, $(wildcard *.cpp))

libs := -pthread -ltcmalloc
# libs := -pthread

%: %.cpp
	$(CXX) $(FLAGS) -std=c++1z -o $(dir_build)/$@ $^ $(object_files) $(include_flags) $(libs)

build_memory:
	$(MAKE) -C ./../../src/memory all

.PHONY: all
all: build_memory $(targets)

.PHONY: clean
clean:
	rm -f -r ./$(dir_build)

testcase_excludes := $(dir_build)/

testcases:
	@for i in $(filter-out $(testcase_excludes), $(wildcard $(dir_build)/*) ); do \
		./$$i; \
	done \
