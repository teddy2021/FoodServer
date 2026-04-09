
#include "WebScraper.hpp"
#include "Logger.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body_fwd.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <fcntl.h>
#include <optional>
#include <string>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

using std::string;
using std::vector;
using std::optional;
using std::nullopt;
using tcp = boost::asio::ip::tcp;


optional<RecipeData> WebScraper::Scrape(const std::string& url){
	Logger::GetInstance().log("[WebScraper::Scrape] scraping from " + url, debug_level::DEBUG);

	if(url.empty() || url.find_first_not_of(" ") == std::string::npos){
		Logger::GetInstance().log("[WebScraper::Scrape] empty url.", debug_level::ERROR);
		return nullopt;
	}

	string base;
	string path;

	auto splitpoint = url.find("/");
	if(splitpoint == url.size()){
		Logger::GetInstance().log("[WebScraper::Scrape] url (" + url + ") is only the base url.", debug_level::DEBUG);
		return nullopt;
	}

	base = url.substr(splitpoint);
	path = url.substr(splitpoint, url.size() - splitpoint);

	boost::asio::io_context io;
	tcp::resolver resolver(io);
	tcp::socket socket(io);

	auto const res = resolver.resolve(base, "80");
	if(res.size() == 0 ){
		Logger::GetInstance().log("[WebScraper::Scrape] failed to resolve '"+ base +"'", debug_level::ERROR);
		return nullopt;
	}

	boost::asio::connect(socket, res.begin(), res.end());
	boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, path, 11);
	req.set(boost::beast::http::field::host, base);
	req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	boost::beast::http::write(socket, req);
	
	boost::beast::flat_buffer buffer;

	boost::beast::http::response<boost::beast::http::dynamic_body> response;

	boost::beast::http::read(socket, buffer, response);

	return std::nullopt;
}
