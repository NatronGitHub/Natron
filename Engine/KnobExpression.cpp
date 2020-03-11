/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Knob.h"
#include "KnobPrivate.h"

#include <sstream> // stringstream
#include <string>

#include "Engine/KnobItemsTable.h"
#include "Engine/Noise.h"
#include "Engine/PyExprUtils.h"
#include "Global/StrUtils.h"


NATRON_NAMESPACE_ENTER

using std::vector;
using std::string;
using std::stringstream;
using std::pair;
using std::make_pair;
using boost::shared_ptr;


NATRON_NAMESPACE_ANONYMOUS_ENTER

/**
 * @brief Given the string str, returns the position of the character "closingChar" corresponding to the "openingChar" at the given openingParenthesisPos
 * For example if the str is "((lala)+toto)" and we want to get the character matching the first '(', this function will return the last parenthesis in the string
 * @return The position of the matching closing character or string::npos if not found
 **/
static std::size_t
getMatchingParenthesisPosition(std::size_t openingParenthesisPos,
                               char openingChar,
                               char closingChar,
                               const string& str)
{
    assert(openingParenthesisPos < str.size() && str.at(openingParenthesisPos) == openingChar);

    int noOpeningParenthesisFound = 0;
    int i = openingParenthesisPos + 1;

    while ( i < (int)str.size() ) {
        if (str.at(i) == closingChar) {
            if (noOpeningParenthesisFound == 0) {
                break;
            } else {
                --noOpeningParenthesisFound;
            }
        } else if (str.at(i) == openingChar) {
            ++noOpeningParenthesisFound;
        }
        ++i;
    }
    if ( i >= (int)str.size() ) {
        return string::npos;
    }

    return i;
} // getMatchingParenthesisPosition


/**
 * @brief Given a string str, assume that the content of the string between startParenthesis and endParenthesis
 * is a well-formed Python signature with comma separated arguments list. The list of arguments is stored in params.
 * This code looks very fragile.
 * Whitespace handling was added, but it seems like anything that looks like a more
 * complicated expression with operators would break (see https://github.com/NatronGitHub/Natron/issues/448).
 */
static void
extractParameters(std::size_t startParenthesis,
                  std::size_t endParenthesis,
                  const string& str,
                  vector<string>* params)
{
    std::size_t i = startParenthesis + 1;
    int insideParenthesis = 0;

    while ( i < str.size() && (i < endParenthesis || insideParenthesis < 0) ) {
        std::string curParam;
        std::size_t prev_i = i;
        while ( i < str.size() && std::isspace( str.at(i) ) ) {
            ++i;
        }
        if ( i >= str.size() ) {
            break;
        }
        if (str.at(i) == '(') {
            ++insideParenthesis;
            ++i;
        } else if (str.at(i) == ')') {
            --insideParenthesis;
            ++i;
        }
        while ( i < str.size() && std::isspace( str.at(i) ) ) {
            ++i;
        }
        if ( i >= str.size() ) {
            break;
        }
        while ( i < str.size() && ((!std::isspace( str.at(i) ) && str.at(i) != ',') || insideParenthesis > 0) ) {
            curParam.push_back( str.at(i) );
            ++i;
            if (str.at(i) == '(') {
                ++insideParenthesis;
            } else if (str.at(i) == ')') {
                if (insideParenthesis > 0) {
                    --insideParenthesis;
                } else {
                    break;
                }
            }
        }
        while ( i < str.size() && std::isspace( str.at(i) ) ) {
            ++i;
        }
        if ( i >= str.size() ) {
            break;
        }
        if (str.at(i) == ',') {
            ++i;
        }
        if (i == prev_i) {
            assert(false && "this should not happen");
            // We didn't move, this is probably a bug.
            break;
        }
        if ( !curParam.empty() ) {
            params->push_back(curParam);
        }
    }
} // extractParameters


/**
 * @brief Given the string str, tries to find the given function name "token" string starting from inputPos.
 * @param fromDim The dimension in the knob on which the function is called
 * @param fromViewName The name of the view in the knob on which the function is called
 * @param dimensionParamPos Indicate the index (0-based) of the "dimension" argument in the function given by token
 * @param viewParamPos Indicate the index (0-based) of the "view" argument in the function given by token
 * E.g: In the function get(frame, dimension, view), the dimension parameter index is 1.
 * @param returnsTuple If true, indicates that the function given in "token" is expected to return a tuple
 * @param output[out] The script to execute to register this parameter as a dependency of the other parameter in the expression
 * @return true on success, false otherwise
 * Note: this function can throw an exception if the expression is incorrect
 **/
static bool
parseTokenFrom(int fromDim,
               const string& fromViewName,
               int dimensionParamPos,
               int viewParamPos,
               bool returnsTuple,
               const string& str,
               const string& token,
               std::size_t inputPos,
               std::size_t *tokenStart,
               string* output)
{
    std::size_t pos;

    // Find the opening parenthesis after the token start
    bool foundMatchingToken = false;
    while (!foundMatchingToken) {
        pos = str.find(token, inputPos);
        if (pos == string::npos) {
            return false;
        }

        *tokenStart = pos;
        pos += token.size();

        ///Find nearest opening parenthesis
        for (; pos < str.size(); ++pos) {
            if (str.at(pos) == '(') {
                foundMatchingToken = true;
                break;
            } else if (str.at(pos) != ' ') {
                //We didn't find a good token
                break;
            }
        }

        if ( pos >= str.size() ) {
            throw std::invalid_argument("Invalid expr");
        }

        if (!foundMatchingToken) {
            inputPos = pos;
        }
    }

    // Get the closing parenthesis for the function
    std::size_t endingParenthesis = getMatchingParenthesisPosition(pos, '(', ')',  str);
    if (endingParenthesis == string::npos) {
        throw std::invalid_argument("Invalid expr");
    }

    // Extract parameters in between the 2 parenthesis
    vector<string> params;
    extractParameters(pos, endingParenthesis, str, &params);


    bool gotViewParam = viewParamPos != -1 && ( viewParamPos < (int)params.size() && !params.empty() );
    bool gotDimensionParam = dimensionParamPos != -1 && ( dimensionParamPos < (int)params.size() && !params.empty() );


    if (!returnsTuple) {
        // Before Natron 2.2 the View parameter of the get(view) function did not exist, hence loading
        // an old expression may use the old get() function without view. If we do not find any view
        // parameter, assume the view is "Main" by default.
        if ( !gotViewParam && (viewParamPos != -1) ) {
            vector<string>::iterator it = params.begin();
            if ( viewParamPos >= (int)params.size() ) {
                it = params.end();
            } else {
                std::advance(it, viewParamPos);
            }
            params.insert(it, "Main");
        }
        if ( !gotDimensionParam && (dimensionParamPos != -1) ) {
            vector<string>::iterator it = params.begin();
            if ( dimensionParamPos >= (int)params.size() ) {
                it = params.end();
            } else {
                std::advance(it, dimensionParamPos);
            }
            params.insert(it, "0");
        }
    } else {
        assert(dimensionParamPos == -1 && !gotDimensionParam);

        // If the function returns a tuple like get()[dimension],
        // also find the parameter in-between the tuple brackets
        //try to find the tuple
        std::size_t it = endingParenthesis + 1;
        while (it < str.size() && str.at(it) == ' ') {
            ++it;
        }
        if ( ( it < str.size() ) && (str.at(it) == '[') ) {
            ///we found a tuple
            std::size_t endingBracket = getMatchingParenthesisPosition(it, '[', ']',  str);
            if (endingBracket == string::npos) {
                throw std::invalid_argument("Invalid expr");
            }
            params.push_back( str.substr(it + 1, endingBracket - it - 1) );
        } else {
            // No parameters in the tuple: assume this is all dimensions.
            params.push_back("-1");
        }
        dimensionParamPos = 1;
        gotDimensionParam = true;

        // Before Natron 2.2 the View parameter of the get(view) function did not exist, hence loading
        // an old expression may use the old get() function without view. If we do not find any view
        // parameter, assume the view is "Main" by default.
        if ( (params.size() == 1) && (viewParamPos >= 0) ) {
            if (viewParamPos < dimensionParamPos) {
                params.insert(params.begin(), "Main");
            } else {
                params.push_back("Main");
            }
        }
    }

    if ( (dimensionParamPos < 0) || ( (int)params.size() <= dimensionParamPos ) ) {
        throw std::invalid_argument("Invalid expr");
    }

    if ( (viewParamPos < 0) || ( (int)params.size() <= viewParamPos ) ) {
        throw std::invalid_argument("Invalid expr");
    }

    string toInsert;
    {
        stringstream ss;
        /*
           When replacing the getValue() (or similar function) call by addAsDepdendencyOf
           the parameter prefixing the addAsDepdendencyOf will register itself its dimension params[dimensionParamPos] as a dependency of the expression
           at the "fromDim" dimension of thisParam
         */
        ss << ".addAsDependencyOf(thisParam, " << fromDim << "," <<
            params[dimensionParamPos] << ", \"" << fromViewName << "\", \"" <<
            params[viewParamPos] << "\")\n";
        toInsert = ss.str();
    }

    // Find the Python attribute which called the function "token"
    if ( (*tokenStart < 2) || (str[*tokenStart - 1] != '.') ) {
        throw std::invalid_argument("Invalid expr");
    }

    //Find the start of the symbol
    int i = (int)*tokenStart - 2;
    int nClosingParenthesisMet = 0;
    while (i >= 0) {
        if (str[i] == ')') {
            ++nClosingParenthesisMet;
        }
        if ( std::isspace(str[i]) ||
             ( str[i] == '=') ||
             ( str[i] == '\n') ||
             ( str[i] == '\t') ||
             ( str[i] == '+') ||
             ( str[i] == '-') ||
             ( str[i] == '*') ||
             ( str[i] == '/') ||
             ( str[i] == '%') ||
             ( ( str[i] == '(') && !nClosingParenthesisMet ) ) {
            break;
        }
        --i;
    }
    ++i;

    // This is the name of the Python attribute that called "token"
    string varName = str.substr(i, *tokenStart - 1 - i);
    output->append(varName + toInsert);

    //assert(*tokenSize > 0);
    return true;
} // parseTokenFrom


/**
 * @brief Calls parseTokenFrom until all tokens in the expression have been found.
 * @param outputScript[out] The script to execute in output to register this parameter as a dependency of the other in the expression
 * @return true on success, false otherwise
 * Note: this function can throw an exception if the expression is incorrect
 **/
static bool
extractAllOcurrences(const string& str,
                     const string& token,
                     bool returnsTuple,
                     int dimensionParamPos,
                     int viewParamPos,
                     int fromDim,
                     const string& fromViewName,
                     string *outputScript)
{
    std::size_t tokenStart = 0;
    bool couldFindToken;

    do {
        try {
            couldFindToken = parseTokenFrom(fromDim, fromViewName, dimensionParamPos, viewParamPos, returnsTuple, str, token, tokenStart == 0 ? 0 : tokenStart + 1, &tokenStart, outputScript);
        } catch (...) {
            return false;
        }
    } while (couldFindToken);

    return true;
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


string
KnobHelperPrivate::getReachablePythonAttributesForExpression(bool addTab,
                                                             DimIdx dimension,
                                                             ViewIdx /*view*/) const
{
    KnobHolderPtr h = holder.lock();

    assert(h);
    if (!h) {
        throw std::runtime_error("This parameter cannot have an expression");
    }


    NodePtr node;
    EffectInstancePtr effect = toEffectInstance(h);
    KnobTableItemPtr tableItem = toKnobTableItem(h);
    if (effect) {
        node = effect->getNode();
    } else if (tableItem) {
        KnobItemsTablePtr model = tableItem->getModel();
        if (model) {
            node = model->getNode();
        }
    }

    assert(node);
    if (!node) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodeCollectionPtr collection = node->getGroup();
    if (!collection) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodeGroupPtr isParentGrp = toNodeGroup(collection);
    string appID = node->getApp()->getAppIDString();
    string tabStr = addTab ? "    " : "";
    stringstream ss;
    if (appID != "app") {
        ss << tabStr << "app = " << appID << "\n";
    }


    //Define all nodes reachable through expressions in the scope


    // Define all nodes in the same group reachable by their bare script-name
    NodesList siblings = collection->getNodes();
    string collectionScriptName;
    if (isParentGrp) {
        collectionScriptName = isParentGrp->getNode()->getFullyQualifiedName();
    } else {
        collectionScriptName = appID;
    }
    for (NodesList::iterator it = siblings.begin(); it != siblings.end(); ++it) {
        string scriptName = (*it)->getScriptName_mt_safe();
        string fullName = appID + "." + (*it)->getFullyQualifiedName();

        // Do not fail the expression if the attribute do not exist to Python, use hasattr
        ss << tabStr << "if hasattr(";
        if (isParentGrp) {
            ss << appID << ".";
        }
        ss << collectionScriptName << ",\"" <<  scriptName << "\"):\n";

        ss << tabStr << "    " << scriptName << " = " << fullName << "\n";
    }

    // Define thisGroup
    if (isParentGrp) {
        ss << tabStr << "thisGroup = " << appID << "." << collectionScriptName << "\n";
    } else {
        ss << tabStr << "thisGroup = " << appID << "\n";
    }


    // Define thisNode
    ss << tabStr << "thisNode = " << node->getScriptName_mt_safe() <<  "\n";

    if (tableItem) {
        const string& tablePythonPrefix = tableItem->getModel()->getPythonPrefix();
        ss << tabStr << "thisItem = thisNode." << tablePythonPrefix << tableItem->getFullyQualifiedName() << "\n";
    }

    // Define thisParam
    if (tableItem) {
        ss << tabStr << "thisParam = thisItem." << common->name << "\n";
    } else {
        ss << tabStr << "thisParam = thisNode." << common->name << "\n";
    }

    // Define dimension variable
    ss << tabStr << "dimension = " << dimension << "\n";

    // Declare common functions
    ss << tabStr << "random = thisParam.random\n";
    ss << tabStr << "randomInt = thisParam.randomInt\n";
    ss << tabStr << "curve = thisParam.curve\n";


    return ss.str();
} // KnobHelperPrivate::declarePythonVariables


void
KnobHelperPrivate::parseListenersFromExpression(DimIdx dimension,
                                                ViewIdx view)
{
    assert( dimension >= 0 && dimension < (int)common->expressions.size() );

    // Extract pointers to knobs referred to by the expression
    // Our heuristic is quite simple: we replace in the python code all calls to:
    // - getValue
    // - getValueAtTime
    // - getDerivativeAtTime
    // - getIntegrateFromTimeToTime
    // - get
    // And replace them by addAsDependencyOf(thisParam) which will register the parameters as a dependency of this parameter

    string viewName = holder.lock()->getApp()->getProject()->getViewName(view);
    assert( !viewName.empty() );
    if ( viewName.empty() ) {
        viewName = "Main";
    }
    string expressionCopy;

    {
        QMutexLocker k(&common->expressionMutex);
        ExprPerViewMap::const_iterator foundView = common->expressions[dimension].find(view);
        if ( foundView == common->expressions[dimension].end() ) {
            return;
        }
        expressionCopy = foundView->second->expressionString;
    }

    // Extract parameters that call the following functions
    string listenersRegistrationScript;
    if  ( !extractAllOcurrences(expressionCopy, "getValue", false, 0, 1, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");

        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getValueAtTime", false, 1, 2, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");

        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getDerivativeAtTime", false, 1, 2, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");

        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getIntegrateFromTimeToTime", false, 2, 3, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");

        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "get", true, -1, 0, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");

        return;
    }

    // Declare all attributes that may be reached through this expression
    string declarations = getReachablePythonAttributesForExpression(false, dimension, view);

    // Make up the internal expression
    stringstream ss;
    ss << "frame=0\n";
    ss << "view=\"Main\"\n";
    ss << declarations << '\n';
    ss << expressionCopy << '\n';
    ss << listenersRegistrationScript;
    string script = ss.str();

    // Execute the expression: this will register listeners
    string error;
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, NULL);
    if ( !error.empty() ) {
        qDebug() << error.c_str();
    }

    // This should always succeed, otherwise there is a bug in this function.
    assert(ok);
    if (!ok) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): interpretPythonScript(" + script + ") failed!");
    }
} // KnobHelperPrivate::parseListenersFromExpression


string
KnobHelperPrivate::validatePythonExpression(const string& expression,
                                            DimIdx dimension,
                                            ViewIdx view,
                                            bool hasRetVariable,
                                            string* resultAsString) const
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    throw std::invalid_argument("NATRON_RUN_WITHOUT_PYTHON is defined");
#endif
    PythonGILLocker pgl;

    if ( expression.empty() ) {
        throw std::invalid_argument("Empty expression");;
    }

    string error;
#if 0
    // the following is OK to detect that there's no "return" in the expression,
    // but fails if the expression contains e.g. "thisGroup"
    // see https://github.com/NatronGitHub/Natron/issues/294

    // Try to execute the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    // with the error.
    {
        ExprRecursionLevel_RAII __recursionLevelIncrementer__(publicInterface);

        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0) ) {
            throw std::runtime_error(error);
        }
    }
#endif

    string exprCpy = expression;

    //if !hasRetVariable the expression is expected to be single-line
    if (!hasRetVariable) {
        std::size_t foundNewLine = expression.find("\n");
        if (foundNewLine != string::npos) {
            throw std::invalid_argument( KnobHelper::tr("Unexpected new line character \'\\n\'.").toStdString() );
        }
        //preprend the line with "ret = ..."
        string toInsert("    ret = ");
        exprCpy.insert(0, toInsert);
    } else {
        exprCpy.insert(0, "    ");
        std::size_t foundNewLine = exprCpy.find("\n");
        while (foundNewLine != string::npos) {
            exprCpy.insert(foundNewLine + 1, "    ");
            foundNewLine = exprCpy.find("\n", foundNewLine + 1);
        }
    }

    KnobHolderPtr holder = publicInterface->getHolder();
    if (!holder) {
        throw std::runtime_error( KnobHelper::tr("This parameter cannot have an expression.").toStdString() );
    }

    NodePtr node;
    EffectInstancePtr effect = toEffectInstance(holder);
    KnobTableItemPtr tableItem = toKnobTableItem(holder);
    if (effect) {
        node = effect->getNode();
    } else if (tableItem) {
        KnobItemsTablePtr model = tableItem->getModel();
        if (model) {
            node = model->getNode();
        }
    }
    if (!node) {
        throw std::runtime_error( KnobHelper::tr("Only parameters of a Node can have an expression.").toStdString() );
    }
    string appID = holder->getApp()->getAppIDString();
    string nodeName = node->getFullyQualifiedName();
    string nodeFullName = appID + "." + nodeName;
    string exprFuncPrefix = nodeFullName + "." + publicInterface->getName() + ".";

    // make-up the expression function
    string exprFuncName;
    {
        stringstream tmpSs;
        tmpSs << "expression" << dimension << "_" << view;
        exprFuncName = tmpSs.str();
    }

    exprCpy.append("\n    return ret\n");


    stringstream ss;
    ss << "def "  << exprFuncName << "(frame, view):\n";

    // First define the attributes that may be used by the expression
    ss << getReachablePythonAttributesForExpression(true, dimension, view);


    string script = ss.str();

    // Append the user expression
    script.append(exprCpy);

    // Set the expression function as an attribute of the knob itself
    script.append(exprFuncPrefix + exprFuncName + " = " + exprFuncName);

    // Try to execute the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    // with the error.
    string funcExecScript = "ret = " + exprFuncPrefix + exprFuncName;

    {
        ExprRecursionLevel_RAII __recursionLevelIncrementer__(publicInterface);

        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0) ) {
            throw std::runtime_error(error);
        }

        string viewName;
        if ( publicInterface->getHolder() && publicInterface->getHolder()->getApp() ) {
            viewName = publicInterface->getHolder()->getApp()->getProject()->getViewName(view);
        }
        if ( viewName.empty() ) {
            viewName = "Main";
        }

        stringstream ss;
        ss << funcExecScript << '(' << publicInterface->getCurrentRenderTime() << ", \"" <<  viewName << "\")\n";
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &error, 0) ) {
            throw std::runtime_error(error);
        }

        PyObject *ret = PyObject_GetAttrString(NATRON_PYTHON_NAMESPACE::getMainModule(), "ret"); //get our ret variable created above

        if ( !ret || PyErr_Occurred() ) {
#ifdef DEBUG
            PyErr_Print();
#endif
            throw std::runtime_error("return value must be assigned to the \"ret\" variable");
        }


        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(publicInterface);
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(publicInterface);
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(publicInterface);
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>(publicInterface);
        if (isDouble) {
            double r = isDouble->pyObjectToType<double>(ret);
            *resultAsString = QString::number(r).toStdString();
        } else if (isInt) {
            int r = isInt->pyObjectToType<int>(ret);
            *resultAsString = QString::number(r).toStdString();
        } else if (isBool) {
            bool r = isBool->pyObjectToType<bool>(ret);
            *resultAsString = r ? "True" : "False";
        } else {
            assert(isString);
            if ( PyUnicode_Check(ret) || PyString_Check(ret) ) {
                *resultAsString = isString->pyObjectToType<string>(ret);
            } else {
            }
        }
    }

    return funcExecScript;
} // validatePythonExpression


void
KnobHelper::validateExpression(const string& expression,
                               ExpressionLanguageEnum language,
                               DimIdx dimension,
                               ViewIdx view,
                               bool hasRetVariable,
                               string* resultAsString)
{
    switch (language) {
    case eExpressionLanguagePython:
        _imp->validatePythonExpression(expression, dimension, view, hasRetVariable, resultAsString);
        break;
    case eExpressionLanguageExprTk: {
        KnobExprExprTk ret;
        _imp->validateExprTkExpression(expression, dimension, view, resultAsString, &ret);
    }
    }
} // KnobHelper::validateExpression


NATRON_NAMESPACE_ANONYMOUS_ENTER

struct ExprToReApply
{
    ViewIdx view;
    DimIdx dimension;
    string expr;
    ExpressionLanguageEnum language;
    bool hasRet;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


bool
KnobHelper::checkInvalidLinks()
{
    int ndims = getNDimensions();


    // For a getValue call which will refresh error if needed
     _signalSlotHandler->s_mustRefreshKnobGui(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonTimeChanged);
    
    vector<ExprToReApply> exprToReapply;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        for (int i = 0; i < ndims; ++i) {
            for (PerViewInvalidLinkError::const_iterator it = _imp->common->linkErrors[i].begin(); it != _imp->common->linkErrors[i].end(); ++it) {
                if ( it->second.empty() ) {
                    continue;
                }

                KnobExprPtr& expr = _imp->common->expressions[i][it->first];
                if (expr && !expr->expressionString.empty()) {
                    exprToReapply.resize(exprToReapply.size() + 1);
                    ExprToReApply& data = exprToReapply.back();
                    data.view = it->first;
                    data.dimension = DimIdx(i);

                    data.expr = expr->expressionString;
                    data.language = expr->language;
                    KnobExprPython* isPythonExpr = dynamic_cast<KnobExprPython*>( expr.get() );
                    if (isPythonExpr) {
                        data.hasRet = isPythonExpr->hasRet;
                    } else {
                        data.hasRet = false;
                    }
                }
            }
        }
    }
    for (std::size_t i = 0; i < exprToReapply.size(); ++i) {
        setExpressionInternal(exprToReapply[i].dimension, exprToReapply[i].view, exprToReapply[i].expr, exprToReapply[i].language, exprToReapply[i].hasRet, false /*throwOnFailure*/);
    }
    {

        QMutexLocker k(&_imp->common->expressionMutex);
        for (int i = 0;i < ndims; ++i) {
            for (PerViewInvalidLinkError::const_iterator it = _imp->common->linkErrors[i].begin(); it != _imp->common->linkErrors[i].end(); ++it) {
                if (!it->second.empty()) {
                    return false;
                }
            }
        }
    }
    

    return true;
} // checkInvalidLinks


bool
KnobHelper::isLinkValid(DimIdx dimension,
                              ViewIdx view,
                              string* error) const
{
    if ( ( dimension >= getNDimensions() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::isExpressionValid(): Dimension out of range");
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        if (error) {
            PerViewInvalidLinkError::const_iterator foundView = _imp->common->linkErrors[dimension].find(view_i);
            if ( ( foundView != _imp->common->linkErrors[dimension].end() ) ) {
                *error = foundView->second;
                return error->empty();
            }
        }
    }

    return true;
}


void
KnobHelper::setLinkStatusInternal(DimIdx dimension,
                                         ViewIdx view,
                                         bool valid,
                                         const string& error)
{

    clearExpressionsResults(dimension, view);
    
    bool wasValid;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        std::string& linkError = _imp->common->linkErrors[dimension][view];
        wasValid = linkError.empty();
        linkError = error;
    }

    if (wasValid && !valid) {
        getHolder()->getApp()->addInvalidExpressionKnob( boost::const_pointer_cast<KnobI>( shared_from_this() ) );
        _signalSlotHandler->s_expressionChanged(dimension, view);
    } else if (!wasValid && valid) {
        bool haveOtherExprInvalid = false;
        {
            int ndims = getNDimensions();
            std::list<ViewIdx> views = getViewsList();
            QMutexLocker k(&_imp->common->expressionMutex);
            for (int i = 0; i < ndims; ++i) {
                if (i != dimension) {
                    for (PerViewInvalidLinkError::const_iterator it = _imp->common->linkErrors[i].begin(); it != _imp->common->linkErrors[i].end(); ++it) {
                        if ( (it->first != view) ) {
                            if ( !it->second.empty() ) {
                                haveOtherExprInvalid = true;
                                break;
                            }
                        }
                    }
                    if (haveOtherExprInvalid) {
                        break;
                    }
                }
            }
        }
        if (!haveOtherExprInvalid) {
            getHolder()->getApp()->removeInvalidExpressionKnob( shared_from_this() );
        }
        _signalSlotHandler->s_expressionChanged(dimension, view);
    }
} // setLinkStatusInternal


void
KnobHelper::setLinkStatus(DimSpec dimension,
                           ViewSetSpec view,
                           bool valid,
                           const string& error)
{
    if ( !getHolder() || !getHolder()->getApp() ) {
        return;
    }
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!dimension.isAll() && DimIdx(i) != dimension) {
                continue;
            }
            setLinkStatusInternal(DimIdx(i), *it, valid, error);
        }
    }
} // setLinkStatus


void
KnobHelper::setExpressionInternal(DimIdx dimension,
                                  ViewIdx view,
                                  const string& expression,
                                  ExpressionLanguageEnum language,
                                  bool hasRetVariable,
                                  bool failIfInvalid)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    assert( dimension >= 0 && dimension < getNDimensions() );


    ///Clear previous expr
    clearExpression( dimension, ViewSetSpec(view) );

    if ( expression.empty() ) {
        return;
    }


    string exprResult;
    KnobExprPtr expressionObj;
    boost::scoped_ptr<PythonGILLocker> pgl;
    std::string error;
    try {
        switch (language) {
        case eExpressionLanguagePython: {
            pgl.reset(new PythonGILLocker);
            shared_ptr<KnobExprPython> obj(new KnobExprPython);
            expressionObj = obj;
            expressionObj->expressionString = expression;
            expressionObj->language = language;
            obj->modifiedExpression = _imp->validatePythonExpression(expression, dimension, view, hasRetVariable, &exprResult);
            obj->hasRet = hasRetVariable;
        }
        break;
        case eExpressionLanguageExprTk: {
            shared_ptr<KnobExprExprTk> obj(new KnobExprExprTk);
            expressionObj = obj;
            expressionObj->expressionString = expression;
            expressionObj->language = language;
            _imp->validateExprTkExpression( expression, dimension, view, &exprResult, obj.get() );
        }
        break;
        }

    } catch (const std::exception &e) {
        error = e.what();
        if (failIfInvalid) {
            throw std::invalid_argument(error);
        }
    }

    {
        QMutexLocker k(&_imp->common->expressionMutex);
        _imp->common->expressions[dimension][view] = expressionObj;
    }

    setLinkStatus(dimension, view, error.empty(), error);


    if ( error.empty() ) {

        // Populate the listeners set so we can keep track of user links.
        // In python, the dependencies tracking is done by executing the expression itself unlike exprtk
        // where we have the dependencies list directly when compiling
        switch (language) {
            case eExpressionLanguagePython: {
                EXPR_RECURSION_LEVEL();
                _imp->parseListenersFromExpression(dimension, view);
            }
                break;
            case eExpressionLanguageExprTk: {
                KnobExprExprTk* obj = dynamic_cast<KnobExprExprTk*>( expressionObj.get() );
                KnobIPtr thisShared = shared_from_this();
                for (std::map<string, KnobDimViewKey>::const_iterator it = obj->knobDependencies.begin(); it != obj->knobDependencies.end(); ++it) {
                    it->second.knob.lock()->addListener(dimension, it->second.dimension, view, it->second.view, thisShared, eExpressionLanguageExprTk);
                }

                for (std::map<std::string, EffectFunctionDependency>::const_iterator it = obj->effectDependencies.begin(); it != obj->effectDependencies.end(); ++it) {
                    EffectInstancePtr effect = it->second.effect.lock();
                    if (!effect) {
                        continue;
                    }

                    effect->addHashListener(thisShared);
                }
            }
                break;
        }
    }



    // Notify the expr. has changed
    expressionChanged(dimension, view);
} // setExpressionInternal


void
KnobHelper::setExpressionCommon(DimSpec dimension,
                                ViewSetSpec view,
                                const string& expression,
                                ExpressionLanguageEnum language,
                                bool hasRetVariable,
                                bool failIfInvalid)
{
    // setExpressionInternal may call evaluateValueChange if it calls clearExpression, hence bracket changes
    ScopedChanges_RAII changes(this);

    std::list<ViewIdx> views = getViewsList();
    if ( dimension.isAll() ) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if ( view.isAll() ) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    setExpressionInternal(DimIdx(i), *it, expression, language, hasRetVariable, failIfInvalid);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView( ViewIdx( view.value() ) );
                setExpressionInternal(DimIdx(i), view_i, expression, language, hasRetVariable, failIfInvalid);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setExpressionCommon(): Dimension out of range");
        }
        if ( view.isAll() ) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                setExpressionInternal(DimIdx(dimension), *it, expression, language, hasRetVariable, failIfInvalid);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView( ViewIdx( view.value() ) );
            setExpressionInternal(DimIdx(dimension), view_i, expression, language, hasRetVariable, failIfInvalid);
        }
    }

    evaluateValueChange(dimension, getHolder()->getCurrentRenderTime(), view, eValueChangedReasonUserEdited);
} // setExpressionCommon


void
KnobHelper::replaceNodeNameInExpressionInternal(DimIdx dimension,
                                                ViewIdx view,
                                                const string& oldName,
                                                const string& newName)
{
    EffectInstancePtr isEffect = toEffectInstance( getHolder() );

    if (!isEffect) {
        return;
    }

    string hasExpr = getExpression(dimension, view);
    if ( hasExpr.empty() ) {
        return;
    }
    bool hasRetVar = isExpressionUsingRetVariable(view, dimension);
    ExpressionLanguageEnum lang = getExpressionLanguage(view, dimension);
    try {
        //Change in expressions the script-name
        QString estr = QString::fromUtf8( hasExpr.c_str() );
        estr.replace( QString::fromUtf8( oldName.c_str() ), QString::fromUtf8( newName.c_str() ) );
        hasExpr = estr.toStdString();
        setExpression(dimension, ViewSetSpec(view), hasExpr, lang, hasRetVar, false);
    } catch (...) {
    }
} // replaceNodeNameInExpressionInternal


void
KnobHelper::replaceNodeNameInExpression(DimSpec dimension,
                                        ViewSetSpec view,
                                        const string& oldName,
                                        const string& newName)
{
    if (oldName == newName) {
        return;
    }
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return;
    }
    ScopedChanges_RAII changes( holder.get() );

    std::list<ViewIdx> views = getViewsList();
    if ( dimension.isAll() ) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if ( view.isAll() ) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    replaceNodeNameInExpressionInternal(DimIdx(i), *it, oldName, newName);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView( ViewIdx( view.value() ) );
                replaceNodeNameInExpressionInternal(DimIdx(i), view_i, oldName, newName);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::replaceNodeNameInExpression(): Dimension out of range");
        }
        if ( view.isAll() ) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                replaceNodeNameInExpressionInternal(DimIdx(dimension), *it, oldName, newName);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView( ViewIdx( view.value() ) );
            replaceNodeNameInExpressionInternal(DimIdx(dimension), view_i, oldName, newName);
        }
    }
} // replaceNodeNameInExpression


ExpressionLanguageEnum
KnobHelper::getExpressionLanguage(Natron::ViewIdx view,
                                  Natron::DimIdx dimension) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->expressions.size() ) ) {
        throw std::invalid_argument("KnobHelper::getExpressionLanguage(): Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if ( foundView == _imp->common->expressions[dimension].end() ) {
        return eExpressionLanguageExprTk;
    }
    if (!foundView->second) {
        return eExpressionLanguageExprTk;
    }

    return foundView->second->language;
}


bool
KnobHelper::isExpressionUsingRetVariable(ViewIdx view,
                                         DimIdx dimension) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->expressions.size() ) ) {
        throw std::invalid_argument("KnobHelper::isExpressionUsingRetVariable(): Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if ( foundView == _imp->common->expressions[dimension].end() ) {
        return false;
    }
    if (!foundView->second) {
        return false;
    }
    KnobExprPython* isPythonExpr = dynamic_cast<KnobExprPython*>( foundView->second.get() );
    if (!isPythonExpr) {
        return false;
    }

    return isPythonExpr->hasRet;
}


bool
KnobHelper::getExpressionDependencies(DimIdx dimension,
                                      ViewIdx view,
                                      KnobDimViewKeySet& dependencies) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->expressions.size() ) ) {
        throw std::invalid_argument("KnobHelper::getExpressionDependencies(): Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if ( ( foundView == _imp->common->expressions[dimension].end() ) || !foundView->second ) {
        return false;
    }
    KnobExprPython* isPythonExpr = dynamic_cast<KnobExprPython*>( foundView->second.get() );
    KnobExprExprTk* isExprtkExpr = dynamic_cast<KnobExprExprTk*>( foundView->second.get() );
    assert(isPythonExpr || isExprtkExpr);
    if (isPythonExpr) {
        dependencies = isPythonExpr->dependencies;
    } else if (isExprtkExpr) {
        for (std::map<string, KnobDimViewKey>::const_iterator it = isExprtkExpr->knobDependencies.begin(); it != isExprtkExpr->knobDependencies.end(); ++it) {
            dependencies.insert(it->second);
        }
    } else {
        return false;
    }

    return true;
}


bool
KnobHelper::clearExpressionInternal(DimIdx dimension,
                                    ViewIdx view)
{
    // Do not take the Python GIL:
    // We do not execute Python code here, and it may crash Natron
    // when Natron is quitting, because Python was already
    // torn down. See https://travis-ci.org/NatronGitHub/Natron/jobs/549547215
    /// PythonGILLocker pgl;
    bool hadExpression = false;
    KnobIPtr thisShared = shared_from_this();
    KnobDimViewKeySet dependencies;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        ExprPerViewMap::iterator foundView = _imp->common->expressions[dimension].find(view);
        if ( ( foundView != _imp->common->expressions[dimension].end() ) && foundView->second ) {
            hadExpression = true;
            KnobExprPython* isPythonExpr = dynamic_cast<KnobExprPython*>( foundView->second.get() );
            KnobExprExprTk* isExprtkExpr = dynamic_cast<KnobExprExprTk*>( foundView->second.get() );
            assert(isPythonExpr || isExprtkExpr);
            if (isPythonExpr) {
                dependencies = isPythonExpr->dependencies;
            } else if (isExprtkExpr) {
                for (std::map<string, KnobDimViewKey>::const_iterator it = isExprtkExpr->knobDependencies.begin(); it != isExprtkExpr->knobDependencies.end(); ++it) {
                    dependencies.insert(it->second);
                }

                // Remove this knob from the listeners list of the effect hash
                for (std::map<std::string, EffectFunctionDependency>::const_iterator it = isExprtkExpr->effectDependencies.begin(); it != isExprtkExpr->effectDependencies.end(); ++it) {
                    EffectInstancePtr effect = it->second.effect.lock();
                    if (!effect) {
                        continue;
                    }
                    effect->removeListener(thisShared);
                }
            }
            foundView->second.reset();
        }
    }
    {
        // Notify all dependencies of the expression that they no longer listen to this knob
        KnobDimViewKey listenerToRemoveKey(thisShared, dimension, view);
        for (KnobDimViewKeySet::iterator it = dependencies.begin();
             it != dependencies.end(); ++it) {
            KnobIPtr otherKnob = it->knob.lock();
            KnobHelper* other = dynamic_cast<KnobHelper*>( otherKnob.get() );
            if (!other) {
                continue;
            }

            // Remove from the other knob's listeners list
            {
                QMutexLocker otherMastersLocker(&other->_imp->common->expressionMutex);
                KnobDimViewKeySet& otherListeners = other->_imp->common->listeners[it->dimension][it->view];
                KnobDimViewKeySet::iterator foundListener = otherListeners.find(listenerToRemoveKey);
                if ( foundListener != otherListeners.end() ) {
                    otherListeners.erase(foundListener);
                }
            }

            // Remove from the hash listeners
            //other->removeListener(thisShared);
        }
    }

    if (hadExpression) {
        expressionChanged(dimension, view);
    }

    return hadExpression;
} // clearExpressionInternal


void
KnobHelper::clearExpression(DimSpec dimension,
                            ViewSetSpec view)
{
    bool didSomething = false;

    std::list<ViewIdx> views = getViewsList();
    if ( dimension.isAll() ) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if ( view.isAll() ) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    didSomething |= clearExpressionInternal(DimIdx(i), *it);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView( ViewIdx( view.value() ) );
                didSomething |= clearExpressionInternal(DimIdx(i), view_i);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::clearExpression(): Dimension out of range");
        }
        if ( view.isAll() ) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                didSomething |= clearExpressionInternal(DimIdx(dimension), *it);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView( ViewIdx( view.value() ) );
            didSomething |= clearExpressionInternal(DimIdx(dimension), view_i);
        }
    }


    if (didSomething) {
        if ( getHolder() ) {
            evaluateValueChange(dimension, getHolder()->getTimelineCurrentTime(), view, eValueChangedReasonUserEdited);
        }
    }
} // KnobHelper::clearExpression


void
KnobHelper::expressionChanged(DimIdx dimension,
                              ViewIdx view)
{
    _signalSlotHandler->s_expressionChanged(dimension, view);

    computeHasModifications();
}


NATRON_NAMESPACE_ANONYMOUS_ENTER

bool
catchErrors(PyObject* mainModule,
            string* error)
{
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        ///Gui session, do stdout, stderr redirection
        if ( PyObject_HasAttrString(mainModule, "catchErr") ) {
            PyObject* errCatcher = PyObject_GetAttrString(mainModule, "catchErr"); //get our catchOutErr created above, new ref
            PyObject *errorObj = 0;
            if (errCatcher) {
                errorObj = PyObject_GetAttrString(errCatcher, "value"); //get the  stderr from our catchErr object, new ref
                assert(errorObj);
                *error = NATRON_PYTHON_NAMESPACE::PyStringToStdString(errorObj);
                PyObject* unicode = PyUnicode_FromString("");
                PyObject_SetAttrString(errCatcher, "value", unicode);
                Py_DECREF(errorObj);
                Py_DECREF(errCatcher);
            }
        }

        return false;
    }

    return true;
} // catchErrors

NATRON_NAMESPACE_ANONYMOUS_EXIT


KnobHelper::ExpressionReturnValueTypeEnum
KnobHelper::evaluateExpression(const string& expr,
                               ExpressionLanguageEnum language,
                               double* retIsScalar,
                               string* retIsString,
                               string* error)
{
    KnobHelper::ExpressionReturnValueTypeEnum retCode = eExpressionReturnValueTypeError;
    switch (language) {
    case eExpressionLanguagePython: {
        PythonGILLocker pgl;
        PyObject *ret;
        bool exprOk = KnobHelper::executePythonExpression(expr, &ret, error);
        if (!exprOk) {
            break;
        }
        if ( PyFloat_Check(ret) ) {
            *retIsScalar =  (double)PyFloat_AsDouble(ret);
            retCode =  eExpressionReturnValueTypeScalar;
        } else if ( PyInt_Check(ret) ) {
            *retIsScalar = (double)PyInt_AsLong(ret);
            retCode =  eExpressionReturnValueTypeScalar;
        } else if ( PyLong_Check(ret) ) {
            *retIsScalar = (double)PyLong_AsLong(ret);
            retCode =  eExpressionReturnValueTypeScalar;
        } else if (PyObject_IsTrue(ret) == 1) {
            *retIsScalar = 1;
            retCode = eExpressionReturnValueTypeScalar;
        } else if ( PyUnicode_Check(ret) ) {
            PyObject* utf8pyobj = PyUnicode_AsUTF8String(ret);     // newRef
            if (utf8pyobj) {
                char* cstr = PyBytes_AS_STRING(utf8pyobj);     // Borrowed pointer
                retIsString->append(cstr);
                Py_DECREF(utf8pyobj);
            }
            retCode = eExpressionReturnValueTypeString;
        } else {
            *retIsScalar = 0;
            retCode = eExpressionReturnValueTypeScalar;
        }

        Py_DECREF(ret);     //< new ref
    }
    break;
    case eExpressionLanguageExprTk: {
        retCode = KnobHelper::executeExprTkExpression(expr, retIsScalar, retIsString, error);
    }
    break;
    }

    return retCode;
}


bool
KnobHelper::executePythonExpression(TimeValue time,
                                    ViewIdx view,
                                    DimIdx dimension,
                                    PyObject** ret,
                                    string* error) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->expressions.size() ) ) {
        throw std::invalid_argument("KnobHelper::executeExpression(): Dimension out of range");
    }

    EffectInstancePtr effect = toEffectInstance( getHolder() );
    if (effect) {
        appPTR->setLastPythonAPICaller_TLS(effect);
    }


    string expr;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view);
        if ( ( foundView == _imp->common->expressions[dimension].end() ) || !foundView->second ) {
            return false;
        }
        KnobExprPython* isPythonExpr = dynamic_cast<KnobExprPython*>( foundView->second.get() );
        assert(isPythonExpr);
        expr = isPythonExpr->modifiedExpression;
    }
    stringstream ss;
    string viewName;
    if ( getHolder() && getHolder()->getApp() ) {
        viewName = getHolder()->getApp()->getProject()->getViewName(view);
    }
    if ( viewName.empty() ) {
        viewName = "Main";
    }

    ss << expr << '(' << time << ", \"" << viewName << "\")\n";
    string script = ss.str();

    ///Reset the random state to reproduce the sequence
    randomSeed( time, hashFunction(dimension) );

    return executePythonExpression(ss.str(), ret, error);
} // executeExpression


bool
KnobHelper::executePythonExpression(const string& expr,
                                    PyObject** ret,
                                    string* error)
{
    //returns a new ref, this function's documentation is not clear onto what it returns...
    //https://docs.python.org/2/c-api/veryhigh.html
    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
    PyObject* globalDict = PyModule_GetDict(mainModule);
    PyObject* v = PyRun_String(expr.c_str(), Py_file_input, globalDict, 0);

    Py_XDECREF(v);

    *ret = 0;

    if ( !catchErrors(mainModule, error) ) {
        return false;
    }
    *ret = PyObject_GetAttrString(mainModule, "ret"); //get our ret variable created above
    if (!*ret) {
        // Do not forget to empty the error stream using catchError, even if we know the error,
        // for subsequent expression evaluations.
        if ( catchErrors(mainModule, error) ) {
            *error = "Missing 'ret' attribute";
        }

        return false;
    }
    if ( !catchErrors(mainModule, error) ) {
        return false;
    }

    return true;
}


string
KnobHelper::getExpression(DimIdx dimension,
                          ViewIdx view) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->expressions.size() ) ) {
        throw std::invalid_argument("Knob::getExpression: Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if ( ( foundView == _imp->common->expressions[dimension].end() ) || !foundView->second ) {
        return string();
    }

    return foundView->second->expressionString;
}

bool
KnobHelper::hasExpression(DimIdx dimension, ViewIdx view) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->common->expressions.size() ) ) {
        throw std::invalid_argument("Knob::hasExpression: Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if ( ( foundView == _imp->common->expressions[dimension].end() ) || !foundView->second ) {
        return false;
    }

    return !foundView->second->expressionString.empty();
}

NATRON_NAMESPACE_EXIT
