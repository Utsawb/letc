#pragma once

#ifndef LETC_SPLINE_SOLVER_HH
#define LETC_SPLINE_SOLVER_HH

#include "pch.hh"

namespace letc
{
    inline double basis(unsigned int i, double u)
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

    // for a set of points in 3d space and number of requested samples
    // returns the u -> v conversion
    inline std::vector<float> chordLengthParametrization(const std::vector<glm::vec3> &points)
    {
        std::vector<float> clp(points.size(), 0.0f);
        for (std::size_t idx = 1; idx < clp.size(); ++idx)
        {
            clp.at(idx) = clp.at(idx - 1) + glm::distance(points.at(idx - 1), points.at(idx));
        }

        float m = clp.back();
        std::for_each(clp.begin(), clp.end(), [m](float &val) { val /= m; });
        return clp;
    }

    // take in vector of points in 3d space, and a k value which is the number of curves in the spline
    // output, the fitted spline
    // so if k = 1, we should have 4 control points
    // if k = 2, we should have 7 control points
    // ... cpoints.size = 3k + 1
    // https://www.math.ucla.edu/%7Ebaker/149.1.02w/handouts/dd_splines.pdf
    // L. Piegl and W. Tiller, The NURBS Book, 2nd ed., Berlin: Springer-Verlag, 1997 pp. 410–413.
    inline std::vector<glm::vec3> splineSolve(const std::vector<glm::vec3> &points, const uint32_t &k)
    {
        const std::size_t ctrlN = 3 * k + 1;
        auto clp = chordLengthParametrization(points);


    }

}; // namespace letc

#endif // LETC_SPLINE_SOLVER_HH
