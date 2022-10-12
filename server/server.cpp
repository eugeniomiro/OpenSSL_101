#include "server.h"
#include <signal.h>
#include <stdio.h>

my::UniquePtr<BIO> operator |(my::UniquePtr<BIO> lower, my::UniquePtr<BIO> upper)
{
    BIO_push(upper.get(), lower.release());
    return upper;
}

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
int main()
{
    auto accept_bio = my::UniquePtr<BIO>(BIO_new_accept("8080"));
    if (BIO_do_accept(accept_bio.get()) <= 0) {
        my::print_errors_and_exit("Error in BIO_do_accept (binding to port 8080)");
    }

    static auto shutdown_the_socket = [fd = BIO_get_fd(accept_bio.get(), nullptr)]() {
        close(fd);
    };
    signal(SIGINT, [](int) { shutdown_the_socket(); });

    while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
        try {
            std::string request = my::receive_http_message(bio.get());
            printf("Got request:\n");
            printf("%s\n", request.c_str());
            my::send_http_response(bio.get(), "okay cool\n");
        }
        catch (const std::exception& ex) {
            printf("Worker exited with exception:\n%s\n", ex.what());
        }
    }
    printf("\nClean exit!\n");
}