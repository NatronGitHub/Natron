#ifndef YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "include/parser.h"
#include "include/emitter.h"
#include "include/emitterstyle.h"
#include "include/stlemitter.h"
#include "include/exceptions.h"

#include "include/node/node.h"
#include "include/node/impl.h"
#include "include/node/convert.h"
#include "include/node/iterator.h"
#include "include/node/detail/impl.h"
#include "include/node/parse.h"
#include "include/node/emit.h"

#include "include/version.h"

#endif  // YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66
