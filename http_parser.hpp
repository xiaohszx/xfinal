#pragma once
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <algorithm>
#include "utils.hpp"
#include "code_parser.hpp"
#include "string_view.hpp"
namespace xfinal {
	enum class parse_state :std::uint8_t
	{
		valid,
		invalid,
		unknow
	};

	class request_meta {
	public:
		request_meta() = default;
		request_meta(request_meta const&) = default;
		request_meta(request_meta &&) = default;
		request_meta& operator=(request_meta const&) = default;
		request_meta& operator=(request_meta &&) = default;
		request_meta(std::string&& method, std::string&& url, std::map<std::string, std::string>&& headers):method_(std::move(method)), url_(std::move(url)), headers_(std::move(headers)){

		}
	public:
		std::string method_;
		std::string url_;
		std::map<std::string, std::string> headers_;
		std::map<nonstd::string_view, nonstd::string_view> form_map_;
		std::string body_;
		std::string decode_body_;
	};

	class http_parser_header final {
	public:
		http_parser_header(std::vector<char>::iterator begin, std::vector<char>::iterator end) :begin_(begin), end_(end) {

		}
		std::pair<parse_state, bool> is_complete_header() {
			auto current = begin_;
			header_begin_ = begin_;
			while (current != end_) {
				if (*current == '\r' && *(current + 1) == '\n' && *(current + 2) == '\r' && *(current + 3) == '\n') {
					header_end_ = current +4;
					return { parse_state::valid,true };
				}
				++current;
			}
			return { parse_state::invalid,false};
		}
		std::pair < parse_state, std::string> get_method() {
			auto start = begin_;
			while (begin_ != end_) {
				//auto c = *begin_;
				//auto c_next = *(begin_ + 1);
				if ((*begin_) == ' ' && (*(begin_ + 1)) !=' ') {
					return { parse_state::valid,std::string(start, begin_++) };
				}
				++begin_;
			}
			return { parse_state::invalid,""};
		}
		std::pair < parse_state, std::string> get_url() {
			auto start = begin_;
			while (begin_ != end_) {
				if ((*begin_) == ' ' && (*(begin_ + 1)) != ' ') {
					return { parse_state::valid,std::string(start, begin_++) };
				}
				++begin_;
			}
			return { parse_state::invalid,"" };
		}
		std::pair < parse_state, std::string> get_version() {
			auto start = begin_;
			while (begin_ != end_) {
				if ((*begin_) == '\r' && (*(begin_ + 1)) == '\n') {
					begin_ += 2;
					return { parse_state::valid ,std::string(start,begin_-2) };
				}
				++begin_;
			}
			return { parse_state::invalid,"" };
		}
		std::pair < parse_state, std::map<std::string, std::string>> get_header() {
			std::map<std::string, std::string> headers;
			auto start = begin_;
			while (begin_ != end_) {
				if ((*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //maybe a header = > key:value
					auto old_start = start;
					while (start != begin_) {
						if ((*start) == ':') {
							auto key = std::string(old_start, start);
							++start;
							while ((*start) == ' ') {
								++start;
							}
							headers.emplace(to_lower(std::move(key)), std::string(start, begin_));
							begin_ += 2;
							start = begin_;
							break;
						}
						++start;
					}
					if (start != begin_) {
						return { parse_state::invalid ,{} };
					}
				}
				++begin_;
			}
			return { parse_state::valid ,headers };
		}
		std::pair<bool, request_meta> parse_request_header() {
			auto method = get_method();
			if (method.first == parse_state::valid) {
				auto url = get_url();
				if (url.first == parse_state::valid) {
					auto version = get_version();
					if (version.first == parse_state::valid) {
						auto headers = get_header();
						if (headers.first == parse_state::valid) {
							return { true,{std::move(method.second),std::move(url.second),std::move(headers.second)} };
						}
						else {
							return { false,request_meta() };
						}
					}
					else {
						return { false,request_meta() };
					}
				}
				else {
					return { false,request_meta() };
				}
			}
			else {
				return { false,request_meta() };
			}
		}
		std::size_t get_header_size() {
			return (header_end_ - header_begin_);
		}
	private:
		std::vector<char>::iterator begin_;
		std::vector<char>::iterator end_;
		std::vector<char>::iterator header_end_;
		std::vector<char>::iterator header_begin_;
	};

	class http_urlform_parser final {
	public:
		http_urlform_parser(std::string& body){
			body = xfinal::get_string_by_urldecode(body);
			begin_ = body.begin();
			end_ = body.end();
		}
		void parse_data(std::map<nonstd::string_view, nonstd::string_view>& form) {
			auto old = begin_;
			while (begin_ != end_) {
				if ((begin_ + 1) == end_) {
					begin_++;
					parse_key_value(form, old);
					break;
				}
				if ((*begin_) == '&') {
					parse_key_value(form, old);
					++begin_;
					old = begin_;
				}
				++begin_;
			}
		}
	protected:
		void parse_key_value(std::map<nonstd::string_view, nonstd::string_view>& form, std::string::iterator old) {
			auto start = old;
			while (old != begin_) {
				if ((*old) == '=') {
					auto key = nonstd::string_view(&(*start), old -start);
					form.insert(std::make_pair(key, nonstd::string_view(&(*(old + 1)), begin_-1 - old)));
				}
				++old;
			 }
		}
	private:
		std::string::iterator begin_;
		std::string::iterator end_;
	};
}