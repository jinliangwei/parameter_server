MATRIXFACT_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)
PETUUM_ROOT = $(MATRIXFACT_DIR)/../../

include $(PETUUM_ROOT)/defns.mk

MATRIXFACT_BIN = $(MATRIXFACT_DIR)/bin

matrixfact: $(MATRIXFACT_BIN)/matrixfact
randomize: $(MATRIXFACT_BIN)/randomize

$(MATRIXFACT_BIN):
	mkdir -p $(MATRIXFACT_BIN)

$(MATRIXFACT_BIN)/matrixfact: $(MATRIXFACT_DIR)/matrixfact.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_DIR)/matrixfact.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/randomize: $(MATRIXFACT_DIR)/randomize.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_DIR)/randomize.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

clean:
	rm -rf $(MATRIXFACT_BIN)

.PHONY: matrixfact randomize clean
