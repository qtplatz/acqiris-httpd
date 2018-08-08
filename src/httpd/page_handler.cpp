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

#include "config.h"
#include "page_handler.hpp"
#include "log.hpp"
#include <adportable/debug.hpp>
#include <acqrscontrols/acqiris_waveform.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

extern bool __debug_mode__;
extern int __verbose_level__;
extern bool __simulate__;

using namespace acqiris;

page_handler::page_handler() : event_id_( 0 )
                             , spin_flag_( ATOMIC_FLAG_INIT )
{
}

page_handler::~page_handler()
{
}

page_handler *
page_handler::instance()
{
    static page_handler __instance;
    return &__instance;
}

size_t
page_handler::size() const
{
    return 0; //pulses_.size();
}

void
page_handler::commit()
{
}

bool
page_handler::http_request( const std::string& method, const std::string& request_path, const std::string& body, std::string& rep )
{
    if ( __verbose_level__ >= 7 )
        ADDEBUG() << __FUNCTION__ << " " << method << "; " << request_path;
    
    if ( request_path.find( "/api$events" ) != std::string::npos ) {
        rep = "SSE";
        return true;
    } else if ( request_path.find( "/api$banner" ) != std::string::npos ) {
        rep += ( boost::format( "<h2>Waveform V%s</h2>" ) % PACKAGE_VERSION ).str();
        return true;
    } else {
        std::ostringstream o;
        o << "page_handler -- unknown request(" << method << ", " << request_path << ")";
        rep = o.str();
        
    }
    return true;
}

boost::signals2::connection 
page_handler::register_sse_handler( const sse_handler_t::slot_type& subscriber )
{
    return sse_handler_.connect( subscriber );
}


void
page_handler::handle_tick()
{
    struct timespec tp;
    
    if ( clock_gettime( CLOCK_MONOTONIC, &tp ) == 0 ) {
        boost::property_tree::ptree tick;
        uint64_t timepoint = uint64_t( tp.tv_sec ) * std::nano::den + tp.tv_nsec;
        tick.put( "tick.time", tp.tv_sec );
        tick.put( "tick.nsec", tp.tv_nsec );
        std::ostringstream o;
        boost::property_tree::write_json( o, tick, false );
        std::string json = o.str().substr( 0, o.str().find_first_of( "\r\n" ) );
        sse_handler_( json, std::to_string( event_id_++ ), "tick" );   // data, id, evnet
    }
}

void
page_handler::handle_temperature( double temp )
{
    struct timespec tp;
    
    if ( clock_gettime( CLOCK_MONOTONIC, &tp ) == 0 ) {
        boost::property_tree::ptree tick;
        uint64_t timepoint = uint64_t( tp.tv_sec ) * std::nano::den + tp.tv_nsec;
        tick.put( "tick.time", tp.tv_sec );
        tick.put( "tick.nsec", tp.tv_nsec );
        tick.put( "tick.temp", temp );
        std::ostringstream o;
        boost::property_tree::write_json( o, tick, false );
        std::string json = o.str().substr( 0, o.str().find_first_of( "\r\n" ) );
        sse_handler_( json, std::to_string( event_id_++ ), "tick" );   // data, id, evnet
    }
}

struct json_waveform {
#if 1    
    std::string operator()( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > d ) {

        QJsonObject j_meta;
        
        const size_t ini = d->dataDesc().indexFirstPoint;

        j_meta[ "serialNumber" ] = QString::number( d->serialNumber() );
        j_meta[ "size" ] = QString::number( d->size() );
        j_meta[ "delayTime" ] = d->delayTime();
        j_meta[ "xMin" ] = d->delayTime();
        j_meta[ "xMax" ] = d->delayTime() + d->size() * d->dataDesc().sampTime;
        j_meta[ "xIncrement" ] = d->xIncrement();
        j_meta[ "timeStamp" ] = QString::number( d->timeStamp() );
        j_meta[ "vOffset" ] = d->vOffset();
        j_meta[ "vGain" ] = d->vGain();
        j_meta[ "wellKnownEvents" ] = QString::number( d->wellKnownEvents() );
        j_meta[ "timeSinceEpoch" ] = QString::number( d->timeSinceEpoch() );
        j_meta[ "timeSinceInject" ] = QString::number( d->timeSinceInject() );
        j_meta[ "clsid" ] = QString::fromStdString( boost::lexical_cast< std::string >( d->clsid() ) );

        QJsonObject j_waveform, j_obj;
        QJsonArray j_data;
        
        std::vector< double > y( d->size() );
        acqrscontrols::aqdrv4::waveform::transform( y, *d, 1000 ); // ->mV
        std::for_each( y.begin(), y.end(), [&](auto d){ j_data.append( d ); });

        j_waveform[ "meta" ] = j_meta;
        j_waveform[ "data" ] = j_data;
        j_obj[ "waveform" ] = j_waveform;

        QJsonDocument j( j_obj );

        QByteArray xdata( j.toJson( QJsonDocument::Compact ) );

        return std::string ( xdata.data() );
    }
#else
    std::string operator()( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > d ) {

        boost::property_tree::ptree pt, wform;

        const size_t ini = d->dataDesc().indexFirstPoint;
        
        wform.put( "meta.serialNumber", d->serialNumber() );
        wform.put( "meta.size", d->size() );
        wform.put( "meta.delayTime", d->delayTime() );
        wform.put( "meta.xMin", d->delayTime() );
        wform.put( "meta.xMax", d->delayTime() + d->size() * d->dataDesc().sampTime );
        wform.put( "meta.xIncrement", d->xIncrement() );
        wform.put( "meta.timeStamp", d->timeStamp() );
        wform.put( "meta.vOffset", d->vOffset() );
        wform.put( "meta.vGain", d->vGain() );
        wform.put( "meta.wellKnownEvents", d->wellKnownEvents() );
        wform.put( "meta.timeSinceEpoch", d->timeSinceEpoch() );
        wform.put( "meta.timeSinceInject", d->timeSinceInject() );
        wform.put( "meta.clsid", d->clsid() );

        std::vector< double > y( d->size() );
        acqrscontrols::aqdrv4::waveform::transform( y, *d, 1000 ); // ->mV
        size_t idx(0);
        boost::property_tree::ptree data;
        std::for_each( y.begin(), y.end(), [&](auto d){
                boost::property_tree::ptree datum;
                datum.put( "", d );
                data.push_back( std::make_pair( "", datum ) );
            });
        wform.add_child( "data", data );
        pt.add_child( "waveform", wform );

        std::ostringstream o;
        boost::property_tree::write_json( o, pt, false );

        return o.str().substr( 0, o.str().find_first_of( "\r\n" ) );
    }
#endif
};

void
page_handler::handle_waveform( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > waveform )
{
    if ( spin_flag_.test_and_set() ) {
        auto p = std::async( std::launch::async, [=]{ return json_waveform()( waveform ); });
        auto json = p.get();
        sse_handler_( json, std::to_string( event_id_++ ), "wave" );   // data, id, evnet
        spin_flag_.clear();
    }
}

