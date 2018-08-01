// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2010-2018 Toshinobu Hondo, Ph.D.
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

#include <cstddef>

namespace dg {
    namespace constants {

        constexpr const char * const msi_master_cdev = "/dev/msi_master0";
        constexpr const char * const adc_cdev = "/dev/adc-fifo0";

        namespace hv {
            constexpr const char * const prefix = "/hv/api$";
            constexpr const std::size_t prefix_size = ( sizeof( "/hv/api$" ) - 1 );
        }

        namespace evbox {
            constexpr const char * const prefix = "/evbox/api$";
            constexpr size_t prefix_size = ( sizeof( "/evbox/api$" ) - 1 );
        }
    }
}
