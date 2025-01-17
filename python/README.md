# Python bindings for DuMu<sup>x</sup>

The Python bindings have two objectives
* making it possible to use DuMu<sup>x</sup> from Python
* use Python code in DuMu<sup>x</sup>

The binding are experimental until further notice which means
the API might undergo unannounced changes breaking backwards
compatibility. Track changes by regularly checking for current merge requests
and newly merged Dumux commits on GitLab. Python bindings require Python 3.

##  Installation

Python bindings are part of the dune core modules >= version 2.8.
We recommend to use the Dune modules on the master branch to try out
Python bindings.

### Example development setup (Dune 2.9-git)

This section is rather new and experimental. Please help improving
this documentation in the future.

Checkout the `master` branch of the Dune core modules and DuMu<sup>x</sup>

```
git clone https://gitlab.dune-project.org/core/dune-common
git clone https://gitlab.dune-project.org/core/dune-geometry
git clone https://gitlab.dune-project.org/core/dune-grid
git clone https://gitlab.dune-project.org/core/dune-localfunctions
git clone https://gitlab.dune-project.org/core/dune-istl
git clone https://git.iws.uni-stuttgart.de/dumux-repositories/dumux
```

Create and activate a new virtual environment in which the
Python modules will be installed in editable mode (symlinked)

```
python3 -m venv venv
source ./venv/bin/activate
```

Run dunecontrol which will setup both C++ and Python bindings and modules.

```
./dune-common/bin/dunecontrol --opts=cmake.opts all
```


### Example development setup (Dune 2.8)

This section is rather new and experimental. Please help improving
this documentation in the future.

Checkout the `releases/2.8` branch of the Dune core modules and DuMu<sup>x</sup> 3.5

```
git clone -b releases/2.8 https://gitlab.dune-project.org/core/dune-common
git clone -b releases/2.8 https://gitlab.dune-project.org/core/dune-geometry
git clone -b releases/2.8 https://gitlab.dune-project.org/core/dune-grid
git clone -b releases/2.8 https://gitlab.dune-project.org/core/dune-localfunctions
git clone -b releases/2.8 https://gitlab.dune-project.org/core/dune-istl
git clone -b releases/3.5 https://git.iws.uni-stuttgart.de/dumux-repositories/dumux

cp dumux/cmake.opts .
```

Enable the Python bindings by adding the flag in `cmake.opts` (see comments inside the `.opts` file).
Then run dunecontrol which also builds the Dune Python bindings.

```
./dune-common/bin/dunecontrol --opts=cmake.opts all
```

Add the Python binding modules to your Python path like this and install them with
the setup environment setup script

```
source dumux/python/setup-python-env.sh
```

If you are getting error with loading MPI in Python you might need to preload the MPI library

```
export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/openmpi/lib/libmpi.so
```

Replace the path with the path to the MPI library on your system.


### Running a test

Run your first DuMu<sup>x</sup> Python test

```
python3 dumux/test/python/test_py_gridgeometry.py
```

The Python bindings are based on just-in-time compilation of C++ code,
so the first execution might take considerably longer than subsequent executions.
In particular compiling the Dune grid interface may a few minutes.

You can run all currently existing DuMu<sup>x</sup> Python tests with
```
cd dumux/build-cmake
ctest -L python
```

##  Development

All Python files should be linted by the tool [`black`](https://pypi.org/project/black/).
You can install `black` with `pip install black` and run it from the dumux top-directory

```
black ./python
```

You can also run it on a specific file (replace `./python` by file name)
This will automatically format the Python files. Run black before every commit changing Python files.

The `dumux` Python module should be get a score of `10` from
the tool [`pylint`](https://pypi.org/project/pylint/).
You can install `pylint` with `pip install pylint` and run it from the dumux top-directory

```
pylint build-cmake/python/dumux
```

Pylint needs to be able to check imports so the modules need to be properly set up
with `setup-dunepy.py` (see above). The `pylint` configuration file `dumux/.pylintrc` can
be used to configure `pylint`. Some exceptions or other parameters than the default
might be sensible in the future but generally advice given by `pylint` leads to better code.
Different from `black`, `pylint` does no itself fix the code, you need to do this yourself.
Always run `black` before checking `pylint`.
