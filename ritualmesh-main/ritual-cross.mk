##########################################
# ritual-cross.mk — Embedded Ritual Build
##########################################

# === CONFIGURATION ===
TARGET   := ritual.elf
SRC      := ritual.c
MCU      := cortex-m4
LINKER   := ritual.ld

# === FLAGS ===
CFLAGS   := -mcpu=$(MCU) -mthumb -nostdlib -O2
LDFLAGS  := -T $(LINKER) -nostdlib

# === BUILD ===
all:
	@echo "🛠️  Compiling $(SRC) for $(MCU)..."
	arm-none-eabi-gcc $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Built $(TARGET)"

# === CLEAN ===
clean:
	rm -f $(TARGET)
	@echo "🧹 Cleaned build artifacts"

.PHONY: all clean

