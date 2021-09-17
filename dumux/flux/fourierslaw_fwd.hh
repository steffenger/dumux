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
 * \ingroup Flux
 * \brief Fourier's law specialized for different discretization schemes
 *        This file contains the data which is required to calculate
 *        diffusive mass fluxes due to molecular diffusion with Fourier's law.
 */
#ifndef DUMUX_FLUX_FOURIERS_LAW_FWD_HH
#define DUMUX_FLUX_FOURIERS_LAW_FWD_HH

#include <dumux/common/properties.hh>
#include <dumux/discretization/method.hh>

namespace Dumux {

// declaration of primary template
template <class TypeTag, class DiscretizationMethod>
class FouriersLawImplementation;

/*!
 * \ingroup Flux
 * \brief Evaluates the heat conduction flux according to Fouriers's law
 */
template <class TypeTag>
using FouriersLaw = FouriersLawImplementation<TypeTag, typename GetPropType<TypeTag, Properties::GridGeometry>::DiscretizationMethod>;

} // end namespace Dumux

#endif
