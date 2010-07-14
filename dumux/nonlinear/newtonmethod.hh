// $Id: newtonmethod.hh 3738 2010-06-15 14:01:09Z lauser $
/*****************************************************************************
 *   Copyright (C) 2008 by Andreas Lauser, Bernd Flemisch                    *
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version, as long as this copyright notice    *
 *   is included in its original form.                                       *
 *                                                                           *
 *   This program is distributed WITHOUT ANY WARRANTY.                       *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief The algorithmic part of the multi dimensional newton method.
 *
 * In order to use the method you need a NewtonController.
 */
#ifndef DUMUX_NEWTONMETHOD_HH
#define DUMUX_NEWTONMETHOD_HH

#ifdef DUMUX_NEWTONMETHOD_DEPRECATED_HH
# error "Never use the old and the new newton method in one program!"
#endif

#include <limits>
#include <dumux/common/exceptions.hh>

#include <dumux/io/vtkmultiwriter.hh>


namespace Dumux
{
/*!
 * \brief The algorithmic part of the multi dimensional newton method.
 *
 * In order to use the method you need a NewtonController.
 */
template <class TypeTag>
class NewtonMethod
{
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Model)) Model;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(LocalJacobian)) LocalJacobian;

    typedef typename GET_PROP(TypeTag, PTAG(SolutionTypes)) SolutionTypes;
    typedef typename SolutionTypes::SolutionVector          SolutionVector;
    typedef typename SolutionTypes::JacobianAssembler       JacobianAssembler;
public:
    NewtonMethod(Model &model)
        : model_(model)
    { }

    /*!
     * \brief Returns a reference to the current numeric model.
     */
    Model &model()
    { return model_; }

    /*!
     * \brief Returns a reference to the current numeric model.
     */
    const Model &model() const
    { return model_; }

    /*!
     * \brief Returns true iff the newton method should be verbose
     *        about what it is going on;
     */
    bool verbose() const
    { return (model().gridView().comm().rank() == 0); }

    /*!
     * \brief Run the newton method. The controller is responsible
     *        for all the strategic decisions.
     */
    template <class NewtonController>
    bool execute(NewtonController &ctl)
    {
        try {
            return execute_(ctl);
        }
        catch (const Dune::ISTLError &e) {
            if (verbose())
                std::cout << "Newton: Caught exception: \"" << e.what() << "\"\n";
            ctl.newtonFail();
            return false;
        }
        catch (const Dumux::NumericalProblem &e) {
            if (verbose())
                std::cout << "Newton: Caught exception: \"" << e.what() << "\"\n";
            ctl.newtonFail();
            return false;
        };
    }

protected:
    template <class NewtonController>
    bool execute_(NewtonController &ctl)
    {
        // TODO (?): u shouldn't be hard coded to the model
        SolutionVector &u = model_.curSol();
        JacobianAssembler &jacobianAsm = model_.jacobianAssembler();

        // tell the controller that we begin solving
        ctl.newtonBegin(this, u);

        // execute the method as long as the controller thinks
        // that we should do another iteration
        while (ctl.newtonProceed(u))
        {
            // notify the controller that we're about to start
            // a new timestep
            ctl.newtonBeginStep();

            // make the current solution to the old one
            uOld_ = u;

            if (verbose()) {
                std::cout << "Assembling global jacobian";
                std::cout.flush();
            }
            // linearize the problem at the current solution
            jacobianAsm.assemble(u);

            // solve the resultuing linear equation system
            if (verbose()) {
                std::cout << "\rSolve Mx = r              ";
                std::cout.flush();
            }

            // set the delta vector to zero before solving the linear system!
            u = 0;

            // ask the controller to solve the linearized system
            ctl.newtonSolveLinear(jacobianAsm.matrix(),
                                  u,
                                  jacobianAsm.residual());

            // update the current solution (i.e. uOld) with the delta
            // (i.e. u). The result is stored in u
            ctl.newtonUpdate(u, uOld_);

            // tell the controller that we're done with this iteration
            ctl.newtonEndStep(u, uOld_);
        }

        // tell the controller that we're done
        ctl.newtonEnd();

        if (!ctl.newtonConverged()) {
            ctl.newtonFail();
            return false;
        }

        ctl.newtonSucceed();
        return true;
    }


private:
    SolutionVector uOld_;
    SolutionVector residual_;

    Model &model_;
    bool verbose_;
};
}

#endif
