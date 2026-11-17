from xml.etree import ElementTree
import os
import re
import sys


def clean_typesystem_cpp_sources(typesystem, out):
    tree = ElementTree.parse(typesystem)
    root = tree.getroot()

    package = root.attrib["package"]

    tags = ["namespace-type", "object-type", "value-type"]
    types = [child.attrib["name"] for child in root if child.tag in tags]

    sources = [f"{package.lower()}_module_wrapper.cpp", f"{package.lower()}_python.h"]
    sources.extend([f"{t.lower()}_wrapper.cpp" for t in types])
    sources.extend([f"{t.lower()}_wrapper.h" for t in types])

    prog = re.compile("([\\s(,])::(?!Natron)(\\w+[^:])")

    for s in sources:
        with open(os.path.normpath(os.path.join(out, package, s)), "r+") as f:
            lines = f.readlines()
            f.seek(0)
            for l in lines:
                f.write(prog.sub("\\1\\2", l))
            f.truncate()

    if package.lower() == "natrongui":
        with open(os.path.normpath(os.path.join(out, package, sources[0])), "r+") as f:
            lines = f.readlines()
            f.seek(0)
            for l in lines:
                if (l.endswith("QtCoreTypes;\n") or
                    l.endswith("QtCoreTypeStructs;\n") or
                    l.endswith("QtCoreTypeConverters;\n") or
                    l.endswith("SbkNatronEngineTypes;\n") or
                    l.endswith("SbkNatronEngineTypeStructs;\n") or
                    l.endswith("SbkNatronEngineTypeConverters;\n")):
                    continue
                f.write(l.replace("cleanTypesAttributes", "cleanGuiTypesAttributes"))
            f.truncate()


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: sourceCleanup.py <typesystem> <gen-directory>", file=sys.stderr)
        exit(1)
    clean_typesystem_cpp_sources(sys.argv[1], sys.argv[2])
