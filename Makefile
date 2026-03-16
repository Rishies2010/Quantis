# --- 路径配置（与 Manuae-Shell 保持一致） ---
RELIBC_ROOT    = ../relibc-seele
RELIBC_PATH    = $(RELIBC_ROOT)/target/x86_64-seele/release
RELIBC_INCLUDE = $(RELIBC_ROOT)/target/x86_64-seele/include
BUILD_DIR      = build
TARGET         = quantis
FULL_TARGET    = $(BUILD_DIR)/$(TARGET)
SYSROOT        = "../sysroot"
TARGET_DIR     = $(SYSROOT)/programs

# --- 编译器与参数设置 ---
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
CFLAGS  = -ffreestanding -mno-sse -D__seele__ -mno-red-zone \
          -fno-stack-protector -fno-builtin -fno-pie -no-pie \
          -nostdinc -I$(RELIBC_INCLUDE)
LDFLAGS = -static -nostdlib --allow-multiple-definition

# --- 源文件收集（仿照 Manuae-Shell） ---
ALL_C_SRCS    = $(shell find . -name "*.c")
# 如有仅用于 Linux 的实现，可在此排除，例如：./quantis_linux.c
EXCLUDED_FILES =
C_SRCS        = $(filter-out $(EXCLUDED_FILES), $(ALL_C_SRCS))
OBJS          = $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.o))

# --- 构建依赖的 relibc（x86_64-seele, release） ---
.PHONY: relibc
relibc:
	@echo "Building relibc-seele (x86_64-seele, release)..."
	@$(MAKE) -C $(RELIBC_ROOT) all

# --- 默认目标 ---
all: clean relibc $(FULL_TARGET) install

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(FULL_TARGET): $(OBJS)
	@echo "Linking $@..."
	@$(LD) $(LDFLAGS) \
	    $(RELIBC_PATH)/crt0.o \
	    $(RELIBC_PATH)/crti.o \
	    $(OBJS) \
	    $(RELIBC_PATH)/libc.a \
	    $(RELIBC_PATH)/crtn.o \
	    -o $@
	@echo "Build Done. Entry Point:"
	@readelf -h $@ | grep Entry

clean:
	rm -rf $(BUILD_DIR)

# --- 安装到 sysroot/programs（逻辑仿照 Manuae-Shell） ---
install: $(FULL_TARGET)
	@echo "Installing $< to $(TARGET_DIR)/$(TARGET) ..."
	# 1. 确保目标目录存在
	@mkdir -p $(TARGET_DIR)
	# 2. 删除旧文件，避免权限或残留问题
	@sudo rm -f $(TARGET_DIR)/$(TARGET)
	# 3. 拷贝新文件
	@sudo cp $< $(TARGET_DIR)/$(TARGET)
	# 4. 强制同步到磁盘
	@echo "Syncing filesystem to disk..."
	@sync
	# 5. 校验安装结果
	@ORIG_SIZE=$$(stat -c%s "$(FULL_TARGET)"); \
	 INST_SIZE=$$(sudo stat -c%s "$(TARGET_DIR)/$(TARGET)"); \
	 echo "Local: $$ORIG_SIZE bytes | Installed: $$INST_SIZE bytes"; \
	 if [ "$$ORIG_SIZE" -ne "$$INST_SIZE" ]; then \
	     echo "\033[0;31m[ERROR]\033[0m: Size mismatch! File might be corrupted."; \
	     exit 1; \
	 else \
	     echo "\033[0;32m[SUCCESS]\033[0m: Installation verified."; \
	 fi

.PHONY: all clean install
