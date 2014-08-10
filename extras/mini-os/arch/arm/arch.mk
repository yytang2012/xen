ifeq ($(XEN_TARGET_ARCH),arm32)
DEF_ASFLAGS += -march=armv7-a -mfpu=vfpv3
ARCH_CFLAGS  := -march=armv7-a -mthumb -fms-extensions -D__arm__ -DXEN_HAVE_PV_GUEST_ENTRY -mthumb-interwork
EXTRA_INC += $(TARGET_ARCH_DIR)/include/$(XEN_TARGET_ARCH)
EXTRA_SRC += arch/$(EXTRA_INC)
endif

