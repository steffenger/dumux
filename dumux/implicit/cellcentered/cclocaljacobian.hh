// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
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
 * \brief Caculates the Jacobian of the local residual for box models
 */
#ifndef DUMUX_CC_LOCAL_JACOBIAN_HH
#define DUMUX_CC_LOCAL_JACOBIAN_HH

#include <dune/istl/matrix.hh>

#include <dumux/common/math.hh>
#include "ccelementboundarytypes.hh"

namespace Dumux
{
/*!
 * \ingroup CCModel
 * \ingroup CCLocalJacobian
 * \brief Calculates the Jacobian of the local residual for box models
 *
 * The default behavior is to use numeric differentiation, i.e.
 * forward or backward differences (2nd order), or central
 * differences (3rd order). The method used is determined by the
 * "NumericDifferenceMethod" property:
 *
 * - if the value of this property is smaller than 0, backward
 *   differences are used, i.e.:
 *   \f[
 \frac{\partial f(x)}{\partial x} \approx \frac{f(x) - f(x - \epsilon)}{\epsilon}
 *   \f]
 *
 * - if the value of this property is 0, central
 *   differences are used, i.e.:
 *   \f[
 \frac{\partial f(x)}{\partial x} \approx \frac{f(x + \epsilon) - f(x - \epsilon)}{2 \epsilon}
 *   \f]
 *
 * - if the value of this property is larger than 0, forward
 *   differences are used, i.e.:
 *   \f[
 \frac{\partial f(x)}{\partial x} \approx \frac{f(x + \epsilon) - f(x)}{\epsilon}
 *   \f]
 *
 * Here, \f$ f \f$ is the residual function for all equations, \f$x\f$
 * is the value of a sub-control volume's primary variable at the
 * evaluation point and \f$\epsilon\f$ is a small value larger than 0.
 */
template<class TypeTag>
class CCLocalJacobian
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, LocalJacobian) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, LocalResidual) LocalResidual;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, Model) Model;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GET_PROP_TYPE(TypeTag, JacobianAssembler) JacobianAssembler;

    enum {
        numEq = GET_PROP_VALUE(TypeTag, NumEq),
        dim = GridView::dimension
    };

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, VertexMapper) VertexMapper;
    typedef typename GET_PROP_TYPE(TypeTag, ElementSolutionVector) ElementSolutionVector;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;

    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementBoundaryTypes) ElementBoundaryTypes;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef Dune::FieldMatrix<Scalar, numEq, numEq> MatrixBlock;
    typedef Dune::Matrix<MatrixBlock> LocalBlockMatrix;

    // copying a local jacobian is not a good idea
    CCLocalJacobian(const CCLocalJacobian &);

public:
    CCLocalJacobian()
    {
        numericDifferenceMethod_ = GET_PARAM(TypeTag, int, ImplicitNumericDifferenceMethod);
        Valgrind::SetUndefined(problemPtr_);
    }


    /*!
     * \brief Initialize the local Jacobian object.
     *
     * At this point we can assume that everything has been allocated,
     * although some objects may not yet be completely initialized.
     *
     * \param prob The problem which we want to simulate.
     */
    void init(Problem &prob)
    {
        problemPtr_ = &prob;
        localResidual_.init(prob);

        // assume quadrilinears as elements with most vertices
        A_.setSize(1, 2<<dim);
        storageJacobian_.resize(1);
    }

    /*!
     * \brief Assemble an element's local Jacobian matrix of the
     *        defect.
     *
     * \param element The DUNE Codim<0> entity which we look at.
     */
    void assemble(const Element &element)
    {
        // set the current grid element and update the element's
        // finite volume geometry
        elemPtr_ = &element;
        fvElemGeom_.update(gridView_(), element);
        reset_();

        bcTypes_.update(problem_(), elem_(), fvElemGeom_);

        // this is pretty much a HACK because the internal state of
        // the problem is not supposed to be changed during the
        // evaluation of the residual. (Reasons: It is a violation of
        // abstraction, makes everything more prone to errors and is
        // not thread save.) The real solution are context objects!
        problem_().updateCouplingParams(elem_());

        // set the hints for the volume variables
        model_().setHints(element, prevVolVars_, curVolVars_);

        // update the secondary variables for the element at the last
        // and the current time levels
        prevVolVars_.update(problem_(),
                            elem_(),
                            fvElemGeom_,
                            true /* isOldSol? */);

        curVolVars_.update(problem_(),
                           elem_(),
                           fvElemGeom_,
                           false /* isOldSol? */);

        // update the hints of the model
        model_().updateCurHints(element, curVolVars_);

        // calculate the local residual
        localResidual().eval(elem_(),
                             fvElemGeom_,
                             prevVolVars_,
                             curVolVars_,
                             bcTypes_);
        residual_ = localResidual().residual();
        storageTerm_ = localResidual().storageTerm();

        model_().updatePVWeights(elem_(), curVolVars_);

        // calculate the local jacobian matrix
        int numNeighbors = fvElemGeom_.numNeighbors;
        ElementSolutionVector partialDeriv(1);
        PrimaryVariables storageDeriv(0.0);
        for (int j = 0; j < numNeighbors; j++) {
            for (int pvIdx = 0; pvIdx < numEq; pvIdx++) {
                asImp_().evalPartialDerivative_(partialDeriv,
                                                storageDeriv,
                                                j,
                                                pvIdx);

                // update the local stiffness matrix with the current partial
                // derivatives
                updateLocalJacobian_(j,
                                     pvIdx,
                                     partialDeriv,
                                     storageDeriv);
            }
        }
    }

    /*!
     * \brief Returns a reference to the object which calculates the
     *        local residual.
     */
    const LocalResidual &localResidual() const
    { return localResidual_; }

    /*!
     * \brief Returns a reference to the object which calculates the
     *        local residual.
     */
    LocalResidual &localResidual()
    { return localResidual_; }

    /*!
     * \brief Returns the Jacobian of the equations at vertex i to the
     *        primary variables at vertex j.
     *
     * \param i The local vertex (or sub-control volume) index on which
     *          the equations are defined
     * \param j The local vertex (or sub-control volume) index which holds
     *          primary variables
     */
    const MatrixBlock &mat(int i, int j) const
    { return A_[i][j]; }

    /*!
     * \brief Returns the Jacobian of the storage term at vertex i.
     *
     * \param i The local vertex (or sub-control volume) index
     */
    const MatrixBlock &storageJacobian(int i) const
    { return storageJacobian_[i]; }

    /*!
     * \brief Returns the residual of the equations at vertex i.
     *
     * \param i The local vertex (or sub-control volume) index on which
     *          the equations are defined
     */
    const PrimaryVariables &residual(int i) const
    { return residual_[i]; }

    /*!
     * \brief Returns the storage term for vertex i.
     *
     * \param i The local vertex (or sub-control volume) index on which
     *          the equations are defined
     */
    const PrimaryVariables &storageTerm(int i) const
    { return storageTerm_[i]; }

    /*!
     * \brief Returns the epsilon value which is added and removed
     *        from the current solution.
     *
     * \param scvIdx     The local index of the element's vertex for
     *                   which the local derivative ought to be calculated.
     * \param pvIdx      The index of the primary variable which gets varied
     */
    Scalar numericEpsilon(int scvIdx,
                          int pvIdx) const
    {
        // define the base epsilon as the geometric mean of 1 and the
        // resolution of the scalar type. E.g. for standard 64 bit
        // floating point values, the resolution is about 10^-16 and
        // the base epsilon is thus approximately 10^-8.
        /*
        static const Scalar baseEps
            = Dumux::geometricMean<Scalar>(std::numeric_limits<Scalar>::epsilon(), 1.0);
        */
        static const Scalar baseEps = 1e-10;
        assert(std::numeric_limits<Scalar>::epsilon()*1e4 < baseEps);
        // the epsilon value used for the numeric differentiation is
        // now scaled by the absolute value of the primary variable...
        Scalar pv = this->curVolVars_[scvIdx].priVar(pvIdx);
        return baseEps*(std::abs(pv) + 1.0);
    }

protected:
    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }
    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }

    /*!
     * \brief Returns a reference to the problem.
     */
    const Problem &problem_() const
    {
        Valgrind::CheckDefined(problemPtr_);
        return *problemPtr_;
    };

    /*!
     * \brief Returns a reference to the grid view.
     */
    const GridView &gridView_() const
    { return problem_().gridView(); }

    /*!
     * \brief Returns a reference to the element.
     */
    const Element &elem_() const
    {
        Valgrind::CheckDefined(elemPtr_);
        return *elemPtr_;
    };

    /*!
     * \brief Returns a reference to the model.
     */
    const Model &model_() const
    { return problem_().model(); };

    /*!
     * \brief Returns a reference to the jacobian assembler.
     */
    const JacobianAssembler &jacAsm_() const
    { return model_().jacobianAssembler(); }

    /*!
     * \brief Returns a reference to the vertex mapper.
     */
    const VertexMapper &vertexMapper_() const
    { return problem_().vertexMapper(); };

    /*!
     * \brief Reset the local jacobian matrix to 0
     */
    void reset_()
    {
        for (int i = 0; i < 1; ++ i) {
            storageJacobian_[i] = 0.0;
            for (int j = 0; j < A_.M(); ++ j) {
                A_[i][j] = 0.0;
            }
        }
    }

    /*!
     * \brief Compute the partial derivatives to a primary variable at
     *        an degree of freedom.
     *
     * This method can be overwritten by the implementation if a
     * better scheme than numerical differentiation is available.
     *
     * The default implementation of this method uses numeric
     * differentiation, i.e. forward or backward differences (2nd
     * order), or central differences (3rd order). The method used is
     * determined by the "NumericDifferenceMethod" property:
     *
     * - if the value of this property is smaller than 0, backward
     *   differences are used, i.e.:
     *   \f[
         \frac{\partial f(x)}{\partial x} \approx \frac{f(x) - f(x - \epsilon)}{\epsilon}
     *   \f]
     *
     * - if the value of this property is 0, central
     *   differences are used, i.e.:
     *   \f[
           \frac{\partial f(x)}{\partial x} \approx \frac{f(x + \epsilon) - f(x - \epsilon)}{2 \epsilon}
     *   \f]
     *
     * - if the value of this property is larger than 0, forward
     *   differences are used, i.e.:
     *   \f[
           \frac{\partial f(x)}{\partial x} \approx \frac{f(x + \epsilon) - f(x)}{\epsilon}
     *   \f]
     *
     * Here, \f$ f \f$ is the residual function for all equations, \f$x\f$
     * is the value of a sub-control volume's primary variable at the
     * evaluation point and \f$\epsilon\f$ is a small value larger than 0.
     *
     * \param dest The vector storing the partial derivatives of all
     *              equations
     * \param destStorage the mass matrix contributions
     * \param scvIdx The sub-control volume index of the current
     *               finite element for which the partial derivative
     *               ought to be calculated
     * \param pvIdx The index of the primary variable at the scvIdx'
     *              sub-control volume of the current finite element
     *              for which the partial derivative ought to be
     *              calculated
     */
    void evalPartialDerivative_(ElementSolutionVector &dest,
                                PrimaryVariables &destStorage,
                                int neighborIdx,
                                int pvIdx)
    {
        const Element& neighbor = *(fvElemGeom_.neighbors[neighborIdx]);
	FVElementGeometry neighborFVGeom;
	neighborFVGeom.updateInner(problemPtr_->gridView(), neighbor);
	
        int globalIdx = problemPtr_->elementMapper().map(neighbor);

        PrimaryVariables priVars(model_().curSol()[globalIdx]);
        VolumeVariables origVolVars(curVolVars_[neighborIdx]);

        curVolVars_[neighborIdx].setEvalPoint(&origVolVars);
        Scalar eps = asImp_().numericEpsilon(neighborIdx, pvIdx);
        Scalar delta = 0;

        if (numericDifferenceMethod_ >= 0) {
            // we are not using backward differences, i.e. we need to
            // calculate f(x + \epsilon)

            // deflect primary variables
            priVars[pvIdx] += eps;
            delta += eps;

            // calculate the residual
            curVolVars_[neighborIdx].update(priVars,
                                       problem_(),
                                       neighbor,
                                       neighborFVGeom,
                                       /*scvIdx=*/0,
                                       false);
            localResidual().eval(elem_(),
                                 fvElemGeom_,
                                 prevVolVars_,
                                 curVolVars_,
                                 bcTypes_);

            // store the residual and the storage term
            dest = localResidual().residual();
	    if (neighborIdx == 0)
                destStorage = localResidual().storageTerm()[neighborIdx];
        }
        else {
            // we are using backward differences, i.e. we don't need
            // to calculate f(x + \epsilon) and we can recycle the
            // (already calculated) residual f(x)
            dest = residual_;
	    if (neighborIdx == 0)
                destStorage = storageTerm_[neighborIdx];
        }


        if (numericDifferenceMethod_ <= 0) {
            // we are not using forward differences, i.e. we don't
            // need to calculate f(x - \epsilon)

            // deflect the primary variables
            priVars[pvIdx] -= delta + eps;
            delta += eps;

            // calculate residual again
            curVolVars_[neighborIdx].update(priVars,
                                       problem_(),
                                       neighbor,
                                       neighborFVGeom,
                                       /*scvIdx=*/0,
                                       false);
            localResidual().eval(elem_(),
                                 fvElemGeom_,
                                 prevVolVars_,
                                 curVolVars_,
                                 bcTypes_);
            dest -= localResidual().residual();
	    if (neighborIdx == 0)
                destStorage -= localResidual().storageTerm()[neighborIdx];
        }
        else {
            // we are using forward differences, i.e. we don't need to
            // calculate f(x - \epsilon) and we can recycle the
            // (already calculated) residual f(x)
            dest -= residual_;
	    if (neighborIdx == 0)
                destStorage -= storageTerm_[neighborIdx];
        }

        // divide difference in residuals by the magnitude of the
        // deflections between the two function evaluation
        dest /= delta;
        destStorage /= delta;

        // restore the original state of the element's volume variables
        curVolVars_[neighborIdx] = origVolVars;

#if HAVE_VALGRIND
        for (unsigned i = 0; i < dest.size(); ++i)
            Valgrind::CheckDefined(dest[i]);
#endif
    }

    /*!
     * \brief Updates the current local Jacobian matrix with the
     *        partial derivatives of all equations in regard to the
     *        primary variable 'pvIdx' at vertex 'scvIdx' .
     */
    void updateLocalJacobian_(int neighborIdx,
                              int pvIdx,
                              const ElementSolutionVector &deriv,
                              const PrimaryVariables &storageDeriv)
    {
        // store the derivative of the storage term
        if (neighborIdx == 0)
            for (int eqIdx = 0; eqIdx < numEq; eqIdx++) {
                storageJacobian_[neighborIdx][eqIdx][pvIdx] = storageDeriv[eqIdx];
            }

        for (int i = 0; i < 1; i++)
        {
            for (int eqIdx = 0; eqIdx < numEq; eqIdx++) {
                // A[i][scvIdx][eqIdx][pvIdx] is the rate of change of
                // the residual of equation 'eqIdx' at vertex 'i'
                // depending on the primary variable 'pvIdx' at vertex
                // 'scvIdx'.
                this->A_[i][neighborIdx][eqIdx][pvIdx] = deriv[i][eqIdx];
                Valgrind::CheckDefined(this->A_[i][neighborIdx][eqIdx][pvIdx]);
            }
        }
    }

    const Element *elemPtr_;
    FVElementGeometry fvElemGeom_;

    ElementBoundaryTypes bcTypes_;

    // The problem we would like to solve
    Problem *problemPtr_;

    // secondary variables at the previous and at the current time
    // levels
    ElementVolumeVariables prevVolVars_;
    ElementVolumeVariables curVolVars_;

    LocalResidual localResidual_;

    LocalBlockMatrix A_;
    std::vector<MatrixBlock> storageJacobian_;

    ElementSolutionVector residual_;
    ElementSolutionVector storageTerm_;

    int numericDifferenceMethod_;
};
}

#endif
