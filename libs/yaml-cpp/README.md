yaml-cpp
========

Version 0.5.3 obtained from https://github.com/jbeder/yaml-cpp/releases
The patch yaml-cpp-fix-namespace.patch is applied to change the YAML namespace to YAML_NAMESPACE which encodes the library version into it. This is to avoid symbol loading clash with system yaml. The includes have also be changed to avoid the compiler picking up system includes. 
Please apply the patch from the root directory of the Natron repository:

    patch -p1 libs/yaml-cpp/yaml-cpp-fix-namespace.patch
