# ns-3 Installation

This document provides a step-by-step guide of installing ns-3 network simulator with Python 3.x in Windows Linux Subsystem and Ubuntu 18.04 LTS.

## Windows Linux Subsystem

### Install necessary packages

```bash
> sudo apt install build-essential python3 python3-dev python3-pip python3-setuptools git mercurial
> sudo apt install gir1.2-goocanvas-2.0 gir1.2-gtk-3.0 python3-gi python3-gi-cairo python3-pygraphviz libgtk-3-dev
> sudo apt install graphviz doxygen gsl-bin libgsl2 libgsl-dev flex bison libfl-dev tcpdump
> sudo apt install libxml2 libxml2-dev sqlite sqlite3 libsqlite3-dev pkg-config castxml libgcrypt20-dev libgcrypt20
> sudo apt install libboost-signals-dev libboost-filesystem-dev
```

### Prepare Python Environment

Though the ns-3 currently only supports Python 2.7, however, we managed to get it working with Python 3.5.x successfully.

To prepare your Python environment, change default version of python under `/usr/bin/python` using `update-alternatives` command.

```bash
> sudo update-alternatives --install /usr/bin/python python /usr/bin/python2.7 1
> sudo update-alternatives --install /usr/bin/python python /usr/bin/python3.5 2
> sudo update-alternatives --install /usr/bin/pip pip /usr/bin/pip2 1
> sudo update-alternatives --install /usr/bin/pip pip /usr/bin/pip3 2
> python --version
Python 3.5.2
> pip --version
pip 8.1.1 from /usr/lib/python3/dist-packages (python 3.5)
> sudo pip install pygccxml
```

### Clone ns-3-allinone

```bash
(repo) > git clone https://gitlab.com/nsnam/ns-3-allinone.git
(repo) > cd ns-3-allinone
(ns-3-allinone) > python download.py
```

The `download.py` will clone all the repositories with source codes.
To run **ns-3** with Python 3, an additional patch needs to be applied to `ns-3-dev` repository.
To do that, simply browse to `ns-3-dev` and execute the following command to fetch the patch.

```bash
(ns-3-allinone) > cd ns-3-dev
(ns-3-dev) > git fetch git@gitlab.com:leavesw/ns-3-dev.git dev-python3
(ns-3-dev) > git checkout -b leavesw/ns-3-dev-dev-python3 FETCH_HEAD
(ns-3-dev) > cd ..
(ns-3-allinone) >
```

### Compile ns-3

```bash
(ns-3-allinone) > python build.py --enable-tests --enable-examples
```







