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

#include <QObject>
#include "semaphore.hpp"
#include <boost/numeric/ublas/fwd.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/signals2.hpp>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

class QSettings;

namespace acqrscontrols {
namespace aqdrv4 {

    class acqiris_client;
    class acqiris_method;
    class waveform;

    enum SubMethodType : unsigned int;
}
}

class AcqirisWidget;

class document : public QObject {
    
    Q_OBJECT
    ~document();
    document( const document& ) = delete;
    document( QObject * parent = 0 );
public:
    static document * instance();

    bool initialSetup();
    bool finalClose();
    QSettings * settings();

    void push( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > );
    std::shared_ptr< const acqrscontrols::aqdrv4::waveform > recentWaveform();

    std::shared_ptr< const acqrscontrols::aqdrv4::acqiris_method > acqiris_method();
    void set_acqiris_method( std::shared_ptr< acqrscontrols::aqdrv4::acqiris_method > );

    static bool save( const std::string& file, std::shared_ptr< const acqrscontrols::aqdrv4::acqiris_method > );
    static std::shared_ptr< acqrscontrols::aqdrv4::acqiris_method > load( const std::string& file );

    void prepare_for_run( const acqrscontrols::aqdrv4::acqiris_method& );
    void eventOut( uint32_t );
    void replyTemperature( int );

    typedef boost::signals2::signal< void( const acqrscontrols::aqdrv4::acqiris_method& ) > prepare_for_run_t;
    typedef boost::signals2::signal< void( uint32_t ) > event_out_t;
    typedef boost::signals2::signal< void() > final_close_t;
    
    boost::signals2::connection connect_prepare( const prepare_for_run_t::slot_type & subscriber );
    boost::signals2::connection connect_event_out( const event_out_t::slot_type & subscriber );
    boost::signals2::connection connect_finalize( const final_close_t::slot_type & subscriber );
    void drawWaveformCompleted();
    
signals:
    void updateData();
    void on_acqiris_method_adapted();

private:
    std::mutex mutex_;
    std::shared_ptr< const acqrscontrols::aqdrv4::waveform > que_;
    std::shared_ptr< acqrscontrols::aqdrv4::acqiris_method > method_;
    std::shared_ptr< acqrscontrols::aqdrv4::acqiris_method > adapted_method_;
    std::unique_ptr< QSettings > settings_;
    std::vector< std::thread > tcp_threads_;
    int temperature_;
    prepare_for_run_t signal_prepare_for_run_;
    final_close_t signal_final_close_;
    event_out_t signal_event_out_;
    std::atomic_flag lock_flag_;
};
