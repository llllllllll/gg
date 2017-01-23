CC := g++
CFLAGS := -std=gnu++17 -Wall -Wextra -O3 -g -fno-strict-aliasing -fconcepts
LDFLAGS := -lgccjit

INCLUDE-DIRS := include/
INCLUDE := $(foreach d,$(INCLUDE-DIRS), -I$d)

EXECUTABLE := gg

# build artifacts
SCRATCH-DIR := .scratch
BISON-INCLUDE-PREFIX := $(EXECUTABLE)/bison
BISON-DIR := include/$(BISON-INCLUDE-PREFIX)
BISON-MARKER := $(BISON-DIR)/.bison-marker
BISON-PARSER-HEADER := $(BISON-DIR)/parser.h
BISON-EXTRA-HEADERS := location.h stack.h position.h
BISON-PARSER-SOURCE := src/parser.cc
FLEX-LEXER-SOURCE := src/lexer.cc
BISON-AND-FLEX-MARKER := $(SCRATCH)/.bison-and-flex-marker

# use sort for unique
SOURCES := $(sort \
		$(wildcard src/*.cc) \
		$(BISON-PARSER-SOURCE) \
		$(FLEX-LEXER-SOURCE))
OBJECTS := $(SOURCES:.cc=.o)
DFILES := $(SOURCES:.cc=.d)

# sed to replace the filepaths of the bison output headers
define fix-bison-artifacts =
sed -i -e 's|parser.hh|$(BISON-INCLUDE-PREFIX)/parser.h|g' $1
sed -i -e 's|location.hh|$(BISON-INCLUDE-PREFIX)/location.h|g' $1
sed -i -e 's|stack.hh|$(BISON-INCLUDE-PREFIX)/stack.h|g' $1
sed -i -e 's|position.hh|$(BISON-INCLUDE-PREFIX)/position.h|g' $1
endef

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(HEADERS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o : %.cc $(BISON-AND-FLEX-MARKER)
	$(CC) $(CFLAGS) $(INCLUDE) -MD -fPIC -c $< -o $@

$(BISON-DIR):
	mkdir -p $@

$(SCRATCH-DIR):
	mkdir -p $@

$(BISON-MARKER): src/parser.yy | $(BISON-DIR) $(SCRATCH-DIR)
	bison $<
	@mv parser.cc $(SCRATCH-DIR)/
	@mv parser.hh $(SCRATCH-DIR)/
	@mv location.hh $(SCRATCH-DIR)/
	@mv stack.hh $(SCRATCH-DIR)/
	@mv position.hh $(SCRATCH-DIR)/
	touch $@

$(BISON-DIR)/%.h: $(BISON-MARKER)
	@cp $(SCRATCH-DIR)/$(shell basename $@)h $@  # note the extra h
	@$(call fix-bison-artifacts,$@)

$(BISON-PARSER-SOURCE): $(BISON-PARSER-HEADER)
	@cp $(SCRATCH-DIR)/parser.cc $@
	@$(call fix-bison-artifacts,$@)

$(FLEX-LEXER-SOURCE): src/lexer.ll $(BISON-PARSER-HEADER)
	flex -o $@ $<

$(BISON-AND-FLEX-MARKER): \
		$(BISON-PARSER-HEADER) \
		$(BISON-PARSER-SOURCE) \
		$(FLEX-LEXER-SOURCE) \
		$(foreach h,$(BISON-EXTRA-HEADERS),$(BISON-DIR)/$h) \
		| $(SCRATCH-DIR)
	touch $@

clean:
	rm -f $(EXECUTABLE) \
		$(OBJECTS) \
		$(DFILES) \
		$(BISON-PARSER-HEADER) \
		$(BISON-PARSER-SOURCE) \
		$(FLEX-LEXER-SOURCE) \
		-r $(SCRATCH-DIR) \
		-r $(BISON-DIR)

-include $(DFILES) $(TEST_DFILES)
