SLANG_FILES := $(wildcard ./*/*.slang)
TARGETS_SPIRV := $(patsubst %.slang,%.spirv,$(SLANG_FILES))

all: $(TARGETS_SPIRV)

%.spirv: %.slang
	slangc $< -target spirv -o $@ -reflection-json $*.json -matrix-layout-column-major

clean:
	rm -f ./*/*.spirv ./*/*.json

.PHONY: all clean
