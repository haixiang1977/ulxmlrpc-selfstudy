/*
 * test file for an xml-rpc client
 */

#include <ulxmlrpcpp/ulxmlrpcpp.h>  // always first header

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

#include <ulxmlrpcpp/ulxr_tcpip_connection.h>  // first, don't move: msvc #include bug
#include <ulxmlrpcpp/ulxr_ssl_connection.h>
#include <ulxmlrpcpp/ulxr_http_protocol.h>
#include <ulxmlrpcpp/ulxr_requester.h>
#include <ulxmlrpcpp/ulxr_value.h>
#include <ulxmlrpcpp/ulxr_except.h>

int main()
{
    ulxr::CppString host("localhost");
    unsigned port = 32000;

    bool secure = false;
    bool chunked = false; // http chunked transfer
    bool persistent = false; // http keep alive

    ulxr::CppString sec = ULXR_PCHAR("unsecured");
    if (secure)
    {
        sec = ULXR_PCHAR("secured");
    }
    std::cout << "Serving " << sec << " rpc requests at " << host << ":" << port << std::endl;
    std::cout << "Chunked transfer: " << chunked << std::endl;
    if (persistent)
    {
        std::cout << "persistent connection" << std::endl;
    }
    else
    {
        std::cout << "non-persistent connection" << std::endl;
    }

    // setup Tcp connection
    std::auto_ptr<ulxr::TcpIpConnection> conn;
    if (secure)
    {
#ifdef ULXR_INCLUDE_SSL_STUFF
        ulxr::SSLConnection *ssl = new ulxr::SSLConnection (false, host, port);
        ssl->setCryptographyData("password", "foo-cert.pem", "foo-cert.pem");
        conn.reset(ssl);
#endif
    }
    else
    {
        conn.reset(new ulxr::TcpIpConnection(false, host, port));
    }

    // setup protocol
    ulxr::HttpProtocol prot(conn.get());
    prot.setChunkedTransfer(chunked);
    prot.setPersistent(persistent);

    ulxr::Requester client(&prot);

    // construct method call
    ulxr::MethodCall testcallOne(ULXR_PCHAR("testcallOne"));
    ulxr::CppString str("rpc_client");
    testcallOne.setParam(ulxr::RpcString(str));

    // call remote procedure
    ulxr::MethodResponse resp;
    try
    {
        resp = client.call(testcallOne, ULXR_PCHAR("/RPC2"));

        // dump the response
        std::cout << "hostname: " << conn->getHostName() << std::endl;
        std::cout << "peername: " << conn->getPeerName() << std::endl;

        std::cout << "call result: " << resp.getXml(0) << std::endl;
    }

    catch(ulxr::Exception &ex)
    {
        std::cout << "Error occured: " << ex.why() << std::endl;
        return -1;
    }

    catch(...)
    {
        std::cout << "unknown Error occured." << std::endl;
        return -1;
    }

    std::cout << "Well done and quit" << std::endl;;

    return 0;
}
