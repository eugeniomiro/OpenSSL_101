// OpenSSL_Client.cpp : Defines the entry point for the application.
//

#include "client.h"
#include "../shared/my.h"

using namespace std;

namespace my 
{
	void send_http_request(BIO* bio, const std::string& line, const std::string& host)
	{
		std::string request = line + "\r\n";
		request += "Host: " + host + "\r\n";
		request += "\r\n";

		BIO_write(bio, request.data(), request.size());
		BIO_flush(bio);
	}
	SSL* get_ssl(BIO* bio)
	{
		SSL* ssl = nullptr;
		BIO_get_ssl(bio, &ssl);
		if (ssl == nullptr) {
			my::print_errors_and_exit("Error in BIO_get_ssl");
		}
		return ssl;
	}
	void verify_the_certificate(SSL* ssl, const std::string& expected_hostname)
	{
		int err = SSL_get_verify_result(ssl);
		if (err != X509_V_OK) {
			const char* message = X509_verify_cert_error_string(err);
			fprintf(stderr, "Certificate verification error: %s (%d)\n", message, err);
			exit(1);
		}
		X509* cert = SSL_get_peer_certificate(ssl);
		if (cert == nullptr) {
			fprintf(stderr, "No certificate was presented by the server\n");
			exit(1);
		}
		if (X509_check_host(cert, expected_hostname.data(), expected_hostname.size(), 0, nullptr) != 1) {
			fprintf(stderr, "Certificate verification error: Hostname mismatch\n");
			exit(1);
		}
	}
} // namespace my

int main()
{
	auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
	SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);

	if (SSL_CTX_set_default_verify_paths(ctx.get()) != 1) {
		my::print_errors_and_exit("Error loading trust store");
	}

	auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:443"));

	if (bio == nullptr) {
		my::print_errors_and_exit("Error in BIO_new_connect");
	}
	if (BIO_do_connect(bio.get()) <= 0) {
		my::print_errors_and_exit("Error in BIO_do_connect");
	}
	auto ssl_bio = std::move(bio) | my::UniquePtr<BIO>(BIO_new_ssl(ctx.get(), 1)); // 1 for client

	SSL_set_tlsext_host_name(my::get_ssl(ssl_bio.get()), "duckduckgo.com");
	if (BIO_do_handshake(ssl_bio.get()) <= 0) {
		my::print_errors_and_exit("Error in TLS handshake");
	}
	my::verify_the_certificate(my::get_ssl(ssl_bio.get()), "duckduckgo.com");
	my::send_http_request(ssl_bio.get(), "GET / HTTP/1.1", "duckduckgo.com");
	std::string response = my::receive_http_message(ssl_bio.get());
	printf("%s", response.c_str());	printf("%s", response.c_str());
	return 0;
}
