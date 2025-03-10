# demucs GUI implementation with JUCE

Compatible with macOS / Windows

## How to build

1. Clone this repository
2. Update submodules
```
git submodule update --init --recursive
```

3. Install C++ dependencies (c.f. [demucs.cpp/README.md](https://github.com/sevagh/demucs.cpp/blob/main/README.md))
```
$ sudo apt-get install gcc g++ cmake clang-tools libopenblas0-openmp libopenblas-openmp-dev
```
4. 
```
mkdir Builds
cd Builds
cmake ..
cmake --build .
```

5. Run Builds/DemucsJUCE_artefacts/Demucs\ JUCE.app
