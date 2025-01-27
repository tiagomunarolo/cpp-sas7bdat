all: build

BUILD_TYPE := Release
PYTHON_VERSION := 3.8.12
VENV_NAME := venv38
ENABLE_CONAN := ON

.PHONY: configure
configure:
	cmake -S . -B ./build -DENABLE_CONAN:BOOL=${ENABLE_CONAN} -DCMAKE_BUILD_TYPE:STRING=${BUILD_TYPE} -DCMAKE_CXX_FLAGS="${CXX_FLAGS}"

.PHONY: build
build: configure
	cmake --build ./build --config ${BUILD_TYPE}

.PHONY: clean
clean:
	make -C build clean

.PHONY: pyenv pyenv-download pyenv-python pyenv-venv
pyenv: pyenv-download pyenv-python pyenv-venv

pyenv-download:
	curl -L https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash

pyenv-python:
	export PYTHON_CONFIGURE_OPTS="--enable-shared"; ~/.pyenv/bin/pyenv install --force $(PYTHON_VERSION)

pyenv-venv:
	~/.pyenv/bin/pyenv virtualenv -p python3.8 $(PYTHON_VERSION) $(VENV_NAME)