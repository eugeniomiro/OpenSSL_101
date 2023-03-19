#include "../shared/my.h"

namespace my
{
	UniquePtr<BIO> accept_new_tcp_connection(BIO* accept_bio);
	void send_http_response(BIO* bio, const std::string& body);
}