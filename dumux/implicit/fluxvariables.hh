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
 * \brief Base class for the flux variables
 */
#ifndef DUMUX_IMPLICIT_FLUXVARIABLES_HH
#define DUMUX_IMPLICIT_FLUXVARIABLES_HH

#include <dumux/implicit/properties.hh>

namespace Dumux
{

namespace Properties
{
NEW_PROP_TAG(NumPhases);
NEW_PROP_TAG(NumComponents);
}

/*!
 * \ingroup ImplicitModel
 * \brief Base class for the flux variables
 *        Actual flux variables inherit from this class
 */
template<class TypeTag, class Implementation>
class FluxVariablesBase
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using IndexType = typename GridView::IndexSet::IndexType;
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Stencil = std::vector<IndexType>;
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);

    enum{ enableFluxVarsCache = GET_PROP_VALUE(TypeTag, EnableFluxVariablesCache) };

public:
    FluxVariablesBase() : problemPtr_(nullptr), scvFacePtr_(nullptr)
    {}

    void init(const Problem& problem,
              const Element& element,
              const SubControlVolumeFace &scvFace)
    {
        problemPtr_ = &problem;
        scvFacePtr_ = &scvFace;

        // update the stencil if needed
        if (!enableFluxVarsCache)
            stencil_ = asImp_().computeStencil(problem, scvFace);
    }

    // when caching is enabled, get the stencil from the cache class
    template <typename T = TypeTag>
    const typename std::enable_if<GET_PROP_VALUE(T, EnableFluxVariablesCache), Stencil>::type& stencil() const
    { return problem().model().fluxVarsCache(scvFace()).stencil(); }

    // when caching is disabled, return the private stencil variable. The update(...) routine has to be called beforehand.
    template <typename T = TypeTag>
    const typename std::enable_if<!GET_PROP_VALUE(T, EnableFluxVariablesCache), Stencil>::type& stencil()
    { return stencil_; }

    const Problem& problem() const
    { return *problemPtr_; }

    const SubControlVolumeFace& scvFace() const
    {
        return *scvFacePtr_;
    }

    Stencil computeStencil(const Problem& problem, const SubControlVolumeFace& scvFace)
    { DUNE_THROW(Dune::InvalidStateException, "computeStencil() routine is not provided by the implementation."); }

private:

    Implementation &asImp_()
    {
        assert(static_cast<Implementation*>(this) != 0);
        return *static_cast<Implementation*>(this);
    }

    const Implementation &asImp_() const
    {
        assert(static_cast<const Implementation*>(this) != 0);
        return *static_cast<const Implementation*>(this);
    }

    const Problem *problemPtr_;              //! Pointer to the problem
    const SubControlVolumeFace *scvFacePtr_; //! Pointer to the sub control volume face for which the flux variables are created
    Stencil stencil_;                        //! The flux stencil
};



/*!
 * \ingroup ImplicitModel
 * \brief the flux variables class
 *        specializations are provided for combinations of physical processes
 */
template<class TypeTag, bool enableAdvection, bool enableMolecularDiffusion, bool enableEnergyBalance>
class FluxVariables {};


// specialization for pure advective flow (e.g. 1p/2p/3p immiscible darcy flow)
template<class TypeTag>
class FluxVariables<TypeTag, true, false, false> : public FluxVariablesBase<TypeTag, FluxVariables<TypeTag, true, false, false>>
{
    using ParentType = FluxVariablesBase<TypeTag, FluxVariables<TypeTag, true, false, false>>;
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using IndexType = typename GridView::IndexSet::IndexType;
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Stencil = std::vector<IndexType>;
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using AdvectionType = typename GET_PROP_TYPE(TypeTag, AdvectionType);

    enum
    {
        isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox),
        enableFluxVarsCache = GET_PROP_VALUE(TypeTag, EnableFluxVariablesCache),
        constantBC = GET_PROP_VALUE(TypeTag, ConstantBoundaryConditions)
    };

public:

    void initAndComputeFluxes(const Problem& problem,
                              const Element& element,
                              const SubControlVolumeFace &scvFace)
    {
        ParentType::init(problem, element, scvFace);
    }

    template<typename FunctionType>
    Scalar advectiveFlux(const int phaseIdx, const FunctionType upwindFunction)
    {
        Scalar flux = AdvectionType::flux(this->problem(), this->scvFace(), phaseIdx);

        const auto& insideVolVars = this->problem().model().curVolVars(this->scvFace().insideScvIdx());
        const auto& outsideVolVars = this->problem().model().curVolVars(this->scvFace().outsideScvIdx());

        if (std::signbit(flux))
            return flux*upwindFunction(outsideVolVars, insideVolVars);
        else
            return flux*upwindFunction(insideVolVars, outsideVolVars);
    }

    Stencil computeStencil(const Problem& problem, const SubControlVolumeFace& scvFace)
    { return AdvectionType::stencil(problem, scvFace); }
};


// specialization for isothermal advection molecularDiffusion equations
template<class TypeTag>
class FluxVariables<TypeTag, true, true, false> : public FluxVariablesBase<TypeTag, FluxVariables<TypeTag, true, true, false>>
{
    using ParentType = FluxVariablesBase<TypeTag, FluxVariables<TypeTag, true, true, false>>;
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using IndexType = typename GridView::IndexSet::IndexType;
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Stencil = std::vector<IndexType>;
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using AdvectionType = typename GET_PROP_TYPE(TypeTag, AdvectionType);
    using MolecularDiffusionType = typename GET_PROP_TYPE(TypeTag, MolecularDiffusionType);

    enum
    {
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents)
    };

public:

    void initAndComputeFluxes(const Problem& problem,
                              const Element& element,
                              const SubControlVolumeFace &scvFace)
    {
        ParentType::init(problem, element, scvFace);

        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
            advectiveFluxes_[phaseIdx] = AdvectionType::flux(problem, scvFace, phaseIdx);
    }

    Stencil computeStencil(const Problem& problem, const SubControlVolumeFace& scvFace)
    {
        // unifiy advective and diffusive stencil
        Stencil stencil = AdvectionType::stencil(problem, scvFace);
        Stencil diffusionStencil = MolecularDiffusionType::stencil(problem, scvFace);

        stencil.insert(stencil.end(), diffusionStencil.begin(), diffusionStencil.end());
        std::sort(stencil.begin(), stencil.end());
        stencil.erase(std::unique(stencil.begin(), stencil.end()), stencil.end());

        return stencil;
    }

    template<typename FunctionType>
    Scalar advectiveFlux(const int phaseIdx, const FunctionType upwindFunction)
    {
        const auto& insideVolVars = this->problem().model().curVolVars(this->scvFace().insideScvIdx());
        const auto& outsideVolVars = this->problem().model().curVolVars(this->scvFace().outsideScvIdx());

        if (std::signbit(advectiveVolumeFluxes_[phaseIdx]))
            return advectiveVolumeFluxes_[phaseIdx]*upwindFunction(outsideVolVars, insideVolVars);
        else
            return advectiveVolumeFluxes_[phaseIdx]*upwindFunction(insideVolVars, outsideVolVars);
    }

    Scalar molecularDiffusionFlux(const int phaseIdx, const int compIdx)
    {
        Scalar flux = MolecularDiffusionType::flux(this->problem(), this->scvFace(), phaseIdx, compIdx);
        return flux;
    }

private:
    // storage for calculated advective fluxes to not having to calculate them again
    std::array<Scalar, numPhases> advectiveVolumeFluxes_;
};


// specialization for pure molecularDiffusion_
template<class TypeTag>
class FluxVariables<TypeTag, false, true, false>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Stencil = std::set<IndexType>;
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using MolecularDiffusionType = typename GET_PROP_TYPE(TypeTag, MolecularDiffusionType);

    enum
    {
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents)
    };

public:
    void update(const Problem& problem, const SubControlVolumeFace &scvf)
    {
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                molecularDiffusion_.update(problem, scvf, phaseIdx, compIdx);
    }

    const MolecularDiffusionType& molecularDiffusion(const int phaseIdx, const int compIdx) const
    {
        return molecularDiffusion_[phaseIdx][compIdx];
    }

    MolecularDiffusionType& molecularDiffusion(const int phaseIdx, const int compIdx)
    {
        return molecularDiffusion_[phaseIdx][compIdx];
    }

private:
    std::array< std::array<MolecularDiffusionType, numComponents>, numPhases> molecularDiffusion_;
};


// specialization for non-isothermal advective flow (e.g. non-isothermal one-phase darcy equation)
template<class TypeTag>
class FluxVariables<TypeTag, true, false, true>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Stencil = std::set<IndexType>;
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using AdvectionType = typename GET_PROP_TYPE(TypeTag, AdvectionType);
    using HeatConductionType = typename GET_PROP_TYPE(TypeTag, HeatConductionType);

public:
    void update(const Problem& problem, const SubControlVolumeFace &scvf)
    {
        advection_.update(problem, scvf);
        heatConduction_.update(problem, scvf);
    }

    const AdvectionType& advection() const
    {
        return advection_;
    }

    AdvectionType& advection()
    {
        return advection_;
    }

    const HeatConductionType& heatConduction() const
    {
        return heatConduction_;
    }

    HeatConductionType& heatConduction()
    {
        return heatConduction_;
    }

private:
    AdvectionType advection_;
    HeatConductionType heatConduction_;
};


// specialization for non-isothermal advection molecularDiffusion_ equations
template<class TypeTag>
class FluxVariables<TypeTag, true, true, true>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Stencil = std::set<IndexType>;
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using AdvectionType = typename GET_PROP_TYPE(TypeTag, AdvectionType);
    using MolecularDiffusionType = typename GET_PROP_TYPE(TypeTag, MolecularDiffusionType);
    using HeatConductionType = typename GET_PROP_TYPE(TypeTag, HeatConductionType);

    enum
    {
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents)
    };

public:
    void update(const Problem& problem, const SubControlVolumeFace &scvf)
    {
        advection_.update(problem, scvf);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                molecularDiffusion_.update(problem, scvf, phaseIdx, compIdx);
        heatConduction_.update(problem, scvf);
    }

    const AdvectionType& advection() const
    {
        return advection_;
    }

    AdvectionType& advection()
    {
        return advection_;
    }

    const HeatConductionType& heatConduction() const
    {
        return heatConduction_;
    }

    HeatConductionType& heatConduction()
    {
        return heatConduction_;
    }

    const MolecularDiffusionType& molecularDiffusion(const int phaseIdx, const int compIdx) const
    {
        return molecularDiffusion_[phaseIdx][compIdx];
    }

    MolecularDiffusionType& molecularDiffusion(const int phaseIdx, const int compIdx)
    {
        return molecularDiffusion_[phaseIdx][compIdx];
    }

private:
    AdvectionType advection_;
    std::array< std::array<MolecularDiffusionType, numComponents>, numPhases> molecularDiffusion_;
    HeatConductionType heatConduction_;
};


// specialization for non-isothermal molecularDiffusion_
template<class TypeTag>
class FluxVariables<TypeTag, false, true, true>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Stencil = std::set<IndexType>;
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using MolecularDiffusionType = typename GET_PROP_TYPE(TypeTag, MolecularDiffusionType);
    using HeatConductionType = typename GET_PROP_TYPE(TypeTag, HeatConductionType);

    enum
    {
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents)
    };

public:
    void update(const Problem& problem, const SubControlVolumeFace &scvf)
    {
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                molecularDiffusion_.update(problem, scvf, phaseIdx, compIdx);
        heatConduction_.update(problem, scvf);
    }

    const HeatConductionType& heatConduction() const
    {
        return heatConduction_;
    }

    HeatConductionType& heatConduction()
    {
        return heatConduction_;
    }

    const MolecularDiffusionType& molecularDiffusion(const int phaseIdx, const int compIdx) const
    {
        return molecularDiffusion_[phaseIdx][compIdx];
    }

    MolecularDiffusionType& molecularDiffusion(const int phaseIdx, const int compIdx)
    {
        return molecularDiffusion_[phaseIdx][compIdx];
    }

private:
    std::array< std::array<MolecularDiffusionType, numComponents>, numPhases> molecularDiffusion_;
    HeatConductionType heatConduction_;
};


// specialization for pure heat conduction (e.g. the heat equation)
template<class TypeTag>
class FluxVariables<TypeTag, false, false, true>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Stencil = std::set<IndexType>;
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using HeatConductionType = typename GET_PROP_TYPE(TypeTag, HeatConductionType);

public:
    void update(const Problem& problem, const SubControlVolumeFace &scvf)
    {
        heatConduction_.update(problem, scvf);
    }

    const HeatConductionType& heatConduction() const
    {
        return heatConduction_;
    }

    HeatConductionType& heatConduction()
    {
        return heatConduction_;
    }

private:
    HeatConductionType heatConduction_;
};

} // end namespace

#endif
