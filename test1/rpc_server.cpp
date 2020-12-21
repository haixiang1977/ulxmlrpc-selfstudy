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
    ulxr::HttpProtocol prot(conn.get())
    prot.setChunkedTransfer(chunked);
    prot.setPersistent(persistent);
    prot.setAcceptCookies(false);

    // setup xml-rpc server
    ulxr::Dispatcher server(&prot);

    // add method
    try
    {
        server.addMethod();

        server.addMethod();

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

