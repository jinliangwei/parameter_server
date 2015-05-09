
LDA_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)
LDA_SRC_DIR = $(LDA_DIR)/src
PETUUM_ROOT = $(LDA_DIR)/../../

include $(PETUUM_ROOT)/defns.mk

LDA_SRC = $(filter-out $(LDA_SRC_DIR)/data_preprocessor.cpp $(LDA_SRC_DIR)/row_test.cpp $(LDA_SRC_DIR)/data_preprocessor_ylda.cpp $(LDA_SRC_DIR)/process_ylda.cpp $(LDA_SRC_DIR)/process_to_ylda.cpp $(LDA_SRC_DIR)/vocab_analyer.cpp, $(wildcard $(LDA_SRC_DIR)/*.cpp))
LDA_HDR = $(wildcard $(LDA_SRC_DIR)/*.hpp)
LDA_BIN = $(LDA_DIR)/bin
LDA_OBJ = $(LDA_SRC:.cpp=.o)
LDA_SN_OBJ = $(LDA_SRC:.cpp=_sn.o)

all: lda data_preprocessor

lda: $(LDA_BIN)/lda_main
lda_sn: $(LDA_BIN)/lda_sn_main

data_preprocessor: $(LDA_BIN)/data_preprocessor
data_preprocessor_ylda: $(LDA_BIN)/data_preprocessor_ylda
process_ylda: $(LDA_BIN)/process_ylda
process_to_ylda: $(LDA_BIN)/process_to_ylda

row_test: $(LDA_BIN)/row_test

$(LDA_BIN):
	mkdir -p $(LDA_BIN)

$(LDA_BIN)/lda_main: $(LDA_OBJ) $(PETUUM_PS_LIB) $(LDA_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(LDA_OBJ) $(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

$(LDA_BIN)/lda_sn_main: $(LDA_SN_OBJ) $(PETUUM_PS_SN_LIB) $(LDA_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) -DPETUUM_SINGLE_NODE $(PETUUM_INCFLAGS) \
	$(LDA_SN_OBJ) $(PETUUM_PS_SN_LIB) $(PETUUM_LDFLAGS) -o $@

$(LDA_BIN)/data_preprocessor: $(LDA_SRC_DIR)/data_preprocessor.cpp $(LDA_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) -Wno-unused-result $(PETUUM_INCFLAGS) \
	$(LDA_SRC_DIR)/data_preprocessor.cpp $(PETUUM_LDFLAGS) -o $@

$(LDA_BIN)/process_ylda: $(LDA_SRC_DIR)/process_ylda.cpp $(LDA_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) -Wno-unused-result $(PETUUM_INCFLAGS) \
	$(LDA_SRC_DIR)/process_ylda.cpp $(PETUUM_LDFLAGS) -o $@

$(LDA_BIN)/process_to_ylda: $(LDA_SRC_DIR)/process_to_ylda.cpp $(LDA_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) -Wno-unused-result $(PETUUM_INCFLAGS) \
	$(LDA_SRC_DIR)/process_to_ylda.cpp $(PETUUM_LDFLAGS) -o $@

$(LDA_BIN)/data_preprocessor_ylda: $(LDA_SRC_DIR)/data_preprocessor_ylda.cpp $(LDA_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) -Wno-unused-result $(PETUUM_INCFLAGS) \
	$(LDA_SRC_DIR)/data_preprocessor_ylda.cpp $(PETUUM_LDFLAGS) -o $@

$(LDA_OBJ): %.o: %.cpp $(LDA_HDR)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) -Wno-unused-result $(PETUUM_INCFLAGS) \
	-c $< -o $@

$(LDA_SN_OBJ): %_sn.o: %.cpp $(LDA_HDR)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) -DPETUUM_SINGLE_NODE -Wno-unused-result \
	$(PETUUM_INCFLAGS) -c $< -o $@

$(LDA_BIN)/row_test: $(LDA_SRC_DIR)/row_test.cpp $(LDA_BIN)
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(LDA_SRC_DIR)/row_test.cpp $(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) -o $@

clean:
	rm -rf $(LDA_OBJ)
	rm -rf $(LDA_BIN)
	rm -rf $(LDA_SN_OBJ)

.PHONY: clean lda lda_sn data_preprocessor process_ylda data_preprocessor_ylda
