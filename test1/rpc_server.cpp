/*
 * test file for an xml-rpc Dispatcher server
 */

#include <ulxmlrpcpp/ulxmlrpcpp.h>  // always first header

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

#include <ulxmlrpcpp/ulxr_tcpip_connection.h>  // first, don't move: msvc #include bug
#include <ulxmlrpcpp/ulxr_ssl_connection.h>
#include <ulxmlrpcpp/ulxr_http_protocol.h>
#include <ulxmlrpcpp/ulxr_except.h>
#include <ulxmlrpcpp/ulxr_signature.h>
#include <ulxmlrpcpp/ulxr_dispatcher.h>

static bool running = true;

// don't use log4j which will send debug message to log4j port 4448
// just use normal std::cout to print debug message

class TestClass
{
 public:
    static ulxr::MethodResponse testcallOne(const ulxr::MethodCall &calldata);
};

ulxr::MethodResponse TestClass::testcallOne(const ulxr::MethodCall &calldata)
{
    ulxr::MethodResponse resp;

    // get response from rpc
    ulxr::RpcString rpcs = calldata.getParam(0);
    // convert rpc string to string
    ulxr::CppString s = rpcs.getString();
    s = ulxr::CppString("Hello from xmlrpc server: ") + s;

    // convert string to rpc string and set to response
    resp.setResult(ulxr::RpcString(s));
    // send response back
    return resp;
}

int main()
{
    bool secure = false;
    ulxr::CppString host("localhost");
    unsigned port = 32000;

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
        ulxr::SSLConnection *ssl = new ulxr::SSLConnection (true, host, port);
        ssl->setCryptographyData("password", "foo-cert.pem", "foo-cert.pem");
        conn.reset(ssl);
#endif
    }
    else
    {
        conn.reset(new ulxr::TcpIpConnection(true, host, port));
    }

    // setup Http protocol
    ulxr::HttpProtocol prot(conn.get());
    prot.setChunkedTransfer(chunked);
    prot.setPersistent(persistent);
    prot.setAcceptCookies(false);

    // setup xml-rpc server
    ulxr::Dispatcher server(&prot);

    // add method
    try
    {
        // make static method
        server.addMethod(ulxr::make_method(TestClass::testcallOne),
            // signature of the return value is RpcString
            ulxr::Signature() << ulxr::RpcString(),
            // the name of the method
            ULXR_PCHAR("testcallOne"),
            // signature of the parameters
            ulxr::Signature() << ulxr::RpcString(),
            // short usage of description
            ULXR_PCHAR("Testcase with a static method in a class")
        );

        while (running)
        {
            // Waits for an incoming method call.
            // defaut timeout = 0, means no timeout
            ulxr::MethodCall call = server.waitForCall();

            std::cout << "hostname: " << conn->getHostName() << std::endl;
            std::cout << "peername: " << conn->getPeerName() << std::endl;

            // dispatch call
            ulxr::MethodResponse resp = server.dispatchCall(call);
            if (!prot.isTransmitOnly())
            {
                // send response back
                server.sendResponse(resp);
            }

            if (!prot.isPersistent())
            {
                // close the connection
                prot.close();
            }
        }
    }

    catch(ulxr::Exception& ex)
    {
        std::cout << "Error occured: " << ex.why() << std::endl;
        if (prot.isOpen())
        {
            try
            {
                ulxr::MethodResponse resp(1, ex.why());
                if (!prot.isTransmitOnly())
                {
                    server.sendResponse(resp);
                }
            }
            catch(...)
            {
                std::cout << "error within exception occured" << std::endl;
            }
            prot.close();
        }
        return -1;
    }

    std::cout << "Well done and quit" << std::endl;
    return 0;
}
