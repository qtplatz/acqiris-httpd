/**************************************************************************
** Copyright (C) 2016 Toshinobu Hondo, Ph.D.
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

#include "app.hpp"
#include "mainwindow.hpp"
#include "document.hpp"
#include <adportable/debug.hpp>
#include <QApplication>

using namespace acqiris_gui;

void
waveform_handler::operator () ( std::shared_ptr< const acqrscontrols::aqdrv4::waveform > ptr )
{
    document::instance()->push( ptr );
}

void
waveform_handler::operator () ( const acqrscontrols::aqdrv4::acqiris_method& m )
{
    document::instance()->prepare_for_run( m );
}

callback_handler::callback_handler()
{
}

callback_handler&
callback_handler::instance()
{
    static callback_handler __instance;
    return __instance;
}

int
app::main( int argc, char * argv [] )
{
    QApplication a( argc, argv );

    MainWindow w;
    w.resize( 800, 600 );

    w.show();
    w.onInitialUpdate();
    // QCoreApplication::processEvents();
    // document::instance()->initialSetup();

    ADDEBUG() << "app::main()";
    
    a.exec();

    return 0;
}

