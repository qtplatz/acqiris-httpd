// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2013-2018 MS-Cheminformatics LLC
*
** Contact: toshi.hondo@qtplatz.com
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
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace acqrscontrols { namespace aqdrv4 { class waveform; class acqiris_method; } }

namespace acqiris {

    class dataStorage {
        dataStorage();
        ~dataStorage();
    public:
        constexpr static const char * const prefix{ "/dataStorage" };
        
        static dataStorage * instance();
        void shutdown();

        void operator << ( std::shared_ptr< acqrscontrols::aqdrv4::waveform >&& );

        typedef boost::signals2::signal< void( const std::string&, const std::string&, const std::string& ) > blob_handler_t;
        typedef boost::signals2::signal< void( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > ) > gui_handler_t;
        
        boost::signals2::connection register_blob_handler( const blob_handler_t::slot_type& );
        boost::signals2::connection register_gui_handler( const gui_handler_t::slot_type& );

    private:
        std::mutex mutex_;
        std::vector< std::shared_ptr< acqrscontrols::aqdrv4::waveform > > queue_;
        blob_handler_t blob_handler_;
        gui_handler_t gui_handler_;
        std::chrono::system_clock::time_point tp_blob_commit_;
        uint32_t id_;
    };
}

