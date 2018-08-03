/**************************************************************************
** Copyright (C) 2014-2018 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this 
** file in accordance with the MS-Cheminformatics Commercial License Agreement
** provided with the Software or, alternatively, in accordance with the terms 
** contained in a written agreement between you and MS-Cheminformatics.
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

//#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/signals2.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace  acqrscontrols { namespace aqdrv4 { class acqiris_method; class waveform; } }

namespace acqiris {

    class digitizer;

    class task : public std::enable_shared_from_this< task > {

    public:
        task( const task& ) = delete;
        task& operator = ( const task& ) = delete;

        task( boost::asio::io_service& t ); // : io_service_( t ), timer_( t ) {}
        ~task();

        void start();
        void stop();
        
        void prepare_for_run( const acqrscontrols::aqdrv4::acqiris_method& );

        typedef boost::signals2::signal< void( const acqrscontrols::aqdrv4::acqiris_method& ) > acqiris_method_adapted_t;
        typedef boost::signals2::signal< void( int ) > replyTemperature_t;

        boost::signals2::connection connect_acqiris_method_adapted( const acqiris_method_adapted_t::slot_type & subscriber );
        boost::signals2::connection connect_replyTemperature( const replyTemperature_t::slot_type & subscriber );
        void connect_push( std::function< void( std::shared_ptr< acqrscontrols::aqdrv4::waveform > ) > );

    private:
        void on_timer( const boost::system::error_code& ec );
        void acquire();
        
        boost::asio::io_service& io_service_;
        boost::asio::steady_timer timer_;
        boost::asio::io_service::strand strand_;
        std::unique_ptr< digitizer > digitizer_;
        std::atomic_flag acquire_posted_;
        const std::chrono::time_point<std::chrono::system_clock> tp_uptime_;
        std::chrono::time_point<std::chrono::system_clock> tp_data_handled_;
        bool inject_requested_;
        bool acquisition_active_;
        uint64_t inject_timepoint_;
        uint64_t inject_serialnumber_;
        std::function< void( std::shared_ptr< acqrscontrols::aqdrv4::waveform > ) > push_handler_;
        acqiris_method_adapted_t acqiris_method_adapted_;
        replyTemperature_t replyTemperature_;
        std::string config_dir_;
    };

}
