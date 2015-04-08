ADAGRAD_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)
ADAGRAD_SRC=$(ADAGRAD_DIR)/src
PETUUM_ROOT = $(ADAGRAD_DIR)/../../

include $(PETUUM_ROOT)/defns.mk

ADAGRAD_BIN = $(ADAGRAD_DIR)/bin

adagrad: $(ADAGRAD_BIN)/adagrad
adagrad_mt: $(ADAGRAD_BIN)/adagrad_mt
adagrad_l2_mt: $(ADAGRAD_BIN)/adagrad_l2_mt
adagrad_l2_shared_mt: $(ADAGRAD_BIN)/adagrad_l2_shared_mt
lr: $(ADAGRAD_BIN)/lr
lr_l2_mt: $(ADAGRAD_BIN)/lr_l2_mt
adarevision_l2_mt: $(ADAGRAD_BIN)/adarevision_l2_mt

$(ADAGRAD_BIN):
	mkdir -p $(ADAGRAD_BIN)

$(ADAGRAD_BIN)/adagrad: $(ADAGRAD_SRC)/adagrad.cpp $(PETUUM_PS_LIB) \
	$(ADAGRAD_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(ADAGRAD_SRC)/adagrad.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_ML_LIB) $(PETUUM_LDFLAGS) -o $@

$(ADAGRAD_BIN)/adagrad_mt: $(ADAGRAD_SRC)/adagrad_mt.cpp $(PETUUM_PS_LIB) \
	$(ADAGRAD_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(ADAGRAD_SRC)/adagrad_mt.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_ML_LIB) $(PETUUM_LDFLAGS) -o $@

$(ADAGRAD_BIN)/adagrad_l2_mt: $(ADAGRAD_SRC)/adagrad_l2_mt.cpp $(PETUUM_PS_LIB) \
	$(ADAGRAD_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(ADAGRAD_SRC)/adagrad_l2_mt.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_ML_LIB) $(PETUUM_LDFLAGS) -o $@

$(ADAGRAD_BIN)/adagrad_l2_shared_mt: $(ADAGRAD_SRC)/adagrad_l2_shared_mt.cpp $(PETUUM_PS_LIB) \
	$(ADAGRAD_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(ADAGRAD_SRC)/adagrad_l2_shared_mt.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_ML_LIB) $(PETUUM_LDFLAGS) -o $@

$(ADAGRAD_BIN)/lr: $(ADAGRAD_SRC)/lr.cpp $(PETUUM_PS_LIB) \
	$(ADAGRAD_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(ADAGRAD_SRC)/lr.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_ML_LIB) $(PETUUM_LDFLAGS) -o $@

$(ADAGRAD_BIN)/lr_l2_mt: $(ADAGRAD_SRC)/lr_l2_mt.cpp $(PETUUM_PS_LIB) \
	$(ADAGRAD_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(ADAGRAD_SRC)/lr_l2_mt.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_ML_LIB) $(PETUUM_LDFLAGS) -o $@

$(ADAGRAD_BIN)/adarevision_l2_mt: $(ADAGRAD_SRC)/adarevision_l2_mt.cpp $(PETUUM_PS_LIB) \
	$(ADAGRAD_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(ADAGRAD_SRC)/adarevision_l2_mt.cpp \
	$(PETUUM_PS_LIB) $(PETUUM_ML_LIB) $(PETUUM_LDFLAGS) -o $@

clean:
	rm -rf $(ADAGRAD_BIN)

.PHONY: adagrad adagrad_mt adagrad_l2_mt adagrad_l2_shared_mt lr lr_l2_mt adarevision_l2_mt
