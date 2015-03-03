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
 *
 * \brief Test for the coupled non-isothermal two-component Stokes and
 *        non-isothermal two-phase two-component Darcy model
 */

#include "config.h"
#include <iostream>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/parametertreeparser.hh>
#include <dumux/io/interfacemeshcreator.hh>

#include "2cnistokes2p2cniproblem.hh"

/*!
 * \brief Print a usage string for simulations.
 *
 * \param progName The name of the executable
 */
void printUsage(const char *progName)
{
    std::cout << "usage: " << progName
              << " -ParameterFile FILENAME\n";
    exit(1);
}

template <class TypeTag>
int start_(int argc,
           char **argv)
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, MultiDomainGrid) MDGrid;
    typedef typename GET_PROP_TYPE(TypeTag, GridCreator) GridCreator;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

//    typedef Dune::GridPtr<Grid> GridPointer;

    ////////////////////////////////////////////////////////////
    // Load the input parameters
    ////////////////////////////////////////////////////////////

    typedef typename GET_PROP(TypeTag, ParameterTree) ParameterTree;
    Dune::ParameterTreeParser::readOptions(argc, argv, ParameterTree::tree());

    if (ParameterTree::tree().hasKey("ParameterFile") or argc==1)
    {
        // read input file, but do not overwrite options specified
        // on the command line, since the latter have precedence.
        std::string inputFileName ;
        if(argc==1) // if there are no arguments given (and there is a file ./<programname>.input) we use it as input file
        {
            std::cout<< "\nNo parameter file given. \n"
                     << "Defaulting to '"
                     << argv[0]
                     << ".input' for input file.\n";
            inputFileName = argv[0];
            inputFileName += ".input";
        }
        else
            inputFileName = GET_RUNTIME_PARAM(TypeTag, std::string, ParameterFile); // otherwise we read from the command line

        std::ifstream parameterFile;

        // check whether the parameter file exists.
        parameterFile.open(inputFileName.c_str());
        if (not parameterFile.is_open()){
            std::cout<< "\n\t -> Could not open file"
                     << inputFileName
                     << ". <- \n\n\n\n";
            printUsage(argv[0]);
            return 1;
        }
        parameterFile.close();

        Dune::ParameterTreeParser::readINITree(inputFileName,
                                               ParameterTree::tree(),
                                               /*overwrite=*/false);
    }

    // initialize MPI, finalize is done automatically on exit
    static Dune::MPIHelper& mpiHelper = Dune::MPIHelper::instance(argc, argv);

    // define the problem dimensions
    const int dim=2;

    std::string dgfFileName;
    Scalar dt, tEnd;
    Dune::FieldVector<int, dim> nElements;
    Scalar interfacePosY, gradingFactorY;
    int gridRefinement;
    bool useInterfaceMeshCreator;

    dgfFileName = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, std::string, Grid, File);
    dt = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, TimeManager, DtInitial);
    tEnd = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, TimeManager, TEnd);
	nElements[0] = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Grid, CellsX);
    nElements[1] = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Grid, CellsY);
    interfacePosY = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, InterfacePosY);
    gradingFactorY = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, GradingFactorY);
    gridRefinement = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, Refinement);
    useInterfaceMeshCreator = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, bool, Grid, UseInterfaceMeshCreator);

    std::cout << "Starting with timestep size = " << dt << "s, simulation end = " << tEnd << "s\n";

    if (useInterfaceMeshCreator)
    {
        Dumux::InterfaceMeshCreator<Grid> interfaceMeshCreator;
        GridCreator::gridPtr() = interfaceMeshCreator.create(dgfFileName, nElements, interfacePosY, gradingFactorY);
    }
    else
    {
        // try to create a grid (from the given grid file)
        try { GridCreator::makeGrid(); }
        catch (...) {
            std::string usageMessage = "\n\t -> Creation of the grid failed! <- \n\n\n\n";
//            usageMessage += usageTextBlock();
//            usage(argv[0], usageMessage);
            throw;
        }
    }

    if (gridRefinement)
    	GridCreator::grid().globalRefine(gridRefinement);

    if (mpiHelper.size() > 1) {
        if (!Dune::Capabilities::isParallel<Grid>::v) {
            std::cerr << "WARNING: THE PROGRAM IS STARTED USING MPI, BUT THE GRID IMPLEMENTATION\n"
                      << "         YOU HAVE CHOSEN IS NOT PARALLEL!\n";
        }
    	GridCreator::loadBalance();
    }

    // Instantiate the time manager
    TimeManager timeManager;

    // instantiate coupled problem
    Dune::shared_ptr<MDGrid> mdGrid_ = Dune::make_shared<MDGrid> (GridCreator::grid());

    // instantiate coupled problem
    Problem problem(*mdGrid_,
                    timeManager);

    Dumux::Parameters::print<TypeTag>();

    // deal with the restart stuff
    bool restart = false;
    Scalar restartTime = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, TimeManager, DtInitial);
    if (ParameterTree::tree().hasKey("Restart") 
        || ParameterTree::tree().hasKey("TimeManager.Restart")) {
        restart = true;
        restartTime = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, TimeManager, Restart);
    }

    // run the simulation
    timeManager.init(problem,
                     restartTime, // initial time
                     dt, // initial time step
                     tEnd, // final time
                     restart);

    // print all properties
    Dumux::Properties::print<TypeTag>();

    timeManager.run();

    return 0;
}

/*!
 * \brief Provides a main function which reads in parameters from the
 *        command line and a parameter file.
 *
 * In this function only the differentiation between debugger
 * or not is made.
 *
 * \tparam TypeTag  The type tag of the problem which needs to be solved
 *
 * \param argc  The number of command line arguments of the program
 * \param argv  The contents of the command line arguments of the program
 */
template <class TypeTag>
int start(int argc,
          char **argv)
{
    try {
        return start_<TypeTag>(argc, argv);
    }
    catch (Dumux::ParameterException &e)
    {
       std::cerr << e << ". Abort!\n";
       printUsage(argv[0]);
       return 1;
    }
    catch (Dune::Exception &e) {
        std::cerr << "Dune reported error: " << e << std::endl;
        return 2;
    }
    catch (...) {
        std::cerr << "Unknown exception thrown!\n";
        return 3;
    }
}

int main(int argc, char** argv)
{
    typedef TTAG(TwoCNIStokesTwoPTwoCNIProblem) ProblemTypeTag;
    return start<ProblemTypeTag>(argc, argv);
}
