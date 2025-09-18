
#include <string>
#include <vector>
using std::string;

const std::vector<string> request_methods {"GET", "HEAD", "OPTIONS", "TRACE", "PUT", "DELETE", "POST", "PATCH", "CONNECT"};

class httpreq{

	private:
		string manipulations;
		string media;
		string charset;
		string datetime;
		string encoding;
		string language;
		string ac_method;
		string ac_headers;
		string auth;
		string cache_ctrl;
		string conn;
		string cont_enc;
		string cont_len;
		string cont_md5;
		string cont_type;
		string cookie;
		string date;
		string expect;
		string forwarded;
		string email_from;
		string host;
		string http2_settings;
		string if_match;
		string if_mod_since;
		string ifn_match;
		string if_range;
		string ifn_mod_since;
		string max_forwards;
		string origin;
		string pragma;
		string prefer;
		string proxy_auth;
		string referer;
		string TE;
		string trailer;
		string trans_enc;
		string user_agent;
		string upgrade;
		string via;
		string warn;

		string custom;
	public:
		httpreq(){return;};
		httpreq(string request);

		string Manipulations();
		string Media();
		string Charset();
		string Datetime();
		string Encoding();
		string Language();
		string Ac_method();
		string Ac_headers();
		string Auth();
		string Cache_ctrl();
		string Conn();
		string Cont_enc();
		string Cont_len();
		string Cont_md5();
		string Cont_type();
		string Cookie();
		string Date();
		string Expect();
		string Forwarded();
		string Email_from();
		string Host();
		string Http2_settings();
		string If_match();
		string If_mod_since();
		string Ifn_match();
		string If_range();
		string Ifn_mod_since();
		string Max_forwards();
		string Origin();
		string Pragma();
		string Prefer();
		string Proxy_auth();
		string Referer();
		string tE();
		string Trailer();
		string Trans_enc();
		string User_agent();
		string Upgrade();
		string Via();
		string Warn();

		string Custom();
};
