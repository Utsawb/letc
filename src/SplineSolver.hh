#pragma once

#ifndef LETC_SPLINE_SOLVER_HH
#define LETC_SPLINE_SOLVER_HH

#include "pch.hh"

namespace letc
{
    inline double bezier(unsigned int i, double u)
    {
        switch (i)
        {
        case 0:
            return (1.0 - u) * (1.0 - u) * (1.0 - u);
        case 1:
            return 3.0 * u * (1.0 - u) * (1.0 - u);
        case 2:
            return 3.0 * u * u * (1.0 - u);
        case 3:
            return u * u * u;
        default:
            return 0.0;
        }
    }

}; // namespace letc

#endif // LETC_SPLINE_SOLVER_HH