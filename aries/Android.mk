ifeq ($(filter-out s5pv210,$(TARGET_BOARD_PLATFORM)),)
    include $(all-subdir-makefiles)
endif
