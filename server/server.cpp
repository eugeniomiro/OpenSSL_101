#include "server.h"
#include <signal.h>
#include <stdio.h>

my::UniquePtr<BIO> my::accept_new_tcp_connection(BIO* accept_bio)
{
    if (BIO_do_accept(accept_bio) <= 0) {
        return nullptr;
    }
    return my::UniquePtr<BIO>(BIO_pop(accept_bio));
}
void my::send_http_response(BIO* bio, const std::string& body)
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "\r\n";

    BIO_write(bio, response.data(), response.size());
    BIO_write(bio, body.data(), body.size());
    BIO_flush(bio);
}
int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("USAGE: %s path-to-cert-file path-to-key-file\n", argv[0]);
        return 0;
    }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    SSL_load_error_strings();
#endif
    std::string certFile = argv[1];
    std::string keyFile = argv[2];

    auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
    SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);

    if (SSL_CTX_use_certificate_file(ctx.get(), certFile.c_str(), SSL_FILETYPE_PEM))
    {
        my::print_errors_and_exit("Error loading server certificate");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx.get(), keyFile.c_str(), SSL_FILETYPE_PEM))
    {
        my::print_errors_and_exit("Error loading server private key");
    }

    auto accept_bio = my::UniquePtr<BIO>(BIO_new_accept("8080"));
    if (BIO_do_accept(accept_bio.get()) <= 0) 
    {
        my::print_errors_and_exit("Error in BIO_do_accept (binding to port 8080)");
    }

    static auto shutdown_the_socket = [fd = BIO_get_fd(accept_bio.get(), nullptr)]() 
    {
        close(fd);
    };
    signal(SIGINT, [](int) { shutdown_the_socket(); });

    while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) 
    {
        bio = std::move(bio) | my::UniquePtr<BIO>(BIO_new_ssl(ctx.get(), 0)); // << 0 is for server, 1 is for client
        try 
        {
            std::string request = my::receive_http_message(bio.get());
            printf("Got request:\n");
            printf("%s\n", request.c_str());
            my::send_http_response(bio.get(), "okay cool\n");
        }
        catch (const std::exception& ex) 
        {
            printf("Worker exited with exception:\n%s\n", ex.what());
        }
    }
    printf("\nClean exit!\n");
}