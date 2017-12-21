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
 * \ingroup Components
 * \brief Corrected material properties of pure Calcium-Oxide \f$CaO\f$ withouf considering a porosity
 * change in the reaction of Calciumoxyde and Calciumhydroxyde.
 */
#ifndef DUMUX_CAO_TEST_HH
#define DUMUX_CAO_TEST_HH


#include <dumux/material/components/CaO.hh>

namespace Dumux
{
/*!
 * \ingroup Components
 * \brief A class for the CaOtest properties
 *
 * This class uses a different CaO density. It is to be  used for calculating the chemical
 * reaction of CaO to Ca(OH)2 without considering the porosity change according to See Shao et
 * al. (2013).
 */
template <class Scalar>
class CaOTest : public  CaO<Scalar>
{
public:

    /*!
     * \brief The corrected mass density \f$\mathrm{[kg/m^3]}\f$ of CaO.
     */
    static Scalar density()
    {
        return 1656;
        // This density is to be used for calculating the chemical reaction of CaO to Ca(OH)2 without considering the solid volume change. See Shao et al. (2013)
    }

};

} // end namespace

#endif
