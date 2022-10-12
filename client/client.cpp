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
} // namespace my


int main()
{
	auto bio = my::UniquePtr<BIO>(BIO_new_connect("localhost:8080"));

	if (bio == nullptr) {
		my::print_errors_and_exit("Error in BIO_new_connect");
	}
	if (BIO_do_connect(bio.get()) <= 0) {
		my::print_errors_and_exit("Error in BIO_do_connect");
	}
	my::send_http_request(bio.get(), "GET / HTTP/1.1", "duckduckgo.com");
	std::string response = my::receive_http_message(bio.get());
	printf("%s", response.c_str());
	return 0;
}
