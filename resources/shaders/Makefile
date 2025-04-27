# Makefile placed in the shaders/ directory

# --- Configuration ---

# Compiler and Tools (Adjust paths if not in PATH)
GLSLC = glslc
SPIRVCROSS = spirv-cross

# Compiler Flags
# -O for optimization, -O0 for debugging
# Add -I. if your shaders use #include directives relative to the shaders/ dir
# Add -D flags for preprocessor defines, e.g., -DMY_DEFINE=1
GLSLC_FLAGS = -g

# SPIRV-Cross Flags
# --reflect: Output reflection data in JSON format
SPIRVCROSS_FLAGS = --reflect

# --- Discover Shaders ---

# Find all subdirectories containing shader files (based on finding vert.glsl)
# This avoids trying to process non-shader directories if any exist
SHADER_DIRS = $(patsubst %/vert.glsl,%,$(wildcard */vert.glsl))

# Generate lists of expected source and target files
VERT_SOURCES = $(foreach dir,$(SHADER_DIRS),$(dir)/vert.glsl)
FRAG_SOURCES = $(foreach dir,$(SHADER_DIRS),$(dir)/frag.glsl)

VERT_SPIRV = $(VERT_SOURCES:.glsl=.spirv)
FRAG_SPIRV = $(FRAG_SOURCES:.glsl=.spirv)

VERT_JSON = $(VERT_SPIRV:.spirv=.json)
FRAG_JSON = $(FRAG_SPIRV:.spirv=.json)

# All target files combined
ALL_TARGETS = $(VERT_SPIRV) $(FRAG_SPIRV) $(VERT_JSON) $(FRAG_JSON)

# --- Rules ---

# Default target: build everything
# Using .PHONY ensures 'all' always runs its commands if needed,
# regardless of whether a file named 'all' exists.
.PHONY: all
all: $(ALL_TARGETS)

# Rule to compile vertex shaders (.glsl -> .spirv)
# $< is the first prerequisite (the .glsl file)
# $@ is the target file name (the .spirv file)
%/vert.spirv: %/vert.glsl
	@echo "Compiling Vertex Shader: $< -> $@"
	$(GLSLC) $(GLSLC_FLAGS) -fshader-stage=vertex -o $@ $<

# Rule to compile fragment shaders (.glsl -> .spirv)
%/frag.spirv: %/frag.glsl
	@echo "Compiling Fragment Shader: $< -> $@"
	$(GLSLC) $(GLSLC_FLAGS) -fshader-stage=fragment -o $@ $<

# Rule to generate reflection for vertex shaders (.spirv -> .json)
%/vert.json: %/vert.spirv
	@echo "Generating Reflection: $< -> $@"
	$(SPIRVCROSS) $(SPIRVCROSS_FLAGS) --output $@ $<

# Rule to generate reflection for fragment shaders (.spirv -> .json)
%/frag.json: %/frag.spirv
	@echo "Generating Reflection: $< -> $@"
	$(SPIRVCROSS) $(SPIRVCROSS_FLAGS) --output $@ $<

# Rule to clean up generated files
.PHONY: clean
clean:
	@echo "Cleaning generated shader files..."
	# Use $(RM) which is typically 'rm -f'
	$(RM) $(ALL_TARGETS)

# --- End of Makefile ---
