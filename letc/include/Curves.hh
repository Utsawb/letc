#pragma once

#ifndef LETC_CURVES_HH
#define LETC_CURVES_HH

#include "pch.hh"

namespace letc
{
    struct Curves
    {
        const std::vector<glm::mat4> &transformations;
        std::vector<float> mapping;
        float totalLength = 0.0f;

        Curves(const std::vector<glm::mat4> &transformations) : transformations(transformations)
        {
            if (transformations.size() >= 2)
            {
                arcLengthParameterization(transformations);
            }
            else if (transformations.size() == 1)
            {
                mapping.push_back(0.0f);
                totalLength = 0.0f;
            }
        }

        void arcLengthParameterization(const std::vector<glm::mat4> &transformations)
        {
            mapping.clear();
            mapping.reserve(transformations.size());
            mapping.emplace_back(0.0f);
            for (std::size_t i = 1; i < transformations.size(); ++i)
            {
                mapping.push_back(mapping.at(i - 1) +
                                  glm::distance(transformations.at(i)[3], transformations.at(i - 1)[3]));
            }

            totalLength = mapping.back();
        }

        glm::mat4 getInterp(float t) const
        {
            const std::size_t &n = transformations.size();

            if (n == 0)
            {
                return glm::mat4(1.0f);
            }

            if (n == 1)
            {
                return transformations.at(0);
            }

            if (totalLength <= std::numeric_limits<float>::epsilon())
            {
                return transformations.at(0);
            }

            t = std::fmod(t, totalLength);
            if (t < 0.0f)
            {
                t += totalLength;
            }
            t = glm::clamp(t, 0.0f, totalLength * (1.0f - std::numeric_limits<float>::epsilon()));

            std::size_t segmentIdx = 0;
            float u = 0.0f;

            for (std::size_t i = 0; i < n - 1; ++i)
            {
                if (t >= mapping.at(i) && t <= mapping.at(i + 1))
                {
                    segmentIdx = i;
                    float segmentLength = mapping.at(i + 1) - mapping.at(i);

                    if (segmentLength > std::numeric_limits<float>::epsilon())
                    {
                        u = (t - mapping.at(i)) / segmentLength;
                    }
                    else
                    {
                        u = 0.0f;
                    }
                    break;
                }
            }

            if (t >= mapping.at(n - 1))
            {
                segmentIdx = n - 1;
                u = 1.0f;
            }

            std::size_t idx0 = (segmentIdx == 0) ? segmentIdx : segmentIdx - 1;
            std::size_t idx1 = segmentIdx;
            std::size_t idx2 = segmentIdx + 1;
            std::size_t idx3 = (segmentIdx == n - 2) ? segmentIdx + 1 : segmentIdx + 2;

            const glm::mat4 &T0 = transformations.at(idx0);
            const glm::mat4 &T1 = transformations.at(idx1);
            const glm::mat4 &T2 = transformations.at(idx2);
            const glm::mat4 &T3 = transformations.at(idx3);

            glm::vec3 t0 = T0[3];
            glm::vec3 t1 = T1[3];
            glm::vec3 t2 = T2[3];
            glm::vec3 t3 = T3[3];

            glm::quat r0 = glm::normalize(glm::quat_cast(T0));
            glm::quat r1 = glm::normalize(glm::quat_cast(T1));
            glm::quat r2 = glm::normalize(glm::quat_cast(T2));
            glm::quat r3 = glm::normalize(glm::quat_cast(T3));

            glm::vec3 tInterp = glm::catmullRom(t0, t1, t2, t3, u);
            glm::quat rInterp = glm::normalize(glm::catmullRom(r0, r1, r2, r3, u));

            return glm::translate(glm::mat4(1.0f), tInterp) * glm::mat4_cast(rInterp);
        }
    };

}; // namespace letc

#endif
