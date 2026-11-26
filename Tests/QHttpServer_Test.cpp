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

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QMetaEnum>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "http_parser.h"
#include "qhttpserver.h"
#include "qhttprequest.h"
#include "qhttpresponse.h"

// The bundled HTTP stack of the documentation server (libs/qhttpserver):
// http-parser and QHttpServer on malformed and oversized requests.

namespace {

// A QHttpServer on 127.0.0.1 (any free port) that answers every complete request with "200 OK" and
// body "ok", and closes the connection unless the request has "Connection: keep-alive".
class LoopbackHttpServer
{
public:
    LoopbackHttpServer()
        : _requests(0)
        , _keepRequests(false)
    {
        QObject::connect( &_server, &QHttpServer::newRequest, [this](QHttpRequest* req, QHttpResponse* resp) {
            ++_requests;
            _lastHeaders = req->headers();
            _lastBody.clear();
            QObject::connect( req, &QHttpRequest::data, [this](const QByteArray& data) { _lastBody += data; } );
            QObject::connect( req, &QHttpRequest::end, [this, req, resp]() {
                if ( req->successful() ) {
                    const bool keepAlive = req->header( QString::fromUtf8("connection") ) == QString::fromUtf8("keep-alive");
                    if ( !_extraHeaderValue.isEmpty() ) {
                        resp->setHeader(QString::fromUtf8("X-Extra"), _extraHeaderValue);
                    }
                    resp->setHeader( QString::fromUtf8("Content-Length"), QString::fromUtf8("2") );
                    resp->setHeader( QString::fromUtf8("Connection"), QString::fromUtf8(keepAlive ? "keep-alive" : "close") );
                    resp->writeHead(200);
                    resp->end("ok");
                }
                // the request belongs to the handler (see QHttpServer::newRequest())
                if (_keepRequests) {
                    _keptRequests.push_back(req);
                } else {
                    req->deleteLater();
                }
            } );
        } );
        _listening = _server.listen(QHostAddress::LocalHost, 0);
    }

    bool listening() const { return _listening; }

    // The value of an "X-Extra" header in the responses
    void setExtraHeaderValue(const QString& value) { _extraHeaderValue = value; }

    void setIdleTimeout(int msecs) { _server.setIdleTimeout(msecs); }

    void setMaxConnections(int maxConnections) { _server.setMaxConnections(maxConnections); }

    // Keep the requests instead of deleting them once received (a handler may keep them)
    void setKeepRequests(bool keep) { _keepRequests = keep; }

    // The requests kept (NULL once deleted)
    const QList<QPointer<QHttpRequest> >& keptRequests() const { return _keptRequests; }

    ~LoopbackHttpServer()
    {
        for (const QPointer<QHttpRequest>& req : _keptRequests) {
            delete req.data();
        }
    }

    // The requests that the server still owns (those it didn't give to the handler)
    int ownedRequests() const { return _server.findChildren<QHttpRequest*>().size(); }

    quint16 port() { return static_cast<quint16>( _server.serverPort() ); }

    int requests() const { return _requests; }

    const HeaderHash& lastHeaders() const { return _lastHeaders; }

    const QByteArray& lastBody() const { return _lastBody; }

    // The sockets of the connections that the server still has
    int sockets() const { return _server.findChildren<QTcpSocket*>().size(); }

    // Sends first, then (after 200 ms, so that the server handles events in between) second, from a new
    // connection, and returns what the server sent back until it closed the connection (or until
    // timeoutMs).
    QByteArray exchange(const QByteArray& first, const QByteArray& second = QByteArray(), int timeoutMs = 10000)
    {
        QTcpSocket socket;
        socket.connectToHost( QHostAddress::LocalHost, static_cast<quint16>( _server.serverPort() ) );
        if ( !socket.waitForConnected(timeoutMs) ) {
            return QByteArray();
        }
        socket.write(first);
        if ( !second.isEmpty() ) {
            QTimer::singleShot( 200, &socket, [&socket, &second]() { socket.write(second); } );
        }

        QByteArray reply;
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
        QObject::connect( &socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit );
        QObject::connect( &socket, &QTcpSocket::readyRead, [&reply, &socket]() { reply += socket.readAll(); } );
        timer.start(timeoutMs);
        if (socket.state() == QAbstractSocket::ConnectedState) {
            loop.exec();
        }
        reply += socket.readAll();
        // let the server delete what it scheduled for deletion (deleteLater())
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        return reply;
    }

private:
    QHttpServer _server;
    bool _listening;
    int _requests;
    HeaderHash _lastHeaders;
    QByteArray _lastBody;
    QString _extraHeaderValue;
    bool _keepRequests;
    QList<QPointer<QHttpRequest> > _keptRequests;
};

// Runs the event loop (deleting what was scheduled for deletion) until condition() is true, for at
// most timeoutMs; returns condition()
template <typename Condition>
bool
processEventsUntil(Condition condition, int timeoutMs = 5000)
{
    QElapsedTimer timer;
    timer.start();
    while ( !condition() && (timer.elapsed() < timeoutMs) ) {
        QEventLoop loop;
        QTimer::singleShot( 10, &loop, &QEventLoop::quit );
        loop.exec();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    return condition();
}

bool
isStatus(const QByteArray& reply, int status)
{
    return reply.startsWith( "HTTP/1.1 " + QByteArray::number(status) + ' ' );
}

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

TEST(QHttpServer, AnswersValidRequest)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    const QByteArray reply = server.exchange("GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
    EXPECT_TRUE( isStatus(reply, 200) ) << reply.constData();
    EXPECT_TRUE( reply.endsWith("\r\n\r\nok") ) << reply.constData();
    EXPECT_EQ( 1, server.requests() );
}

// A malformed request was ignored and the connection stayed open: the client waited forever.
TEST(QHttpServer, RejectsMalformedRequest)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    QByteArray reply = server.exchange("NOT AN HTTP REQUEST\r\n\r\n");
    EXPECT_TRUE( isStatus(reply, 400) ) << reply.constData();

    reply = server.exchange("POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 1 2\r\n\r\n");
    EXPECT_TRUE( isStatus(reply, 400) ) << reply.constData();

    // The server closes the connection while unread data remains: on Windows, the TCP stack may reset it
    // and drop the 400. Check that the server closed the connection long before the timeout.
    QElapsedTimer timer;
    timer.start();
    reply = server.exchange("GET / HTTP/1.1\r\nHost: localhost\r\nX-Big: " + QByteArray(100 * 1024, 'a') + "\r\n\r\n");
#ifdef Q_OS_WIN
    EXPECT_TRUE( reply.isEmpty() || isStatus(reply, 400) ) << reply.constData();
#else
    EXPECT_TRUE( isStatus(reply, 400) ) << reply.constData();
#endif
    EXPECT_LT( timer.elapsed(), 5000 );

    EXPECT_EQ( 0, server.requests() );
}

// http-parser accepts "CONNECT /x", but http_parser_parse_url() can't split it (CONNECT needs host:port):
// the request was then built from an uninitialised http_parser_url (out-of-bounds reads).
TEST(QHttpServer, RejectsUrlThatCannotBeSplit)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    const QByteArray reply = server.exchange("CONNECT /x HTTP/1.1\r\nHost: localhost\r\n\r\n");
    EXPECT_TRUE( isStatus(reply, 400) ) << reply.constData();
    EXPECT_EQ( 0, server.requests() );
}

// The sockets of closed connections belonged to the QTcpServer and were only deleted with the server.
TEST(QHttpServer, DeletesClosedConnections)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE( isStatus(server.exchange("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"), 200) );
    }
    EXPECT_EQ( 0, server.sockets() );
}

// Pipelined requests: deleting the first request (the handler owns it) after the second one began
// "invalidated" the second one, whose body then crashed the server (null QHttpRequest).
TEST(QHttpServer, PipelinedRequestOutlivesDeletedRequest)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    const QByteArray reply = server.exchange("GET /1 HTTP/1.1\r\nConnection: keep-alive\r\nHost: localhost\r\n\r\n"
                                             "POST /2 HTTP/1.1\r\nHost: localhost\r\nContent-Length: 4\r\n\r\nab",
                                             "cd");
    EXPECT_EQ( 2, reply.count("HTTP/1.1 200 OK") ) << reply.constData();
    EXPECT_EQ( 2, server.requests() );
    EXPECT_EQ( QByteArray("abcd"), server.lastBody() );
}

// A header with an empty value was not stored, and the next header name was appended to its name.
TEST(QHttpServer, KeepsHeaderAfterEmptyHeader)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    EXPECT_TRUE( isStatus(server.exchange("GET / HTTP/1.1\r\nX-Empty:\r\nHost: localhost\r\n\r\n"), 200) );
    EXPECT_EQ( QString::fromUtf8("localhost"), server.lastHeaders().value( QString::fromUtf8("host") ) );
    EXPECT_TRUE( server.lastHeaders().contains( QString::fromUtf8("x-empty") ) );
    EXPECT_EQ( 2, server.lastHeaders().size() );
}

// The last header of a request was added to the next request on the same connection.
TEST(QHttpServer, DoesNotCarryHeadersToNextRequest)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    const QByteArray reply = server.exchange("GET /1 HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
                                             "GET /2 HTTP/1.1\r\nHost: localhost\r\n\r\n", QByteArray(), 2000);
    EXPECT_EQ( 2, reply.count("HTTP/1.1 200 OK") ) << reply.constData();
    // the second response closes the connection (the first one keeps it open)
    EXPECT_TRUE( reply.contains("Connection: close") ) << reply.constData();
    EXPECT_FALSE( server.lastHeaders().contains( QString::fromUtf8("connection") ) );
}

// A repeated header is one comma-separated value (RFC 7230 section 3.2.2), not the last value only.
TEST(QHttpServer, CombinesRepeatedHeaders)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    EXPECT_TRUE( isStatus(server.exchange("GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n"), 200) );
    EXPECT_EQ( QString::fromUtf8("localhost, example.com"), server.lastHeaders().value( QString::fromUtf8("host") ) );
}

// A CR or LF in a response header value would split the response.
TEST(QHttpServer, RefusesResponseHeaderWithNewline)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    server.setExtraHeaderValue( QString::fromUtf8("a\r\nX-Injected: 1") );
    const QByteArray reply = server.exchange("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    EXPECT_TRUE( isStatus(reply, 200) ) << reply.constData();
    EXPECT_FALSE( reply.contains("X-Injected") ) << reply.constData();
}

// The convenience overloads listen on the loopback interface only (upstream: all interfaces).
TEST(QHttpServer, ListensOnLoopbackByDefault)
{
    QHttpServer server;
    ASSERT_TRUE( server.listen(0) );
    const QList<QTcpServer*> tcpServers = server.findChildren<QTcpServer*>();
    ASSERT_EQ( 1, tcpServers.size() );
    EXPECT_EQ( QHostAddress(QHostAddress::LocalHost), tcpServers.first()->serverAddress() );
}

// serverPort() dereferenced a null QTcpServer when listen() had failed (DocumentationManager calls it
// to build the documentation URLs, and falls back to the online documentation when it is 0).
TEST(QHttpServer, ServerPortIsZeroWhenNotListening)
{
    QHttpServer first;
    ASSERT_TRUE( first.listen(QHostAddress::LocalHost, 0) );
    const int port = first.serverPort();
    EXPECT_NE( 0, port );

    QHttpServer notStarted;
    EXPECT_EQ( 0, notStarted.serverPort() );

    QHttpServer second; // the port is in use
    EXPECT_FALSE( second.listen( QHostAddress::LocalHost, static_cast<quint16>(port) ) );
    EXPECT_EQ( 0, second.serverPort() );
}

// QHttpRequest::HttpMethod is registered with the meta-object system (Q_ENUM: QHttpRequest::methodString())
TEST(QHttpServer, MethodNames)
{
    const QMetaEnum methods = QMetaEnum::fromType<QHttpRequest::HttpMethod>();
    ASSERT_TRUE( methods.isValid() );
    EXPECT_STREQ( "HTTP_GET", methods.valueToKey(QHttpRequest::HTTP_GET) );
    EXPECT_STREQ( "HTTP_SOURCE", methods.valueToKey(QHttpRequest::HTTP_SOURCE) );
}

// A request that the handler never got (the client left before the end of the headers) leaked.
TEST(QHttpServer, DeletesRequestOfClosedConnection)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.port());
    ASSERT_TRUE( socket.waitForConnected(10000) );
    socket.write("GET / HTTP/1.1\r\nHost: localh");
    EXPECT_TRUE( processEventsUntil([&server]() { return server.ownedRequests() == 1; }) );

    socket.abort();
    EXPECT_TRUE( processEventsUntil([&server]() { return server.ownedRequests() == 0; }) );
    EXPECT_EQ( 0, server.requests() );
}

// A parse error after a complete request (here "X") deleted that request, which is the handler's.
TEST(QHttpServer, KeepsHandlerRequestAfterParseError)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );

    server.setKeepRequests(true);
    const QByteArray reply = server.exchange("GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\nX");
    EXPECT_TRUE( isStatus(reply, 200) ) << reply.constData();
    ASSERT_EQ( 1, server.keptRequests().size() );
    EXPECT_FALSE( server.keptRequests().first().isNull() );
}

// A connection without any data was kept forever: a local program could hold any number of them.
TEST(QHttpServer, ClosesIdleConnections)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );
    server.setIdleTimeout(200);

    QTcpSocket idle;
    idle.connectToHost(QHostAddress::LocalHost, server.port());
    ASSERT_TRUE( idle.waitForConnected(10000) );
    EXPECT_TRUE( processEventsUntil([&idle]() { return idle.state() == QAbstractSocket::UnconnectedState; }) );

    // a request still gets its answer
    EXPECT_TRUE( isStatus(server.exchange("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"), 200) );
}

TEST(QHttpServer, LimitsConnections)
{
    LoopbackHttpServer server;
    ASSERT_TRUE( server.listening() );
    server.setMaxConnections(2);

    QTcpSocket first, second, third;
    for (QTcpSocket* socket : {&first, &second, &third}) {
        socket->connectToHost(QHostAddress::LocalHost, server.port());
        ASSERT_TRUE( socket->waitForConnected(10000) );
        processEventsUntil([]() { return false; }, 100); // let the server accept it
    }
    EXPECT_TRUE( processEventsUntil([&third]() { return third.state() == QAbstractSocket::UnconnectedState; }) );
    EXPECT_EQ( QAbstractSocket::ConnectedState, first.state() );
    EXPECT_EQ( QAbstractSocket::ConnectedState, second.state() );

    // once one closes, a new connection is answered
    first.abort();
    EXPECT_TRUE( processEventsUntil([&server]() { return server.sockets() == 1; }) );
    EXPECT_TRUE( isStatus(server.exchange("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"), 200) );
}
