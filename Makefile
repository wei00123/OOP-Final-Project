# Compiler & Linker settings
CXX = g++
NVCC = /usr/lib/nvidia-cuda-toolkit/bin/nvcc
CXXFLAGS = -I ./inc -I ./third-party/CImg -I ./third-party/libjpeg -I ./Data-Loader -std=c++11
# CUDA 編編參數：包含相同標頭檔路徑，並指定 C++11 標準
NVCCFLAGS = -I ./inc -I ./third-party/CImg -I ./third-party/libjpeg -I ./Data-Loader -std=c++11 -Xcompiler "-O3 -march=native"
#  修正：已移除 -flto 避免 GCC 版本不一致造成的編譯中斷
OPTFLAGS = -march=native -funroll-loops -finline-functions -ffast-math -O3
WARNINGS = -g -Wall
LINKER = -L/usr/X11R6/lib -lm -lpthread -lX11 -L./third-party/libjpeg -ljpeg -lpng

# Valgrind for memory issue
CHECKCC = valgrind
CHECKFLAGS = --leak-check=full -s --show-leak-kinds=all --track-origins=yes 

# Source files and object files
SRCDIR = src
OBJDIR = obj
INCDIR = inc
OUTDIR = output

# 同時偵測 .cpp 與 .cu 檔案
SRCS_CPP = $(wildcard $(SRCDIR)/*.cpp)
SRCS_CU  = $(wildcard $(SRCDIR)/*.cu)

# 將它們各自轉為對應的 .o 物件檔
OBJS_CPP = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS_CPP))
OBJS_CU  = $(patsubst $(SRCDIR)/%.cu,$(OBJDIR)/%.o,$(SRCS_CU))
OBJS     = $(OBJS_CPP) $(OBJS_CU)

# 相依性檔案
DEPS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.d,$(SRCS_CPP)) $(patsubst $(SRCDIR)/%.cu,$(OBJDIR)/%.d,$(SRCS_CU))

# Control the build verbosity
ifeq ("$(VERBOSE)","1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @printf
endif

.PHONY: all install check clean

# Name of the executable
TARGET = Image_Processing Data_Loader_Example

all: $(OUTDIR) $(TARGET)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OUTDIR):
	@mkdir -p $(OUTDIR)

# 1. 編譯 main.o (拿掉複雜參數，純粹編譯成物件檔)
$(OBJDIR)/main.o: main.cpp | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# 2. 編譯 data_loader_demo.o
$(OBJDIR)/data_loader_demo.o: data_loader_demo.cpp | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# 3. 最終連結：Image_Processing
#  修正：利用 filter-out 確保 $(OBJS) 內若已含 data_loader.o，不會在指令中重複出現導致 Linker 混亂
Image_Processing: $(OBJDIR)/main.o $(OBJS) $(OBJDIR)/data_loader.o
	$(VECHO) "  LD\t$@\n"
	$(Q)$(NVCC) $(OBJDIR)/main.o $(OBJDIR)/data_loader.o $(filter-out $(OBJDIR)/data_loader.o, $(OBJS)) -o $@ $(LINKER) -lpthread -lcudart

# 4. 最終連結：Data_Loader_Example
Data_Loader_Example: $(OBJDIR)/data_loader_demo.o $(OBJDIR)/data_loader.o
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CXX) $(OBJDIR)/data_loader_demo.o $(OBJDIR)/data_loader.o -o $@ $(LINKER)

# Include generated dependency files
-include $(DEPS)

# C++ 檔案的編譯規則
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -MMD -c $< -o $@

# CUDA (.cu) 檔案的編譯規則
$(OBJDIR)/%.o: $(SRCDIR)/%.cu | $(OBJDIR) Makefile
	$(VECHO) "  NVCC\t$@\n"
	$(Q)$(NVCC) $(NVCCFLAGS) -M -MT $@ -o $(patsubst $(OBJDIR)/%.o,$(OBJDIR)/%.d,$@) $<
	$(Q)$(NVCC) $(NVCCFLAGS) -c $< -o $@

# Compilation rule for data_loader.o with explicit dependencies
#  修改後（把 $(OPTFLAGS) 拿掉，改用單純的編譯）：
$(OBJDIR)/data_loader.o: ./Data-Loader/data_loader.cpp ./Data-Loader/data_loader.h | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(WARNINGS) $(CXXFLAGS) -MMD -c $< -o $@

install:
	$(VECHO) "Installing third party dependencies\n"
	$(Q)chmod +x scripts/clone_env.sh  
	$(Q)./scripts/clone_env.sh  > /dev/null 2>&1
	$(VECHO) "Finished installing third party dependencies!!\n"

check:
	$(CHECKCC) $(CHECKFLAGS) ./Image_Processing

clean:
	$(Q)rm -rf $(OBJDIR) $(TARGET) $(OUTDIR)