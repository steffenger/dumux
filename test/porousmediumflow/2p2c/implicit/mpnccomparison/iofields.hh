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
 * \ingroup TwoPTwoCModel
 * \brief Adds I/O fields specific to the twop model
 */
#ifndef DUMUX_TWOPTWOC_MPNC_IO_FIELDS_HH
#define DUMUX_TWOPTWOC_MPNC_IO_FIELDS_HH

namespace Dumux {

/*!
 * \ingroup TwoPTwoCModel
 * \brief Adds I/O fields specific to the two-phase two-component model
 */
class TwoPTwoCMPNCIOFields
{
public:
    template <class OutputModule>
    static void initOutputModule(OutputModule& out)
    {
        using VolumeVariables = typename OutputModule::VolumeVariables;
        using FS = typename VolumeVariables::FluidSystem;

        // register standardized output fields
        out.addVolumeVariable([](const auto& v){ return v.porosity(); }, "porosity");
        out.addVolumeVariable([](const auto& v){ return v.saturation(FS::phase0Idx); }, "S_"+ FS::phaseName(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.saturation(FS::phase1Idx); }, "S_"+ FS::phaseName(FS::phase1Idx));
        out.addVolumeVariable([](const auto& v){ return v.pressure(FS::phase0Idx); }, "p_"+ FS::phaseName(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.pressure(FS::phase1Idx); }, "p_"+ FS::phaseName(FS::phase1Idx));

        out.addVolumeVariable([](const auto& v){ return v.density(FS::phase0Idx); }, "rho_"+ FS::phaseName(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.density(FS::phase1Idx); }, "rho_"+ FS::phaseName(FS::phase1Idx));
        out.addVolumeVariable([](const auto& v){ return v.mobility(FS::phase0Idx); }, "mob_"+ FS::phaseName(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.mobility(FS::phase1Idx); }, "mob_"+ FS::phaseName(FS::phase1Idx));

        for (int i = 0; i < VolumeVariables::numPhases(); ++i)
            for (int j = 0; j < VolumeVariables::numComponents(); ++j)
                out.addVolumeVariable([i,j](const auto& v){ return v.massFraction(i,j); },"X^"+ FS::componentName(j) + "_" + FS::phaseName(i));

        for (int i = 0; i < VolumeVariables::numPhases(); ++i)
            for (int j = 0; j < VolumeVariables::numComponents(); ++j)
                out.addVolumeVariable([i,j](const auto& v){ return v.moleFraction(i,j); },"x^"+ FS::componentName(j) + "_" + FS::phaseName(i));
    }

    template <class OutputModule>
    DUNE_DEPRECATED_MSG("use initOutputModule instead")
    static void init(OutputModule& out)
    {
        initOutputModule(out);
    }
};

} // end namespace Dumux

#endif
