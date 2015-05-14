import numpy as np
from distutils.core import setup, Extension
from Cython.Build import cythonize
from distutils.command.build_ext import build_ext as _build_ext
from subprocess import call

extensions = [
        Extension(
            "_pypsmcpp",
            # sources=["src/_pypsmcpp.pyx", "src/conditioned_sfs.cpp", "src/hmm.cpp"],
            sources=["src/_pypsmcpp.pyx", "src/transition.cpp", "src/conditioned_sfs.cpp", "src/hmm.cpp"],
            language="c++",
            include_dirs=["/usr/include/eigen3", "/usr/local/include/eigen3", np.get_include()],
            extra_compile_args=["-Og", "-g", "-std=c++11", "-Wfatal-errors", "-Wno-unused-variable", "-Wno-unused-function"], 
            # extra_compile_args=["-O0", "-g", "-std=c++11", "-Wfatal-errors", "-Wno-unused-variable", "-Wno-unused-function"], 
            ),
        Extension(
            "_expm",
            # sources=["src/_pypsmcpp.pyx", "src/conditioned_sfs.cpp", "src/hmm.cpp"],
            sources=["src/_expm.pyx"],
            language="c++",
            include_dirs=["/usr/include/eigen3", "/usr/local/include/eigen3", np.get_include()],
            extra_compile_args=["-O3", "-DNDEBUG", "-std=c++11", "-Wfatal-errors", "-Wno-unused-variable", "-Wno-unused-function"],
            ),
        ]

setup(name='psmc++',
        version='0.01',
        description='PSMC++',
        author='Jonathan Terhorst, Jack Kamm, Yun S. Song',
        author_email='terhorst@stat.berkeley.edu',
        url='https://github.com/terhorst/psmc++',
        ext_modules=cythonize(extensions),
    )