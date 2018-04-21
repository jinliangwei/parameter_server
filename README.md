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
