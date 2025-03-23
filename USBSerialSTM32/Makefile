include libs/tinyusb/examples/build_system/make/make.mk

INC += \
  src \
  $(TOP)/hw \
  libs/lwjson-1.7.0/lwjson/src/include \

# Example source
EXAMPLE_SOURCE += $(src/*.c)
SRC_C += $(addprefix $(CURRENT_PATH)/, $(EXAMPLE_SOURCE))

include libs/tinyusb/examples/build_system/make/rules.mk
