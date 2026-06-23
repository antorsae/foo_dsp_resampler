# Makefile for building foo_dsp_resampler.component for macOS (foobar2000 Mac)
#
# Usage:
#   make            # build Release (arm64)
#   make debug      # build Debug (arm64)
#   make universal  # build universal (arm64 + x86_64)
#   make clean

# Paths
SDK_DIR   ?= third_party/foobar2000_sdk
SRC_DIR    = .
BUILD_DIR  = build$(BUILD_SUFFIX)

# SDK static libraries (must be built first)
SDK_LIBS = \
	$(SDK_DIR)/pfc/build/Release/libpfc-Mac.a \
	$(SDK_DIR)/foobar2000/SDK/build/Release/libfoobar2000_SDK.a \
	$(SDK_DIR)/foobar2000/helpers/build/Release/libfoobar2000_SDK_helpers.a \
	$(SDK_DIR)/foobar2000/shared/build/Release/libshared.a \
	$(SDK_DIR)/foobar2000/foobar2000_component_client/build/Release/libfoobar2000_component_client.a

# Compiler / linker (prefer system clang for cross-compilation support)
CC         = /usr/bin/clang
CXX        = /usr/bin/clang++
LD         = /usr/bin/clang++

# Architecture
ARCH ?= arm64
# Convert space-separated arch list to repeated -arch flags
ARCH_FLAGS = $(patsubst %,-arch %,$(ARCH))

# Common flags
DEPLOY_TARGET = 11.0

CPPFLAGS = \
	-DNDEBUG=1 \
	-D_USE_MATH_DEFINES \
	-I$(SDK_DIR) \
	-I$(SDK_DIR)/foobar2000 \
	-I$(SDK_DIR)/foobar2000/SDK \
	-I$(SDK_DIR)/foobar2000/helpers \
	-I$(SRC_DIR) \
	-I$(SRC_DIR)/rate \
	-I$(SRC_DIR)/rate/fft-double \
	-I$(SRC_DIR)/rate/fft-float

CFLAGS = \
	-std=gnu11 \
	$(ARCH_FLAGS) \
	-mmacosx-version-min=$(DEPLOY_TARGET) \
	-fvisibility=hidden \
	-O2 \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable

CXXFLAGS = \
	-std=gnu++20 \
	-stdlib=libc++ \
	$(ARCH_FLAGS) \
	-mmacosx-version-min=$(DEPLOY_TARGET) \
	-fvisibility=hidden \
	-fvisibility-inlines-hidden \
	-O2 \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-deprecated-declarations

OBJCXXFLAGS = $(CXXFLAGS) -fobjc-arc

LDFLAGS = \
	$(ARCH_FLAGS) \
	-mmacosx-version-min=$(DEPLOY_TARGET) \
	-bundle \
	-framework Cocoa \
	$(SDK_LIBS) \
	-lc++

# Source files (common - non-SSE, compiled for all archs)
C_SOURCES = \
	main.c \
	rate/rate_double.c \
	rate/rate_float.c \
	rate/rate_uni.c \
	rate/effects_i_dsp.c \
	rate/fft-double/fft4g_dbl.c \
	rate/fft-float/fft.c \
	rate/fft-float/rdft.c \
	rate/xmalloc.c

# SSE sources (x86_64 only)
# Note: rdft_sse.c and rate_SSE.c excluded (need x86 assembly for ff_fft_calc_sse)
SSE_C_SOURCES = \
	rate/rate_SSE3.c \
	rate/fft-double/fft4g_sse3.c

# SSE sources (x86_64 only) - always compiled with -arch x86_64
SSE_CFLAGS = \
	-std=gnu11 \
	-arch x86_64 \
	-mmacosx-version-min=$(DEPLOY_TARGET) \
	-fvisibility=hidden \
	-O2 \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable

# Only include SSE objects when building for x86_64
ifeq ($(findstring x86_64,$(ARCH)),x86_64)
SSE_OBJS = $(patsubst %.c,$(BUILD_DIR)/sse/%.o,$(SSE_C_SOURCES))
else
SSE_OBJS =
endif

CXX_SOURCES = \
	foo_dsp_resampler.cpp \
	dsp_config.cpp \
	foo_dsp_rate.cpp \
	PCH.cpp \
	lpc/lpc.cpp

OBJCXX_SOURCES = \
	Mac/ResamplerDSPView.mm

# Object files
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CXX_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CXX_SOURCES))
OBJCXX_OBJS = $(patsubst %.mm,$(BUILD_DIR)/%.o,$(OBJCXX_SOURCES))

ALL_OBJS = $(C_OBJS) $(CXX_OBJS) $(OBJCXX_OBJS) $(SSE_OBJS)

# Output
COMPONENT_NAME = foo_dsp_resampler.component
BUNDLE_DIR = $(BUILD_DIR)/$(COMPONENT_NAME)
BUILD_BINARY = $(BUILD_DIR)/foo_dsp_resampler
BUNDLE_BINARY = $(BUNDLE_DIR)/Contents/MacOS/foo_dsp_resampler

.PHONY: all debug universal clean sdk-libs

all: $(BUNDLE_DIR)

debug: CPPFLAGS += -DDEBUG=1
debug: CFLAGS += -g -O0
debug: CXXFLAGS += -g -O0
debug: $(BUNDLE_DIR)

# Universal build: build arm64 and x86_64 separately, then combine with lipo
universal:
	$(MAKE) ARCH=arm64  BUILD_SUFFIX=_arm64  build/foo_dsp_resampler_arm64
	$(MAKE) ARCH=x86_64 BUILD_SUFFIX=_x86_64 build/foo_dsp_resampler_x86_64
	@mkdir -p build/foo_dsp_resampler.component/Contents/MacOS
	@mkdir -p build/foo_dsp_resampler.component/Contents/Resources
	lipo -create build/foo_dsp_resampler_arm64 build/foo_dsp_resampler_x86_64 -output build/foo_dsp_resampler.component/Contents/MacOS/foo_dsp_resampler
	@printf '<?xml version="1.0" encoding="UTF-8"?>\n<plist version="1.0">\n<dict>\n\t<key>CFBundleExecutable</key>\n\t<string>foo_dsp_resampler</string>\n\t<key>CFBundleIdentifier</key>\n\t<string>com.foobar2000.foo-dsp-resampler</string>\n\t<key>CFBundleName</key>\n\t<string>SoX Resampler</string>\n\t<key>CFBundlePackageType</key>\n\t<string>BNDL</string>\n\t<key>CFBundleVersion</key>\n\t<string>1.0</string>\n</dict>\n</plist>\n' > build/foo_dsp_resampler.component/Contents/Info.plist
	@echo "Bundle is ready: build/foo_dsp_resampler.component"

# Per-architecture binary (used by universal target via recursive make)
build/foo_dsp_resampler_%: $(ALL_OBJS) $(SDK_LIBS)
	@echo "Linking ($*)..."
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(ALL_OBJS) -o $@

# Build SDK libraries if needed
sdk-libs:
	cd $(SDK_DIR) && xcodebuild -project pfc/pfc.xcodeproj -target pfc-Mac -configuration Release build
	cd $(SDK_DIR) && xcodebuild -project foobar2000/SDK/foobar2000_SDK.xcodeproj -target foobar2000_SDK -configuration Release build
	cd $(SDK_DIR) && xcodebuild -project foobar2000/helpers/foobar2000_SDK_helpers.xcodeproj -target foobar2000_SDK_helpers -configuration Release build
	cd $(SDK_DIR) && xcodebuild -project foobar2000/shared/shared.xcodeproj -target shared -configuration Release build
	cd $(SDK_DIR) && xcodebuild -project foobar2000/foobar2000_component_client/foobar2000_component_client.xcodeproj -target foobar2000_component_client -configuration Release build

# Bundle structure
$(BUNDLE_DIR): $(BUNDLE_BINARY)
	@echo "Bundle is ready: $@"

$(BUNDLE_BINARY): $(BUILD_BINARY)
	@echo "Creating bundle structure..."
	@mkdir -p $(BUNDLE_DIR)/Contents/MacOS
	@mkdir -p $(BUNDLE_DIR)/Contents/Resources
	@cp $(BUILD_BINARY) $(BUNDLE_BINARY)
	@printf '<?xml version="1.0" encoding="UTF-8"?>\n<plist version="1.0">\n<dict>\n\t<key>CFBundleExecutable</key>\n\t<string>foo_dsp_resampler</string>\n\t<key>CFBundleIdentifier</key>\n\t<string>com.foobar2000.foo-dsp-resampler</string>\n\t<key>CFBundleName</key>\n\t<string>SoX Resampler</string>\n\t<key>CFBundlePackageType</key>\n\t<string>BNDL</string>\n\t<key>CFBundleVersion</key>\n\t<string>1.0</string>\n</dict>\n</plist>\n' > $(BUNDLE_DIR)/Contents/Info.plist

# Link
$(BUILD_BINARY): $(ALL_OBJS) $(SDK_LIBS)
	@echo "Linking..."
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(ALL_OBJS) -o $@

# Pattern rules
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.mm
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(OBJCXXFLAGS) -c $< -o $@

# SSE sources: always x86_64 only
$(BUILD_DIR)/sse/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SSE_CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
