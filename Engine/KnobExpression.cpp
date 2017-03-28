/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/KnobItemsTable.h"
#include "Global/StrUtils.h"

NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

/**
 * @brief Given the string str, returns the position of the character "closingChar" corresponding to the "openingChar" at the given openingParenthesisPos
 * For example if the str is "((lala)+toto)" and we want to get the character matching the first '(', this function will return the last parenthesis in the string
 * @return The position of the matching closing character or std::string::npos if not found
 **/
static std::size_t
getMatchingParenthesisPosition(std::size_t openingParenthesisPos,
                               char openingChar,
                               char closingChar,
                               const std::string& str)
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
        return std::string::npos;
    }

    return i;
} // getMatchingParenthesisPosition

/**
 * @brief Given a string str, assume that the content of the string between startParenthesis and endParenthesis 
 * is a well-formed Python signature with comma separated arguments list. The list of arguments is stored in params.
 **/
static void
extractParameters(std::size_t startParenthesis,
                  std::size_t endParenthesis,
                  const std::string& str,
                  std::vector<std::string>* params)
{
    std::size_t i = startParenthesis + 1;
    int insideParenthesis = 0;

    while (i < endParenthesis || insideParenthesis < 0) {
        std::string curParam;
        if (str.at(i) == '(') {
            ++insideParenthesis;
        } else if (str.at(i) == ')') {
            --insideParenthesis;
        }
        while ( i < str.size() && (str.at(i) != ',' || insideParenthesis > 0) ) {
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
        params->push_back(curParam);
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
               const std::string& fromViewName,
               int dimensionParamPos,
               int viewParamPos,
               bool returnsTuple,
               const std::string& str,
               const std::string& token,
               std::size_t inputPos,
               std::size_t *tokenStart,
               std::string* output)
{
    std::size_t pos;

    // Find the opening parenthesis after the token start
    bool foundMatchingToken = false;
    while (!foundMatchingToken) {
        pos = str.find(token, inputPos);
        if (pos == std::string::npos) {
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
    if (endingParenthesis == std::string::npos) {
        throw std::invalid_argument("Invalid expr");
    }

    // Extract parameters in between the 2 parenthesis
    std::vector<std::string> params;
    extractParameters(pos, endingParenthesis, str, &params);


    bool gotViewParam = viewParamPos != -1 && (viewParamPos < (int)params.size() && !params.empty());
    bool gotDimensionParam = dimensionParamPos != -1 && (dimensionParamPos < (int)params.size() && !params.empty());


    if (!returnsTuple) {
        // Before Natron 2.2 the View parameter of the get(view) function did not exist, hence loading
        // an old expression may use the old get() function without view. If we do not find any view
        // parameter, assume the view is "Main" by default.
        if (!gotViewParam && viewParamPos != -1) {
            std::vector<std::string>::iterator it = params.begin();
            if (viewParamPos >= (int)params.size()) {
                it = params.end();
            } else {
                std::advance(it, viewParamPos);
            }
            params.insert(it, "Main");
        }
        if (!gotDimensionParam && dimensionParamPos != -1) {
            std::vector<std::string>::iterator it = params.begin();
            if (dimensionParamPos >= (int)params.size()) {
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
            if (endingBracket == std::string::npos) {
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
        if (params.size() == 1 && viewParamPos >= 0) {
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

    std::string toInsert;
    {
        std::stringstream ss;
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

    std::locale loc;
    //Find the start of the symbol
    int i = (int)*tokenStart - 2;
    int nClosingParenthesisMet = 0;
    while (i >= 0) {
        if (str[i] == ')') {
            ++nClosingParenthesisMet;
        }
        if ( std::isspace(str[i], loc) ||
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
    std::string varName = str.substr(i, *tokenStart - 1 - i);
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
extractAllOcurrences(const std::string& str,
                     const std::string& token,
                     bool returnsTuple,
                     int dimensionParamPos,
                     int viewParamPos,
                     int fromDim,
                     const std::string& fromViewName,
                     std::string *outputScript)
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

std::string
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
    std::string appID = node->getApp()->getAppIDString();
    std::string tabStr = addTab ? "    " : "";
    std::stringstream ss;
    if (appID != "app") {
        ss << tabStr << "app = " << appID << "\n";
    }


    //Define all nodes reachable through expressions in the scope


    // Define all nodes in the same group reachable by their bare script-name
    NodesList siblings = collection->getNodes();
    std::string collectionScriptName;
    if (isParentGrp) {
        collectionScriptName = isParentGrp->getNode()->getFullyQualifiedName();
    } else {
        collectionScriptName = appID;
    }
    for (NodesList::iterator it = siblings.begin(); it != siblings.end(); ++it) {
        if ((*it)->isActivated()) {
            std::string scriptName = (*it)->getScriptName_mt_safe();
            std::string fullName = appID + "." + (*it)->getFullyQualifiedName();

            // Do not fail the expression if the attribute do not exist to Python, use hasattr
            ss << tabStr << "if hasattr(";
            if (isParentGrp) {
                ss << appID << ".";
            }
            ss << collectionScriptName << ",\"" <<  scriptName << "\"):\n";

            ss << tabStr << "    " << scriptName << " = " << fullName << "\n";
        }
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
        const std::string& tablePythonPrefix = tableItem->getModel()->getPythonPrefix();
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
KnobHelperPrivate::parseListenersFromExpression(DimIdx dimension, ViewIdx view)
{
    assert(dimension >= 0 && dimension < (int)common->expressions.size());

    // Extract pointers to knobs referred to by the expression
    // Our heuristic is quite simple: we replace in the python code all calls to:
    // - getValue
    // - getValueAtTime
    // - getDerivativeAtTime
    // - getIntegrateFromTimeToTime
    // - get
    // And replace them by addAsDependencyOf(thisParam) which will register the parameters as a dependency of this parameter

    std::string viewName = holder.lock()->getApp()->getProject()->getViewName(view);
    assert(!viewName.empty());
    if (viewName.empty()) {
        viewName = "Main";
    }
    std::string expressionCopy;

    {
        QMutexLocker k(&common->expressionMutex);
        ExprPerViewMap::const_iterator foundView = common->expressions[dimension].find(view);
        if (foundView == common->expressions[dimension].end()) {
            return;
        }
        expressionCopy = foundView->second->expressionString;
    }

    // Extract parameters that call the following functions
    std::string listenersRegistrationScript;
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
    std::string declarations = getReachablePythonAttributesForExpression(false, dimension, view);

    // Make up the internal expression
    std::stringstream ss;
    ss << "frame=0\n";
    ss << "view=\"Main\"\n";
    ss << declarations << '\n';
    ss << expressionCopy << '\n';
    ss << listenersRegistrationScript;
    std::string script = ss.str();

    // Execute the expression: this will register listeners
    std::string error;
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

std::string
KnobHelperPrivate::validatePythonExpression(const std::string& expression, DimIdx dimension, ViewIdx view, bool hasRetVariable, std::string* resultAsString) const
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    throw std::invalid_argument("NATRON_RUN_WITHOUT_PYTHON is defined");
#endif
    PythonGILLocker pgl;

    if ( expression.empty() ) {
        throw std::invalid_argument("Empty expression");;
    }


    std::string exprCpy = expression;

    //if !hasRetVariable the expression is expected to be single-line
    if (!hasRetVariable) {
        std::size_t foundNewLine = expression.find("\n");
        if (foundNewLine != std::string::npos) {
            throw std::invalid_argument(publicInterface->tr("unexpected new line character \'\\n\'").toStdString());
        }
        //preprend the line with "ret = ..."
        std::string toInsert("    ret = ");
        exprCpy.insert(0, toInsert);
    } else {
        exprCpy.insert(0, "    ");
        std::size_t foundNewLine = exprCpy.find("\n");
        while (foundNewLine != std::string::npos) {
            exprCpy.insert(foundNewLine + 1, "    ");
            foundNewLine = exprCpy.find("\n", foundNewLine + 1);
        }
    }

    KnobHolderPtr holder = publicInterface->getHolder();
    if (!holder) {
        throw std::runtime_error(publicInterface->tr("This parameter cannot have an expression").toStdString());
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
        throw std::runtime_error(publicInterface->tr("Only parameters of a Node can have an expression").toStdString());
    }
    std::string appID = holder->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string exprFuncPrefix = nodeFullName + "." + publicInterface->getName() + ".";

    // make-up the expression function
    std::string exprFuncName;
    {
        std::stringstream tmpSs;
        tmpSs << "expression" << dimension << "_" << view;
        exprFuncName = tmpSs.str();
    }

    exprCpy.append("\n    return ret\n");


    std::stringstream ss;
    ss << "def "  << exprFuncName << "(frame, view):\n";

    // First define the attributes that may be used by the expression
    ss << getReachablePythonAttributesForExpression(true, dimension, view);


    std::string script = ss.str();

    // Append the user expression
    script.append(exprCpy);

    // Set the expression function as an attribute of the knob itself
    script.append(exprFuncPrefix + exprFuncName + " = " + exprFuncName);

    // Try to execute the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    // with the error.
    std::string error;
    std::string funcExecScript = "ret = " + exprFuncPrefix + exprFuncName;

    {
        ExprRecursionLevel_RAII __recursionLevelIncrementer__(publicInterface);

        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0) ) {
            throw std::runtime_error(error);
        }

        std::string viewName;
        if (publicInterface->getHolder() && publicInterface->getHolder()->getApp()) {
            viewName = publicInterface->getHolder()->getApp()->getProject()->getViewName(view);
        }
        if (viewName.empty()) {
            viewName = "Main";
        }

        std::stringstream ss;
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
            if (PyUnicode_Check(ret) || PyString_Check(ret)) {
                *resultAsString = isString->pyObjectToType<std::string>(ret);
            } else {
            }
        }
    }
    
    
    return funcExecScript;
} // validatePythonExpression

static bool isDimensionIndex(const std::string& str, int* index)
{
    if (str == "r" || str == "x" || str == "0") {
        *index = 0;
        return true;
    }
    if (str == "g" || str == "y" || str == "1") {
        *index = 1;
        return true;
    }
    if (str == "b" || str == "z" || str == "2") {
        *index = 2;
        return true;
    }
    if (str == "a" || str == "w" || str == "3") {
        *index = 3;
        return true;
    }
    return false;
}


class SymbolResolver
{
    KnobI* _knob;
    DimIdx _dimension;
    ViewIdx _view;
    std::string _symbol;

public:

    enum ResultTypeEnum
    {
        eResultTypeInvalid,
        eResultTypeKnobValue,
        eResultTypeKnobChoiceOption,
        eResultTypeEffectRoD,
        eResultTypeObjectName,
    };

    ResultTypeEnum _resultType;
    std::string _objectName;
    std::string _error;
    bool _testingEnabled;

    // If result is eResultTypeEffectRoD, this is the effect on which to retrieve the property
    EffectInstancePtr _effectProperty;

    // If the result is eResultTypeKnobValue, this is the knob on which to retrieve the value
    KnobIPtr _targetKnob;
    ViewIdx _targetView;
    DimIdx _targetDimension;

    SymbolResolver(KnobI* knob, DimIdx dimension, ViewIdx view, const std::string& symbol)
    : _knob(knob)
    , _dimension(dimension)
    , _view(view)
    , _symbol(symbol)
    , _resultType(eResultTypeInvalid)
    , _error()
    , _testingEnabled(false)
    {
        resolve();
    }

private:

    void resolve()
    {

        // Split the variable with dots
        std::vector<std::string> splits = StrUtils::split(_symbol, '.');

        EffectInstancePtr currentNode = getThisNode();
        KnobHolderPtr currentHolder = _knob->getHolder();
        KnobTableItemPtr currentTableItem = getThisTableItem();
        NodeCollectionPtr currentGroup = getThisGroup();
        KnobIPtr currentKnob;
        DimIdx currentDimension = _dimension;
        ViewIdx currentView = _view;
        assert(currentNode && currentGroup);

        // If "exists" is suffixed, we never fail but instead return 0 or 1
        _testingEnabled = !splits.empty() && splits.back() == "exists";
        if (_testingEnabled) {
            splits.resize(splits.size() - 1);
        }
        for (std::size_t i = 0; i < splits.size(); ++i) {
            bool isLastToken = (i == splits.size() - 1);
            const std::string& token = splits[i];


            {
                NodeCollectionPtr curGroupOut;
                if (checkForGroup(token, &curGroupOut)) {
                    // If we caught a group, check if it is a node too
                    currentGroup = curGroupOut;
                    currentNode = toNodeGroup(currentGroup);
                    currentHolder = currentNode;
                    currentTableItem.reset();
                    currentKnob.reset();
                    if (isLastToken) {
                        if (_testingEnabled) {
                            // If testing is enabled, set the result to be valid
                            _resultType = eResultTypeKnobValue;
                        } else {
                            std::stringstream ss;
                            ss << _symbol << ": a variable can only be bound to a value";
                            _error = ss.str();
                        }
                        return;
                    }
                    continue;
                }
            }

            {
                EffectInstancePtr curNodeOut;
                if (checkForNode(token, currentGroup, currentNode, &curNodeOut)) {
                    // If we caught a node, check if it is a group too
                    currentNode = curNodeOut;
                    currentGroup = toNodeGroup(currentNode);
                    currentHolder = currentNode;
                    currentTableItem.reset();
                    currentKnob.reset();
                    if (isLastToken) {
                        if (_testingEnabled) {
                            // If testing is enabled, set the result to be valid
                            _resultType = eResultTypeKnobValue;
                        } else {
                            std::stringstream ss;
                            ss << _symbol << ": a variable can only be bound to a value";
                            _error = ss.str();
                        }
                        return;
                    }
                    continue;
                }
            }

            {
                KnobTableItemPtr curTableItemOut;
                if (checkForTableItem(token, currentHolder, &curTableItemOut)) {
                    // If we caught a node, check if it is a group too
                    currentTableItem = curTableItemOut;
                    currentHolder = currentTableItem;
                    currentNode.reset();
                    currentGroup.reset();
                    currentKnob.reset();
                    if (isLastToken) {
                        if (_testingEnabled) {
                            // If testing is enabled, set the result to be valid
                            _resultType = eResultTypeKnobValue;
                        } else {
                            std::stringstream ss;
                            ss << _symbol << ": a variable can only be bound to a value";
                            _error = ss.str();
                        }
                        return;
                    }
                    continue;
                }
            }

            {
                if (checkForProject(token)) {
                    ProjectPtr proj = _knob->getHolder()->getApp()->getProject();
                    currentHolder = proj;
                    currentTableItem.reset();
                    currentNode.reset();
                    currentGroup = proj;
                    currentKnob.reset();
                    if (isLastToken) {
                        if (_testingEnabled) {
                            // If testing is enabled, set the result to be valid
                            _resultType = eResultTypeKnobValue;
                        } else {
                            std::stringstream ss;
                            ss << _symbol << ": a variable can only be bound to a value";
                            _error = ss.str();
                        }
                        return;
                    }
                    continue;
                }
            }

            {
                KnobIPtr curKnobOut;
                if (checkForKnob(token, currentHolder, &curKnobOut)) {
                    // If we caught a node, check if it is a group too
                    currentKnob = curKnobOut;
                    currentHolder.reset();
                    currentTableItem.reset();
                    currentNode.reset();
                    currentGroup.reset();
                    if (isLastToken) {
                        if (currentKnob->getNDimensions() > 1) {
                            if (_testingEnabled) {
                                // If testing is enabled, set the result to be valid
                                _resultType = eResultTypeKnobValue;
                            } else {
                                std::stringstream ss;
                                ss << _symbol << ": this parameter has multiple dimension, please specify one";
                                _error = ss.str();
                            }
                            return ;
                        } else {

                            // single dimension, return the value of the knob at dimension 0
                            _targetView = _view;
                            _targetKnob = currentKnob;
                            _targetDimension = DimIdx(0);
                            _resultType = eResultTypeKnobValue;
                            return;
                        }
                    }
                    continue;
                }
            }


            if (currentKnob && checkForDimension(token, currentKnob, &currentDimension)) {
                // If we caught a node, check if it is a group too
                currentHolder.reset();
                currentTableItem.reset();
                currentNode.reset();
                currentGroup.reset();
                if (!isLastToken) {
                    std::stringstream ss;
                    ss << _symbol << ": a variable can only be bound to a value";
                    _error = ss.str();
                    return ;
                }
                _targetKnob = currentKnob;
                _targetDimension = currentDimension;
                _targetView = currentView;
                _resultType = eResultTypeKnobValue;

                return;
            }

            if (currentKnob && token == "option" && dynamic_cast<KnobChoice*>(currentKnob.get())) {
                // For a KnobChoice, we can get the option string directly
                if (!isLastToken) {
                    std::stringstream ss;
                    ss << _symbol << ": a variable can only be bound to a value";
                    _error = ss.str();
                    return ;
                }
                _targetKnob = currentKnob;
                _targetDimension = currentDimension;
                _targetView = currentView;
                _resultType = eResultTypeKnobChoiceOption;
                return;
            }

            // Check is the user wants the region of definition of an effect
            if (currentNode) {


                if (token == "rod") {
                    // The rod of the effect
                    _resultType = eResultTypeEffectRoD;
                    _effectProperty = currentNode;
                    return;
                }
            }

            // Now check if we want the name of an object

            if (token == "name") {
                bool gotName = false;
                if (currentKnob) {
                    gotName = true;
                    _objectName = currentKnob->getName();
                } else if (currentTableItem) {
                    gotName = true;
                    _objectName = currentTableItem->getScriptName_mt_safe();
                } else if (currentNode) {
                    gotName = true;
                    _objectName = currentNode->getScriptName_mt_safe();
                }

                if (gotName) {
                    _resultType = eResultTypeObjectName;
                    return;
                }
            }

            _error = "Undefined symbol " + _symbol;
            _resultType = eResultTypeInvalid;
            return;
            
        } // for each splits

    } // resolve

    NodeCollectionPtr getThisGroup() const
    {
        EffectInstancePtr thisNode = getThisNode();
        if (!thisNode) {
            return NodeCollectionPtr();
        }
        return thisNode->getNode()->getGroup();
    }

    bool checkForGroup(const std::string& str, NodeCollectionPtr* retIsGroup) const
    {
        retIsGroup->reset();
        if (str == "thisGroup") {
            *retIsGroup = getThisGroup();
            return true;

        }
        return false;
    }

    EffectInstancePtr getThisNode() const
    {
        KnobHolderPtr holder = _knob->getHolder();
        if (!holder) {
            return EffectInstancePtr();
        }
        EffectInstancePtr effect = toEffectInstance(_knob->getHolder());
        KnobTableItemPtr tableItem = toKnobTableItem(_knob->getHolder());
        if (tableItem) {
            effect = tableItem->getModel()->getNode()->getEffectInstance();
        }
        return effect;
    }

    bool checkForNode(const std::string& str, const NodeCollectionPtr& callerGroup, const EffectInstancePtr& callerIsNode, EffectInstancePtr* retIsNode) const
    {
        retIsNode->reset();
        if (str == "thisNode") {
            *retIsNode = getThisNode();
            return true;
        } else if (callerGroup) {
            // Loop over all nodes within the caller group
            NodePtr foundNode = callerGroup->getNodeByName(str);
            if (foundNode) {
                *retIsNode = foundNode->getEffectInstance();
                return true;
            }
        }

        // Check for an inputNumber
        if (callerIsNode) {
            std::string prefix("input");
            std::size_t foundPrefix = str.find(prefix);
            if (foundPrefix != std::string::npos) {
                std::string inputNumberStr = str.substr(prefix.size());
                int inputNb = -1;
                if (inputNumberStr.empty()) {
                    inputNb = 0;
                } else {
                    bool isValidNumber = true;
                    for (std::size_t i = 0 ; i < inputNumberStr.size(); ++i) {
                        if (!std::isdigit(inputNumberStr[i])) {
                            isValidNumber = false;
                            break;
                        }
                    }
                    if (isValidNumber) {
                        inputNb = std::atoi(inputNumberStr.c_str());
                    }
                }
                if (inputNb != -1) {
                    *retIsNode = callerIsNode->getInputMainInstance(inputNb);
                    if (*retIsNode) {
                        return true;
                    }
                }

            }
        }
        return false;

    }

    KnobTableItemPtr getThisTableItem() const
    {
        KnobHolderPtr holder = _knob->getHolder();
        if (!holder) {
            return KnobTableItemPtr();
        }
        return toKnobTableItem(_knob->getHolder());
    }

    bool checkForProject(const std::string& str) const
    {
        if (str == "app") {
            return true;
        }
        return false;
    }

    bool checkForTableItem(const std::string& str, const KnobHolderPtr& callerHolder, KnobTableItemPtr* retIsTableItem) const
    {
        retIsTableItem->reset();
        if (str == "thisItem") {
            *retIsTableItem = getThisTableItem();
            return true;
        } else if (callerHolder) {
            EffectInstancePtr callerIsEffect = toEffectInstance(callerHolder);
            KnobTableItemPtr callerIsTableItem = toKnobTableItem(callerHolder);
            assert(callerIsEffect || callerIsTableItem);
            if (callerIsEffect) {
                KnobItemsTablePtr table = callerIsEffect->getItemsTable();
                if (table) {
                    *retIsTableItem = table->getTopLevelItemByScriptName(str);
                    if (*retIsTableItem) {
                        return true;
                    }
                }
            } else if (callerIsTableItem) {
                *retIsTableItem = callerIsTableItem->getChildItemByScriptName(str);
                if (*retIsTableItem) {
                    return true;
                }
            }

        }
        return false;
    }

    KnobIPtr getThisKnob() const
    {
        return _knob->shared_from_this();
    }

    bool checkForKnob(const std::string& str, const KnobHolderPtr& callerHolder, KnobIPtr* retIsKnob) const
    {
        retIsKnob->reset();
        if (str == "thisKnob") {
            *retIsKnob = getThisKnob();
            return true;
        } else if (callerHolder) {
            *retIsKnob = callerHolder->getKnobByName(str);
            if (*retIsKnob) {
                return true;
            }
        }
        return false;
    }

    bool checkForDimension(const std::string& str, const KnobIPtr& callerKnob, DimIdx* retIsDimension) const
    {
        if (str == "dimension") {
            *retIsDimension = _dimension;
            return true;
        } else if (callerKnob) {
            int idx;
            if (isDimensionIndex(str, &idx)) {
                *retIsDimension = DimIdx(idx);
                return true;
            }
        }
        return false;
    }

};

struct ExprUnresolvedSymbolResolver : public exprtk::parser<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::unknown_symbol_resolver
{
    typedef typename exprtk::parser<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::unknown_symbol_resolver usr_t;

    KnobHelper* _knob;
    TimeValue _time;
    DimIdx _dimension;
    ViewIdx _view;
    KnobExprTkExpr* _ret;
    usr_variable_user_type _varType;
    EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t _resolvedScalar;
    std::vector<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t> _resolvedVector;
    std::string _resolvedString;


    ExprUnresolvedSymbolResolver(KnobHelper* knob, TimeValue time, DimIdx dimension, ViewIdx view, KnobExprTkExpr* ret)
    : exprtk::parser<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::unknown_symbol_resolver()
    , _knob(knob)
    , _time(time)
    , _dimension(dimension)
    , _view(view)
    , _ret(ret)
    , _varType(e_usr_variable_user_type_scalar)
    , _resolvedScalar(0)
    , _resolvedVector()
    , _resolvedString()
    {

    }
    virtual usr_symbol_type getSymbolType() const OVERRIDE FINAL
    {
        // Values are variables since we will update them later on
        return e_usr_variable_type;
    }

    virtual usr_variable_user_type getVariableType() const OVERRIDE FINAL
    {
        return _varType;
    }

    virtual EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t getResolvedScalar() OVERRIDE FINAL
    {
        assert(_varType == e_usr_variable_user_type_scalar);
        return _resolvedScalar;
    }

    virtual std::string& getResolvedString() OVERRIDE FINAL
    {
        assert(_varType == e_usr_variable_user_type_string);
        return _resolvedString;
    }

    virtual std::vector<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>& getResolvedVector()  OVERRIDE FINAL
    {
        assert(_varType == e_usr_variable_user_type_vector);
        return _resolvedVector;
    }

    virtual bool process(const std::string& unknown_symbol, std::string& error_message) OVERRIDE FINAL
    {

        SymbolResolver resolver(_knob, _dimension, _view, unknown_symbol);
        if (resolver._testingEnabled) {
            _varType = e_usr_variable_user_type_scalar;
            if (resolver._resultType == SymbolResolver::eResultTypeInvalid) {
                _resolvedScalar = 0;
            } else {
                _resolvedScalar = 1;
            }
            return true;
        }
        switch (resolver._resultType) {
            case SymbolResolver::eResultTypeInvalid: {
                std::stringstream ss;
                ss << "Error when parsing symbol " << unknown_symbol;
                if (!resolver._error.empty()) {
                    ss << ": " << resolver._error;
                }
                error_message = ss.str();
            }   return false;
            case SymbolResolver::eResultTypeObjectName: {
                _resolvedString = resolver._objectName;
                _varType = e_usr_variable_user_type_string;
            }   break;
            case SymbolResolver::eResultTypeKnobChoiceOption: {
                KnobChoice* isChoice = dynamic_cast<KnobChoice*>(_knob);
                if (!isChoice) {
                    return false;
                }
                _resolvedString = isChoice->getActiveEntry(_view).id;
                _varType = e_usr_variable_user_type_string;
            }   break;
            case SymbolResolver::eResultTypeEffectRoD: {
                GetRegionOfDefinitionResultsPtr results;
                ActionRetCodeEnum stat = resolver._effectProperty->getRegionOfDefinition_public(_time, RenderScale(1.), _view, &results);
                _varType = e_usr_variable_user_type_vector;
                _resolvedVector.resize(4);
                if (isFailureRetCode(stat)) {
                    _resolvedVector[0] = _resolvedVector[1] = _resolvedVector[2] = _resolvedVector[3] = 0.;
                } else {
                    const RectD& rod = results->getRoD();
                    _resolvedVector[0] = rod.x1;
                    _resolvedVector[1] = rod.y1;
                    _resolvedVector[2] = rod.x2;
                    _resolvedVector[3] = rod.y2;
                }
            }   break;

            case SymbolResolver::eResultTypeKnobValue: {

                // Register the target knob as a dependency of this expression
                {
                    KnobDimViewKey dep;
                    dep.knob = resolver._targetKnob;
                    dep.dimension = resolver._targetDimension;
                    dep.view = resolver._targetView;
                    _ret->knobDependencies.insert(std::make_pair(unknown_symbol, dep));
                }


                // Return the value of the knob at the given dimension
                KnobBoolBasePtr isBoolean = toKnobBoolBase(resolver._targetKnob);
                KnobStringBasePtr isString = toKnobStringBase(resolver._targetKnob);
                KnobIntBasePtr isInt = toKnobIntBase(resolver._targetKnob);
                KnobDoubleBasePtr isDouble = toKnobDoubleBase(resolver._targetKnob);
                if (isBoolean) {
                    _resolvedScalar = EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t(isBoolean->getValueAtTime(_time, resolver._targetDimension, resolver._targetView));
                    _varType = e_usr_variable_user_type_scalar;
                } else if (isInt) {
                    _resolvedScalar = EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t(isInt->getValueAtTime(_time, resolver._targetDimension, resolver._targetView));
                    _varType = e_usr_variable_user_type_scalar;
                } else if (isDouble) {
                    _resolvedScalar = EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t(isDouble->getValueAtTime(_time, resolver._targetDimension, resolver._targetView));
                    _varType = e_usr_variable_user_type_scalar;
                } else if (isString) {
                    _varType = e_usr_variable_user_type_string;
                    _resolvedString = isString->getValueAtTime(_time, resolver._targetDimension, resolver._targetView);
                }
            }   break;
        }

        return true;
    } // process

};

struct curve_func : public exprtk::igeneric_function<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>
{

    typedef typename exprtk::igeneric_function<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::parameter_list_t parameter_list_t;
    KnobIWPtr _knob;
    ViewIdx _view;

    curve_func(const KnobIPtr& knob, ViewIdx view)
    : exprtk::igeneric_function<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>("T|TT|TTS")
    , _knob(knob)
    , _view(view)
    {
        /*
         Overloads:
         curve(frame)
         curve(frame, dimension)
         curve(frame, dimension, view)
         */
    }


    virtual EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t operator()(const std::size_t& overloadIdx, parameter_list_t parameters) OVERRIDE FINAL
    {
        typedef typename exprtk::igeneric_function<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::generic_type generic_type;
        typedef typename generic_type::scalar_view scalar_t;
        typedef typename generic_type::string_view string_t;

        assert(overloadIdx + 1 == parameters.size());
        assert(parameters.size() == 1 || parameters.size() == 2 || parameters.size() == 3);
        assert(parameters[0].type == generic_type::e_scalar);
        assert(parameters[1].type == generic_type::e_scalar);

        KnobIPtr knob = _knob.lock();
        if (!knob) {
            return 0.;
        }

        TimeValue frame(0);
        ViewIdx view(0);
        DimIdx dimension(0);
        frame = TimeValue(scalar_t(parameters[0])());
        if (parameters.size() > 1) {
            dimension = DimIdx((int)scalar_t(parameters[1])());
        }
        if (parameters.size() > 2) {
            std::string viewStr = exprtk::to_str(string_t(parameters[2]));
            if (viewStr == "view") {
                // use the current view
                view = _view;
            } else {
                // Find the view by name in the project
                const std::vector<std::string>& views = knob->getHolder()->getApp()->getProject()->getProjectViewNames();
                for (std::size_t i = 0; i < views.size(); ++i) {
                    if (views[i] == viewStr) {
                        view = ViewIdx(i);
                    }
                }
            }
        }
        


        return knob->getRawCurveValueAt(frame, view, dimension);
    }

};

static void addStandardFunctions(const std::string& expr,
                                 TimeValue time,
                                 EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t& symbol_table,
                                 std::vector<std::pair<std::string, EXPRTK_FUNCTIONS_NAMESPACE::func_ptr> >& functions,
                                 std::vector<std::pair<std::string, EXPRTK_FUNCTIONS_NAMESPACE::vararg_func_ptr> >& varargFunctions,
                                 std::vector<std::pair<std::string, EXPRTK_FUNCTIONS_NAMESPACE::generic_func_ptr> >& genericFunctions,
                                 std::string* modifiedExpression)
{
    // Add all functions from the API to the symbol table

    EXPRTK_FUNCTIONS_NAMESPACE::addFunctions(time, &functions);
    EXPRTK_FUNCTIONS_NAMESPACE::addGenericFunctions(time, &genericFunctions);
    EXPRTK_FUNCTIONS_NAMESPACE::addVarargFunctions(time, &varargFunctions);

    for (std::size_t i = 0; i < functions.size(); ++i) {
        bool ok = symbol_table.add_function(functions[i].first, *functions[i].second);
        assert(ok);
    }
    for (std::size_t i = 0; i < varargFunctions.size(); ++i) {
        bool ok = symbol_table.add_function(varargFunctions[i].first, *varargFunctions[i].second);
        assert(ok);
    }

    for (std::size_t i = 0; i < genericFunctions.size(); ++i) {
        bool ok = symbol_table.add_function(genericFunctions[i].first, *genericFunctions[i].second);
        assert(ok);
    }

    if (modifiedExpression) {
        // Modify the expression so that the last line is modified by "var exprResult:= ..."; return [exprResult]"
        *modifiedExpression = expr;
        {

            // Check for the last ';' indicating the previous statement
            std::size_t foundLastStatement = modifiedExpression->find_last_of(';');
            std::string toPrepend = "var NatronExprtkExpressionResult := ";
            bool mustAddSemiColon = true;
            if (foundLastStatement == std::string::npos) {
                // There's no ';', the expression is a single statement, pre-pend directly
                modifiedExpression->insert(0, toPrepend);
            } else {
                // Check that there's no whitespace after the last ';' otherwise the user
                // just wrote a ';' after the last statement, which is allowed.
                bool hasNonWhitespace = false;
                for (std::size_t i = foundLastStatement + 1; i < modifiedExpression->size(); ++i) {
                    if (!std::isspace((*modifiedExpression)[i])) {
                        hasNonWhitespace = true;
                        break;
                    }
                }
                if (!hasNonWhitespace) {
                    foundLastStatement = modifiedExpression->find_last_of(';', foundLastStatement - 1);
                    mustAddSemiColon = false;
                }
                if (foundLastStatement == std::string::npos) {
                    modifiedExpression->insert(0, toPrepend);
                } else {
                    modifiedExpression->insert(foundLastStatement + 1, toPrepend);
                }
            }
            if (mustAddSemiColon) {
                (*modifiedExpression) += ';';
            }
            std::string toAppend = "\nreturn [NatronExprtkExpressionResult]";
            modifiedExpression->append(toAppend);
        }
    } // modifiedExpression

} // addStandardFunctions

static bool parseExprtkExpression(const std::string& originalExpression,
                                  const std::string& expressionString,
                                  EXPRTK_FUNCTIONS_NAMESPACE::parser_t& parser,
                                  EXPRTK_FUNCTIONS_NAMESPACE::expression_t& expressionObject,
                                  std::string* error)
{

    // Compile the expression
    if (!parser.compile(expressionString, expressionObject)) {

        std::stringstream ss;
        ss << "Error(s) while compiling the following expression:" << std::endl;
        ss << originalExpression << std::endl;
        for (std::size_t i = 0; i < parser.error_count(); ++i) {
            // Include the specific nature of each error
            // and its position in the expression string.

            exprtk::parser_error::type error = parser.get_error(i);
            ss << "Error: " << i << " Position: " << error.token.position << std::endl;
            ss << "Type: " << exprtk::parser_error::to_str(error.mode) << std::endl;
            ss << "Message: " << error.diagnostic << std::endl;
        }
        *error = ss.str();
        return false;
    }
    return true;
} // parseExprtkExpression


static KnobHelper::ExpressionReturnValueTypeEnum handleExprTkReturn(EXPRTK_FUNCTIONS_NAMESPACE::expression_t& expressionObject, double* retValueIsScalar, std::string* retValueIsString, std::string* error)
{
    const EXPRTK_FUNCTIONS_NAMESPACE::results_context_t& results = expressionObject.results();
    if (results.count() != 1) {
        std::stringstream ss;
        ss << "The expression must return one value using the \"return\" keyword";
        *error = ss.str();
        return KnobHelper::eExpressionReturnValueTypeError;
    }

    switch (results[0].type) {
        case exprtk::type_store<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::e_scalar:
            *retValueIsScalar = (double)*reinterpret_cast<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t*>(results[0].data);
            return KnobHelper::eExpressionReturnValueTypeScalar;
        case exprtk::type_store<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::e_string:
            *retValueIsString = std::string(reinterpret_cast<const char*>(results[0].data));
            return KnobHelper::eExpressionReturnValueTypeString;

        case exprtk::type_store<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::e_vector:
        case exprtk::type_store<EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t>::e_unknown:
            std::stringstream ss;
            ss << "The expression must either return a scalar or string value depending on the parameter type";
            *error = ss.str();
            return KnobHelper::eExpressionReturnValueTypeError;
    }
    
} // handleExprTkReturn



void
KnobHelperPrivate::validateExprTKExpression(const std::string& expression, DimIdx dimension, ViewIdx view, std::string* resultAsString, KnobExprTkExpr* ret) const
{

    // Symbol table containing all variables that the user may use but that we do not pre-declare, such
    // as knob values etc...
    EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t unknown_var_symbol_table;

    // Symbol table containing all pre-declared variables (frame, view etc...)
    EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t symbol_table;

    QThread* curThread = QThread::currentThread();

    KnobExprTkExpr::ExpressionData& data = ret->data[curThread];
    data.expressionObject.reset(new EXPRTK_FUNCTIONS_NAMESPACE::expression_t);
    data.expressionObject->register_symbol_table(unknown_var_symbol_table);
    data.expressionObject->register_symbol_table(symbol_table);

    // Pre-declare the variables with a stub value, they will be updated at evaluation time
    TimeValue time = publicInterface->getCurrentRenderTime();

    KnobIPtr thisShared = publicInterface->shared_from_this();

    {
        double time_f = (double)time;
        symbol_table.add_variable("frame", time_f);
    }

    std::string viewName = publicInterface->getHolder()->getApp()->getProject()->getViewName(view);
    symbol_table.add_stringvar("view", viewName, false);

    {
        // The object that resolves undefined knob dependencies at compile time
        ExprUnresolvedSymbolResolver musr(publicInterface, time, dimension, view, ret);
        EXPRTK_FUNCTIONS_NAMESPACE::parser_t parser;
        parser.enable_unknown_symbol_resolver(&musr);


        addStandardFunctions(expression, time, symbol_table, data.functions, data.varargFunctions, data.genericFunctions, &ret->modifiedExpression);


        EXPRTK_FUNCTIONS_NAMESPACE::generic_func_ptr curveFunc(new curve_func(thisShared, view));
        data.genericFunctions.push_back(std::make_pair("curve", curveFunc));
        symbol_table.add_function("curve", *curveFunc);

        std::string error;
        if (!parseExprtkExpression(expression, ret->modifiedExpression, parser, *data.expressionObject, &error)) {
            throw std::runtime_error(error);
        }


    } // parser

    data.expressionObject->value();
    double retValueIsScalar;
    std::string error;
    KnobHelper::ExpressionReturnValueTypeEnum stat = handleExprTkReturn(*data.expressionObject, &retValueIsScalar, resultAsString, &error);
    switch (stat) {
        case KnobHelper::eExpressionReturnValueTypeError:
            throw std::runtime_error(error);
            break;
        case KnobHelper::eExpressionReturnValueTypeScalar: {
            std::stringstream ss;
            ss << retValueIsScalar;
            *resultAsString = ss.str();
        }   break;
        case KnobHelper::eExpressionReturnValueTypeString:
            break;
    }
} // validateExprTKExpression


void
KnobHelper::validateExpression(const std::string& expression,
                               ExpressionLanguageEnum language,
                               DimIdx dimension,
                               ViewIdx view,
                               bool hasRetVariable,
                               std::string* resultAsString)
{
    switch (language) {
        case eExpressionLanguagePython:
            _imp->validatePythonExpression(expression, dimension, view, hasRetVariable, resultAsString);
            break;
        case eExpressionLanguageExprTK: {
            KnobExprTkExpr ret;
            _imp->validateExprTKExpression(expression, dimension, view, resultAsString, &ret);
        }
    }
} // KnobHelper::validateExpression

NATRON_NAMESPACE_ANONYMOUS_ENTER
struct ExprToReApply {
    ViewIdx view;
    DimIdx dimension;
    std::string expr;
    ExpressionLanguageEnum language;
    bool hasRet;
};
NATRON_NAMESPACE_ANONYMOUS_EXIT

bool
KnobHelper::checkInvalidExpressions()
{
    int ndims = getNDimensions();


    std::vector<ExprToReApply> exprToReapply;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        for (int i = 0; i < ndims; ++i) {
            for (ExprPerViewMap::const_iterator it = _imp->common->expressions[i].begin(); it != _imp->common->expressions[i].end(); ++it) {
                if (!it->second || it->second->exprInvalid.empty()) {
                    continue;
                }
                exprToReapply.resize(exprToReapply.size() + 1);
                ExprToReApply& data = exprToReapply.back();
                data.view = it->first;
                data.dimension = DimIdx(i);

                data.expr = it->second->expressionString;
                data.language = it->second->language;
                KnobPythonExpr* isPythonExpr = dynamic_cast<KnobPythonExpr*>(it->second.get());
                if (isPythonExpr) {
                    data.hasRet = isPythonExpr->hasRet;
                } else {
                    data.hasRet = false;
                }

            }
        }
    }
    bool isInvalid = false;
    for (std::size_t i = 0; i < exprToReapply.size(); ++i) {
        setExpressionInternal(exprToReapply[i].dimension, exprToReapply[i].view, exprToReapply[i].expr, exprToReapply[i].language, exprToReapply[i].hasRet, false /*throwOnFailure*/);
        std::string err;
        if ( !isExpressionValid(exprToReapply[i].dimension, exprToReapply[i].view, &err) ) {
            isInvalid = true;
        }
    }
    return !isInvalid;
}

bool
KnobHelper::isExpressionValid(DimIdx dimension,
                              ViewIdx view,
                              std::string* error) const
{
    if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::isExpressionValid(): Dimension out of range");
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        if (error) {
            ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
            if (foundView != _imp->common->expressions[dimension].end() && foundView->second) {
                *error = foundView->second->exprInvalid;
                return error->empty();
            }
        }
    }
    return true;
}

void
KnobHelper::setExpressionInvalidInternal(DimIdx dimension, ViewIdx view, bool valid, const std::string& error)
{
    bool wasValid;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        ExprPerViewMap::iterator foundView = _imp->common->expressions[dimension].find(view);
        if (foundView == _imp->common->expressions[dimension].end() || !foundView->second) {
            return;
        }
        wasValid = foundView->second->exprInvalid.empty();
        foundView->second->exprInvalid = error;
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
                    for (ExprPerViewMap::const_iterator it = _imp->common->expressions[i].begin(); it != _imp->common->expressions[i].end(); ++it) {
                        if (it->first != view && it->second) {
                            if ( !it->second->exprInvalid.empty() ) {
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

}

void
KnobHelper::setExpressionInvalid(DimSpec dimension,
                                 ViewSetSpec view,
                                 bool valid,
                                 const std::string& error)
{
    if (!getHolder() || !getHolder()->getApp()) {
        return;
    }
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    setExpressionInvalidInternal(DimIdx(i), *it, valid, error);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
                setExpressionInvalidInternal(DimIdx(i), view_i, valid, error);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setExpressionInvalid(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                setExpressionInvalidInternal(DimIdx(dimension), *it, valid, error);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
            setExpressionInvalidInternal(DimIdx(dimension), view_i, valid, error);
        }
    }
} // setExpressionInvalid


void
KnobHelper::setExpressionInternal(DimIdx dimension,
                                  ViewIdx view,
                                  const std::string& expression,
                                  ExpressionLanguageEnum language, 
                                  bool hasRetVariable,
                                  bool failIfInvalid)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    assert( dimension >= 0 && dimension < getNDimensions() );



    ///Clear previous expr
    clearExpression(dimension, ViewSetSpec(view));

    if ( expression.empty() ) {
        return;
    }


    std::string exprResult;

    KnobExprPtr expressionObj;
    boost::scoped_ptr<PythonGILLocker> pgl;
    try {
        switch (language) {
            case eExpressionLanguagePython: {
                pgl.reset(new PythonGILLocker);
                boost::shared_ptr<KnobPythonExpr> obj(new KnobPythonExpr);
                expressionObj = obj;
                obj->modifiedExpression = _imp->validatePythonExpression(expression, dimension, view, hasRetVariable, &exprResult);
                obj->hasRet = hasRetVariable;
            }   break;
            case eExpressionLanguageExprTK: {
                boost::shared_ptr<KnobExprTkExpr> obj(new KnobExprTkExpr);
                expressionObj = obj;
                _imp->validateExprTKExpression(expression, dimension, view, &exprResult, obj.get());
            }   break;
        }
        expressionObj->expressionString = expression;
        expressionObj->language = language;

    } catch (const std::exception &e) {
        expressionObj->exprInvalid = e.what();
        if (failIfInvalid) {
            throw std::invalid_argument(expressionObj->exprInvalid);
        }
    }

    {
        QMutexLocker k(&_imp->common->expressionMutex);
        _imp->common->expressions[dimension][view] = expressionObj;
    }

    KnobHolderPtr holder = getHolder();
    if (holder) {
        if (!expressionObj->exprInvalid.empty()) {
            assert(!failIfInvalid);
            AppInstancePtr app = holder->getApp();
            if (app) {
                app->addInvalidExpressionKnob( shared_from_this() );
            }
        } else {
            // Populate the listeners set so we can keep track of user links.
            // In python, the dependencies tracking is done by executing the expression itself unlike exprtk
            // where we have the dependencies list directly when compiling
            switch (language) {
                case eExpressionLanguagePython: {
                    EXPR_RECURSION_LEVEL();
                    _imp->parseListenersFromExpression(dimension, view);
                }   break;
                case eExpressionLanguageExprTK: {
                    KnobExprTkExpr* obj = dynamic_cast<KnobExprTkExpr*>(expressionObj.get());
                    KnobIPtr thisShared = shared_from_this();
                    for (std::map<std::string, KnobDimViewKey>::const_iterator it = obj->knobDependencies.begin(); it != obj->knobDependencies.end(); ++it) {
                        it->second.knob.lock()->addListener(dimension, it->second.dimension, view, it->second.view, thisShared, eExpressionLanguageExprTK);
                    }
                }   break;
            }
            
        }
    }
    

    // Notify the expr. has changed
    expressionChanged(dimension, view);
} // setExpressionInternal

void
KnobHelper::setExpressionCommon(DimSpec dimension,
                                ViewSetSpec view,
                                const std::string& expression,
                                ExpressionLanguageEnum language,
                                bool hasRetVariable,
                                bool failIfInvalid)
{
    // setExpressionInternal may call evaluateValueChange if it calls clearExpression, hence bracket changes
    ScopedChanges_RAII changes(this);

    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    setExpressionInternal(DimIdx(i), *it, expression, language, hasRetVariable, failIfInvalid);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
                setExpressionInternal(DimIdx(i), view_i, expression, language, hasRetVariable, failIfInvalid);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setExpressionCommon(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                setExpressionInternal(DimIdx(dimension), *it, expression, language, hasRetVariable, failIfInvalid);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
            setExpressionInternal(DimIdx(dimension), view_i, expression, language, hasRetVariable, failIfInvalid);
        }
    }

    evaluateValueChange(dimension, getHolder()->getCurrentRenderTime(), view, eValueChangedReasonUserEdited);
} // setExpressionCommon

void
KnobHelper::replaceNodeNameInExpressionInternal(DimIdx dimension,
                                                ViewIdx view,
                                                const std::string& oldName,
                                                const std::string& newName)
{

    EffectInstancePtr isEffect = toEffectInstance(getHolder());
    if (!isEffect) {
        return;
    }

    std::string hasExpr = getExpression(dimension, view);
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
                                        const std::string& oldName,
                                        const std::string& newName)
{
    if (oldName == newName) {
        return;
    }
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return;
    }
    ScopedChanges_RAII changes(holder.get());

    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    replaceNodeNameInExpressionInternal(DimIdx(i), *it, oldName, newName);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
                replaceNodeNameInExpressionInternal(DimIdx(i), view_i, oldName, newName);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::replaceNodeNameInExpression(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                replaceNodeNameInExpressionInternal(DimIdx(dimension), *it, oldName, newName);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
            replaceNodeNameInExpressionInternal(DimIdx(dimension), view_i, oldName, newName);
        }
    }


} // replaceNodeNameInExpression

ExpressionLanguageEnum
KnobHelper::getExpressionLanguage(Natron::ViewIdx view, Natron::DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_imp->common->expressions.size()) {
        throw std::invalid_argument("KnobHelper::getExpressionLanguage(): Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if (foundView == _imp->common->expressions[dimension].end()) {
        return eExpressionLanguageExprTK;
    }
    if (!foundView->second) {
        return eExpressionLanguageExprTK;
    }
    return foundView->second->language;
}

bool
KnobHelper::isExpressionUsingRetVariable(ViewIdx view, DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_imp->common->expressions.size()) {
        throw std::invalid_argument("KnobHelper::isExpressionUsingRetVariable(): Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if (foundView == _imp->common->expressions[dimension].end()) {
        return false;
    }
    if (!foundView->second) {
        return false;
    }
    KnobPythonExpr* isPythonExpr = dynamic_cast<KnobPythonExpr*>(foundView->second.get());
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
    if (dimension < 0 || dimension >= (int)_imp->common->expressions.size()) {
        throw std::invalid_argument("KnobHelper::getExpressionDependencies(): Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if (foundView == _imp->common->expressions[dimension].end() || !foundView->second) {
        return false;
    }
    KnobPythonExpr* isPythonExpr = dynamic_cast<KnobPythonExpr*>(foundView->second.get());
    KnobExprTkExpr* isExprtkExpr = dynamic_cast<KnobExprTkExpr*>(foundView->second.get());
    assert(isPythonExpr || isExprtkExpr);
    if (isPythonExpr) {
        dependencies = isPythonExpr->dependencies;
    } else if (isExprtkExpr) {
        for (std::map<std::string, KnobDimViewKey>::const_iterator it = isExprtkExpr->knobDependencies.begin(); it != isExprtkExpr->knobDependencies.end(); ++it) {
            dependencies.insert(it->second);
        }
    } else {
        return false;
    }

    return true;
}

bool
KnobHelper::clearExpressionInternal(DimIdx dimension, ViewIdx view)
{
    PythonGILLocker pgl;
    bool hadExpression = false;
    KnobDimViewKeySet dependencies;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        ExprPerViewMap::iterator foundView = _imp->common->expressions[dimension].find(view);
        if (foundView != _imp->common->expressions[dimension].end() && foundView->second) {
            hadExpression = true;
            KnobPythonExpr* isPythonExpr = dynamic_cast<KnobPythonExpr*>(foundView->second.get());
            KnobExprTkExpr* isExprtkExpr = dynamic_cast<KnobExprTkExpr*>(foundView->second.get());
            assert(isPythonExpr || isExprtkExpr);
            if (isPythonExpr) {
                dependencies = isPythonExpr->dependencies;
            } else if (isExprtkExpr) {
                for (std::map<std::string, KnobDimViewKey>::const_iterator it = isExprtkExpr->knobDependencies.begin(); it != isExprtkExpr->knobDependencies.end(); ++it) {
                    dependencies.insert(it->second);
                }
            }
            foundView->second.reset();
        }
    }
    KnobIPtr thisShared = shared_from_this();
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

            {
                QMutexLocker otherMastersLocker(&other->_imp->common->expressionMutex);

                KnobDimViewKeySet& otherListeners = other->_imp->common->listeners[it->dimension][it->view];
                KnobDimViewKeySet::iterator foundListener = otherListeners.find(listenerToRemoveKey);
                if (foundListener != otherListeners.end()) {
                    otherListeners.erase(foundListener);
                }
            }
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
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    didSomething |= clearExpressionInternal(DimIdx(i), *it);
                }
            } else {
                ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
                didSomething |= clearExpressionInternal(DimIdx(i), view_i);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->common->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::clearExpression(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                didSomething |= clearExpressionInternal(DimIdx(dimension), *it);
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
            didSomething |= clearExpressionInternal(DimIdx(dimension), view_i);
        }
    }


    if (didSomething) {
        evaluateValueChange(dimension, getHolder()->getTimelineCurrentTime(), view, eValueChangedReasonUserEdited);
    }

} // KnobHelper::clearExpression

void
KnobHelper::expressionChanged(DimIdx dimension, ViewIdx view)
{ 
    _signalSlotHandler->s_expressionChanged(dimension, view);

    computeHasModifications();
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

static bool
catchErrors(PyObject* mainModule,
            std::string* error)
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
KnobHelper::executeExprTKExpression(TimeValue time, ViewIdx view, DimIdx dimension, double* retValueIsScalar, std::string* retValueIsString, std::string* error)
{

    boost::shared_ptr<KnobExprTkExpr> obj;

    // Take the expression mutex. Copying the exprtk expression does not actually copy all variables and functions, it just
    // increments a shared reference count.
    // To be thread safe we have 2 solutions:
    // 1) Compile the expression for each thread and then run it without a mutex
    // 2) Compile only once and run the expression under a lock
    // We picked solution 1)
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view);
        if (foundView == _imp->common->expressions[dimension].end() || !foundView->second) {
            return eExpressionReturnValueTypeError;
        }
        KnobExprTkExpr* isExprtkExpr = dynamic_cast<KnobExprTkExpr*>(foundView->second.get());
        assert(isExprtkExpr);
        // Copy the expression object so it is local to this thread
        obj = boost::dynamic_pointer_cast<KnobExprTkExpr>(foundView->second);
        assert(obj);
    }

    QThread* curThread = QThread::currentThread();

    KnobExprTkExpr::ExpressionData* data = 0;
    {
        QMutexLocker k(&obj->lock);
        KnobExprTkExpr::PerThreadDataMap::iterator foundThreadData = obj->data.find(curThread);
        if (foundThreadData == obj->data.end()) {
            std::pair<KnobExprTkExpr::PerThreadDataMap::iterator, bool> ret = obj->data.insert(std::make_pair(curThread, KnobExprTkExpr::ExpressionData()));
            assert(ret.second);
            foundThreadData = ret.first;
        }
        data = &foundThreadData->second;
    }

    bool isRenderClone = getHolder()->isRenderClone();


    EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t* unknown_symbols_table = 0;//
    EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t* symbol_table = 0; //obj->expressionObject->get_symbol_table(1);

    bool existingExpression = true;
    if (!data->expressionObject) {

        // We did not build the expression already
        existingExpression = false;
        data->expressionObject.reset(new EXPRTK_FUNCTIONS_NAMESPACE::expression_t);
        EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t tmpUnresolved;
        EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t tmpResolved;
        data->expressionObject->register_symbol_table(tmpUnresolved);
        data->expressionObject->register_symbol_table(tmpResolved);
    }

    unknown_symbols_table = &data->expressionObject->get_symbol_table(0);
    symbol_table = &data->expressionObject->get_symbol_table(1);


    if (existingExpression) {
        // Update the frame & view in the know table
        symbol_table->variable_ref("frame") = (double)time;

        // Remove from the symbol table functions that hold a state, and re-add a new fresh local copy of them so that the state
        // is local to this thread.
        std::vector<std::pair<std::string, EXPRTK_FUNCTIONS_NAMESPACE::func_ptr > > functionsCopy;
        EXPRTK_FUNCTIONS_NAMESPACE::makeLocalCopyOfStateFunctions(time, *symbol_table, &functionsCopy);

    } else {
        double time_f = (double)time;
        symbol_table->add_variable("frame", time_f);
        std::string viewName = getHolder()->getApp()->getProject()->getViewName(view);
        symbol_table->add_stringvar("view", viewName, false);

        addStandardFunctions(obj->expressionString, time, *symbol_table, data->functions, data->varargFunctions, data->genericFunctions, 0);


        KnobIPtr thisShared = shared_from_this();

        EXPRTK_FUNCTIONS_NAMESPACE::generic_func_ptr curveFunc(new curve_func(thisShared, view));
        data->genericFunctions.push_back(std::make_pair("curve", curveFunc));
        symbol_table->add_function("curve", *curveFunc);

        EXPRTK_FUNCTIONS_NAMESPACE::parser_t parser;
        std::string error;
        if (!parseExprtkExpression(obj->expressionString, obj->modifiedExpression, parser, *data->expressionObject, &error)) {
            throw std::runtime_error(error);
        }

    } // existingExpression

    for (std::map<std::string, KnobDimViewKey>::const_iterator it = obj->knobDependencies.begin(); it != obj->knobDependencies.end(); ++it) {
        KnobIPtr knob = it->second.knob.lock();
        if (!knob) {
            continue;
        }

        if (isRenderClone) {
            // Get the render clone for this knob
            // First ensure a clone is created for the effect holding the knob
            // and then fetch the knob clone on it
            TreeRenderPtr render = getHolder()->getCurrentRender();
            assert(render);
            TimeValue time = getHolder()->getCurrentRenderTime();
            ViewIdx view = getHolder()->getCurrentRenderView();
            FrameViewRenderKey key = {time, view, render};
            KnobHolderPtr holderClone = knob->getHolder()->createRenderClone(key);

            knob = knob->getCloneForHolderInternal(holderClone);

        }

        KnobBoolBasePtr isBoolean = toKnobBoolBase(knob);
        KnobStringBasePtr isString = toKnobStringBase(knob);
        KnobIntBasePtr isInt = toKnobIntBase(knob);
        KnobDoubleBasePtr isDouble = toKnobDoubleBase(knob);
        if (existingExpression) {
            if (isBoolean) {
                unknown_symbols_table->variable_ref(it->first) = isBoolean->getValueAtTime(time, it->second.dimension, it->second.view);
            } else if (isInt) {
                unknown_symbols_table->variable_ref(it->first) = isInt->getValueAtTime(time, it->second.dimension, it->second.view);
            } else if (isDouble) {
                double val = isDouble->getValueAtTime(time, it->second.dimension, it->second.view);
                unknown_symbols_table->variable_ref(it->first) = val;
            } else if (isString) {
                unknown_symbols_table->stringvar_ref(it->first) = isString->getValueAtTime(time, it->second.dimension, it->second.view);
            }
        } else {
            if (isBoolean) {
                double value = (double)isBoolean->getValueAtTime(time, it->second.dimension, it->second.view);
                unknown_symbols_table->add_variable(it->first, value);
            } else if (isInt) {
                double value = (double)isInt->getValueAtTime(time, it->second.dimension, it->second.view);;
                unknown_symbols_table->add_variable(it->first, value);
            } else if (isDouble) {
                double val = isDouble->getValueAtTime(time, it->second.dimension, it->second.view);
                unknown_symbols_table->add_variable(it->first, val);
            } else if (isString) {
                std::string val = isString->getValueAtTime(time, it->second.dimension, it->second.view);
                unknown_symbols_table->add_stringvar(it->first, val);
            }

        }
    }

    for (std::map<std::string, EffectFunctionDependency>::const_iterator it = obj->effectDependencies.begin(); it != obj->effectDependencies.end(); ++it) {
        EffectInstancePtr effect = it->second.effect.lock();
        if (!effect) {
            continue;
        }

        if (isRenderClone) {
            // Get the render clone
            TreeRenderPtr render = getHolder()->getCurrentRender();
            assert(render);
            TimeValue time = getHolder()->getCurrentRenderTime();
            ViewIdx view = getHolder()->getCurrentRenderView();
            FrameViewRenderKey key = {time, view, render};
            effect = toEffectInstance(effect->createRenderClone(key));
            assert(effect && effect->isRenderClone());
        }
        switch (it->second.type) {
            case EffectFunctionDependency::eEffectFunctionDependencyRoD: {
                GetRegionOfDefinitionResultsPtr results;
                ActionRetCodeEnum stat = effect->getRegionOfDefinition_public(time, RenderScale(1.), view, &results);
                if (isFailureRetCode(stat)) {
                    std::stringstream ss;
                    ss << it->first << ": Could not get region of definition";
                    *error = ss.str();
                    return eExpressionReturnValueTypeError;
                }
                const RectD& rod = results->getRoD();

                if (existingExpression) {
                    EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t::vector_holder_ptr vecHolderPtr = unknown_symbols_table->get_vector(it->first);
                    assert(vecHolderPtr->size() == 4);
                    *(*vecHolderPtr)[0] = EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t(rod.x1);
                    *(*vecHolderPtr)[1] = EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t(rod.y1);
                    *(*vecHolderPtr)[2] = EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t(rod.x2);
                    *(*vecHolderPtr)[3] = EXPRTK_FUNCTIONS_NAMESPACE::exprtk_scalar_t(rod.y2);
                } else {
                    std::vector<double> vec(4);
                    vec[0] = rod.x1;
                    vec[1] = rod.y1;
                    vec[2] = rod.x2;
                    vec[3] = rod.y2;
                    unknown_symbols_table->add_vector(it->first, &vec[0], vec.size());
                }
                break;
            }
        }
    }


    // Evaluate the expression
    data->expressionObject->value();
    return handleExprTkReturn(*data->expressionObject, retValueIsScalar, retValueIsString, error);
} // executeExprTKExpression

KnobHelper::ExpressionReturnValueTypeEnum
KnobHelper::evaluateExpression(const std::string& expr,
                               ExpressionLanguageEnum language,
                               double* retIsScalar, std::string* retIsString,
                               std::string* error)
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
            } else if (PyUnicode_Check(ret)) {

                PyObject* utf8pyobj = PyUnicode_AsUTF8String(ret); // newRef
                if (utf8pyobj) {
                    char* cstr = PyBytes_AS_STRING(utf8pyobj); // Borrowed pointer
                    retIsString->append(cstr);
                    Py_DECREF(utf8pyobj);
                }
                retCode = eExpressionReturnValueTypeString;
            } else {
                *retIsScalar = 0;
                retCode = eExpressionReturnValueTypeScalar;
            }

            Py_DECREF(ret); //< new ref
        }   break;
        case eExpressionLanguageExprTK: {
            retCode = KnobHelper::executeExprTKExpression(expr, retIsScalar, retIsString, error);
        }   break;
    }
    
    return retCode;
}


bool
KnobHelper::executePythonExpression(TimeValue time,
                                    ViewIdx view,
                                    DimIdx dimension,
                                    PyObject** ret,
                                    std::string* error) const
{
    if (dimension < 0 || dimension >= (int)_imp->common->expressions.size()) {
        throw std::invalid_argument("KnobHelper::executeExpression(): Dimension out of range");
    }

    EffectInstancePtr effect = toEffectInstance(getHolder());
    if (effect) {
        appPTR->setLastPythonAPICaller_TLS(effect);
    }


    std::string expr;
    {
        QMutexLocker k(&_imp->common->expressionMutex);
        ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view);
        if (foundView == _imp->common->expressions[dimension].end() || !foundView->second) {
            return false;
        }
        KnobPythonExpr* isPythonExpr = dynamic_cast<KnobPythonExpr*>(foundView->second.get());
        assert(isPythonExpr);
        expr = isPythonExpr->modifiedExpression;
    }

    std::stringstream ss;

    std::string viewName;
    if (getHolder() && getHolder()->getApp()) {
        viewName = getHolder()->getApp()->getProject()->getViewName(view);
    }
    if (viewName.empty()) {
        viewName = "Main";
    }

    ss << expr << '(' << time << ", \"" << viewName << "\")\n";
    std::string script = ss.str();

    ///Reset the random state to reproduce the sequence
    randomSeed( time, hashFunction(dimension) );

    return executePythonExpression(ss.str(), ret, error);
} // executeExpression

bool
KnobHelper::executePythonExpression(const std::string& expr,
                              PyObject** ret,
                              std::string* error)
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

KnobHelper::ExpressionReturnValueTypeEnum
KnobHelper::executeExprTKExpression(const std::string& expr, double* retValueIsScalar, std::string* retValueIsString, std::string* error)
{
    EXPRTK_FUNCTIONS_NAMESPACE::symbol_table_t symbol_table;

    EXPRTK_FUNCTIONS_NAMESPACE::expression_t expressionObj;
    expressionObj.register_symbol_table(symbol_table);

    std::vector<std::pair<std::string, EXPRTK_FUNCTIONS_NAMESPACE::func_ptr> > functions;
    std::vector<std::pair<std::string, EXPRTK_FUNCTIONS_NAMESPACE::vararg_func_ptr> > varargFunctions;
    std::vector<std::pair<std::string, EXPRTK_FUNCTIONS_NAMESPACE::generic_func_ptr> > genericFunctions;

    TimeValue time(0);
    EXPRTK_FUNCTIONS_NAMESPACE::parser_t parser;
    std::string modifiedExpr;
    addStandardFunctions(expr, time, symbol_table, functions, varargFunctions, genericFunctions, &modifiedExpr);

    if (!parseExprtkExpression(expr, modifiedExpr, parser, expressionObj, error)) {
        return eExpressionReturnValueTypeError;
    }

    // Evaluate the expression
    expressionObj.value();
    return handleExprTkReturn(expressionObj, retValueIsScalar, retValueIsString, error);

} // executeExprTKExpression

std::string
KnobHelper::getExpression(DimIdx dimension, ViewIdx view) const
{
    if (dimension < 0 || dimension >= (int)_imp->common->expressions.size()) {
        throw std::invalid_argument("Knob::getExpression: Dimension out of range");
    }
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->common->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->common->expressions[dimension].find(view_i);
    if (foundView == _imp->common->expressions[dimension].end() || !foundView->second) {
        return std::string();
    }
    return foundView->second->expressionString;
}



NATRON_NAMESPACE_EXIT
