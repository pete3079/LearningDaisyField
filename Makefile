# Project Name
TARGET = oscillator

# Sources
CPP_SOURCES = oscillator.cpp

# Library Locations
LIBDAISY_DIR = /home/pete/Developer/Daisy/libDaisy
DAISYSP_DIR =  /home/pete/Developer/Daisy/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

