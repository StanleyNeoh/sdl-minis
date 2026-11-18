#ifndef UTILS_COORDINATE_HPP
#define UTILS_COORDINATE_HPP

namespace Utils {
    float scale1(float x, float srcL, float dstL) {
        return (x / srcL) * dstL;
    }

    void map1(float& x, float srcP, float srcL, float dstP, float dstL) {
        x = scale1(x - srcP, srcL, dstL) + dstP;
    }
};

#endif