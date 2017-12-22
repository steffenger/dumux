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
 * \ingroup StaggeredDiscretization
 * \brief Specialization of Maxwell Stefan's Law for the Staggered method.
 */
#ifndef DUMUX_DISCRETIZATION_STAGGERED_MAXWELL_STEFAN_LAW_HH
#define DUMUX_DISCRETIZATION_STAGGERED_MAXWELL_STEFAN_LAW_HH

#include <dune/common/float_cmp.hh>

#include <dumux/common/math.hh>
#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/discretization/methods.hh>
#include <dumux/discretization/fluxvariablescaching.hh>

namespace Dumux {

// forward declaration
template <class TypeTag, DiscretizationMethods DM>
class MaxwellStefansLawImplementation;

/*!
 * \ingroup StaggeredDiscretization
 * \brief Specialization of Maxwell Stefan's Law for the Staggered method.
 */
template <class TypeTag>
class MaxwellStefansLawImplementation<TypeTag, DiscretizationMethods::Staggered >
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using CellCenterPrimaryVariables = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);

    static const int dim = GridView::dimension;
    static const int dimWorld = GridView::dimensionworld;

    static const int numComponents = GET_PROP_VALUE(TypeTag,NumComponents);
    static constexpr bool useMoles = GET_PROP_VALUE(TypeTag, UseMoles);

    using ReducedComponentVector = Dune::FieldVector<Scalar, numComponents-1>;
    using ReducedComponentMatrix = Dune::FieldMatrix<Scalar, numComponents-1, numComponents-1>;

    static_assert(GET_PROP_VALUE(TypeTag, NumPhases) == 1, "Only one phase allowed supported!");

    enum {
        pressureIdx = Indices::pressureIdx,
        conti0EqIdx = Indices::conti0EqIdx,
        replaceCompEqIdx = Indices::replaceCompEqIdx,
        phaseIdx = Indices::phaseIdx
    };

public:
    // state the discretization method this implementation belongs to
    static const DiscretizationMethods myDiscretizationMethod = DiscretizationMethods::Staggered;

    //! state the type for the corresponding cache and its filler
    //! We don't cache anything for this law
    using Cache = FluxVariablesCaching::EmptyDiffusionCache<TypeTag>;
    using CacheFiller = FluxVariablesCaching::EmptyCacheFiller<TypeTag>;

    static CellCenterPrimaryVariables diffusiveFluxForCellCenter(const Problem& problem,
                                                                 const FVElementGeometry& fvGeometry,
                                                                 const ElementVolumeVariables& elemVolVars,
                                                                 const SubControlVolumeFace& scvf)
    {
        //this is to calculate the maxwellStefan diffusion in a multicomponent system.
        //see: Multicomponent Mass Transfer. R. Taylor u. R. Krishna. J. Wiley & Sons, New York 1993
        CellCenterPrimaryVariables componentFlux(0.0);
        ReducedComponentVector moleFracInside(0.0);
        ReducedComponentVector moleFracOutside(0.0);
        ReducedComponentVector reducedFlux(0.0);
        ReducedComponentMatrix reducedDiffusionMatrixInside(0.0);
        ReducedComponentMatrix reducedDiffusionMatrixOutside(0.0);

        // get inside/outside volume variables
        const auto& insideVolVars = elemVolVars[scvf.insideScvIdx()];
        const auto& outsideVolVars = elemVolVars[scvf.outsideScvIdx()];
        const auto rhoInside = insideVolVars.molarDensity();
        const auto rhoOutside = scvf.boundary() ? insideVolVars.molarDensity() : outsideVolVars.molarDensity();
        //calculate the mole fraction vectors
        for (int compIdx = 0; compIdx < numComponents-1; compIdx++)
        {
            //calculate x_inside
            const auto xInside = insideVolVars.moleFraction(phaseIdx, compIdx);
            //calculate outside molefraction with the respective transmissibility
            const auto xOutside = outsideVolVars.moleFraction(phaseIdx, compIdx);

            moleFracInside[compIdx] = xInside;
            moleFracOutside[compIdx] = xOutside;

            // get equation index
            const auto eqIdx = conti0EqIdx + compIdx;

            if(scvf.boundary())
            {
               const auto bcTypes = problem.boundaryTypesAtPos(scvf.center());
                 if(bcTypes.isOutflow(eqIdx) && eqIdx != pressureIdx)
                    return componentFlux;
            }
        }

        //now we have to do the tpfa: J_i = J_j which leads to: tij(xi -xj) = -rho Bi^-1 omegai(x*-xi) with x* = (omegai Bi^-1 + omegaj Bj^-1)^-1 (xi omegai Bi^-1 + xj omegaj Bj^-1) with i inside and j outside
        reducedDiffusionMatrixInside = setupMSMatrix_(problem, fvGeometry, insideVolVars, scvf);

        reducedDiffusionMatrixOutside = setupMSMatrix_(problem, fvGeometry, outsideVolVars, scvf);

        const auto insideScvIdx = scvf.insideScvIdx();
        const auto& insideScv = fvGeometry.scv(insideScvIdx);
        const auto outsideScvIdx = scvf.outsideScvIdx();
        const auto& outsideScv = fvGeometry.scv(outsideScvIdx);

        const Scalar omegai = calculateOmega_(scvf,
                                                insideScv,
                                                1);

        //if on boundary
        if (scvf.boundary())
        {
            moleFracOutside -= moleFracInside;
            reducedDiffusionMatrixInside.solve(reducedFlux, moleFracOutside);
            reducedFlux *= omegai;
        }
        else
        {
            Scalar omegaj;
            omegaj = -1.0*calculateOmega_(scvf,
                                          outsideScv,
                                          1);

            reducedDiffusionMatrixInside.invert();
            reducedDiffusionMatrixOutside.invert();
            reducedDiffusionMatrixInside *= omegai;
            reducedDiffusionMatrixOutside *= omegaj;

            //in the helpervector we store the values for x*
            ReducedComponentVector helperVector(0.0);
            ReducedComponentVector gradientVectori(0.0);
            ReducedComponentVector gradientVectorj(0.0);

            reducedDiffusionMatrixInside.mv(moleFracInside, gradientVectori);
            reducedDiffusionMatrixOutside.mv(moleFracOutside, gradientVectorj);

            auto gradientVectorij = (gradientVectori + gradientVectorj);

            //add the two matrixes to each other
            reducedDiffusionMatrixOutside += reducedDiffusionMatrixInside;

            reducedDiffusionMatrixOutside.solve(helperVector, gradientVectorij);

            //Bi^-1 omegai rho(x*-xi)
            helperVector -=moleFracInside;
            reducedDiffusionMatrixInside.mv(helperVector, reducedFlux);
        }

        reducedFlux *= -0.5*(rhoInside+rhoOutside)*scvf.area();

        for (int compIdx = 0; compIdx < numComponents-1; compIdx++)
        {
            componentFlux[compIdx] = reducedFlux[compIdx];
            componentFlux[numComponents-1] -=reducedFlux[compIdx];
        }

        if(useMoles && replaceCompEqIdx <= numComponents)
            componentFlux[replaceCompEqIdx] = 0.0;

        return componentFlux ;
    }

private:
   static Scalar calculateOmega_(const SubControlVolumeFace& scvf,
                                 const SubControlVolume &scv,
                                 const Scalar extrusionFactor)
    {
        auto distanceVector = scvf.ipGlobal();
        distanceVector -= scv.center();
        distanceVector /= distanceVector.two_norm2();

        Scalar omega = (distanceVector * scvf.unitOuterNormal());
        omega *= extrusionFactor;

        return omega;
    }

    static ReducedComponentMatrix setupMSMatrix_(const Problem& problem,
                                                const FVElementGeometry& fvGeometry,
                                                const VolumeVariables& volVars,
                                                const SubControlVolumeFace& scvf)
    {
        ReducedComponentMatrix reducedDiffusionMatrix(0.0);

        for (int compIIdx = 0; compIIdx < numComponents-1; compIIdx++)
        {
            const auto xi = volVars.moleFraction(phaseIdx, compIIdx);

            //calculate diffusivity for i,numComponents
            auto fluidState = volVars.fluidState();
            typename FluidSystem::ParameterCache paramCache;
            paramCache.updateAll(fluidState);
            auto tin = FluidSystem::binaryDiffusionCoefficient(fluidState,
                                                               paramCache,
                                                               phaseIdx,
                                                               compIIdx,
                                                               numComponents-1);
            //set the entrys of the diffusion matrix of the diagonal
            reducedDiffusionMatrix[compIIdx][compIIdx] += xi/tin;

            for (int compkIdx = 0; compkIdx < numComponents; compkIdx++)
            {
                if (compkIdx == compIIdx)
                            continue;

                const auto xk = volVars.moleFraction(phaseIdx, compkIdx);
                Scalar tik = FluidSystem::binaryDiffusionCoefficient(fluidState,
                                                                    paramCache,
                                                                    phaseIdx,
                                                                    compIIdx,
                                                                    compkIdx);
                reducedDiffusionMatrix[compIIdx][compIIdx] += xk/tik;
            }

            // now set the rest of the entries (off-diagonal)
            for (int compJIdx = 0; compJIdx < numComponents-1; compJIdx++)
            {
                //we don't want to calculate e.g. water in water diffusion
                if (compIIdx == compJIdx)
                    continue;
                //calculate diffusivity for compIIdx, compJIdx
                Scalar tij = FluidSystem::binaryDiffusionCoefficient(fluidState,
                                                                    paramCache,
                                                                    phaseIdx,
                                                                    compIIdx,
                                                                    compJIdx);
                reducedDiffusionMatrix[compIIdx][compJIdx] +=xi*(1/tin - 1/tij);
            }
        }
        return reducedDiffusionMatrix;
    }
};
} // end namespace

#endif