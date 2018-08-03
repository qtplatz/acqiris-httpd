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

#include "task.hpp"
#include "../log.hpp"
#include "../datastorage.hpp"
#include "../page_handler.hpp"
#include "digitizer.hpp"
#include <acqrscontrols/acqiris_waveform.hpp>
#include <acqrscontrols/acqiris_method.hpp>
#include <acqrscontrols/acqiris_protocol.hpp>
#include <adacquire/constants.hpp>
#include <adportable/debug.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace acqiris;
using namespace std::chrono_literals;

task::task( boost::asio::io_service& t ) : io_service_( t )
                                         , timer_( t )
                                         , strand_( t )
                                         , digitizer_( std::make_unique< digitizer >() )
                                         , tp_uptime_( std::chrono::system_clock::now() )
                                         , tp_data_handled_( tp_uptime_ )
                                         , inject_requested_( false )
                                         , acquisition_active_( false )
                                         , inject_timepoint_( 0 )
                                         , inject_serialnumber_( 0 )
{
    acquire_posted_.clear();
    digitizer_->initialize();
    digitizer_->findDevice();

    if ( auto pw = getpwuid( getuid() ) ) {
        config_dir_ = ( boost::filesystem::path( pw->pw_dir ) / ".config" ).string();
        if ( ! boost::filesystem::exists( config_dir_ ) )
            boost::filesystem::create_directory( config_dir_ );
    }
}

task::~task()
{
    stop();
    log() << "============ task dtor ==========";
}

void
task::start()
{
    acqrscontrols::aqdrv4::acqiris_method m;
    
    auto file = config_dir_ + "/dc122.json";
    if ( boost::filesystem::exists( file ) ) {

        std::ifstream in( file.c_str() );
        acqrscontrols::aqdrv4::acqiris_method::read_json( in, m );

        ADDEBUG() << "<========== json loaded";

    } else {
        auto trig = m.mutable_trig();
        auto hor = m.mutable_hor();
        auto ext = m.mutable_ext();
        auto ch1 = m.mutable_ch1();

        ADDEBUG() << "<========== json initialized";
    }

    {
        std::ofstream o( ( config_dir_ + "/startup.json" ).c_str() );
        acqrscontrols::aqdrv4::acqiris_method::write_json( o, m );
    }
    
    boost::system::error_code ec;
    timer_.expires_from_now( 3s, ec );
    timer_.async_wait( [&]( const boost::system::error_code& ec ){ on_timer( ec ); });

    prepare_for_run( m );
}

void
task::stop()
{
    timer_.cancel();
}

void
task::on_timer( const boost::system::error_code& ec )
{
    if ( ec != boost::asio::error::operation_aborted ) {
        strand_.post( [this] {
                int temp = digitizer_->readTemperature();
                ADDEBUG() << "Temperature: " << temp;
                page_handler::instance()->handle_temperature( temp );
            } );
        timer_.expires_from_now( 6s );
        timer_.async_wait( [&]( const boost::system::error_code& ec ){ on_timer( ec ); });
    }
}

void
task::prepare_for_run( const acqrscontrols::aqdrv4::acqiris_method& m )
{
    {
        std::string file( config_dir_ + "/dc122.json" );
        std::ofstream of( file.c_str() );
        m.write_json( of, m );
    }
    
    strand_.post( [=] { digitizer_->digitizer_setup( m ); } );

    if ( !std::atomic_flag_test_and_set( &acquire_posted_) )
        strand_.post( [=] { acquire(); } );
}

void
task::acquire()
{
    static int count = 0;

    if ( std::atomic_flag_test_and_set( &acquire_posted_ ) )
        strand_.post( [=] { acquire(); } );    // scedule for next acquire
    else
        std::atomic_flag_clear( &acquire_posted_ ); // keep it false
    
    using namespace std::chrono_literals;

    if ( digitizer_->acquire() ) {

        auto tp_trig = std::chrono::system_clock::now();
        
        if ( digitizer_->waitForEndOfAcquisition( 6000 ) == digitizer::success ) {
            
            static const int nbrADCBits = digitizer_->nbrADCBits();
            auto d = std::make_shared< acqrscontrols::aqdrv4::waveform >( ( nbrADCBits > 8 ) ? sizeof( int16_t ) : sizeof( int8_t ) );
            // d->setMethodNumber( methodNumber_ );

            bool success;
            if ( nbrADCBits <= 8 ) {
                success = digitizer_->readData<int8_t>( 1, d->dataDesc(), d->segDesc(), d->d() );
            } else {
                success = digitizer_->readData<int16_t>( 1, d->dataDesc(), d->segDesc(), d->d() );
            }
            
            if ( success && digitizer_->isSimulated() ) {
                uint64_t ps = std::chrono::nanoseconds( std::chrono::system_clock::now() - tp_uptime_ ).count() * 1000;
                d->segDesc().timeStampLo = int32_t( ps );
                d->segDesc().timeStampHi = int32_t( ps >> 32 );
            }

            static uint64_t serialCounter_ = 0;

            d->serialNumber() = serialCounter_++;
            if ( inject_requested_ ) {
                d->wellKnownEvents() |= adacquire::SignalObserver::wkEvent_INJECT;
                inject_timepoint_    = d->timeStamp(); // ps
                inject_serialnumber_ = d->serialNumber();
                inject_requested_    = false;
            }
            d->delayTime()          = digitizer_->delayTime();
            d->timeSinceInject()    = ( d->timeStamp() - inject_timepoint_ ) / 1000; // ps -> ns
            d->serialNumber0()      = inject_serialnumber_;
            d->timeSinceEpoch()     = std::chrono::nanoseconds( tp_trig.time_since_epoch() ).count();

            (*dataStorage::instance()) << std::move( d );

        } else {
            ADDEBUG() << "acquire timed out " << count++;
        }
    } else {
        digitizer_->stop();
        ADDEBUG() << "acquire failed. " << count++;
        std::this_thread::sleep_for( 1s );
    }
}
