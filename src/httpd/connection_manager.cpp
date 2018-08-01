/**************************************************************************
** Copyright (C) 2016-2018 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2018 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with
** the Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and MS-Cheminformatics.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.TXT included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/
// This file was modified from the original code below
// 
// connection_manager.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection_manager.hpp"
#include "page_handler.hpp"
#include "datastorage.hpp"
#include <iostream>

namespace http {
namespace server {

    connection_manager::connection_manager() : sse_connected_( false )
                                             , blob_connected_( false )
    {
    }

    connection_manager::~connection_manager()
    {
    }

    void
    connection_manager::sse_connected( bool f )
    {
        sse_connected_ = f;
    }

    void
    connection_manager::blob_connected( bool f )
    {
        blob_connected_ = f;
    }
    
    void
    connection_manager::start(connection_ptr c)
    {
        std::lock_guard< std::mutex > lock( mutex_ );
        connections_.insert(c);
        c->start();
    }

    void
    connection_manager::stop(connection_ptr c)
    {
        std::lock_guard< std::mutex > lock( mutex_ );
        connections_.erase( c );
        c->stop();
    }

    void
    connection_manager::stop_all()
    {
        std::lock_guard< std::mutex > lock( mutex_ );
        for (auto c: connections_)
            c->stop();
        connections_.clear();
    }

    void
    connection_manager::sse_start( connection_ptr c ) 
    {
        if ( !sse_connected_ ) {
            acqiris::page_handler::instance()->register_sse_handler(
                [this] ( const std::string& d, const std::string& id, const std::string& ev ){
                    sse_handler( d, id, ev );  } );
            sse_connected_ = true;
        }
        std::lock_guard< std::mutex > lock( mutex_ );
        sse_objects_.insert( c );
        connections_.erase( c );
        c->sse_start();
    }

    void
    connection_manager::sse_stop( connection_ptr c ) 
    {
        std::lock_guard< std::mutex > lock( mutex_ );        
        sse_objects_.erase( c );
        c->stop();
    }

    void
    connection_manager::sse_stop_all()
    {
        std::lock_guard< std::mutex > lock( mutex_ );
        for ( auto c: sse_objects_ )
            c->stop();
        sse_objects_.clear();
    }

    void
	connection_manager::sse_handler( const std::string& data, const std::string& id, const std::string& event )
    {
        std::lock_guard< std::mutex > lock( mutex_ );
        if ( ! sse_objects_.empty() ) {
            for ( auto c : sse_objects_ )
				c->sse_write( data, id, event );
        }
    }

    void
    connection_manager::blob_start( connection_ptr c ) 
    {
        if ( !blob_connected_ ) {
            acqiris::dataStorage::instance()->register_blob_handler(
                [this] ( const std::string& d, const std::string& id, const std::string& ev ){
                    blob_handler( d, id, ev );
                } );
            blob_connected_ = true;
        }
        std::lock_guard< std::mutex > lock( mutex_ );
        blob_objects_.insert( c );
        connections_.erase( c );
        c->blob_start();
    }

    void
    connection_manager::blob_stop( connection_ptr c ) 
    {
        std::lock_guard< std::mutex > lock( mutex_ );        
        blob_objects_.erase( c );
        c->stop();
    }

    void
    connection_manager::blob_stop_all()
    {
        std::lock_guard< std::mutex > lock( mutex_ );
        for ( auto c: blob_objects_ )
            c->stop();
        blob_objects_.clear();
    }

    void
	connection_manager::blob_handler( const std::string& data, const std::string& id, const std::string& event )
    {
        std::lock_guard< std::mutex > lock( mutex_ );
        if ( ! blob_objects_.empty() ) {
            for ( auto c : blob_objects_ )
				c->blob_write( data, id, event );
        }
    }

} // namespace server
} // namespace http
