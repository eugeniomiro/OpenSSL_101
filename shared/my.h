#ifndef MY_H
#define MY_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory>
#include <vector>

namespace my
{
	template<class T> struct DeleterOf;
	template<> struct DeleterOf<SSL_CTX> { void operator()(SSL_CTX* p) const { SSL_CTX_free(p); } };
	template<> struct DeleterOf<SSL> { void operator()(SSL* p) const { SSL_free(p); } };
	template<> struct DeleterOf<BIO> { void operator()(BIO* p) const { BIO_free_all(p); } };
	template<> struct DeleterOf<BIO_METHOD> { void operator()(BIO_METHOD* p) const { BIO_meth_free(p); } };

	template<class OpenSSLType>
	using UniquePtr = std::unique_ptr<OpenSSLType, my::DeleterOf<OpenSSLType>>;

	class StringBIO 
	{
		std::string str_;
		my::UniquePtr<BIO_METHOD> methods_;
		my::UniquePtr<BIO> bio_;

	public:
		StringBIO(StringBIO&&) = delete;
		StringBIO& operator=(StringBIO&&) = delete;

		explicit StringBIO() {
			methods_.reset(BIO_meth_new(BIO_TYPE_SOURCE_SINK, "StringBIO"));
			if (methods_ == nullptr) {
				throw std::runtime_error("StringBIO: error in BIO_meth_new");
			}
			BIO_meth_set_write(methods_.get(), [](BIO* bio, const char* data, int len) -> int {
				std::string* str = reinterpret_cast<std::string*>(BIO_get_data(bio));
				str->append(data, len);
				return len;
				});
			bio_.reset(BIO_new(methods_.get()));
			if (bio_ == nullptr) {
				throw std::runtime_error("StringBIO: error in BIO_new");
			}
			BIO_set_data(bio_.get(), &str_);
			BIO_set_init(bio_.get(), 1);
		}
		BIO* bio() { return bio_.get(); }
		std::string str()&& { return std::move(str_); }
	};

	[[noreturn]] void print_errors_and_throw(const char* message)
	{
		my::StringBIO bio;
		ERR_print_errors(bio.bio());
		throw std::runtime_error(std::string(message) + "\n" + std::move(bio).str());
	}

	[[noreturn]] void print_errors_and_exit(const char* message)
	{
		fprintf(stderr, "%s\n", message);
		ERR_print_errors_fp(stderr);
		exit(1);
	}

	std::string receive_some_data(BIO* bio)
	{
		char buffer[1024];
		int len = BIO_read(bio, buffer, sizeof(buffer));
		if (len < 0) {
			my::print_errors_and_throw("error in BIO_read");
		}
		else if (len > 0) {
			return std::string(buffer, len);
		}
		else if (BIO_should_retry(bio)) {
			return receive_some_data(bio);
		}
		else {
			my::print_errors_and_throw("empty BIO_read");
		}
	}

	std::vector<std::string> split_headers(const std::string& text)
	{
		// TODO: Improve this method with a callback with signature void cb(std::string_view), which 
		//		 we�d call for each header � thus avoiding materialization of a bunch of std::strings 
		//		 as well as avoiding materialization of a std::vector to hold them.
		std::vector<std::string> lines;
		const char* start = text.c_str();
		while (const char* end = strstr(start, "\r\n")) {
			lines.push_back(std::string(start, end));
			start = end + 2;
		}
		return lines;
	}

	std::string receive_http_message(BIO* bio)
	{
		std::string headers = my::receive_some_data(bio);
		char* end_of_headers = strstr(&headers[0], "\r\n\r\n");
		while (end_of_headers == nullptr) {
			headers += my::receive_some_data(bio);
			end_of_headers = strstr(&headers[0], "\r\n\r\n");
		}
		std::string body = std::string(end_of_headers + 4, &headers[headers.size()]);
		headers.resize(end_of_headers + 2 - &headers[0]);
		size_t content_length = 0;
		for (const std::string& line : my::split_headers(headers)) {
			if (const char* colon = strchr(line.c_str(), ':')) {
				auto header_name = std::string(&line[0], colon);
				if (header_name == "Content-Length") {
					content_length = std::stoul(colon + 1);
				}
			}
		}
		while (body.size() < content_length) {
			body += my::receive_some_data(bio);
		}
		return headers + "\r\n" + body;
	}

	UniquePtr<BIO> operator | (my::UniquePtr<BIO> lower, my::UniquePtr<BIO> upper)
	{
		BIO_push(upper.get(), lower.release());
		return upper;
	}
}

#endif // !MY_H
