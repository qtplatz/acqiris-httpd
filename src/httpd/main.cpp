//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
/**************************************************************************
** Copyright (C) 2014-2016 Toshinobu Hondo, for delay generator
** Contact: toshi.hondo@qtplatz.com or info@ms-cheminfo.com
*/

#include "config.h"
#include "log.hpp"
#include "server.hpp"
#include "datastorage.hpp"
#include "page_handler.hpp"
#include "acqiris/task.hpp"
#if HAVE_QT5
# include <acqiris-gui/app.hpp>
#endif
#include <acqrscontrols/acqiris_method.hpp>
#include <adportable/debug.hpp>
#include <eventbroker/eventbroker.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <memory>
#include <string>

int __verbose_level__ = 0;
bool __debug_mode__ = false;
bool __simulate__ = false;

int
main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    
    try {
        po::variables_map vm;
        
        po::options_description desc("options");
        desc.add_options()
            ( "help,h",  "print help message" )
            ( "version", "print version number" )            
            ( "port", po::value<std::string>()->default_value("80"), "http port number" )
            ( "recv", po::value<std::string>()->default_value("0.0.0.0"), "For IPv4, try 0.0.0.0, IPv6, try 0::0" )
            ( "doc_root", po::value<std::string>()->default_value( DOC_ROOT ), "document root" )
            ( "verbose", po::value<int>()->default_value(0), "verbose level" )
            ( "debug,d", "debug mode" )
            ( "simulate", "simulate mode" )            
            ;
        po::store( po::command_line_parser( argc, argv ).options( desc ).allow_unregistered().run(), vm );
        po::notify( vm );

        if ( vm.count( "help" ) ) {
            std::cout << desc;
            return 0;
        } else if ( vm.count( "version" ) ) {
            std::cout << PACKAGE_VERSION << std::endl;
            return 0;
        }

        __debug_mode__ = vm.count( "debug" ) > 0 ;
        __simulate__ = vm.count( "simulate" ) > 0 ;
        
        if ( ! __debug_mode__ ) {
            int fd = open( PID_NAME, O_RDWR|O_CREAT, 0644 );
            if ( fd < 0 ) {
                std::cerr << "Can't open " PID_NAME << std::endl;
                exit(1);
            }
            int lock = lockf( fd, F_TLOCK, 0 );
            if ( lock < 0 ) {
                std::cerr << "Process " << argv[0] << " already running" << std::endl;
                exit(1);
            }
            std::ostringstream o;
            o << getpid() << std::endl;
            write( fd, o.str().c_str(), o.str().size() );
        }

        boost::asio::io_service io_service;
        std::vector< std::thread > threads;
        
        auto task = std::make_shared< acqiris::task >( io_service );
        task->start();

        threads.emplace_back( [&]{ io_service.run(); } );
        threads.emplace_back( [&]{ io_service.run(); } );

        // Initialise the server.
        acqiris::log() << boost::format( "started on %1% %2% %3%" )
            % vm["recv"].as< std::string >()
            % vm["port"].as< std::string >()
            % vm["doc_root"].as< std::string >();

        __verbose_level__ = vm["verbose"].as< int >();

        http::server::server s( vm["recv"].as< std::string >().c_str()
                                , vm["port"].as< std::string >().c_str()
                                , vm["doc_root"].as< std::string >().c_str() );

        threads.emplace_back( [&]{ s.run(); } );
        using acqiris::page_handler;
        using acqiris::dataStorage;
        dataStorage::instance()->register_gui_handler( [&](auto w){ page_handler::instance()->handle_waveform( w ); } );
        
#if HAVE_QT5
        acqiris_gui::waveform_handler sender;
        dataStorage::instance()->register_gui_handler( sender );
        acqiris_gui::callback_handler::instance().method_changed_.connect( [&](auto& m){ task->prepare_for_run( m ); });
        
        acqiris_gui::app::main( argc, argv );
        
        s.io_service().stop();
        io_service.stop();
#endif
        for ( auto& t: threads )
            t.join();

    }  catch (std::exception& e)  {
        std::cerr << "exception: " << e.what() << "\n";
    }
    return 0;
}
