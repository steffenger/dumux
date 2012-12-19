// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2011 by Andreas Lauser                               *
 *   Copyright (C) 2007-2009 by Bernd Flemisch                               *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \brief Calculates the residual of models based on the box scheme element-wise.
 */
#ifndef DUMUX_CC_LOCAL_RESIDUAL_HH
#define DUMUX_CC_LOCAL_RESIDUAL_HH

#include <dune/istl/matrix.hh>
#include <dune/grid/common/geometry.hh>

#include <dumux/common/valgrind.hh>

#include "ccproperties.hh"

namespace Dumux
{
/*!
 * \ingroup CCModel
 * \ingroup CCLocalResidual
 * \brief Element-wise calculation of the residual matrix for models
 *        based on the box scheme.
 *
 * \todo Please doc me more!
 */
template<class TypeTag>
class CCLocalResidual
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, LocalResidual) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, Model) Model;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum {
        numEq = GET_PROP_VALUE(TypeTag, NumEq),
        dim = GridView::dimension
    };

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<dim>::EntityPointer VertexPointer;
    typedef typename GridView::IntersectionIterator IntersectionIterator;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, VertexMapper) VertexMapper;
    typedef typename GET_PROP_TYPE(TypeTag, ElementSolutionVector) ElementSolutionVector;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, ElementBoundaryTypes) ElementBoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;

    // copying the local residual class is not a good idea
    CCLocalResidual(const CCLocalResidual &);

public:
    CCLocalResidual()
    { }

    ~CCLocalResidual()
    { }

    /*!
     * \brief Initialize the local residual.
     *
     * This assumes that all objects of the simulation have been fully
     * allocated but not necessarily initialized completely.
     *
     * \param prob The representation of the physical problem to be
     *             solved.
     */
    void init(Problem &prob)
    { problemPtr_ = &prob; }

    /*!
     * \brief Compute the local residual, i.e. the deviation of the
     *        equations from zero.
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     */
    void eval(const Element &element)
    {
        FVElementGeometry fvElemGeom;

        fvElemGeom.update(gridView_(), element);
        fvElemGeomPtr_ = &fvElemGeom;

        ElementVolumeVariables volVarsPrev, volVarsCur;
        // update the hints
        model_().setHints(element, volVarsPrev, volVarsCur);

        volVarsPrev.update(problem_(),
                           element,
                           fvGeometry_(),
                           true /* oldSol? */);
        volVarsCur.update(problem_(),
                          element,
                          fvGeometry_(),
                          false /* oldSol? */);

        ElementBoundaryTypes bcTypes;
        bcTypes.update(problem_(), element, fvGeometry_());

        // this is pretty much a HACK because the internal state of
        // the problem is not supposed to be changed during the
        // evaluation of the residual. (Reasons: It is a violation of
        // abstraction, makes everything more prone to errors and is
        // not thread save.) The real solution are context objects!
        problem_().updateCouplingParams(element);

        asImp_().eval(element, fvGeometry_(), volVarsPrev, volVarsCur, bcTypes);
    }

    /*!
     * \brief Compute the storage term for the current solution.
     *
     * This can be used to figure out how much of each conservation
     * quantity is inside the element.
     *
     * \param element The DUNE Codim<0> entity for which the storage
     *                term ought to be calculated
     */
    void evalStorage(const Element &element)
    {
        elemPtr_ = &element;

        FVElementGeometry fvElemGeom;
        fvElemGeom.update(gridView_(), element);
        fvElemGeomPtr_ = &fvElemGeom;

        ElementBoundaryTypes bcTypes;
        bcTypes.update(problem_(), element, fvGeometry_());
        bcTypesPtr_ = &bcTypes;

        // no previous volume variables!
        prevVolVarsPtr_ = 0;

        ElementVolumeVariables volVars;

        // update the hints
        model_().setHints(element, volVars);

        // calculate volume current variables
        volVars.update(problem_(), element, fvGeometry_(), false);
        curVolVarsPtr_ = &volVars;

        asImp_().evalStorage_();
    }

    /*!
     * \brief Compute the flux term for the current solution.
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     * \param curVolVars The volume averaged variables for all
     *                   sub-contol volumes of the element
     */
    void evalFluxes(const Element &element,
                    const ElementVolumeVariables &curVolVars)
    {
        elemPtr_ = &element;

        FVElementGeometry fvElemGeom;
        fvElemGeom.update(gridView_(), element);
        fvElemGeomPtr_ = &fvElemGeom;

        ElementBoundaryTypes bcTypes;
        bcTypes.update(problem_(), element, fvGeometry_());

        residual_.resize(1);
        residual_ = 0;

        bcTypesPtr_ = &bcTypes;
        prevVolVarsPtr_ = 0;
        curVolVarsPtr_ = &curVolVars;
        asImp_().evalFluxes_();
    }

    /*!
     * \brief Compute the local residual, i.e. the deviation of the
     *        equations from zero.
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     * \param fvElemGeom The finite-volume geometry of the element
     * \param prevVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the previous
     *                   time level
     * \param curVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the current
     *                   time level
     * \param bcTypes The types of the boundary conditions for all
     *                vertices of the element
     */
    void eval(const Element &element,
              const FVElementGeometry &fvElemGeom,
              const ElementVolumeVariables &prevVolVars,
              const ElementVolumeVariables &curVolVars,
              const ElementBoundaryTypes &bcTypes)
    {
        Valgrind::CheckDefined(prevVolVars);
        Valgrind::CheckDefined(curVolVars);

#if !defined NDEBUG && HAVE_VALGRIND
        for (int i=0; i < fvElemGeom.numNeighbors; i++) {
            prevVolVars[i].checkDefined();
            curVolVars[i].checkDefined();
        }
#endif // HAVE_VALGRIND

        elemPtr_ = &element;
        fvElemGeomPtr_ = &fvElemGeom;
        bcTypesPtr_ = &bcTypes;
        prevVolVarsPtr_ = &prevVolVars;
        curVolVarsPtr_ = &curVolVars;

        // resize the vectors for all terms
        residual_.resize(1);
        storageTerm_.resize(1);

        residual_ = 0.0;
        storageTerm_ = 0.0;

        asImp_().evalFluxes_();

#if !defined NDEBUG && HAVE_VALGRIND
        Valgrind::CheckDefined(residual_[0]);
#endif // HAVE_VALGRIND

        asImp_().evalVolumeTerms_();

#if !defined NDEBUG && HAVE_VALGRIND
        Valgrind::CheckDefined(residual_[0]);
#endif // HAVE_VALGRIND

        // evaluate the boundary conditions
        asImp_().evalBoundary_();

#if !defined NDEBUG && HAVE_VALGRIND
        Valgrind::CheckDefined(residual_[0]);
#endif // HAVE_VALGRIND
    }

    /*!
     * \brief Add Outflow boundary conditions for a single sub-control
     *        volume face to the local residual.
     *
     * \note This is so far an empty method doing not more than
     *       nothing. A beta version is available for the 2p2c and
     *       2p2cni model. There you can find a sample implementation.
     */
    void evalOutflowSegment(const IntersectionIterator &isIt,
                            int scvIdx,
                            int boundaryFaceIdx)
    {}

    /*!
     * \brief Returns the local residual for all sub-control
     *        volumes of the element.
     */
    const ElementSolutionVector &residual() const
    { return residual_; }

    /*!
     * \brief Returns the local residual for a given sub-control
     *        volume of the element.
     *
     * \param scvIdx The local index of the sub-control volume
     *               (i.e. the element's local vertex index)
     */
    const PrimaryVariables &residual(int scvIdx) const
    { return residual_[scvIdx]; }

    /*!
     * \brief Returns the storage term for all sub-control volumes of the
     *        element.
     */
    const ElementSolutionVector &storageTerm() const
    { return storageTerm_; }

    /*!
     * \brief Returns the storage term for a given sub-control volumes
     *        of the element.
     */
    const PrimaryVariables &storageTerm(int scvIdx) const
    { return storageTerm_[scvIdx]; }

    void evalOutflowSegment(const IntersectionIterator &isIt)
    {
        const BoundaryTypes &bcTypes = this->bcTypes_(0);
        
        // deal with outflow boundaries
        if (bcTypes.hasOutflow())
        {
            PrimaryVariables values(0.0);
            asImp_().computeFlux(values, 
                                  /*boundaryFaceIdx=*/isIt->indexInInside(), 
                                  true);
            Valgrind::CheckDefined(values);
            
            for (int eqIdx = 0; eqIdx < numEq; ++eqIdx)
            {
                if (!bcTypes.isOutflow(eqIdx) )
                    continue;
                this->residual_[0][eqIdx] += values[eqIdx];
            }
        }
    }
protected:
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

    /*!
     * \brief Evaluate the boundary conditions
     *        of the current element.
     */
    void evalBoundary_()
    {
        if (bcTypes_().hasNeumann() || bcTypes_().hasOutflow())
            asImp_().evalBoundaryFluxes_();

#if !defined NDEBUG && HAVE_VALGRIND
        Valgrind::CheckDefined(residual_[0]);
#endif // HAVE_VALGRIND

        if (bcTypes_().hasDirichlet())
            asImp_().evalDirichlet_();
    }

    /*!
     * \brief Set the values of the Dirichlet boundary control volumes
     *        of the current element.
     */
    void evalDirichlet_()
    {
        PrimaryVariables tmp(0);

        const BoundaryTypes &bcTypes = bcTypes_(0);
        if (! bcTypes.hasDirichlet())
            return;

        Valgrind::SetUndefined(tmp);
	// HACK: ask for Dirichlet value at element center
        asImp_().problem_().dirichletAtPos(tmp, element_().geometry().center());

        // set the dirichlet conditions
        for (int eqIdx = 0; eqIdx < numEq; ++eqIdx) {
            if (!bcTypes.isDirichlet(eqIdx))
                continue;
            int pvIdx = bcTypes.eqToDirichletIndex(eqIdx);
            assert(0 <= pvIdx && pvIdx < numEq);
            Valgrind::CheckDefined(tmp[pvIdx]);

            residual_[0][eqIdx] =
                    (curPriVar_(0, pvIdx) - tmp[pvIdx]);

            storageTerm_[0][eqIdx] = 0.0;
        }
    }

    /*!
     * \brief Add all Neumann and outflow boundary conditions to the local
     *        residual.
     */
    void evalBoundaryFluxes_()
    {
        IntersectionIterator isIt = gridView_().ibegin(element_());
        const IntersectionIterator &endIt = gridView_().iend(element_());
        for (; isIt != endIt; ++isIt)
        {
            // handle only faces on the boundary
            if (!isIt->boundary())
                continue;

            // add the residual of all vertices of the boundary
            // segment
            asImp_().evalNeumannSegment_(isIt);
		
            // evaluate the outflow conditions at the boundary face
            // ATTENTION: This is so far a beta version that is only for the 2p2c and 2p2cni model
            //              available and not thoroughly tested.
            asImp_().evalOutflowSegment(isIt);
        }
    }

    /*!
     * \brief Add Neumann boundary conditions for a single sub-control
     *        volume face to the local residual.
     */
    void evalNeumannSegment_(const IntersectionIterator &isIt)
    {
        // temporary vector to store the neumann boundary fluxes
        PrimaryVariables values(0.0);

        BoundaryTypes bcTypes;
        problem_().boundaryTypes(bcTypes, *isIt);

        // deal with neumann boundaries
        if (bcTypes.hasNeumann()) {
            Valgrind::SetUndefined(values);
            problem_().boxSDNeumann(values,
                                    element_(),
                                    fvGeometry_(),
                                    *isIt,
                                    /*scvIdx=*/0,
                                    /*boundaryFaceIdx=*/isIt->indexInInside(),
                                    curVolVars_());
            values *= isIt->geometry().volume()
                * curVolVars_(0).extrusionFactor();
            Valgrind::CheckDefined(values);

            // set the neumann conditions
            for (int eqIdx = 0; eqIdx < numEq; ++eqIdx) {
                if (!bcTypes.isNeumann(eqIdx))
                    continue;
                residual_[0][eqIdx] += values[eqIdx];
            }
        }
    }

    /*!
     * \brief Add the flux terms to the local residual of all
     *        sub-control volumes of the current element.
     */
    void evalFluxes_()
    {
        // calculate the mass flux over the faces and subtract
        // it from the local rates
        int faceIdx = -1;
        IntersectionIterator isIt = gridView_().ibegin(element_());
        IntersectionIterator isEndIt = gridView_().iend(element_());
        for (; isIt != isEndIt; ++isIt) {
            if (!isIt->neighbor())
                continue;

            faceIdx++;
	    PrimaryVariables flux;

            Valgrind::SetUndefined(flux);
            asImp_().computeFlux(flux, faceIdx);
            Valgrind::CheckDefined(flux);

            flux *= curVolVars_(0).extrusionFactor();

            // The balance equation for a finite volume is:
            //
            // dStorage/dt = Flux + Source
            //
            // where the 'Flux' and the 'Source' terms represent the
            // mass per second which _ENTER_ the finite
            // volume. Re-arranging this, we get
            //
            // dStorage/dt - Source - Flux = 0
            //
            // Since the flux calculated by computeFlux() goes _OUT_
            // of sub-control volume i and _INTO_ sub-control volume
            // j, we need to add the flux to finite volume i and
            // subtract it from finite volume j
            residual_[0] += flux;
        }
    }

    /*!
     * \brief Set the local residual to the storage terms of all
     *        sub-control volumes of the current element.
     */
    void evalStorage_()
    {
        storageTerm_.resize(1);
        storageTerm_ = 0;

        // calculate the amount of conservation each quantity inside
        // all sub control volumes
        Valgrind::SetUndefined(storageTerm_[0]);
        asImp_().computeStorage(storageTerm_[0], 0, /*isOldSol=*/false);
        storageTerm_[0] *= fvGeometry_().subContVol[0].volume
                * curVolVars_(0).extrusionFactor();
        Valgrind::CheckDefined(storageTerm_[0]);
    }

    /*!
     * \brief Add the change in the storage terms and the source term
     *        to the local residual of all sub-control volumes of the
     *        current element.
     */
    void evalVolumeTerms_()
    {
        // evaluate the volume terms (storage + source terms)
        for (int i=0; i < 1; i++)
        {
            Scalar extrusionFactor =
                curVolVars_(i).extrusionFactor();

            PrimaryVariables tmp(0.);

            // mass balance within the element. this is the
            // \f$\frac{m}{\partial t}\f$ term if using implicit
            // euler as time discretization.
            //
            // TODO (?): we might need a more explicit way for
            // doing the time discretization...
            Valgrind::SetUndefined(storageTerm_[i]);
            Valgrind::SetUndefined(tmp);
            asImp_().computeStorage(storageTerm_[i], i, false);
            asImp_().computeStorage(tmp, i, true);
            Valgrind::CheckDefined(storageTerm_[i]);
            Valgrind::CheckDefined(tmp);

            storageTerm_[i] -= tmp;
            storageTerm_[i] *=
                fvGeometry_().subContVol[i].volume
                / problem_().timeManager().timeStepSize()
                * extrusionFactor;
            residual_[i] += storageTerm_[i];

            // subtract the source term from the local rate
            Valgrind::SetUndefined(tmp);
            asImp_().computeSource(tmp, i);
            Valgrind::CheckDefined(tmp);
            tmp *= fvGeometry_().subContVol[i].volume * extrusionFactor;
            residual_[i] -= tmp;

            // make sure that only defined quantities were used
            // to calculate the residual.
            Valgrind::CheckDefined(residual_[i]);
        }
    }

    /*!
     * \brief Returns a reference to the problem.
     */
    const Problem &problem_() const
    { return *problemPtr_; };

    /*!
     * \brief Returns a reference to the model.
     */
    const Model &model_() const
    { return problem_().model(); };

    /*!
     * \brief Returns a reference to the vertex mapper.
     */
    const VertexMapper &vertexMapper_() const
    { return problem_().vertexMapper(); };

    /*!
     * \brief Returns a reference to the grid view.
     */
    const GridView &gridView_() const
    { return problem_().gridView(); }

    /*!
     * \brief Returns a reference to the current element.
     */
    const Element &element_() const
    {
        Valgrind::CheckDefined(elemPtr_);
        return *elemPtr_;
    }

    /*!
     * \brief Returns a reference to the current element's finite
     *        volume geometry.
     */
    const FVElementGeometry &fvGeometry_() const
    {
        Valgrind::CheckDefined(fvElemGeomPtr_);
        return *fvElemGeomPtr_;
    }

    /*!
     * \brief Returns a reference to the primary variables of
     *           the last time step of the i'th
     *        sub-control volume of the current element.
     */
    const PrimaryVariables &prevPriVars_(int i) const
    {
        return prevVolVars_(i).priVars();
    }

    /*!
     * \brief Returns a reference to the primary variables of the i'th
     *        sub-control volume of the current element.
     */
    const PrimaryVariables &curPriVars_(int i) const
    {
        return curVolVars_(i).priVars();
    }

    /*!
     * \brief Returns the j'th primary of the i'th sub-control volume
     *        of the current element.
     */
    Scalar curPriVar_(int i, int j) const
    {
        return curVolVars_(i).priVar(j);
    }

    /*!
     * \brief Returns a reference to the current volume variables of
     *        all sub-control volumes of the current element.
     */
    const ElementVolumeVariables &curVolVars_() const
    {
        Valgrind::CheckDefined(curVolVarsPtr_);
        return *curVolVarsPtr_;
    }

    /*!
     * \brief Returns a reference to the volume variables of the i-th
     *        sub-control volume of the current element.
     */
    const VolumeVariables &curVolVars_(int i) const
    {
        return curVolVars_()[i];
    }

    /*!
     * \brief Returns a reference to the previous time step's volume
     *        variables of all sub-control volumes of the current
     *        element.
     */
    const ElementVolumeVariables &prevVolVars_() const
    {
        Valgrind::CheckDefined(prevVolVarsPtr_);
        return *prevVolVarsPtr_;
    }

    /*!
     * \brief Returns a reference to the previous time step's volume
     *        variables of the i-th sub-control volume of the current
     *        element.
     */
    const VolumeVariables &prevVolVars_(int i) const
    {
        return prevVolVars_()[i];
    }

    /*!
     * \brief Returns a reference to the boundary types of all
     *        sub-control volumes of the current element.
     */
    const ElementBoundaryTypes &bcTypes_() const
    {
        Valgrind::CheckDefined(bcTypesPtr_);
        return *bcTypesPtr_;
    }

    /*!
     * \brief Returns a reference to the boundary types of the i-th
     *        sub-control volume of the current element.
     */
    const BoundaryTypes &bcTypes_(int i) const
    {
        return bcTypes_()[i];
    }

protected:
    ElementSolutionVector storageTerm_;
    ElementSolutionVector residual_;

    // The problem we would like to solve
    Problem *problemPtr_;

    const Element *elemPtr_;
    const FVElementGeometry *fvElemGeomPtr_;

    // current and previous secondary variables for the element
    const ElementVolumeVariables *prevVolVarsPtr_;
    const ElementVolumeVariables *curVolVarsPtr_;

    const ElementBoundaryTypes *bcTypesPtr_;
};

}

#endif
