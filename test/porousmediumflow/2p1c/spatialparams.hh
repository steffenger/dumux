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
 * \ingroup TwoPOneCTests
 * \brief Spatial parameters non-isothermal steam injection test problem for the 2p1cni model.
 */

#ifndef DUMUX_STEAMINJECTION_SPATIAL_PARAMS_HH
#define DUMUX_STEAMINJECTION_SPATIAL_PARAMS_HH

#include <dumux/porousmediumflow/properties.hh>
#include <dumux/porousmediumflow/fvspatialparamsmp.hh>
#include <dumux/material/fluidmatrixinteractions/2p/vangenuchten.hh>

namespace Dumux {
/*!
 * \ingroup TwoPOneCTests
 * \brief Definition of the spatial parameters for various steam injection problems.
 */
template<class GridGeometry, class Scalar>
class InjectionProblemSpatialParams
: public FVPorousMediumFlowSpatialParamsMP<GridGeometry, Scalar,
                                       InjectionProblemSpatialParams<GridGeometry, Scalar>>
{
    using GridView = typename GridGeometry::GridView;
    using FVElementGeometry = typename GridGeometry::LocalView;
    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using Element = typename GridView::template Codim<0>::Entity;
    using ThisType = InjectionProblemSpatialParams<GridGeometry, Scalar>;
    using ParentType = FVPorousMediumFlowSpatialParamsMP<GridGeometry, Scalar, ThisType>;

    static constexpr int dimWorld = GridView::dimensionworld;

    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

    using DimWorldMatrix = Dune::FieldMatrix<Scalar, dimWorld, dimWorld>;

    using PcKrSwCurve = FluidMatrix::VanGenuchtenDefault<Scalar>;

public:
    using PermeabilityType = DimWorldMatrix;

    InjectionProblemSpatialParams(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    , pcKrSwCurve_("SpatialParams")
    {
        gasWetting_ = getParam<bool>("SpatialParams.GasWetting", false);
    }

    /*!
     * \brief Returns the hydraulic conductivity \f$[m^2]\f$
     *
     * \param globalPos The global position
     */
    DimWorldMatrix permeabilityAtPos(const GlobalPosition& globalPos) const
    {
        DimWorldMatrix permMatrix(0.0);

        // intrinsic permeability
        permMatrix[0][0] = 1e-9;
        permMatrix[1][1] = 1e-9;

        return permMatrix; //default value
    }

    /*!
     * \brief Defines the porosity \f$[-]\f$ of the spatial parameters
     *
     * \param globalPos The global position
     */
    Scalar porosityAtPos(const GlobalPosition& globalPos) const
    {
        return 0.4;
    }

    /*!
     * \brief Returns the parameter object for the capillary-pressure/
     *        saturation material law.
     *
     * \param globalPos The global position
     */
    auto fluidMatrixInteractionAtPos(const GlobalPosition& globalPos) const
    {
        return makeFluidMatrixInteraction(pcKrSwCurve_);
    }

    /*!
     * \brief Function for defining which phase is to be considered as the wetting phase.
     *
     * \param globalPos The position of the center of the element
     * \return The wetting phase index
     */
    template<class FluidSystem>
    int wettingPhaseAtPos(const GlobalPosition& globalPos) const
    {
        if (gasWetting_)
            return FluidSystem::gasPhaseIdx;
        else
            return FluidSystem::liquidPhaseIdx;
    }

private:
    bool gasWetting_;
    const PcKrSwCurve pcKrSwCurve_;
};

}

#endif
