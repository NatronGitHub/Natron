/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "http_parser.h"

// The bundled HTTP stack of the documentation server (libs/qhttpserver):
// http-parser on malformed and oversized requests.

namespace {

// Parses a whole request with http-parser and returns its error code (HPE_OK if it was accepted).
http_errno
parseRequest(const std::string& request)
{
    http_parser_settings settings;
    std::memset( &settings, 0, sizeof(settings) );
    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    http_parser_execute( &parser, &settings, request.data(), request.size() );

    return HTTP_PARSER_ERRNO(&parser);
}

} // anon namespace

TEST(QHttpServerParser, AcceptsValidRequest)
{
    EXPECT_EQ( HPE_OK, parseRequest("GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n") );
    EXPECT_EQ( HPE_OK, parseRequest("POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 3\r\n\r\nabc") );
}

// CVE-2018-7159: "Content-Length: 1 2" was read as 12 by http-parser < 2.8.0
TEST(QHttpServerParser, RejectsInteriorBlankInContentLength)
{
    EXPECT_EQ( HPE_INVALID_CONTENT_LENGTH, parseRequest("POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 1 2\r\n\r\n") );
}

TEST(QHttpServerParser, RejectsDuplicateContentLength)
{
    EXPECT_EQ( HPE_UNEXPECTED_CONTENT_LENGTH,
               parseRequest("POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\nab") );
}

TEST(QHttpServerParser, RejectsEmptyContentLength)
{
    EXPECT_EQ( HPE_INVALID_CONTENT_LENGTH, parseRequest("POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length:\r\n\r\n") );
}

// CVE-2019-15605: a request whose Transfer-Encoding doesn't end with "chunked" has no valid length
TEST(QHttpServerParser, RejectsTransferEncodingNotEndingWithChunked)
{
    EXPECT_EQ( HPE_INVALID_TRANSFER_ENCODING,
               parseRequest("POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked, identity\r\n\r\n1\r\na\r\n0\r\n\r\n") );
}

TEST(QHttpServerParser, RejectsContentLengthOverflow)
{
    EXPECT_EQ( HPE_INVALID_CONTENT_LENGTH,
               parseRequest("POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 99999999999999999999999\r\n\r\n") );
}

// (http-parser reports a chunk size overflow as HPE_INVALID_CONTENT_LENGTH)
TEST(QHttpServerParser, RejectsChunkSizeOverflow)
{
    EXPECT_EQ( HPE_INVALID_CONTENT_LENGTH,
               parseRequest("POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n"
                            "fffffffffffffffffffffff\r\n") );
}

// The request line and the headers are limited to HTTP_MAX_HEADER_SIZE (80 kB): a longer URL or header
// section doesn't grow the server's buffers without bound.
TEST(QHttpServerParser, RejectsOversizedUrlAndHeaders)
{
    const std::string big(100 * 1024, 'a');

    EXPECT_EQ( HPE_HEADER_OVERFLOW, parseRequest("GET /" + big + " HTTP/1.1\r\nHost: localhost\r\n\r\n") );
    EXPECT_EQ( HPE_HEADER_OVERFLOW, parseRequest("GET / HTTP/1.1\r\nHost: localhost\r\nX-Big: " + big + "\r\n\r\n") );
}
