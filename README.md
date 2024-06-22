CGIF Helper
===========

[![Build Status](https://github.com/tlsa/libcgifh/workflows/CI/badge.svg)](https://github.com/tlsa/libcyaml/actions)

LibCGIFH is a simple C library containing some helper routines designed to make
it easy to create animated diagrams with [CGIF](https://github.com/dloebl/cgif).

Overview
--------

The library provides an interface to create an image and render into it. It also
provides simple palette construction helpers. It has no direct dependency on
CGIF (or anything beyond the standard C Library), so it may be useful for any
8-bit indexed colour image drawing task.

### Goals

* Simple.

### Features

* Render lines and rectangles.
* Render text at different scales.
* Automatically clip to image dimensions.
