/**************************************************************************
** Copyright (C) 2016-2018 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2018 MS-Cheminformatics LLC, Toin, Mie Japan
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

#include <algorithm>
#include <array>

struct endian_decoder_t {

    template< typename T > 
    void operator()( T& a ) const {
        char * ptr = reinterpret_cast< char * >(&a);
        std::reverse( ptr, ptr + sizeof( a ) );
    }

    // endian decode for all items in an std::array
    template< typename T, size_t N > void operator()( std::array< T, N >& data ) const {
        std::for_each( data.begin(), data.end(), [&]( auto& a ){ (*this)( a ); } );
    }
};

