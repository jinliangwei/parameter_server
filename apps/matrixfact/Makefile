MATRIXFACT_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)
MATRIXFACT_SRC=$(MATRIXFACT_DIR)/src
PETUUM_ROOT = $(MATRIXFACT_DIR)/../../

include $(PETUUM_ROOT)/defns.mk

MATRIXFACT_BIN = $(MATRIXFACT_DIR)/bin

matrixfact: $(MATRIXFACT_BIN)/matrixfact
matrixfact_split: $(MATRIXFACT_BIN)/matrixfact_split
matrixfact_adarevision: $(MATRIXFACT_BIN)/matrixfact_adarevision
matrixfact_adahalf: $(MATRIXFACT_BIN)/matrixfact_adahalf
matrixfact_split16: $(MATRIXFACT_BIN)/matrixfact_split16
process_snapshot: $(MATRIXFACT_BIN)/process_snapshot
randomize: $(MATRIXFACT_BIN)/randomize
duplicate: $(MATRIXFACT_BIN)/duplicate
data_split: $(MATRIXFACT_BIN)/data_split

$(MATRIXFACT_BIN):
	mkdir -p $(MATRIXFACT_BIN)

$(MATRIXFACT_BIN)/matrixfact: $(MATRIXFACT_SRC)/matrixfact.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/matrixfact.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/matrixfact_split: $(MATRIXFACT_SRC)/matrixfact_split.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/matrixfact_split.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/matrixfact_adarevision: $(MATRIXFACT_SRC)/matrixfact_adarevision.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/matrixfact_adarevision.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/matrixfact_adahalf: $(MATRIXFACT_SRC)/matrixfact_adahalf.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/matrixfact_adahalf.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/matrixfact_split16: $(MATRIXFACT_SRC)/matrixfact_split16.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/matrixfact_split16.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/process_snapshot: $(MATRIXFACT_SRC)/process_snapshot.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/process_snapshot.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/randomize: $(MATRIXFACT_SRC)/randomize.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/randomize.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/duplicate: $(MATRIXFACT_SRC)/duplicate.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/duplicate.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(MATRIXFACT_BIN)/data_split: $(MATRIXFACT_SRC)/data_split.cpp $(PETUUM_PS_LIB) \
	$(MATRIXFACT_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(MATRIXFACT_SRC)/data_split.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

clean:
	rm -rf $(MATRIXFACT_BIN)

.PHONY: matrixfact duplicate data_split matrixfact_split matrixfact_adarevision \
	matrixfact_adahalf matrixfact_split16 process_snapshot clean
