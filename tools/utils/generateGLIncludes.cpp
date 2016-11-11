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
        "#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)\n"
        "#ifndef WIN32_LEAN_AND_MEAN\n"
        "#define WIN32_LEAN_AND_MEAN 1\n"
        "#endif\n"
        "#include <windows.h>\n"
        "#endif\n"
        "\n"
        "#include <stddef.h>\n"
        "#ifndef GLEXT_64_TYPES_DEFINED\n"
        "/* This code block is duplicated in glxext.h, so must be protected */\n"
        "#define GLEXT_64_TYPES_DEFINED\n"
        "/* Define int32_t, int64_t, and uint64_t types for UST/MSC */\n"
        "/* (as used in the GL_EXT_timer_query extension). */\n"
        "#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L\n"
        "#include <inttypes.h>\n"
        "#elif defined(__sun__) || defined(__digital__)\n"
        "#include <inttypes.h>\n"
        "#if defined(__STDC__)\n"
        "#if defined(__arch64__) || defined(_LP64)\n"
        "typedef long int int64_t;\n"
        "typedef unsigned long int uint64_t;\n"
        "#else\n"
        "typedef long long int int64_t;\n"
        "typedef unsigned long long int uint64_t;\n"
        "#endif /* __arch64__ */\n"
        "#endif /* __STDC__ */\n"
        "#elif defined( __VMS ) || defined(__sgi)\n"
        "#include <inttypes.h>\n"
        "#elif defined(__SCO__) || defined(__USLC__)\n"
        "#include <stdint.h>\n"
        "#elif defined(__UNIXOS2__) || defined(__SOL64__)\n"
        "typedef long int int32_t;\n"
        "typedef long long int int64_t;\n"
        "typedef unsigned long long int uint64_t;\n"
        "#elif defined(_WIN32) && defined(__GNUC__)\n"
        "#include <stdint.h>\n"
        "#elif defined(_WIN32)\n"
        "typedef __int32 int32_t;\n"
        "typedef __int64 int64_t;\n"
        "typedef unsigned __int64 uint64_t;\n"
        "#else\n"
        "/* Fallback if nothing above works */\n"
        "#include <inttypes.h>\n"
        "#endif\n"
        "#endif\n"
        "\n"
        "\n"
    ;
} // writeHeader

static void
writePODs(QTextStream& ts)
{
    ts <<
        "typedef unsigned int GLenum;\n"
        "typedef unsigned char GLboolean;\n"
        "typedef unsigned int GLbitfield;\n"
        "typedef void GLvoid;\n"
        "typedef signed char GLbyte;\n"
        "typedef short GLshort;\n"
        "typedef int GLint;\n"
        "typedef int GLclampx;\n"
        "typedef unsigned char GLubyte;\n"
        "typedef unsigned short GLushort;\n"
        "typedef unsigned int GLuint;\n"
        "typedef int GLsizei;\n"
        "typedef float GLfloat;\n"
        "typedef float GLclampf;\n"
        "typedef double GLdouble;\n"
        "typedef double GLclampd;\n"
        "\n"
        "#if !defined(GL_VERSION_2_0)\n"
        "typedef char GLchar;\n"
        "#endif\n"
        "#if !defined(GL_ARB_shader_objects)\n"
        "typedef char GLcharARB;\n"
        "#ifdef __APPLE__\n"
        "typedef void *GLhandleARB;\n"
        "#else\n"
        "typedef unsigned int GLhandleARB;\n"
        "#endif\n"
        "#endif\n"
        "#if !defined(ARB_ES2_compatibility) && !defined(GL_VERSION_4_1)\n"
        "typedef GLint GLfixed;\n"
        "#endif\n"
        "#if !defined(GL_ARB_half_float_vertex) && !defined(GL_VERSION_3_0)\n"
        "typedef unsigned short GLhalf;\n"
        "#endif\n"
        "#if !defined(GL_ARB_half_float_pixel)\n"
        "typedef unsigned short GLhalfARB;\n"
        "#endif\n"
        "#if !defined(GL_ARB_sync) && !defined(GL_VERSION_3_2)\n"
        "typedef int64_t GLint64;\n"
        "typedef struct __GLsync *GLsync;\n"
        "typedef uint64_t GLuint64;\n"
        "#endif\n"
        "#if !defined(GL_EXT_timer_query)\n"
        "typedef int64_t GLint64EXT;\n"
        "typedef uint64_t GLuint64EXT;\n"
        "#endif\n"
        "#if !defined(GL_VERSION_1_5)\n"
        "#ifdef __APPLE__\n"
        "typedef intptr_t GLintptr;\n"
        "typedef intptr_t GLsizeiptr;\n"
        "#else\n"
        "typedef ptrdiff_t GLintptr;\n"
        "typedef ptrdiff_t GLsizeiptr;\n"
        "#endif\n"
        "#endif\n"
        "#if !defined(GL_ARB_vertex_buffer_object)\n"
        "#ifdef __APPLE__\n"
        "typedef intptr_t GLintptrARB;\n"
        "typedef intptr_t GLsizeiptrARB;\n"
        "#else\n"
        "typedef ptrdiff_t GLintptrARB;\n"
        "typedef ptrdiff_t GLsizeiptrARB;\n"
        "#endif\n"
        "#endif\n"
        "\n"
        "\n"
    ;
} // writePODs

static void
writeStartClass(const QString& namespaceName,
                QTextStream& ts)
{
    ts <<
        "namespace " << namespaceName << " {\n"
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
        "} // namespace " << namespaceName << "\n"
        "\n";
}

static void
writeFooter(QTextStream& ts)
{
    ts << "#endif // OSGLFUNCTIONS_H\n";
}

struct FunctionSignature
{
    QString funcName;
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
                           bool templateValue,
                           bool includeDebug)
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
    if (includeDebug) {
        ots << "#include <glad/glad.h> // libs.pri sets the right include path. glads.h may set GLAD_DEBUG\n\n";
    }
    if (templateValue) {
        ots << "extern \"C\" {\n";
        for (std::list<FunctionSignature>::const_iterator it = functions.begin(); it != functions.end(); ++it) {
            if (includeDebug) {
                ots <<
                    "#ifdef GLAD_DEBUG\n"
                    "extern " << it->funcPNType << " glad_debug_" << it->funcName << ";\n"
                    "#else\n";
            }
            ots <<
                "extern " << it->funcPNType << " glad_" << it->funcName << ";\n";
            if (includeDebug) {
                ots <<
                    "#endif\n";
            }
        }
        ots <<
            "} // extern C\n"
            "\n";
    }

    ots <<
        "namespace " << namespaceName << " {\n"
        "\n";

    // Write the load functions
    ots <<
        "template <>\n"
        "void OSGLFunctions<" << (templateValue ? "true" : "false") << ">::load_functions() {\n";
    if (!templateValue) {
        ots <<
            "#ifdef HAVE_OSMESA\n"
            "\n";
    }
    for (std::list<FunctionSignature>::const_iterator it = functions.begin(); it != functions.end(); ++it) {
        if (templateValue) {
            // OpenGL functions are directly pointing to the ones loaded by glad
            if (includeDebug) {
                ots <<
                    "#ifdef GLAD_DEBUG\n"
                    "    _" << it->funcName << " = glad_debug_" << it->funcName << ";\n"
                    "#else\n";
            }
            {
                ots <<
                    "    _" << it->funcName << " = glad_" << it->funcName << ";\n";
            }
            if (includeDebug) {
                ots <<
                    "#endif\n";
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
    ots <<
        "} // namespace " << namespaceName << "\n"
        "\n";
} // writeImplementationCppFile

int
main(int argc,
     char** argv)
{
    if (argc != 6) {
        std::cout << "This program takes in input glad.h and outputs the include and implementation files for OSMesa OpenGL function and regular OpenGL functions." << std::endl;
        std::cout << "Usage: generateGLIncludes <glad.h path> <output dir path> <namespace name> <baseFileName> <inlcude glad debug symbols>" << std::endl;
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
    bool supportGladDebug;
    {
        QString supportDebugSymbolsStr(argv[5]);
        supportGladDebug = (bool)supportDebugSymbolsStr.toInt();
    }
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

            signatures.push_back(signature);
        } // if (foundFuncDef != -1 && foundPNFToken != -1) {

        prevLine = line;
    }

    writeHeader(ots_header);
    writePODs(ots_header);
    ots_header <<
        definesStr << "\n"
        "\n";
    ots_header <<
        functionTypedefsStr << "\n"
        "\n";
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
        "    OSGLFunctions() { load_functions(); }\n"
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
        QString lineStart = "    static " +  it->returnType + " " + it->funcName;
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

    writeImplementationCppFile(namespaceName, baseFilename, signatures, absoluteDirPath, "gl", true, supportGladDebug);
    writeImplementationCppFile(namespaceName, baseFilename, signatures, absoluteDirPath, "mesa", false, supportGladDebug);


    return 0;
} // main
