// OpenSSL_Client.cpp : Defines the entry point for the application.
//

#include "OpenSSL_Client.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory>

using namespace std;

namespace my {

	template<class T> struct DeleterOf;
	template<> struct DeleterOf<SSL_CTX> { void operator()(SSL_CTX* p) const { SSL_CTX_free(p); } };
	template<> struct DeleterOf<SSL> { void operator()(SSL* p) const { SSL_free(p); } };
	template<> struct DeleterOf<BIO> { void operator()(BIO* p) const { BIO_free_all(p); } };
	template<> struct DeleterOf<BIO_METHOD> { void operator()(BIO_METHOD* p) const { BIO_meth_free(p); } };

	template<class OpenSSLType>
	using UniquePtr = std::unique_ptr<OpenSSLType, my::DeleterOf<OpenSSLType>>;

	[[noreturn]] void print_errors_and_exit(const char* message)
	{
		fprintf(stderr, "%s\n", message);
		ERR_print_errors_fp(stderr);
		exit(1);
	}

} // namespace my


int main()
{
	auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));

	auto bio = my::UniquePtr<BIO>(BIO_new_connect("example.com:80"));
	if (BIO_do_connect(bio.get()) <= 0) {
		my::print_errors_and_exit("Error in BIO_do_connect");
	}
	return 0;
}
