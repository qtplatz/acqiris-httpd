// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2010-2018 Toshinobu Hondo, Ph.D.
** Copyright (C) 2013-2018 MS-Cheminformatics LLC
*
** Contact: toshi.hondo@scienceliaison.com
**
** Commercial Usage
**
** Licensees holding valid ScienceLiaison commercial licenses may use this
** file in accordance with the ScienceLiaison Commercial License Agreement
** provided with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and ScienceLiaison.
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

#pragma once

#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
#include <functional>

namespace acqrscontrols { namespace aqdrv4 { class waveform; } }

namespace acqiris {

    class page_handler {
        std::mutex mutex_;
        page_handler();
    public:
        ~page_handler();

        constexpr static const char * prefix{ "/api$" };
        constexpr static const size_t prefix_size = sizeof( "/api$" ) - 1;
        
        static page_handler * instance();
        enum { nitem = 32 };

        size_t size() const;

        void commit();
        
        // bool http_request( const std::string& method, const std::string& request_path, std::string& );
        bool http_request( const std::string& method, const std::string& request_path, const std::string& body, std::string& reply );

        void handle_tick();
        void handle_temperature( double );
        void handle_waveform( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > );
        
        typedef boost::signals2::signal< void( const std::string&, const std::string&, const std::string& ) > sse_handler_t;
        boost::signals2::connection register_sse_handler( const sse_handler_t::slot_type& );

    private:
        sse_handler_t sse_handler_;
        uint32_t event_id_;
        std::atomic_flag spin_flag_;
    };
}

