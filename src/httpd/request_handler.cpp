//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "page_handler.hpp"
#include "datastorage.hpp"
#include "connection.hpp"
#include "connection_manager.hpp"
#include "mime_types.hpp"
#include "request_handler.hpp"
#include "reply.hpp"
#include "request.hpp"
#include <adportable/debug.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

extern int __verbose_level__;

namespace http {
namespace server {

request_handler::request_handler( const std::string& doc_root )
    : doc_root_(doc_root)
{
}

void
request_handler::handle_request( const request& req, reply& rep )
{
    // Decode url to path.
    std::string request_path;
    if (!url_decode(req.uri, request_path)) {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }

    // SSE and c++ api -- intercept if request start with '/api$'
    if ( request_path.find( "/api$" ) != std::string::npos ) {

        acqiris::page_handler::instance()->http_request( req.method, request_path, req.body, rep.content );
        rep.status = reply::ok;
        
        rep.headers.push_back( { "Content-Length", std::to_string(rep.content.size()) } );
        rep.headers.push_back( { "Content-Type", "text/json" } );

        if ( request_path.find( "/api$events" ) != std::string::npos ) { 
            rep.headers[1] = { "Content-Type", "text/event-stream" };
            rep.headers.push_back( { "Cache-Control", "no-cache" } );
            rep.headers.push_back( { "connection", "keep-alive" } );            
        }

        // c++ api
        if ( request_path.find( "/api$blob" ) != std::string::npos ) {
            rep.status = reply::ok;
            rep.headers.push_back( { "Content-Length", std::to_string(rep.content.size()) } );
            rep.headers.push_back( { "Content-Type", "blob" } );
            rep.headers[1] = { "Content-Type", "blob/event-stream" };
            rep.headers.push_back( { "Cache-Control", "no-cache" } );
            rep.headers.push_back( { "connection", "keep-alive" } );            
            return;
        }

        return;
    }
    
    // standard http protocol below
    
    // Request path must be absolute and not contain "..".
    if (request_path.empty() || request_path[0] != '/'
        || request_path.find("..") != std::string::npos)   {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }

    // If path ends in slash (i.e. is a directory) then add "index.html".
    if (request_path[request_path.size() - 1] == '/')  {
        request_path += "index.html";
    }

    // Determine the file extension.
    std::size_t last_slash_pos = request_path.find_last_of("/");
    std::size_t last_dot_pos = request_path.find_last_of(".");
    std::string extension;
    if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)   {
        extension = request_path.substr(last_dot_pos + 1);
    }

    // Open the file to send back.
    std::string full_path = doc_root_ + request_path;
    std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!is) {
        rep = reply::stock_reply(reply::not_found);
        return;
    }

    // Fill out the reply to be sent to the client.
    rep.status = reply::ok;
    char buf[4096];

    while (is.read(buf, sizeof(buf)).gcount() > 0)
        rep.content.append(buf, is.gcount());

    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = std::to_string(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = mime_types::extension_to_type(extension);
}

bool
request_handler::url_decode(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i)    {
        if ( in[i] == '%') {
            if ( i + 3 <= in.size() ) {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value)                {
                    out += static_cast<char>(value);
                    i += 2;
                } else {
                    return false;
                }
            } else  {
                return false;
            }
        }
        else if (in[i] == '+')  {
            out += ' ';
        } else  {
            out += in[i];
        }
    }
    return true;
}

} // namespace server
} // namespace http