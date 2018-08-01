/**************************************************************************
** Copyright (C) 2016 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016 MS-Cheminformatics LLC, Toin, Mie Japan
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

#include "document.hpp"
#include "digitizer.hpp"
#include <acqrscontrols/acqiris_client.hpp>
#include <acqrscontrols/acqiris_waveform.hpp>
#include <acqrscontrols/acqiris_method.hpp>
#include <acqrscontrols/acqiris_protocol.hpp>
#include <QSettings>
#include <adportable/debug.hpp>
#include <adportable/portable_binary_oarchive.hpp>
#include <adportable/portable_binary_iarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>
#include <boost/filesystem/path.hpp>
#include <chrono>
#include <fstream>
#include <iostream>

static std::once_flag flag;

document *
document::instance()
{
    static document __instance;
    return &__instance;
}

document::document( QObject * parent ) : QObject( parent )
                                       , method_( std::make_shared< acqrscontrols::aqdrv4::acqiris_method >() )
                                       , settings_( std::make_unique< QSettings >( QSettings::IniFormat
                                                                                   , QSettings::UserScope
                                                                                   , QLatin1String( "acqiris" )
                                                                                   , QLatin1String( "acqiris" )
                                                        ) )
                                       , temperature_( 0 )
{
}

document::~document()
{
}

bool
document::initialSetup()
{
    boost::filesystem::path path ( settings_->fileName().toStdString() );

    auto name = path.parent_path() / "acqiris_method.xml";

    if ( auto m = load( name.string() ) ) {

        set_acqiris_method( m );

    } else {

        m = std::make_shared< acqrscontrols::aqdrv4::acqiris_method >();

        auto trig = m->mutable_trig();
        auto hor = m->mutable_hor();
        auto ch1 = m->mutable_ch1();
        auto ext = m->mutable_ext();
        set_acqiris_method( m );

    }
    return true;
}

bool
document::finalClose()
{
    signal_final_close_();

    boost::filesystem::path path ( settings_->fileName().toStdString() );
    auto name = path.parent_path() / "acqiris_method.xml";

    settings_->setValue( "DefaultMethod", QString::fromStdString( name.string() ) );
    
    save( name.string(), method_ );
    
    return true;
}

QSettings *
document::settings()
{
    return settings_.get();
}

void
document::push( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > d )
{
    std::lock_guard< std::mutex > lock( mutex_ );
    
    que_.emplace_back( d ); // push should be called in strand so that no race should be exist

    static auto tp_data_handled = std::chrono::steady_clock::now();
    static auto tp_rate_handled = std::chrono::steady_clock::now();

    using namespace std::chrono_literals;

    auto tp = std::chrono::steady_clock::now();
    if ( ( tp - tp_data_handled ) > 200ms ) {
        emit updateData();
        tp_data_handled = tp;
    }

    if ( que_.size() >= 1000 ) {
        if ( tp - tp_rate_handled > 10s ) {
            tp_rate_handled = tp;
            double rate = ( que_.back()->timeStamp() - que_.front()->timeStamp() ) / ( que_.size() - 1 );
            ADDEBUG() << "average trig. interval: " << rate / std::nano::den << "s";
        }
        que_.erase( que_.begin(), que_.begin() + ( que_.size() - 250 ) );
    }
    
}

std::shared_ptr< const acqrscontrols::aqdrv4::waveform >
document::recentWaveform()
{
    std::lock_guard< std::mutex > lock( mutex_ );
    if ( !que_.empty() )
        return que_.back();
    return nullptr;
}

std::shared_ptr< const acqrscontrols::aqdrv4::acqiris_method >
document::acqiris_method()
{
    std::lock_guard< std::mutex > lock( mutex_ );
    return method_;
}

void
document::set_acqiris_method( std::shared_ptr< acqrscontrols::aqdrv4::acqiris_method > p )
{
    std::lock_guard< std::mutex > lock( mutex_ );
    method_ = p;
}

bool
document::save( const std::string& file, std::shared_ptr< const acqrscontrols::aqdrv4::acqiris_method > p )
{
    std::wofstream of( file );
    try {
        boost::archive::xml_woarchive ar( of );
        ar & boost::serialization::make_nvp("aqdrv4", *p );
    } catch ( ... ) {
        return false;
    }
    return true;
}

std::shared_ptr< acqrscontrols::aqdrv4::acqiris_method >
document::load( const std::string& file )
{
    auto p = std::make_shared< acqrscontrols::aqdrv4::acqiris_method >();
    std::wifstream of( file );
    try {
        boost::archive::xml_wiarchive ar( of );
        ar & boost::serialization::make_nvp("aqdrv4", *p );
        return p;
    } catch ( ... ) {
        return nullptr;
    }
}

boost::signals2::connection
document::connect_prepare( const prepare_for_run_t::slot_type & subscriber )
{
    // call from tcp_task
    return signal_prepare_for_run_.connect( subscriber );
}

boost::signals2::connection
document::connect_event_out( const event_out_t::slot_type & subscriber )
{
    return signal_event_out_.connect( subscriber );
}

boost::signals2::connection
document::connect_finalize( const final_close_t::slot_type & subscriber )
{
    return signal_final_close_.connect( subscriber );
}


// mainwindow|request_handler(mediator) -> this
void
document::prepare_for_run( const acqrscontrols::aqdrv4::acqiris_method& m )
{
    set_acqiris_method( std::make_shared< acqrscontrols::aqdrv4::acqiris_method >( m ) );
}

// local ui | request_handler(mediator) -> this; a client request event out -> this -> send it to 'task' if this is server
void
document::eventOut( uint32_t event )
{
    signal_event_out_( event );
}

void
document::replyTemperature( int temp )
{
    if ( temp >= 58 )
        std::cout << "WARNING: Temperature " << temp << " too high" << std::endl;
        
    temperature_ = temp;
}
