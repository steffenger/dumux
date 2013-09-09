/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
#ifndef DUMUX_FVMPFAL2OFABOUNDPRESSURE2P_ADAPTIVE_HH
#define DUMUX_FVMPFAL2OFABOUNDPRESSURE2P_ADAPTIVE_HH

#warning This file is deprecated. Use fvmpfal2dpressure2padaptive.hh instead.
#include "fvmpfal2dpressure2padaptive.hh"

namespace Dumux
{
template<class TypeTag>
class FVMPFAL2PFABoundPressure2PAdaptive: public FvMpfaL2dPressure2pAdaptive<TypeTag>
{
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    DUNE_DEPRECATED_MSG("use FvMpfaL2dPressure2pAdaptive(problem) instead")
    FVMPFAL2PFABoundPressure2PAdaptive(Problem& problem):
        FvMpfaL2dPressure2pAdaptive<TypeTag>(problem)
        {}
};
}
#endif
