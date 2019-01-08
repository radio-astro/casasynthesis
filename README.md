# introduction
This is a fork of some of the synthesis submodule (combined with inter project
dependencies) of CASA 4.4. It is taken from the NRAO svn repository:

https://svn.cv.nrao.edu/svn/casa/branches/release-4_4/code/

# Usage

## requirements

 * Cmake
 * Casacore >= 3.0

## Compilation

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/opt/casasynthesis ..
$ make 
$ make install
```

## development

[![Build Status](https://travis-ci.org/radio-astro/casasynthesis.svg?branch=master)](https://travis-ci.org/radio-astro/casasynthesis)
