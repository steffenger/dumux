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
 * \ingroup Elastic
 * \brief Element-wise calculation of the local residual for problems
 *        using the elastic model considering linear elasticity.
 */
#ifndef DUMUX_GEOMECHANICS_ELASTIC_LOCAL_RESIDUAL_HH
#define DUMUX_GEOMECHANICS_ELASTIC_LOCAL_RESIDUAL_HH

#include <dumux/common/parameters.hh>
#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/numeqvector.hh>

namespace Dumux {

/*!
 * \ingroup Elastic
 * \brief Element-wise calculation of the local residual for problems
 *        using the elastic model considering linear elasticity.
 */
template<class TypeTag>
class ElasticLocalResidual : public GetPropType<TypeTag, Properties::BaseLocalResidual>
{
    using ParentType = GetPropType<TypeTag, Properties::BaseLocalResidual>;

    using GridView = typename GetPropType<TypeTag, Properties::GridGeometry>::GridView;
    using Element = typename GridView::template Codim<0>::Entity;

    using Problem = GetPropType<TypeTag, Properties::Problem>;
    using Indices = typename GetPropType<TypeTag, Properties::ModelTraits>::Indices;
    using FVElementGeometry = typename GetPropType<TypeTag, Properties::GridGeometry>::LocalView;
    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using SubControlVolumeFace = typename FVElementGeometry::SubControlVolumeFace;
    using NumEqVector = Dumux::NumEqVector<GetPropType<TypeTag, Properties::PrimaryVariables>>;
    using ElementFluxVariablesCache = typename GetPropType<TypeTag, Properties::GridFluxVariablesCache>::LocalView;
    using ElementVolumeVariables = typename GetPropType<TypeTag, Properties::GridVolumeVariables>::LocalView;
    using VolumeVariables = typename ElementVolumeVariables::VolumeVariables;

    // class assembling the stress tensor
    using StressType = GetPropType<TypeTag, Properties::StressType>;

public:
    using ParentType::ParentType;

    /*!
     * \brief For the elastic model the storage term is zero since
     *        we neglect inertia forces.
     *
     * \param problem The problem
     * \param scv The sub control volume
     * \param volVars The current or previous volVars
     */
    NumEqVector computeStorage(const Problem& problem,
                               const SubControlVolume& scv,
                               const VolumeVariables& volVars) const
    {
        return NumEqVector(0.0);
    }

    /*!
     * \brief Evaluates the force in all grid directions acting
     *        on a face of a sub-control volume.
     *
     * \param problem The problem
     * \param element The current element.
     * \param fvGeometry The finite-volume geometry
     * \param elemVolVars The volume variables of the current element
     * \param scvf The sub control volume face to compute the flux on
     * \param elemFluxVarsCache The cache related to flux computation
     */
    NumEqVector computeFlux(const Problem& problem,
                            const Element& element,
                            const FVElementGeometry& fvGeometry,
                            const ElementVolumeVariables& elemVolVars,
                            const SubControlVolumeFace& scvf,
                            const ElementFluxVariablesCache& elemFluxVarsCache) const
    {
        // obtain force on the face from stress type
        return StressType::force(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache);
    }

    /*!
     * \brief Calculate the source term of the equation
     *
     * \param problem The problem to solve
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     * \param fvGeometry The finite-volume geometry of the element
     * \param elemVolVars The volume variables associated with the element stencil
     * \param scv The sub-control volume over which we integrate the source term
     * \note This is the default implementation for geomechanical models adding to
     *       the user defined sources the source stemming from the gravitational acceleration.
     *
     */
    NumEqVector computeSource(const Problem& problem,
                              const Element& element,
                              const FVElementGeometry& fvGeometry,
                              const ElementVolumeVariables& elemVolVars,
                              const SubControlVolume &scv) const
    {
        NumEqVector source(0.0);

        // add contributions from volume flux sources
        source += problem.source(element, fvGeometry, elemVolVars, scv);

        // add contribution from possible point sources
        source += problem.scvPointSources(element, fvGeometry, elemVolVars, scv);

        // maybe add gravitational acceleration
        static const bool gravity = getParamFromGroup<bool>(problem.paramGroup(), "Problem.EnableGravity");
        if (gravity)
        {
            const auto& g = problem.spatialParams().gravity(scv.center());
            for (int dir = 0; dir < GridView::dimensionworld; ++dir)
                source[Indices::momentum(dir)] += elemVolVars[scv].solidDensity()*g[dir];
        }

        return source;
    }
};

} // end namespace Dumux

#endif
