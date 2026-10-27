TARGET = servo_fw
BUILD  = build
PORT  ?= COM7

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

CFLAGS  = -mcpu=cortex-m3 -mthumb -O2 -Wall -ffunction-sections -fdata-sections
CFLAGS += -DSTM32F103xE
LDFLAGS = -Tld/stm32f103xe.ld -Wl,--gc-sections -nostartfiles

SRCS = src/main.c startup/startup_stm32f103xe.s
OBJS = $(BUILD)/main.o $(BUILD)/startup_stm32f103xe.o

.PHONY: all clean flash

all: $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/main.o: src/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/startup_stm32f103xe.o: startup/startup_stm32f103xe.s | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILD)

flash: $(BUILD)/$(TARGET).bin
	python -m stm32loader -p $(PORT) -b 115200 -e -w -v $(BUILD)/$(TARGET).bin
