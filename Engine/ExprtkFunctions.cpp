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

#include "ExprtkFunctions.h"

#include "Engine/Noise.h"
#include "Engine/PyExprUtils.h"
#include "Engine/KnobPrivate.h"

NATRON_NAMESPACE_ENTER

EXPRTK_FUNCTIONS_NAMESPACE_ENTER


template <typename T, typename FuncType>
void registerFunction(const std::string& name, std::vector<std::pair<std::string, boost::shared_ptr<FuncType> > >* functions)
{
    boost::shared_ptr<FuncType> ptr(new T);
    functions->push_back(std::make_pair(name, ptr));
}

struct boxstep : public exprtk::ifunction<exprtk_scalar_t>
{
    boxstep() : exprtk::ifunction<exprtk_scalar_t>(2) {}

    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& a) {
        return NATRON_PYTHON_NAMESPACE::ExprUtils::boxstep(x, a);
    }

};

struct linearstep : public exprtk::ifunction<exprtk_scalar_t>
{
    linearstep() : exprtk::ifunction<exprtk_scalar_t>(3) {}

    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& a, const exprtk_scalar_t& b) {
        return NATRON_PYTHON_NAMESPACE::ExprUtils::linearstep(x, a, b);

    }
};

struct smoothstep : public exprtk::ifunction<exprtk_scalar_t>
{
    smoothstep() : exprtk::ifunction<exprtk_scalar_t>(3) {}

    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& a, const exprtk_scalar_t& b) {
        return NATRON_PYTHON_NAMESPACE::ExprUtils::smoothstep(x, a, b);

    }
};


struct gaussstep : public exprtk::ifunction<exprtk_scalar_t>
{
    gaussstep() : exprtk::ifunction<exprtk_scalar_t>(3) {}

    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& a, const exprtk_scalar_t& b) {
        return NATRON_PYTHON_NAMESPACE::ExprUtils::gaussstep(x, a, b);

    }
};

struct remap : public exprtk::ifunction<exprtk_scalar_t>
{
    remap() : exprtk::ifunction<exprtk_scalar_t>(5) {}

    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& source, const exprtk_scalar_t& range, const exprtk_scalar_t& falloff, const exprtk_scalar_t& interp)
    {
        return NATRON_PYTHON_NAMESPACE::ExprUtils::remap(x, source, range, falloff, interp);

    }
};

struct mix : public exprtk::ifunction<exprtk_scalar_t>
{
    mix() : exprtk::ifunction<exprtk_scalar_t>(3) {}

    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& y, const exprtk_scalar_t& alpha)
    {
        return NATRON_PYTHON_NAMESPACE::ExprUtils::mix(x, y, alpha);

    }
};

struct hash : public exprtk::ivararg_function<exprtk_scalar_t>
{
    exprtk_scalar_t operator()(const std::vector<double>& arglist)
    {
        return NATRON_PYTHON_NAMESPACE::ExprUtils::hash(arglist);

    }
};

struct noise1 : public exprtk::ifunction<exprtk_scalar_t>
{
    noise1() : exprtk::ifunction<exprtk_scalar_t>(1) {}
    exprtk_scalar_t operator()(const exprtk_scalar_t& x) {
        double ret = 0.;
        double input = x;
        // coverity[callee_ptr_arith]
        Noise<1, 1>(&input, &ret);
        return ret;
    }
};

struct noise2 : public exprtk::ifunction<exprtk_scalar_t>
{
    noise2() : exprtk::ifunction<exprtk_scalar_t>(2) {}
    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& y) {
        double ret = 0.;
        double input[2] = {x, y};
        // coverity[callee_ptr_arith]
        Noise<2, 1>(input, &ret);
        return ret;
    }
};

struct noise3 : public exprtk::ifunction<exprtk_scalar_t>
{
    noise3() : exprtk::ifunction<exprtk_scalar_t>(3) {}
    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& y, const exprtk_scalar_t& z)
    {
        double ret = 0.;
        double input[3] = {x, y, z};
        // coverity[callee_ptr_arith]
        Noise<3, 1>(input, &ret);
        return ret;
    }
};

struct noise4 : public exprtk::ifunction<exprtk_scalar_t>
{
    noise4() : exprtk::ifunction<exprtk_scalar_t>(4) {}
    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& y, const exprtk_scalar_t& z, const exprtk_scalar_t& w)
    {
        double ret = 0.;
        double input[4] = {x, y, z, w};
        // coverity[callee_ptr_arith]
        Noise<4, 1>(input, &ret);
        return ret;
    }
};

struct turbulence : public exprtk::igeneric_function<exprtk_scalar_t>
{

    typedef typename exprtk::igeneric_function<exprtk_scalar_t>::parameter_list_t parameter_list_t;

    turbulence()
    : exprtk::igeneric_function<exprtk_scalar_t>("TTT|TTTT|TTTTT|TTTTTT")
    {}
    virtual exprtk_scalar_t operator()(const std::size_t& overloadIdx, parameter_list_t parameters) OVERRIDE FINAL
    {
        typedef typename exprtk::igeneric_function<exprtk_scalar_t>::generic_type generic_type;
        typedef typename generic_type::scalar_view scalar_t;

        assert(parameters.size() == overloadIdx + 3);
        assert(parameters.size() == 3 || parameters.size() == 4 || parameters.size() == 5 || parameters.size() == 6);
        assert(parameters[0].type == generic_type::e_scalar);
        assert(parameters[1].type == generic_type::e_scalar);
        assert(parameters[2].type == generic_type::e_scalar);
        NATRON_PYTHON_NAMESPACE::Double3DTuple p;
        p.x = scalar_t(parameters[0])();
        p.y = (double)scalar_t(parameters[1])();
        p.z = (double)scalar_t(parameters[2])();


        double octaves = 6;
        double lacunarity = 2.;
        double gain = 0.5;
        if (parameters.size() > 3) {
            octaves = scalar_t(parameters[3])();
        }
        if (parameters.size() > 4) {
            lacunarity = scalar_t(parameters[4])();
        }
        if (parameters.size() > 5) {
            gain = scalar_t(parameters[5])();
        }
        return NATRON_PYTHON_NAMESPACE::ExprUtils::turbulence(p, octaves, lacunarity, gain);
    }

};

struct fbm : public exprtk::igeneric_function<exprtk_scalar_t>
{

    typedef typename exprtk::igeneric_function<exprtk_scalar_t>::parameter_list_t parameter_list_t;

    fbm()
    : exprtk::igeneric_function<exprtk_scalar_t>("TTT|TTTT|TTTTT|TTTTTT")
    {}
    virtual exprtk_scalar_t operator()(const std::size_t& overloadIdx, parameter_list_t parameters) OVERRIDE FINAL
    {
        typedef typename exprtk::igeneric_function<exprtk_scalar_t>::generic_type generic_type;
        typedef typename generic_type::scalar_view scalar_t;

        assert(parameters.size() == overloadIdx + 3);
        assert(parameters.size() == 3 || parameters.size() == 4 || parameters.size() == 5 || parameters.size() == 6);
        assert(parameters[0].type == generic_type::e_scalar);
        assert(parameters[1].type == generic_type::e_scalar);
        assert(parameters[2].type == generic_type::e_scalar);
        NATRON_PYTHON_NAMESPACE::Double3DTuple p;
        p.x = scalar_t(parameters[0])();
        p.y = (double)scalar_t(parameters[1])();
        p.z = (double)scalar_t(parameters[2])();


        double octaves = 6;
        double lacunarity = 2.;
        double gain = 0.5;
        if (parameters.size() > 3) {
            octaves = scalar_t(parameters[3])();
        }
        if (parameters.size() > 4) {
            lacunarity = scalar_t(parameters[4])();
        }
        if (parameters.size() > 5) {
            gain = scalar_t(parameters[5])();
        }
        return NATRON_PYTHON_NAMESPACE::ExprUtils::fbm(p, octaves, lacunarity, gain);
    }

};


struct cellnoise : public exprtk::ifunction<exprtk_scalar_t>
{
    cellnoise() : exprtk::ifunction<exprtk_scalar_t>(3) {}
    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& y, const exprtk_scalar_t& z)
    {
        double result = 0.;
        double input[3] = {x, y ,z};
        // coverity[callee_ptr_arith]
        CellNoise<3, 1>(input, &result);
        return result;
    }
};

struct pnoise : public exprtk::ifunction<exprtk_scalar_t>
{
    pnoise() : exprtk::ifunction<exprtk_scalar_t>(6) {}
    exprtk_scalar_t operator()(const exprtk_scalar_t& x, const exprtk_scalar_t& y, const exprtk_scalar_t& z,
                               const exprtk_scalar_t& px, const exprtk_scalar_t& py, const exprtk_scalar_t& pz)
    {
        double result = 0.;
        double p[3] = {x, y ,z};
        int pargs[3] = {std::max(1,(int)px),
            std::max(1,(int)py),
            std::max(1,(int)pz)};
        // coverity[callee_ptr_arith]
        PNoise<3, 1>((const double*)p, pargs, &result);
        return result;
    }
};


struct random : public exprtk::ifunction<exprtk_scalar_t>
{
    U32 lastRandomHash;

    random(TimeValue time)
    : exprtk::ifunction<exprtk_scalar_t>(2)
    , lastRandomHash(0)
    {
        // Make the hash vary from time
        {
            alias_cast_float ac;
            ac.data = (float)time;
            lastRandomHash += ac.raw;
        }
    }
    exprtk_scalar_t operator()(const exprtk_scalar_t& min, const exprtk_scalar_t& max)
    {
        lastRandomHash = hashFunction(lastRandomHash);

        return ( (double)lastRandomHash / (double)0x100000000LL ) * (max - min)  + min;

    }
};

struct randomInt : public exprtk::ifunction<exprtk_scalar_t>
{
    U32 lastRandomHash;

    randomInt(TimeValue time)
    : exprtk::ifunction<exprtk_scalar_t>(2)
    , lastRandomHash(0)
    {
        // Make the hash vary from time
        {
            alias_cast_float ac;
            ac.data = (float)time;
            lastRandomHash += ac.raw;
        }
    }
    exprtk_scalar_t operator()(const exprtk_scalar_t& min, const exprtk_scalar_t& max)
    {
        lastRandomHash = hashFunction(lastRandomHash);

        return int(( (double)lastRandomHash / (double)0x100000000LL ) * ((int)max - (int)min)  + (int)min);
        
    }
};

struct numtostr : public exprtk::igeneric_function<exprtk_scalar_t>
{
    typedef exprtk::igeneric_function<exprtk_scalar_t> generic_func;
    typedef typename generic_func::parameter_list_t parameter_list_t;

    numtostr()
    : exprtk::igeneric_function<exprtk_scalar_t>("T|TS|TST", generic_func::e_rtrn_string)
    {
        /*overloads:
         str(value)
         str(value, format)
         str(value, format, precision)
         */
    }
    virtual exprtk_scalar_t operator()(const std::size_t& overloadIdx,
                                       std::string& result,
                                       parameter_list_t parameters) OVERRIDE FINAL
    {
        typedef typename exprtk::igeneric_function<exprtk_scalar_t>::generic_type generic_type;
        typedef typename generic_type::scalar_view scalar_t;
        typedef typename generic_type::string_view string_t;

        assert(parameters.size() == overloadIdx + 1);
        assert(parameters.size() == 1 || parameters.size() == 2 || parameters.size() == 3);

        double value = scalar_t(parameters[0])();
        std::string format;
        format += 'f';
        if (parameters.size() > 1) {
            format = exprtk::to_str(string_t(parameters[1]));
            if (format != "f" && format != "g" && format != "e" && format != "G" && format != "E") {
                return 0.;
            }
        }
        int precision = 6;
        if (parameters.size() > 2) {
            precision = std::floor(scalar_t(parameters[2])());
        }

        QString str = QString::number(value, format[0], precision);
        result = str.toStdString();

        return 1.;
    }
    
};

void
addVarargFunctions(TimeValue /*time*/, std::vector<std::pair<std::string, vararg_func_ptr > >* functions)
{
    registerFunction<hash>("hash", functions);
}

void
addFunctions(TimeValue time, std::vector<std::pair<std::string, func_ptr > >* functions)
{
    registerFunction<boxstep>("boxstep", functions);
    registerFunction<linearstep>("linearstep", functions);
    registerFunction<smoothstep>("smoothstep", functions);
    registerFunction<gaussstep>("gaussstep", functions);
    registerFunction<remap>("remap", functions);
    registerFunction<mix>("mix", functions);
    registerFunction<noise1>("noise1", functions);
    registerFunction<noise2>("noise2", functions);
    registerFunction<noise3>("noise3", functions);
    registerFunction<noise4>("noise4", functions);
    registerFunction<pnoise>("pnoise", functions);
    registerFunction<cellnoise>("cellnoise", functions);
    {
        boost::shared_ptr<func_t> ptr(new random(time));
        functions->push_back(std::make_pair("random", ptr));
    }
    {
        boost::shared_ptr<func_t> ptr(new randomInt(time));
        functions->push_back(std::make_pair("randomInt", ptr));
    }
}

void
addGenericFunctions(TimeValue /*time*/, std::vector<std::pair<std::string, generic_func_ptr > >* functions)
{


    registerFunction<turbulence>("turbulence", functions);
    registerFunction<fbm>("fbm", functions);
    registerFunction<numtostr>("str", functions);
}

void makeLocalCopyOfStateFunctions(TimeValue time, symbol_table_t& symbol_table, std::vector<std::pair<std::string, func_ptr > >* functions)
{
    symbol_table.remove_function("random");
    symbol_table.remove_function("randomInt");
    {
        boost::shared_ptr<func_t> ptr(new random(time));
        functions->push_back(std::make_pair("random", ptr));
    }
    {
        boost::shared_ptr<func_t> ptr(new randomInt(time));
        functions->push_back(std::make_pair("randomInt", ptr));
    }
}

EXPRTK_FUNCTIONS_NAMESPACE_EXIT

NATRON_NAMESPACE_EXIT
