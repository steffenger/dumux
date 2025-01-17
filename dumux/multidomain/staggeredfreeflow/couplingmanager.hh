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
 * \ingroup MultiDomain
 * \brief Freeflow coupling managers (Navier-Stokes mass-momentum coupling)
 */
#ifndef DUMUX_MULTIDOMAIN_STAGGERED_FREEFLOW_COUPLING_MANAGER_HH
#define DUMUX_MULTIDOMAIN_STAGGERED_FREEFLOW_COUPLING_MANAGER_HH

#warning "This header is deprecated and will be removed after release 3.6"
#include <dumux/multidomain/freeflow/couplingmanager.hh>

namespace Dumux {

/*!
 * \ingroup MultiDomain
 * \brief The interface of the coupling manager for free flow systems
 */
template<class Traits>
using StaggeredFreeFlowCouplingManager [[deprecated("Will be removed after release 3.6. Use FreeFlowCouplingManager.")]] = FreeFlowCouplingManager<Traits>;

} // end namespace Dumux

#endif
