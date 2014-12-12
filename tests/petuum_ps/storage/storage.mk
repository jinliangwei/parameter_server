TESTS_STORAGE_DIR=$(TESTS)/petuum_ps/storage

storage_test: $(TESTS_STORAGE_DIR)/storage_test.cpp
	$(PETUUM_CXX) $(PETUUM_CXXFLAGS) $(PETUUM_INCFLAGS) \
	$(TESTS_STORAGE_DIR)/storage_test.cpp $(PETUUM_PS_LIB) $(PETUUM_LDFLAGS) \
	-o $(TESTS_STORAGE_DIR)/storage_test

run_storage_test: storage_test
	GLOG_logtostderr=true \
	$(TESTS_STORAGE_DIR)/storage_test \
	--num_testers 64 \
	--cache_capacity 128 \
	--test_case FindTest \
	$(TESTS_STORAGE_DIR)/storage_test

clean_storage_test:
	rm -rf storage_test

.PHONY: storage_test run_storage_test clean_storage_test
