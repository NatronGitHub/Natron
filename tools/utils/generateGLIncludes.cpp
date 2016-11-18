#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <cassert>
#include <iostream>

static void
writeHeader(QTextStream& ts)
{
    ts <<
        "/*\n"
        " * THIS FILE WAS GENERATED AUTOMATICALLY FROM glad.h by tools/utils/generateGLIncludes, DO NOT EDIT\n"
        " */\n"
        "\n"
        "#ifndef OSGLFUNCTIONS_H\n"
        "#define OSGLFUNCTIONS_H\n"
        "\n"
        "#include <cstring> // memset\n"
        "\n"
        "#include <glad/glad.h> // libs.pri sets the right include path. glads.h may set GLAD_DEBUG\n"
        "\n"
        "#include \"Global/Macros.h\"\n"
        "\n"
        "#ifdef GLAD_DEBUG\n"
        "#define glad_defined(glFunc) glad_debug_ ## glFunc\n"
        "#else\n"
        "#define glad_defined(glFunc) glad_ ## glFunc\n"
        "#endif\n"
        "\n"
        ;
} // writeHeader

static void
writeStartClass(const QString& namespaceName,
                QTextStream& ts)
{
    if (namespaceName == "Natron") {
        ts <<
            "NATRON_NAMESPACE_ENTER;\n\n";
    } else {
        ts <<
            "namespace " << namespaceName << " {\n\n";
    }
    ts <<
        "template <bool USEOPENGL>\n"
        "class OSGLFunctions\n"
        "{\n";
}

static void
writeEndClass(const QString& namespaceName,
              QTextStream& ts)
{
    ts <<
        "};\n"
        "\n"
        "typedef OSGLFunctions<true> GL_GPU;\n"
        "typedef OSGLFunctions<false> GL_CPU;\n"
        "\n";
    if (namespaceName == "Natron") {
        ts <<
            "NATRON_NAMESPACE_EXIT;\n";
    } else {
        ts <<
            "} // namespace " << namespaceName << "\n";
    }

}

static void
writeFooter(QTextStream& ts)
{
    ts << "\n#endif // OSGLFUNCTIONS_H\n";
}

struct FunctionSignature
{
    QString funcName;
    QString funcNameNoGL;
    QString signature;
    QString returnType;
    QString funcPNType;
    QStringList parameters;
};

static void
writeImplementationCppFile(const QString& namespaceName,
                           const QString& baseFileName,
                           const std::list<FunctionSignature>& functions,
                           const QString& path,
                           const QString &API,
                           bool templateValue)
{
    QString outputFilename = path + "/" + baseFileName + "_" + API + ".cpp";
    QFile of(outputFilename);

    if ( !of.open(QIODevice::WriteOnly) ) {
        std::cout << "Could not open " << outputFilename.toStdString() << std::endl;
        throw std::runtime_error("");
    }
    QTextStream ots(&of);
    ots <<
        "/*\n"
        " * THIS FILE WAS GENERATED AUTOMATICALLY FROM glad.h by tools/utils/generateGLIncludes, DO NOT EDIT\n"
        " */\n"
        "\n"
        "#include \"" << baseFileName << ".h\"\n"
        "#include \"Global/Macros.h\"\n"
        "\n";
    if (!templateValue) {
        ots <<
            "#ifdef HAVE_OSMESA\n"
            "#include <GL/gl_mangle.h>\n"
            "#include <GL/glu_mangle.h>\n"
            "#include <GL/osmesa.h>\n"
            "#endif // HAVE_OSMESA\n";
        ots << "\n\n";
    }

    if (namespaceName == "Natron") {
        ots <<
            "NATRON_NAMESPACE_ENTER;\n\n";
    } else {
        ots <<
            "namespace " << namespaceName << " {\n\n";
    }
    // Write the load functions
    ots <<
        "template <>\n"
        "void OSGLFunctions<" << (templateValue ? "true" : "false") << ">::load_functions() {\n";
    if (!templateValue) {
        ots <<
            "#ifdef HAVE_OSMESA\n";
    }
    for (std::list<FunctionSignature>::const_iterator it = functions.begin(); it != functions.end(); ++it) {
        if (templateValue) {
            // OpenGL functions are directly pointing to the ones loaded by glad
            {
                ots <<
                    "    _" << it->funcName << " = glad_defined(" << it->funcName << ");\n";
            }
        } else {
            // Mesa functions are loaded
            ots <<
                "    _" << it->funcName << " = (" << it->funcPNType << ")OSMesaGetProcAddress(\"" << it->funcName << "\");\n";
        }
    }
    if (!templateValue) {
        ots <<
            "#endif // HAVE_OSMESA\n";
    }
    ots <<
        "} // load_functions\n"
        "\n";


    ots <<
        "template class OSGLFunctions<" << (templateValue ? "true" : "false") << ">;\n"
        "\n";
    if (namespaceName == "Natron") {
        ots <<
            "NATRON_NAMESPACE_EXIT;\n";
    } else {
        ots <<
            "} // namespace " << namespaceName << "\n";
    }
} // writeImplementationCppFile

int
main(int argc,
     char** argv)
{
    if (argc != 6) {
        std::cout << "This program takes in input glad.h and outputs the include and implementation files for OSMesa OpenGL function and regular OpenGL functions." << std::endl;
        std::cout << "Usage: generateGLIncludes <glad.h path> <output dir path> <namespace name> <baseFileName>" << std::endl;
        std::cout << "Example: generateGLIncludes /Users/alexandre/development/Natron/Global/gladRel/include/glad/glad.h /Users/alexandre/development/Natron/Engine Natron OSGLFunctions 1" << std::endl;

        return 1;
    }

    QFile f(argv[1]);
    if ( !f.open(QIODevice::ReadOnly) ) {
        std::cout << "Could not open " << argv[1] << std::endl;

        return 1;
    }

    // Check that output path exists

    QDir outputDir(argv[2]);
    if ( !outputDir.exists() ) {
        std::cout << argv[2] << " does not seem to be a valid directory" << std::endl;

        return 1;
    }

    QString namespaceName(argv[3]);
    QString baseFilename(argv[4]);
    QString absoluteDirPath = outputDir.absolutePath();
    QString outputHeaderFilename = absoluteDirPath + "/" + baseFilename + ".h";
    QFile of_header(outputHeaderFilename);
    if ( !of_header.open(QIODevice::WriteOnly) ) {
        std::cout << "Could not open " << outputHeaderFilename.toStdString() << std::endl;

        return 1;
    }
    QTextStream ots_header(&of_header);
    QTextStream its(&f);
    QString definesStr;

    std::list<FunctionSignature> signatures;
    QString functionTypedefsStr;
    QString prevLine;
    while ( !its.atEnd() ) {
        // Read each line of glad.h
        QString line = its.readLine();

        {
            // Search for a define
            QString toFind = "#define GL_";
            int found = line.indexOf(toFind);
            if (found != -1) {
                definesStr += line;
                definesStr += "\n";
            }
        }

        // Search for a function
        QString typedefToken("typedef ");
        QString pfnToken("(APIENTRYP PFNGL");
        int foundFuncDef = line.indexOf(typedefToken);
        int foundPNFToken = line.indexOf(pfnToken);
        if ( (foundFuncDef != -1) && (foundPNFToken != -1) ) {
            int pos = foundPNFToken + pfnToken.size();
            int foundFirstEndParenthesis = line.indexOf(')', pos);
            assert(foundFirstEndParenthesis != -1);


            FunctionSignature signature;
            QString lastFuncNameCaps = line.mid(pos, foundFirstEndParenthesis - pos);
            signature.signature = line.mid(foundFirstEndParenthesis);
            // "near" and "far" are defined as macros in windows.h
            signature.signature.replace("GLdouble near, GLdouble far", "GLdouble nearVal, GLdouble farVal");

            signature.returnType = line.mid( foundFuncDef + typedefToken.size(), foundPNFToken - 1 - ( foundFuncDef + typedefToken.size() ) );
            QString funcTypeDefStr = "typedef ";
            funcTypeDefStr += signature.returnType;
            funcTypeDefStr += " (*PFNGL";
            funcTypeDefStr += lastFuncNameCaps;
            funcTypeDefStr += signature.signature;
            funcTypeDefStr += "\n";
            functionTypedefsStr += funcTypeDefStr;

            // Remove the extraneous ; at the end of the signature
            // Also remove the prepending )
            signature.signature.remove(0, 1);
            signature.signature.remove(signature.signature.size() - 1, 1);

            signature.funcPNType = "PFNGL";
            signature.funcPNType += lastFuncNameCaps;

            // extract parameters
            {
                int i = 1; // start after the leading (
                while ( i < signature.signature.size() ) {
                    QString param;
                    while ( signature.signature[i] != QChar(',') && signature.signature[i] != QChar(')') ) {
                        param.append(signature.signature[i]);
                        ++i;
                    }

                    // Now only keep the name of the parameter
                    {
                        int j = param.size() - 1;
                        while ( j >= 0 && param[j].isLetterOrNumber() ) {
                            --j;
                        }
                        param = param.mid(j + 1);
                    }
                    signature.parameters.append(param);

                    assert( signature.signature[i] == QChar(',') || signature.signature[i] == QChar(')') );
                    ++i; // bypass last character

                    if ( signature.signature[i] == QChar(')') ) {
                        break;
                    }
                }
            }

            // we caught a function typedef before, we expect to read the following #define glXxxx function with the appropriate case
            // in release glad.h, the next line is of the type GLAPI PFNGLCOLOR4FVPROC glad_glColor4fv;
            // the line after that is the one we want #define glColor4fv glad_glColor4fv
            line = its.readLine();
            assert( !its.atEnd() );
            line = its.readLine();

            QString toFind("#define gl");
            int foundDefine = line.indexOf(toFind);
            if (foundDefine == -1) {
                std::cout << "Parser failed to find #define glXXX statement 2 lines after a function typedef, make sure that you are running this program against a release version of glad.h" << std::endl;

                return 1;
            }

            // Check that this is the same symbol
            // Remove the PROC at the end of the func def
            lastFuncNameCaps.remove("PROC");
            int checkIndex = toFind.size();
            QString symbolStart = line.mid(checkIndex);
            assert( symbolStart.startsWith(lastFuncNameCaps, Qt::CaseInsensitive) );

            {
                int i = 8; // start on the g
                //extract the function name
                while ( i < line.size() && line.at(i) != QChar(' ') ) {
                    signature.funcName.push_back( line.at(i) );
                    ++i;
                }
            }
            signature.funcNameNoGL = signature.funcName;
            signature.funcNameNoGL.remove(0,2); // remove "gl"
            signatures.push_back(signature);
        } // if (foundFuncDef != -1 && foundPNFToken != -1) {

        prevLine = line;
    }

    writeHeader(ots_header);
#if 0 // moved to Global/GLObfuscate.h
    ots_header <<
        "// remove global macro definitions of OpenGL functions by glad.h\n"
        "// fgrep \"#define gl\" glad.h |sed -e 's/#define/#undef/g' |awk '{print $1, $2}'\n";
    // undef glad's defines
    for (std::list<FunctionSignature>::iterator it = signatures.begin(); it != signatures.end(); ++it) {
      ots_header << "#undef " << it->funcName << '\n';
    }
#endif

    writeStartClass(namespaceName, ots_header);

    // Define the singleton
    ots_header <<
        "    static OSGLFunctions<USEOPENGL>& getInstance()\n"
        "    {\n"
        "        static OSGLFunctions<USEOPENGL> instance;\n"
        "\n"
        "        return instance;\n"
        "    }\n"
        "\n"
        "    // load function, implemented in _gl.h and _mesa.h\n"
        "    void load_functions();\n"
        "\n"
        "    // private constructor\n"
        "    OSGLFunctions() { std::memset(this, 0, sizeof(*this)); load_functions(); }\n"
        "\n";

    // Declare member functions
    for (std::list<FunctionSignature>::iterator it = signatures.begin(); it != signatures.end(); ++it) {
        ots_header << "    " << it->funcPNType << " _" << it->funcName << ";\n";
    }

    ots_header <<
        "\n";

    ots_header <<
        "public:\n"
        "\n";


    ots_header <<
        "    // static non MT-safe load function that must be called once to initialize functions\n"
        "    static void load()\n"
        "    {\n"
        "        (void)getInstance();\n"
        "    }\n"
        "\n"
        "    static bool isGPU()\n"
        "    {\n"
        "        return USEOPENGL;\n"
        "    }\n";

    for (std::list<FunctionSignature>::iterator it = signatures.begin(); it != signatures.end(); ++it) {
        QString lineStart = "    static " +  it->returnType + " " + it->funcNameNoGL;
        QString indentedSig = it->signature;
        indentedSig.replace( ", ", ",\n" + QString(lineStart.size() + 1, ' ') );
        ots_header << "\n" <<
            lineStart << indentedSig << "\n"
            "    {\n";
        if (it->returnType == "void") {
            ots_header << "        ";
        } else {
            ots_header << "        return ";
        }
        ots_header << "getInstance()._" << it->funcName << "(";
        QStringList::const_iterator next = it->parameters.begin();
        if ( !it->parameters.isEmpty() ) {
            ++next;
        }
        for (QStringList::const_iterator it2 = it->parameters.begin(); it2 != it->parameters.end(); ++it2) {
            ots_header << *it2;
            if ( next != it->parameters.end() ) {
                ots_header << ", ";
                ++next;
            }
        }

        ots_header <<
            ");\n"
            "    }\n";
    }
    writeEndClass(namespaceName, ots_header);

    writeFooter(ots_header);

    writeImplementationCppFile(namespaceName, baseFilename, signatures, absoluteDirPath, "gl", true);
    writeImplementationCppFile(namespaceName, baseFilename, signatures, absoluteDirPath, "mesa", false);


    return 0;
} // main
