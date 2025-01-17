// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
/*!
 * \file
 * \ingroup Common
 * \brief Helpers for deprecation
 */

#ifndef DUMUX_COMMON_DEPRECATED_HH
#define DUMUX_COMMON_DEPRECATED_HH

namespace Dumux {

#ifndef DOXYGEN // hide from doxygen
// Helper classes/functions for deprecation
// Each implementation has to state after which release
// it will be removed. Implementations in the Deprecated
// namespace will be removed without
// deprecation after their usage in the code expired,
// so most likely you don't want to use this in your code
namespace Deprecated {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif // __clang__

template<class G>
using DetectFVGeometryHasSCVGeometry = decltype(std::declval<G>().geometry(std::declval<typename G::SubControlVolume>()));

template<class G>
using DetectFVGeometryHasSCVFGeometry = decltype(std::declval<G>().geometry(std::declval<typename G::SubControlVolumeFace>()));

template<class G>
constexpr inline bool hasSCVGeometryInterface()
{ return Dune::Std::is_detected<DetectFVGeometryHasSCVGeometry, G>::value; }

template<class G>
constexpr inline bool hasSCVFGeometryInterface()
{ return Dune::Std::is_detected<DetectFVGeometryHasSCVFGeometry, G>::value; }

#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__

} // end namespace Deprecated
#endif

} // end namespace Dumux
#endif
