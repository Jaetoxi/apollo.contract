# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/joslin/code/workspace/opensource/apollo.contract/contracts"
  "/Users/joslin/code/workspace/opensource/apollo.contract/build/contracts"
  "/Users/joslin/code/workspace/opensource/apollo.contract/build/contracts_project-prefix"
  "/Users/joslin/code/workspace/opensource/apollo.contract/build/contracts_project-prefix/tmp"
  "/Users/joslin/code/workspace/opensource/apollo.contract/build/contracts_project-prefix/src/contracts_project-stamp"
  "/Users/joslin/code/workspace/opensource/apollo.contract/build/contracts_project-prefix/src"
  "/Users/joslin/code/workspace/opensource/apollo.contract/build/contracts_project-prefix/src/contracts_project-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/joslin/code/workspace/opensource/apollo.contract/build/contracts_project-prefix/src/contracts_project-stamp/${subDir}")
endforeach()
