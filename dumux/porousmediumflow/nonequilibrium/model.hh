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
 * \file
 * \ingroup PorousmediumNonEquilibriumModel
 * \brief This is the specialization that is able to capture non-equilibrium mass and / or energy transfer.
 * \todo DocMe
 */
#ifndef DUMUX_NONEQUILIBRIUM_MODEL_HH
#define DUMUX_NONEQUILIBRIUM_MODEL_HH

#include <dumux/common/properties.hh>
#include <dumux/common/dimensionlessnumbers.hh>
#include <dumux/material/fluidstates/nonequilibrium.hh>

#include <dumux/discretization/fourierslawnonequilibrium.hh>
#include <dumux/discretization/fourierslawnonequilibrium.hh>
#include <dumux/porousmediumflow/nonequilibrium/thermal/localresidual.hh>
#include <dumux/porousmediumflow/nonequilibrium/localresidual.hh>

#include "localresidual.hh"
#include "indices.hh"
#include "gridvariables.hh"
#include "vtkoutputfields.hh"

/*!
 * \ingroup \ingroup PorousmediumNonEquilibriumModel
 * \brief Defines the properties required for non-equilibrium models
 */
namespace Dumux
{
namespace Properties
{

//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////
NEW_TYPE_TAG(NonEquilibrium);

/////////////////////////////////////////////////
// Properties for the non-equilibrium mpnc model
/////////////////////////////////////////////////

SET_BOOL_PROP(NonEquilibrium, EnableThermalNonEquilibrium, true);
//TODO: make that more logical: when enableEnergyBalance is false, thermalnonequilibrium should also not be computed.
SET_BOOL_PROP(NonEquilibrium, EnableEnergyBalance, true);
SET_BOOL_PROP(NonEquilibrium, EnableChemicalNonEquilibrium, true);

SET_INT_PROP(NonEquilibrium, NumEnergyEqFluid, GET_PROP_VALUE(TypeTag, NumPhases));
SET_INT_PROP(NonEquilibrium, NumEnergyEqSolid, 1);

SET_TYPE_PROP(NonEquilibrium, EnergyLocalResidual, EnergyLocalResidualNonEquilibrium<TypeTag, GET_PROP_VALUE(TypeTag, NumEnergyEqFluid)>);
SET_TYPE_PROP(NonEquilibrium, LocalResidual, NonEquilibriumLocalResidual<TypeTag>);

SET_TYPE_PROP(NonEquilibrium, HeatConductionType , FouriersLawNonEquilibrium<TypeTag>);

//! add the energy balances and chemical nonequilibrium numeq = numPhases*numComp+numEnergy
SET_INT_PROP(NonEquilibrium, NumEq, GET_PROP_VALUE(TypeTag, NumEqBalance) +  GET_PROP_VALUE(TypeTag, NumEnergyEqFluid) + GET_PROP_VALUE(TypeTag, NumEnergyEqSolid));

//! indices for non-isothermal models
SET_TYPE_PROP(NonEquilibrium, Indices, NonEquilbriumIndices<TypeTag, 0>);

SET_PROP(NonEquilibrium, FluidState)
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
public:
     using type = NonEquilibriumFluidState<Scalar, FluidSystem>;
};

//! The grid variables
SET_TYPE_PROP(NonEquilibrium, GridVariables, NonEquilibriumGridVariables<TypeTag>);

//! indices for non-isothermal models
SET_TYPE_PROP(NonEquilibrium, VtkOutputFields, NonEquilibriumVtkOutputFields<TypeTag>);

SET_PROP(NonEquilibrium, NusseltFormulation )
{
    private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using DimLessNum =  DimensionlessNumbers<Scalar>;
    public:
    static constexpr int value = DimLessNum::NusseltFormulation::WakaoKaguei;
};

/*!
 * \brief Set the default formulation for the sherwood correlation
 *        Other possible parametrizations can be found in the dimensionlessnumbers
 */
SET_PROP(NonEquilibrium, SherwoodFormulation )
{
    private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using DimLessNum =  DimensionlessNumbers<Scalar>;
    public:
    static constexpr int value = DimLessNum::SherwoodFormulation::WakaoKaguei;
};

} //end namespace Properties
} //end namespace Dumux

#endif