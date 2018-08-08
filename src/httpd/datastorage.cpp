// -*- C++ -*-
/**************************************************************************
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

#include "datastorage.hpp"
#include "connection_manager.hpp"
#include "acqiris/task.hpp"
#include <adportable/debug.hpp>
#include <adportable/binary_serializer.hpp>
#include <acqrscontrols/acqiris_waveform.hpp>
#include <acqrscontrols/acqiris_method.hpp>
#include <acqrscontrols/acqiris_protocol.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/format.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <adportable/portable_binary_oarchive.hpp>
#include <adportable/portable_binary_iarchive.hpp>
#include <ratio>

using namespace acqiris;

dataStorage *
dataStorage::instance()
{
    static dataStorage __instance;
    return &__instance;
}

dataStorage::~dataStorage()
{
}

dataStorage::dataStorage() : id_( 0 )
                           , spin_flag_( ATOMIC_FLAG_INIT )
{
}

void
dataStorage::shutdown()
{
}

void
dataStorage::operator << ( std::shared_ptr< acqrscontrols::aqdrv4::waveform >&& data )
{
    do {
        std::lock_guard< std::mutex > lock( mutex_ );
        queue_.emplace_back( data );
    } while ( 0 );
    
    using namespace std::chrono_literals;
    auto tp = std::chrono::system_clock::now();

    if ( tp - tp_sse_commit_ >= 100ms ) {
        auto ptr( queue_.back() );
        task_->io_service().post( [ptr,this]{ gui_handler_( ptr ); } );
        tp_sse_commit_ = tp;
    }
    
    if ( ( tp - tp_blob_commit_ ) >= 250ms && !queue_.empty() ) {
        if ( spin_flag_.test_and_set() ) {
            task_->io_service().post( [&]{
                    handle_data_out();
                    spin_flag_.clear();
                });
            tp_blob_commit_ = tp;
        }
    }
}

void
dataStorage::handle_data_out()
{
    acqrscontrols::aqdrv4::waveforms vec;
    do {
        std::lock_guard< std::mutex > lock( mutex_ );
        vec.data.reserve( queue_.size() );
        std::move( queue_.begin(), queue_.end(), std::back_inserter( vec.data ) );
        queue_.clear();
    } while ( 0 );
    
    std::ostringstream o;
    adportable::binary::serialize<>()( vec, o );
    blob_handler_( "waveforms", std::to_string( id_++ ), o.str() );
}

boost::signals2::connection
dataStorage::register_blob_handler( const blob_handler_t::slot_type& subscriber )
{
    return blob_handler_.connect( subscriber );
}

boost::signals2::connection
dataStorage::register_gui_handler( const gui_handler_t::slot_type& subscriber )
{
    return gui_handler_.connect( subscriber );
}

void
dataStorage::setTask( std::shared_ptr< acqiris::task > task )
{
    task_ = task;
}
