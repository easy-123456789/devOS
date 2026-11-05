# Circle-based USB HID firmware is the default output for this repository.
# Circle itself is fetched into vendor/ and is not committed here.
CIRCLE_REF ?= 7a81e1b480fe7d19fc1fc695b89bff0a0848f44b
CIRCLE_DIR := ../vendor/circle

OBJS = main.o kernel.o
LIBS = $(CIRCLEHOME)/lib/usb/libusb.a \
       $(CIRCLEHOME)/lib/input/libinput.a \
       $(CIRCLEHOME)/lib/fs/libfs.a \
       $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
