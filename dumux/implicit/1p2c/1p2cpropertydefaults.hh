// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
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
/*!
 * \ingroup Properties
 * \ingroup BoxProperties
 * \ingroup OnePTwoCBoxModel
 * \file
 *
 * \brief Defines some default values for the properties of the the
 *        single-phase, two-component BOX model.
 */

#ifndef DUMUX_1P2C_PROPERTY_DEFAULTS_HH
#define DUMUX_1P2C_PROPERTY_DEFAULTS_HH

#include "1p2cproperties.hh"

#include "1p2cmodel.hh"
#include "1p2clocalresidual.hh"
#include "1p2cvolumevariables.hh"
#include "1p2cfluxvariables.hh"
#include "1p2cindices.hh"

#include <dumux/material/spatialparams/boxspatialparams1p.hh>

namespace Dumux
{
// \{
namespace Properties
{
//////////////////////////////////////////////////////////////////
// Property values
//////////////////////////////////////////////////////////////////

SET_INT_PROP(BoxOnePTwoC, NumEq, 2); //!< set the number of equations to 2
SET_INT_PROP(BoxOnePTwoC, NumPhases, 1); //!< The number of phases in the 1p2c model is 1
SET_INT_PROP(BoxOnePTwoC, NumComponents, 2); //!< The number of components in the 1p2c model is 2
SET_SCALAR_PROP(BoxOnePTwoC, Scaling, 1); //!< Scaling of the model is set to 1 by default
SET_BOOL_PROP(BoxOnePTwoC, UseMoles, false); //!< Define that mass fractions are used in the balance equations

//! Use the 1p2c local residual function for the 1p2c model
SET_TYPE_PROP(BoxOnePTwoC, LocalResidual, OnePTwoCLocalResidual<TypeTag>);

//! define the model
SET_TYPE_PROP(BoxOnePTwoC, Model, OnePTwoCBoxModel<TypeTag>);

//! define the VolumeVariables
SET_TYPE_PROP(BoxOnePTwoC, VolumeVariables, OnePTwoCVolumeVariables<TypeTag>);

//! define the FluxVariables
SET_TYPE_PROP(BoxOnePTwoC, FluxVariables, OnePTwoCFluxVariables<TypeTag>);

//! set default upwind weight to 1.0, i.e. fully upwind
SET_SCALAR_PROP(BoxOnePTwoC, ImplicitMassUpwindWeight, 1.0);

//! Set the indices used by the 1p2c model
SET_TYPE_PROP(BoxOnePTwoC, Indices, Dumux::OnePTwoCIndices<TypeTag, 0>);

//! The spatial parameters to be employed. 
//! Use BoxSpatialParamsOneP by default.
SET_TYPE_PROP(BoxOnePTwoC, SpatialParams, BoxSpatialParamsOneP<TypeTag>);

//! Set the phaseIndex per default to zero (important for two-phase fluidsystems).
SET_INT_PROP(BoxOnePTwoC, PhaseIdx, 0);

// enable gravity by default
SET_BOOL_PROP(BoxOnePTwoC, ProblemEnableGravity, true);

//! default value for the forchheimer coefficient
// Source: Ward, J.C. 1964 Turbulent flow in porous media. ASCE J. Hydraul. Div 90.
//        Actually the Forchheimer coefficient is also a function of the dimensions of the
//        porous medium. Taking it as a constant is only a first approximation
//        (Nield, Bejan, Convection in porous media, 2006, p. 10)
SET_SCALAR_PROP(BoxModel, SpatialParamsForchCoeff, 0.55);
}
// \}
}

#endif

