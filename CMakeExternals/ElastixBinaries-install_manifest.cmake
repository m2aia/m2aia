message(STATUS "Creating install manifest")

file(GLOB installed_binaries "lib/libANNlib*.*" "bin/elastix" "bin/transformix")

set(install_manifest "src/ElastixBinaries-build/install_manifest.txt")
file(WRITE ${install_manifest} "")

foreach(f ${installed_binaries})
  file(APPEND ${install_manifest} "${f}\n")
endforeach()