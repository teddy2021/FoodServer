#include "httpreq.hpp"

#include <vector>
#include <string>
#include <map>
using std::string;
using std::vector;
using std::map;


httpreq::httpreq(string request){

	size_t pos_start = 0;
	size_t pos_end = 1;
	string token;
	vector<string> fields;

	string delim1("\r\n");
	while((pos_end = request.find("\r\n"))){
		token = request.substr(pos_start, pos_end);
		pos_end = pos_start + delim1.size();
		fields.push_back(token);
	}

	map<string, string> headers;

	for(int x = 0; x < fields.size(); x += 1){
	pos_start = 0;
	pos_end = fields[x].size();
	token = "";
		pos_start = request.find(":");
		token = request.substr(pos_start, pos_end - pos_start);
		headers.insert({fields[x].substr(0,pos_start), token});		
	}

	manipulations = string(headers["A-IM"]);
	media = string(headers["Accept"]);
	charset = string(headers["Accept-Charset"]);
	datetime = string(headers["Accept-Datetime"]);
	encoding = string(headers["Accept-Encoding"]);
	language = string(headers["Accept-Language"]);
	ac_method = string(headers["Access-Control-Request-Method"]);
	ac_headers = string(headers["Access-Control-Request-Headers"]);
	auth = string(headers["Authorizaton"]);
	cache_ctrl = string(headers["Cache-Control"]);
	conn = string(headers["Connection"]);
	cont_enc = string(headers["Content-Encoding"]);
	cont_len = string(headers["Content-Length"]);
	cont_md5 = string(headers["Content-MD5"]);
	cont_type = string(headers["Content-Type"]);
	cookie = string(headers["Cookie"]);
	date = string(headers["Date"]);
	expect = string(headers["Expect"]);
	forwarded = string(headers["Forwarded"]);
	email_from = string(headers["From"]);
	host = string(headers["Host"]);
	http2_settings = string(headers["HTTP2-Settings"]);
	if_match = string(headers["If-Match"]);
	if_mod_since = string(headers["If-Modified-Since"]);
	ifn_match = string(headers["If-None-Match"]);
	if_range = string(headers["If-Range"]);
	ifn_mod_since = string(headers["If-Unmodified-Since"]);
	max_forwards = string(headers["Max-Forwards"]);
	origin = string(headers["Origin"]);
	pragma = string(headers["Pragma"]);
	prefer = string(headers["Prefer"]);
	proxy_auth = string(headers["Proxy-Authorization"]);
	referer = string(headers["Referer"]);
	TE = string(headers["TE"]);
	trailer = string(headers["Trailer"]);
	trans_enc = string(headers["Transfer-Encoding"]);
	user_agent = string(headers["User-Agent"]);
	upgrade = string(headers["Upgrade"]);
	via = string(headers["Via"]);
	warn = string(headers["Warning"]);

	custom = string(headers["Custom"]);
};



string httpreq::Manipulations(){ return manipulations; };
string httpreq::Media(){ return media; };
string httpreq::Charset(){ return charset; };
string httpreq::Datetime(){ return datetime; };
string httpreq::Encoding(){ return encoding; };
string httpreq::Language(){ return language; };
string httpreq::Ac_method(){ return ac_method; };
string httpreq::Ac_headers(){ return ac_headers; };
string httpreq::Auth(){ return auth; };
string httpreq::Cache_ctrl(){ return cache_ctrl; };
string httpreq::Conn(){ return conn; };
string httpreq::Cont_enc(){ return cont_enc; };
string httpreq::Cont_len(){ return cont_len; };
string httpreq::Cont_md5(){ return cont_md5; };
string httpreq::Cont_type(){ return cont_type; };
string httpreq::Cookie(){ return cookie; };
string httpreq::Date(){ return date; };
string httpreq::Expect(){ return expect; };
string httpreq::Forwarded(){ return forwarded; };
string httpreq::Email_from(){ return email_from; };
string httpreq::Host(){ return host; };
string httpreq::Http2_settings(){ return http2_settings; };
string httpreq::If_match(){ return if_match; };
string httpreq::If_mod_since(){ return if_mod_since; };
string httpreq::Ifn_match(){ return ifn_match; };
string httpreq::If_range(){ return if_range; };
string httpreq::Ifn_mod_since(){ return ifn_mod_since; };
string httpreq::Max_forwards(){ return max_forwards; };
string httpreq::Origin(){ return origin; };
string httpreq::Pragma(){ return pragma; };
string httpreq::Prefer(){ return prefer; };
string httpreq::Proxy_auth(){ return proxy_auth; };
string httpreq::Referer(){ return referer; };
string httpreq::tE(){ return TE; };
string httpreq::Trailer(){ return trailer; };
string httpreq::Trans_enc(){ return trans_enc; };
string httpreq::User_agent(){ return user_agent; };
string httpreq::Upgrade(){ return upgrade; };
string httpreq::Via(){ return via; };
string httpreq::Warn(){ return warn; };

string httpreq::Custom(){return custom;};
