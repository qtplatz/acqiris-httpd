/**************************************************************************
** Copyright (C) 2016,2018 MS-Cheminformatics LLC, Toin, Mie Japan
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

#pragma once

#include <boost/signals2.hpp>
#include <memory>
#include <functional>

namespace acqrscontrols { namespace aqdrv4 { class waveform; class acqiris_method; } };

namespace acqiris_gui {

    class waveform_handler {
    public:
        void operator()( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > );
        void operator()( const acqrscontrols::aqdrv4::acqiris_method& );
    };

    class callback_handler {
        callback_handler( const callback_handler& ) = delete;
        callback_handler& operator = ( const callback_handler& ) = delete;
        callback_handler();
    public:
        static callback_handler& instance();
        typedef boost::signals2::signal< void( const acqrscontrols::aqdrv4::acqiris_method& ) > acqiris_method_changed_t;
        acqiris_method_changed_t method_changed_;
    };

    class app {
    public:
        static int main( int argc, char * argv [] );
    };

}

