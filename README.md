# COP4520 Data Structure Project

Team #1: John Albury, Dax Borde, Richard Snyder, Matthew Garrison

Scalable Lock-Free Vector with Combining: https://ieeexplore.ieee.org/document/7967182

## STM Implementation
### Setting up RSTM
First, download the `main.cpp` file from Webcourses and download RSTM v7 from https://code.google.com/archive/p/rstm/downloads.

Next, unpack rstm to somewhere like `/home/rstm` and run the following from `/home` or wherever you unpacked rstm.

```
mkdir rstm_build
cd rstm_build
cmake ../rstm
make
```

This should build RSTM on your system. If it doesn't work, try running it on an Ubuntu 18.04 VM; that's what I ended up using.

Next, test that everything is working by copying `main.cpp` to `rstm/bench`. In `rstm/bench/CMakeLists.txt`, add `main` to the list starting on line 22. Then, run:
```
cd rstm_build
make
bench/mainSSB64
```
It should output something like `x = 40000`.

### Running STM Implementation
Now, move the `stm_vector.cpp` and `stm_vector.h` files to `rstm/bench` (you can remove `main.cpp` from there now). In `rstm/bench/CMakeLists.txt`, replace `main` with `stm_vector` in the list starting on line 22. Also, move `run_stm.py` to `rstm_build/`. Then, you can run the STM implementation by running:
```
cd rstm_build
make
python3 run_stm.py
```
The results will be located in the directory you end up in (`rstm_build/`) under the name `results.txt`. If you re-run the test, make sure to delete or move `results.txt` before re-running, as the output will be appended to `results.txt` if it already exists.
