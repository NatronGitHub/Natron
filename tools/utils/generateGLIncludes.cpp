#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <cassert>
#include <iostream>

static void writePODs(QTextStream& ts)
{
    ts << "typedef unsigned int GLenum;\n";
    ts << "typedef unsigned char GLboolean;\n";
    ts << "typedef unsigned int GLbitfield;\n";
    ts << "typedef void GLvoid;\n";
    ts << "typedef signed char GLbyte;\n";
    ts << "typedef short GLshort;\n";
    ts << "typedef int GLint;\n";
    ts << "typedef int GLclampx;\n";
    ts << "typedef unsigned char GLubyte;\n";
    ts << "typedef unsigned short GLushort;\n";
    ts << "typedef unsigned int GLuint;\n";
    ts << "typedef int GLsizei;\n";
    ts << "typedef float GLfloat;\n";
    ts << "typedef float GLclampf;\n";
    ts << "typedef double GLdouble;\n";
    ts << "typedef double GLclampd;\n";
    ts << "typedef char GLchar;\n";
    ts << "typedef char GLcharARB;\n";
    ts << "#ifdef __APPLE__\n";
    ts << "typedef void *GLhandleARB;\n";
    ts << "#else\n";
    ts << "typedef unsigned int GLhandleARB;\n";
    ts << "#endif\n";
    ts << "typedef unsigned short GLhalfARB;\n";
    ts << "typedef unsigned short GLhalf;\n";
    ts << "typedef GLint GLfixed;\n";
    ts << "typedef ptrdiff_t GLintptr;\n";
    ts << "typedef ptrdiff_t GLsizeiptr;\n";
    ts << "typedef int64_t GLint64;\n";
    ts << "typedef uint64_t GLuint64;\n";
    ts << "typedef ptrdiff_t GLintptrARB;\n";
    ts << "typedef ptrdiff_t GLsizeiptrARB;\n";
    ts << "typedef int64_t GLint64EXT;\n";
    ts << "typedef uint64_t GLuint64EXT;\n";
    ts << "\n\n\n\n";
}

static void writeHeader(QTextStream& ts)
{
    ts << "/*\n";
    ts << "THIS FILE WAS GENERATED AUTOMATICALLY FROM glad.h, DO NOT EDIT\n";
    ts << "*/\n\n\n\n";
    ts << "#ifndef OSGLFUNCTIONS_H\n";
    ts << "#define OSGLFUNCTIONS_H\n\n\n\n";


    ts << "#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)\n";
    ts << "#ifndef WIN32_LEAN_AND_MEAN\n";
    ts << "#define WIN32_LEAN_AND_MEAN 1\n";
    ts << "#endif\n";
    ts << "#include <windows.h>\n";
    ts << "#endif\n\n\n\n";

    ts << "#include <stddef.h>\n";
    ts << "#ifndef GLEXT_64_TYPES_DEFINED\n";
    ts << "/* This code block is duplicated in glxext.h, so must be protected */\n";
    ts << "#define GLEXT_64_TYPES_DEFINED\n";
    ts << "/* Define int32_t, int64_t, and uint64_t types for UST/MSC */\n";
    ts << "/* (as used in the GL_EXT_timer_query extension). */\n";
    ts << "#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L\n";
    ts << "#include <inttypes.h>\n";
    ts << "#elif defined(__sun__) || defined(__digital__)\n";
    ts << "#include <inttypes.h>\n";
    ts << "#if defined(__STDC__)\n";
    ts << "#if defined(__arch64__) || defined(_LP64)\n";
    ts << "typedef long int int64_t;\n";
    ts << "typedef unsigned long int uint64_t;\n";
    ts << "#else\n";
    ts << "typedef long long int int64_t;\n";
    ts << "typedef unsigned long long int uint64_t;\n";
    ts << "#endif /* __arch64__ */\n";
    ts << "#endif /* __STDC__ */\n";
    ts << "#elif defined( __VMS ) || defined(__sgi)\n";
    ts << "#include <inttypes.h>\n";
    ts << "#elif defined(__SCO__) || defined(__USLC__)\n";
    ts << "#include <stdint.h>\n";
    ts << "#elif defined(__UNIXOS2__) || defined(__SOL64__)\n";
    ts << "typedef long int int32_t;\n";
    ts << "typedef long long int int64_t;\n";
    ts << "typedef unsigned long long int uint64_t;\n";
    ts << "#elif defined(_WIN32) && defined(__GNUC__)\n";
    ts << "#include <stdint.h>\n";
    ts << "#elif defined(_WIN32)\n";
    ts << "typedef __int32 int32_t;\n";
    ts << "typedef __int64 int64_t;\n";
    ts << "typedef unsigned __int64 uint64_t;\n";
    ts << "#else\n";
    ts << "/* Fallback if nothing above works */\n";
    ts << "#include <inttypes.h>\n";
    ts << "#endif\n";
    ts << "#endif\n";
    ts << "\n\n\n\n";



}


static void writeStartClass(const QString& namespaceName, QTextStream& ts)
{
    ts << "namespace " << namespaceName << " {\n\n";
    ts << "template <bool USEOPENGL>\n";
    ts << "class OSGLFunctions\n";
    ts << "{\n\n\n";
}

static void writeEndClass(const QString& namespaceName, QTextStream& ts)
{
    ts << "}; // class OSGLFunctions\n\n\n";
    ts << "typedef OSGLFunctions<true> GL_GPU;\n";
    ts << "typedef OSGLFunctions<false> GL_CPU;\n\n\n";
    ts << "} // namespace " << namespaceName << "\n\n";
}

static void writeFooter(QTextStream& ts)
{
    ts << "#endif // OSGLFUNCTIONS_H\n";

}

struct FunctionSignature {
    QString funcName;
    QString signature;
    QString returnType;
    QString funcPNType;
    QStringList parameters;
};


static void writeImplementationCppFile(const QString& namespaceName, const QString& baseFileName, const std::list<FunctionSignature>& functions, const QString& path, const QString &API, bool templateValue, bool includeDebug)
{
    QString outputFilename = path + "/" + baseFileName + "_" + API + ".cpp";
    QFile of(outputFilename);
    if (!of.open(QIODevice::WriteOnly)) {
        std::cout << "Could not open " << outputFilename.toStdString() << std::endl;
        throw std::runtime_error("");
    }
    QTextStream ots(&of);
    ots << "/*THIS FILE WAS GENERATED AUTOMATICALLY FROM glad.h, DO NOT EDIT*/\n\n\n\n";
    ots << "#include \"" << baseFileName << ".h\"\n\n";
    if (!templateValue) {
        ots << "#ifdef HAVE_OSMESA\n\n";
        ots << "#include <GL/gl_mangle.h>\n";
        ots << "#include <GL/glu_mangle.h>\n";
        ots << "#include <GL/osmesa.h>\n";
        ots << "#endif // HAVE_OSMESA\n";
    }
    ots << "\n\n";

    if (templateValue) {
        ots << "extern \"C\" {\n";
        for (std::list<FunctionSignature>::const_iterator it = functions.begin(); it != functions.end(); ++it) {
            if (includeDebug) {
                ots << "#ifdef DEBUG\n";
                ots << "extern " << it->funcPNType << " glad_debug_" << it->funcName << ";\n";
                ots << "#else\n";
            }
            ots << "extern " << it->funcPNType << " glad_" << it->funcName << ";\n";
            if (includeDebug) {
                ots << "#endif\n";
            }
        }
        ots << "} // extern C\n";
        ots << "\n\n";
    }

    ots << "namespace " << namespaceName << " {\n\n";

    // Write the load functions
    ots << "template <>\n";
    ots << "void OSGLFunctions<";
    if (templateValue) {
        ots << "true";
    } else {
        ots << "false";
    }
    ots << ">::load_functions() {\n";
    if (!templateValue) {
        ots << "#ifdef HAVE_OSMESA\n\n";
    }
    for (std::list<FunctionSignature>::const_iterator it = functions.begin(); it != functions.end(); ++it) {
        if (templateValue) {
            // OpenGL functions are directly pointing to the ones loaded by glad
            if (includeDebug) {
                ots << "#ifdef DEBUG\n";
                ots << "    _m" << it->funcName << " = glad_debug_" << it->funcName << ";\n";
                ots << "#else\n";
            }
            ots << "    _m" << it->funcName << " = glad_" << it->funcName << ";\n";
            if (includeDebug) {
                ots << "#endif\n";
            }
        } else {
            // Mesa functions are loaded
            ots << "    _m" << it->funcName << " = (" << it->funcPNType << ")OSMesaGetProcAddress(\"" << it->funcName << "\");\n";
        }

    }
    if (!templateValue) {
        ots << "#endif // HAVE_OSMESA\n";
    }
    ots << "} // load_functions \n\n";


    ots << "template class OSGLFunctions<";
    if (templateValue) {
        ots << "true";
    } else {
        ots << "false";
    }
    ots << ">;\n\n";
    ots << "} // namespace " << namespaceName << " \n\n";
}


int main(int argc, char** argv)
{

    if (argc != 6) {
        std::cout << "This program takes in input glad.h and outputs the include and implementation files for OSMesa OpenGL function and regular OpenGL functions." << std::endl;
        std::cout << "Usage: generateGLIncludes <glad.h path> <output dir path> <namespace name> <baseFileName> <inlcude glad debug symbols>" << std::endl;
        std::cout << "Example: generateGLIncludes /Users/alexandre/development/Natron/Global/gladRel/include/glad/glad.h /Users/alexandre/development/Natron/Engine Natron OSGLFunctions 1" << std::endl;
        return 1;
    }

    QFile f(argv[1]);
    if (!f.open(QIODevice::ReadOnly)) {
        std::cout << "Could not open " << argv[1] << std::endl;
        return 1;
    }

    // Check that output path exists

    QDir outputDir(argv[2]);
    if (!outputDir.exists()) {
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
    if (!of_header.open(QIODevice::WriteOnly)) {
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
        if (foundFuncDef != -1 && foundPNFToken != -1) {
            
            int pos = foundPNFToken + pfnToken.size();

            int foundFirstEndParenthesis = line.indexOf(')', pos);
            assert(foundFirstEndParenthesis != -1);


            FunctionSignature signature;
            QString lastFuncNameCaps = line.mid(pos, foundFirstEndParenthesis - pos);
            signature.signature = line.mid(foundFirstEndParenthesis);

            signature.returnType = line.mid(foundFuncDef + typedefToken.size(), foundPNFToken -1 - (foundFuncDef + typedefToken.size()));
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
                while (i < signature.signature.size()) {
                    QString param;
                    while (signature.signature[i] != QChar(',') && signature.signature[i] != QChar(')')) {
                        param.append(signature.signature[i]);
                        ++i;
                    }

                    // Now only keep the name of the parameter
                    {
                        int j = param.size() - 1;
                        while (j >= 0 && param[j].isLetterOrNumber()) {
                            --j;
                        }
                        param = param.mid(j + 1);
                    }
                    signature.parameters.append(param);

                    assert(signature.signature[i] == QChar(',') || signature.signature[i]== QChar(')'));
                    ++i; // bypass last character

                    if (signature.signature[i]== QChar(')')) {
                        break;
                    }
                }
            }

            // we caught a function typedef before, we expect to read the following #define glXxxx function with the appropriate case
            // in release glad.h, the next line is of the type GLAPI PFNGLCOLOR4FVPROC glad_glColor4fv;
            // the line after that is the one we want #define glColor4fv glad_glColor4fv
            line = its.readLine();
            assert(!its.atEnd());
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
           assert(symbolStart.startsWith(lastFuncNameCaps, Qt::CaseInsensitive));

            {
                int i = 8; // start on the g
                //extract the function name
                while (i < line.size() && line.at(i) != QChar(' ')) {
                    signature.funcName.push_back(line.at(i));
                    ++i;
                }
            }

            signatures.push_back(signature);
        } // if (foundFuncDef != -1 && foundPNFToken != -1) {

        prevLine = line;
    }

    writeHeader(ots_header);
    writePODs(ots_header);
    ots_header << definesStr;
    ots_header << "\n\n\n";
    ots_header << functionTypedefsStr;
    ots_header << "\n\n\n";
    writeStartClass(namespaceName, ots_header);
    ots_header << "\n\n\n";

    // Define the singleton
    ots_header << "    static OSGLFunctions<USEOPENGL>& getInstance() {\n";
    ots_header << "        static OSGLFunctions<USEOPENGL> instance;\n";
    ots_header << "        return instance;\n";
    ots_header << "    }\n\n";

    // Define the load function, implemented in _gl.h and _mesa.h
    ots_header << "    void load_functions();\n\n";

    // Define a private constructor
    ots_header << "    OSGLFunctions() { load_functions(); } \n\n";

    // Declare member functions
    for (std::list<FunctionSignature>::iterator it = signatures.begin(); it != signatures.end(); ++it)  {
        ots_header << "    " << it->funcPNType << " _m" << it->funcName << ";\n";
    }

    ots_header << "\n\n";

    ots_header << "public: \n\n\n";


    ots_header << "// Declare a static non MT-safe load function that must be called once to initialize functions\n";
    ots_header << "    static void load() {\n";
    ots_header << "         (void)getInstance();\n";
    ots_header << "    }\n\n";


    ots_header << "    static bool isGPU() {\n";
    ots_header << "         return USEOPENGL;\n";
    ots_header << "    }\n\n";

    for (std::list<FunctionSignature>::iterator it = signatures.begin(); it != signatures.end(); ++it) {
        ots_header << "    static " <<  it->returnType << " " << it->funcName << it->signature << " {\n";
        if (it->returnType == "void") {
            ots_header << "        ";
        } else {
            ots_header << "        return ";
        }
        ots_header << "getInstance()._m" << it->funcName << "(";
        QStringList::const_iterator next = it->parameters.begin();
        if (!it->parameters.isEmpty()) {
            ++next;
        }
        for (QStringList::const_iterator it2 = it->parameters.begin(); it2 != it->parameters.end(); ++it2) {
            ots_header << *it2;
            if (next != it->parameters.end()) {
                ots_header << ", ";
                ++next;
            }
        }

        ots_header << ");\n";
        ots_header << "    }\n\n";
    }
    ots_header << "\n\n\n\n";
    writeEndClass(namespaceName, ots_header);

    writeFooter(ots_header);

    writeImplementationCppFile(namespaceName, baseFilename, signatures, absoluteDirPath, "gl", true, supportGladDebug);
    writeImplementationCppFile(namespaceName, baseFilename, signatures, absoluteDirPath, "mesa", false, supportGladDebug);



    return 0;
}

