obj-m := smart_clock.o

smart_clock-m := \
	sc.o \
	sc_control.o \
	sc_button.o \
	sc_panel.o \
	sc_sensors.o

EXTRA_CFLAGS += -Wno-unused-variable -Wno-unused-result

KDIR ?= ../../kernel_old/linux/
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

default:dtbs
	$(MAKE) -C $(KDIR) M=$$PWD CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

dtbs:
	$(KDIR)scripts/dtc/dtc -I dts -O dtbo smart_clock.dts -o smart_clock.dtbo

.PHONY: clean default
