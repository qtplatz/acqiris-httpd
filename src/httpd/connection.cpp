//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include <adportable/debug.hpp>
#include <utility>
#include <vector>
#include <iostream>

extern int __verbose_level__;
extern bool __debug_mode__;

namespace http {
namespace server {

    connection::connection(boost::asio::ip::tcp::socket socket,
                           connection_manager& manager, request_handler& handler)
        : socket_(std::move(socket))
        , connection_manager_(manager)
        , request_handler_(handler)
        , sse_connected_( false )
    {
        //busy_.clear();
    }

    void
    connection::start()
    {
        do_read();
    }

    void
    connection::stop()
    {
        socket_.close();
    }

    void
    connection::do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    request_parser::result_type result;
                    decltype( buffer_.data() ) pos; // << --- 2017.OCT.17
                    
                    // std::string debug( buffer_.data(), bytes_transferred );
                    // std::cerr << "---- connection::do_read --->\n" << debug << "\n<----- end do_read." << std::endl;
                    
                    std::tie(result, pos) = request_parser_.parse(
                        request_, buffer_.data(), buffer_.data() + bytes_transferred);
                    
                    if (result == request_parser::good) {

                        if ( size_t bsize = bytes_transferred - std::distance( buffer_.data(), pos ) ) {
                            auto it = std::find_if( request_.headers.begin(), request_.headers.end(), []( const header& h ){
                                    return ( h.name == "Content-Length" ); });
                            if ( it != request_.headers.end() ) {
                                if ( bsize < std::stol( it->value ) )
                                    return;  // continue to read
                                request_.body = std::string( pos, buffer_.data() + bytes_transferred );
                                // if ( __debug_mode__ && __verbose_level__ >= 9 ) {
                                //     for ( auto& h: request_.headers )
                                //         ADDEBUG() << h.name << ": " << h.value;
                                //     ADDEBUG() << "body: " << request_.body;
                                // }
                            }
                        }
                        
                        request_handler_.handle_request( request_, reply_ );
                        
                        auto it = std::find_if( reply_.headers.begin(), reply_.headers.end(), []( const header& h ){
                                return ( h.name == "Content-Type" ); });
                        if ( it != reply_.headers.end() && it->value == "text/event-stream" ) {
                            connection_manager_.sse_start( shared_from_this() );
                        } else if ( it != reply_.headers.end() && it->value == "blob/event-stream" ) {
                            // c++ specific entry for dataStorage
                            connection_manager_.blob_start( shared_from_this() );
                        } else {
                            do_write();
                        }
                        
                    } else if (result == request_parser::bad) {
                        reply_ = reply::stock_reply(reply::bad_request);
                        do_write();
                    } else {
                        do_read();
                    }
                } else if (ec != boost::asio::error::operation_aborted) {
                    connection_manager_.stop(shared_from_this());
                }
            });
    }

    void
    connection::do_write()
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, reply_.to_buffers(),
                                 [this, self](boost::system::error_code ec, std::size_t) {
                                     if ( !ec ) {
                                         // Initiate graceful connection closure.
                                         boost::system::error_code ignored_ec;
                                         socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                                                          ignored_ec);
                                     }
                                     
                                     if (ec != boost::asio::error::operation_aborted) {
                                         connection_manager_.stop(shared_from_this());
                                     }
                                 });
    }

    void
    connection::do_sse_write( std::shared_ptr< reply > reply )
    {
        auto self(shared_from_this());

        boost::asio::async_write(socket_
                                 , reply->to_sse_buffers()
                                 , [this, self, reply](boost::system::error_code ec, std::size_t) {
                                     if ( ec ) {
                                         std::cerr << ec.message() << std::endl;
                                         boost::system::error_code ignored_ec;
                                         socket_.shutdown( boost::asio::ip::tcp::socket::shutdown_both, ignored_ec );
                                         connection_manager_.sse_stop( shared_from_this() ); // remove
                                     }
                                 });
    }

    bool
    connection::sse_start()
    {
        reply_.headers.clear();
        reply_.status = reply::ok;
        reply_.headers.push_back( { "connection", "keep-alive" } ); 
        reply_.headers.push_back( { "Content-Type", "text/event-stream" } );
        reply_.headers.push_back( { "Cache-Control", "no-cache" } );

        // auto dbufs = reply_.to_sse_buffers();
        // for ( auto& d : dbufs )
        //     std::cerr << boost::asio::buffer_cast<const char *>(d);

        auto self(shared_from_this());
        boost::asio::async_write(socket_
                                 , reply_.to_sse_buffers( true )
                                 , [this, self](boost::system::error_code ec, std::size_t) {
                                     if ( ec )
                                         std::cerr << ec.message() << std::endl;
                                 } );
        return true;
    }
    
    bool
    connection::sse_write( const std::string& data, const std::string& id, const std::string& event )
    {
        // ---
        auto reply = std::make_shared< http::server::reply >();
        reply->headers.clear();
        reply->headers.push_back( { "data", data } );
        if ( !id.empty() )
            reply->headers.push_back( { "id", id } );

        if ( !event.empty() )
            reply->headers.push_back( { "event", event } );

        // ADDEBUG() << "sse_write( data: '" << data << "' id:'" << id << "' event: '" << event << "')";

        do_sse_write( reply );

        return true;
    }

    ///////////////////
    void
    connection::do_blob_write( std::shared_ptr< boost::asio::streambuf > blob )
    {
        auto self(shared_from_this());

        boost::asio::async_write(socket_
                                 , *blob //->to_sse_buffers()
                                 , [this, self, blob](boost::system::error_code ec, std::size_t bytes_transferred ) {
                                     if ( ec ) {
                                         std::cerr << ec.message() << std::endl;
                                         boost::system::error_code ignored_ec;
                                         socket_.shutdown( boost::asio::ip::tcp::socket::shutdown_both, ignored_ec );
                                         connection_manager_.blob_stop( shared_from_this() ); // remove
                                         ADDEBUG() << __FUNCTION__ << " blob write remove";
                                     } else {
                                         // ADDEBUG() << __FUNCTION__ << " blob wrote " << bytes_transferred << " bytes";
                                     }
                                 });
    }
    
    bool
    connection::blob_start()
    {
        reply_.headers.clear();
        reply_.status = reply::ok;
        reply_.headers.push_back( { "connection", "keep-alive" } ); 
        reply_.headers.push_back( { "Content-Type", "blob/event-stream" } );
        reply_.headers.push_back( { "Cache-Control", "no-cache" } );

        // --> debug
        ADDEBUG() << "blob_start";
        auto dbufs = reply_.to_sse_buffers();
        for ( auto& d : dbufs )
            std::cerr << boost::asio::buffer_cast<const char *>(d);
        // <-- debug
        
        auto self(shared_from_this());
        boost::asio::async_write(socket_
                                 , reply_.to_sse_buffers( true )
                                 , [this, self](boost::system::error_code ec, std::size_t) {
                                     if ( ec )
                                         std::cerr << ec.message() << std::endl;
                                 } );
        return true;
    }
    
    bool
    connection::blob_write( const std::string& key, const std::string& id, const std::string& blob )
    {
        auto envelope = std::make_shared< boost::asio::streambuf >();
        std::ostream stream( envelope.get() );

        stream << "class: " << key << "\r\n";
        stream << "id: "    << id << "\r\n";
        stream << "Content-Type: blob" << "\r\n";
        stream << "Content-Length: " << std::to_string( blob.size() ) << "\r\n\r\n";
        stream.write( blob.data(), blob.size() );

        // auto reply = std::make_shared< http::server::reply >();
        // reply->headers.clear();

        // reply->headers.push_back( { "class", key } );
        // if ( !id.empty() )
        //     reply->headers.push_back( { "id", id } );

        // reply_.headers.push_back( { "Content-Type", "blob" } );
        

        // if ( !blob.empty() ) {
        //     reply->headers.push_back( { "Content-Length", std::to_string( blob.size() ) } );
        //     reply->headers.push_back( { "blob", blob } );
        // }

        //ADDEBUG() << "blob_write( data: '" << data << "' id:'" << id << "') blob.size=" << blob.size();
        
        do_blob_write( envelope );

        return true;
    }

} // namespace server
} // namespace http
