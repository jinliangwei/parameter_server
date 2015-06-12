COMM_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)
COMM_SRC=$(COMM_DIR)/src
PETUUM_ROOT = $(COMM_DIR)/../../

include $(PETUUM_ROOT)/defns.mk

COMM_BIN = $(COMM_DIR)/bin

comm: $(COMM_BIN)/comm

$(COMM_BIN):
	mkdir -p $(COMM_BIN)

$(COMM_BIN)/comm: $(COMM_SRC)/comm.cpp $(PETUUM_PS_LIB) $(COMM_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(COMM_SRC)/comm.cpp $(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

clean:
	rm -rf $(COMM_BIN)

.PHONY: comm
