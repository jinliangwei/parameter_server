# The Bosen Parameter Server System

The adagrad branch contains code used for the SoCC'15 paper: [Managed Communication and Consistency for Fast Data-Parallel Iterative Analytics](http://www.cs.cmu.edu/~jinlianw/papers/socc15_wei.pdf)

## To Build The Parameter Server System and Applications

1. The system was tested on Ubuntu 14.04 and requires g++-4.8.

2. Install dependencies from Ubuntu's package manager: 

```bash
sudo apt-get install uuid-dev, autoconf, libtool
sudo ln -s /usr/lib/x86_64-linux-gnu/librt.so /usr/lib/librt.so
```

3. Download and build third-party dependencies:
```bash
make third_party_core
```

4. Build the parameter server:
```bash
make ps_lib ml_lib
```

5. Go to each application directory under apps to build applications

### To Build the SGD matrix factorization application
```bash
cd apps/matrixfact
make matrixfact_split
```
Before running the application, you need to prepare your dataset using `data_split` which partitions the data file into N pieces (suppose you are using N machines) and convert it into a efficient binary format.

### To Build the Latent Dirichlet Allocation application
```bash
cd apps/lda
make lda
```
### Host file
Bosen needs a host file listing all machines to be used in the following format, suppose we are using 4 machines:

```bash
0 10.0.0.1 10000
2 10.0.0.2 10000
3 10.0.0.3 10000
4 10.0.0.4 10000
```
Each line contains `<host-id> <host-ip> <port number>`

### Reproduce the TuX2 MatrixFact experiment
1. Prepare your dataset
```bash
make data_split
./scripts/run_data_split.sh
```
See `scripts/run_data_split.sh` for options. 

2. Run the matrix program 
```bash
./scripts/run_matrixfact_nsdi17.sh
```

See `scripts/run_matrixfact_nsdi17.sh` for options, you don't need to worry aboud anything below table staleness if you just use the Netflix dataset.

### Reproduce the TuX2 LDA experiment
1. Prepare the pubmed dataset
```bash
make data_preprocessor
./scripts/run_data_processor.sh [input-data-path] [output-data-path] [number-of-partitions]
```
`[input-data-path]` is in libsvm format.
`[number-of-partitions]` is number of machines to use.

2. Run the LDA program
```bash
./scripts/run_lda_nsdi17.sh
```
See `scripts/pubmed.config.sh` for options.
