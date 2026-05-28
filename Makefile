# Compiler & Linker settings
CXX = g++
NVCC = /usr/lib/nvidia-cuda-toolkit/bin/nvcc
CXXFLAGS = -I ./inc -I ./third-party/CImg -I ./third-party/libjpeg -I ./Data-Loader -std=c++11
# CUDA 編譯參數：包含相同標頭檔路徑，並指定 C++11 標準
NVCCFLAGS = -I ./inc -I ./third-party/CImg -I ./third-party/libjpeg -I ./Data-Loader -std=c++11 -Xcompiler "-O3 -march=native"
#  修正：移除 -flto 避免
OPTFLAGS = -march=native -funroll-loops -finline-functions -ffast-math -O3
WARNINGS = -g -Wall
LINKER = -L/usr/X11R6/lib -lm -lpthread -lX11 -L./third-party/libjpeg -ljpeg -lpng

CHECKCC = valgrind
SUPP_FILE = valgrind.supp
CHECKFLAGS = --leak-check=full -s --show-leak-kinds=definite,indirect --track-origins=yes --suppressions=$(SUPP_FILE)

# 確保 Shell 在 echo 時讀得到這個多行變數
export VALGRIND_SUPP_CONTENT
# 定義自動生成的抑制檔內容避(免WSL下NVIDIA驅動產生的錯誤報告）
define VALGRIND_SUPP_CONTENT
{
   Ignore_Glibc_Dlopen_Cond
   Memcheck:Cond
   ...
   fun:_dl_*
}
{
   Ignore_Glibc_Dlerror_Cond
   Memcheck:Cond
   ...
   fun:*dlerror*
}
{
   Ignore_Glibc_Dlopen_Param_Openat
   Memcheck:Param
   openat(filename)
   ...
   fun:_dl_*
}
{
   Ignore_WSL_Dir_Cond
   Memcheck:Cond
   ...
   obj:*/usr/lib/wsl/*
}
{
   Ignore_WSL_Dir_Value8
   Memcheck:Value8
   ...
   obj:*/usr/lib/wsl/*
}
{
   Ignore_WSL_Dir_Value4
   Memcheck:Value4
   ...
   obj:*/usr/lib/wsl/*
}
{
   Ignore_WSL_Param_Mmap_Length
   Memcheck:Param
   mmap(length)
   ...
   obj:*/usr/lib/wsl/*
}
{
   Ignore_WSL_Param_Ioctl
   Memcheck:Param
   ioctl(generic)
   ...
   obj:*/usr/lib/wsl/*
}
{
   Ignore_Libcuda_All_Cond
   Memcheck:Cond
   ...
   obj:*libcuda*
}
{
   Ignore_Libcudart_All_Cond
   Memcheck:Cond
   ...
   obj:*libcudart*
}
{
   Ignore_Libnvdx_All_Param
   Memcheck:Param
   ...
   obj:*libnvdx*
}
{
   Ignore_System_Pthread_Leak
   Memcheck:Leak
   ...
   obj:*libpthread*.so*
}
{
   Ignore_System_Glibc_Leak
   Memcheck:Leak
   ...
   obj:*libc*.so*
}
{
   Ignore_System_Ld_Linker_Leak
   Memcheck:Leak
   ...
   obj:*ld*.so*
}
{
   Ignore_System_Libstdcxx_Leak
   Memcheck:Leak
   ...
   obj:*libstdc++*.so*
}
endef
# =======================================================

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

# 編譯 main.o
$(OBJDIR)/main.o: main.cpp | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# 編譯 data_loader_demo.o
$(OBJDIR)/data_loader_demo.o: data_loader_demo.cpp | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# 最終連結：Image_Processing
Image_Processing: $(OBJDIR)/main.o $(OBJS) $(OBJDIR)/data_loader.o
	$(VECHO) "  LD\t$@\n"
	$(Q)$(NVCC) $(OBJDIR)/main.o $(OBJDIR)/data_loader.o $(filter-out $(OBJDIR)/data_loader.o, $(OBJS)) -o $@ $(LINKER) -lpthread -lcudart

# Data_Loader_Example
Data_Loader_Example: $(OBJDIR)/data_loader_demo.o $(OBJDIR)/data_loader.o
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CXX) $(OBJDIR)/data_loader_demo.o $(OBJDIR)/data_loader.o -o $@ $(LINKER)

# Include generated dependency files
-include $(DEPS)

# C++(.cpp)檔案的編譯規則
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -MMD -c $< -o $@

# CUDA(.cu)檔案的編譯規則
$(OBJDIR)/%.o: $(SRCDIR)/%.cu | $(OBJDIR) Makefile
	$(VECHO) "  NVCC\t$@\n"
	$(Q)$(NVCC) $(NVCCFLAGS) -M -MT $@ -o $(patsubst $(OBJDIR)/%.o,$(OBJDIR)/%.d,$@) $<
	$(Q)$(NVCC) $(NVCCFLAGS) -c $< -o $@

# Compilation rule for data_loader.o
$(OBJDIR)/data_loader.o: ./Data-Loader/data_loader.cpp ./Data-Loader/data_loader.h | $(OBJDIR) Makefile
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CXX) $(WARNINGS) $(CXXFLAGS) -MMD -c $< -o $@

install:
	$(VECHO) "Installing third party dependencies\n"
	$(Q)chmod +x scripts/clone_env.sh  
	$(Q)./scripts/clone_env.sh  > /dev/null 2>&1
	$(VECHO) "Finished installing third party dependencies!!\n"

# 動態檢測
check: $(SUPP_FILE)
	$(CHECKCC) $(CHECKFLAGS) ./Image_Processing

# 動態生成抑制檔的規則
$(SUPP_FILE):
	$(VECHO) "  GEN\t$@\n"
	$(Q)echo "$$VALGRIND_SUPP_CONTENT" > $@

clean:
	$(Q)rm -rf $(OBJDIR) $(TARGET) $(OUTDIR) $(SUPP_FILE)