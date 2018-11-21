# ONI Reader

Early development

A small python library for reading .oni files produced by
[OpenNI](https://github.com/occipital/openni2). It uses pybind11, so
it returns numpy arrays for RGB and depth images.

Currently, it has a simplified interface. Maybe, in the future, the
project may add bindings closer to the OpenNI C++ API.

## Install

Requirements:

* Linux environment
* OpenNI2 C++ library

Installing openni2-dev on **Ubuntu**:

```shell
$ sudo apt install libopenni2-dev libusb-1.0-0-dev
```

Install onireader using pip:

```shell
$ pip install pip install git+https://github.com/otaviog/onireader.git
```
