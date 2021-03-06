//
// Created by Anselm Eickhoff on 28/01/16.
//

#ifndef COMPASS_PRIMITIVES_H
#define COMPASS_PRIMITIVES_H
#include <Eigen/Dense>
#include "at-most.h"
#include "angles.h"

typedef Eigen::Vector2f vec2;

const float thickness = 0.0001;

class Circle {
public:
    vec2 center;
    float radius;

    Circle (vec2 center, float radius) : center(center), radius(radius) {};

    bool contains(vec2 point) {
        return (center - point).norm() <= radius + thickness/2;
    }

    float offsetAt(vec2 point) {
        return angleBetweenWithDirection({1, 0}, {0, 1}, point - center) * radius;
    }
};

class Line {
public:
    vec2 start;
    vec2 direction;

    Line (vec2 start, vec2 direction) : start(start), direction(direction) {};
};

class Ray {
public:
    vec2 start;
    vec2 direction;

    Ray (vec2 start, vec2 direction) : start(start), direction(direction) {};
};

class Segment {
    float _lengthAndStraightInfo;

public:
    const vec2 start;
    const vec2 end;
    const vec2 direction;

    Segment () {}

    // LineSegment
    Segment (vec2 start, vec2 end)
        :start(start), direction((end - start).normalized()), end(end)
    {
        _lengthAndStraightInfo = (end - start).norm();
    }

    // CircleSegment
    Segment (vec2 start, vec2 direction, vec2 end)
        :start(start), direction(direction), end(end)
    {
        bool isStraight = (end - start).normalized() == direction;
        if (isStraight) {
            _lengthAndStraightInfo = (end - start).norm();
        } else {
            _lengthAndStraightInfo = - angleSpan() * radius();
        }
    }

private:
    float angleSpan() {
        vec2 center = radialCenter();
        return angleBetweenWithDirection(start - center, direction, end - center);
    }

public:

    float length() {
        return std::abs(_lengthAndStraightInfo);
    }

    bool isStraight() {
        return _lengthAndStraightInfo > 0;
    }

    vec2 radialCenter () {
        return start + signedRadius() * direction.unitOrthogonal();
    }
private:
    float signedRadius () {
        auto halfChord = (end - start) / 2;
        return halfChord.squaredNorm() / (direction.unitOrthogonal().dot(halfChord));
    }
public:

    float radius () {
        return std::abs(signedRadius());
    }

    vec2 midpoint () {
        auto linearMidpoint = (end + start) / 2;
        if (isStraight()) return linearMidpoint;
        else {
            auto center = radialCenter();
            return center + ((linearMidpoint - center).normalized() * radius());
        }
    }

    vec2 endDirection () {
        if (isStraight()) return direction;
        else return std::copysign(1, signedRadius()) * (end - radialCenter()).unitOrthogonal();
    }

    vec2 directionOf (float offset) {
        if (isStraight()) return direction;
        else {
            auto rotation = Eigen::Rotation2D<float>((offset/length()) * angleSpan());
            return std::copysign(1, signedRadius()) * (rotation * (start - radialCenter())).unitOrthogonal();
        }
    }

    float offsetAt (vec2 point) {
        if (isStraight()) return direction.dot(point - start);
        else {
            float angleAToPoint = angleBetweenWithDirection(start - radialCenter(), direction, point - radialCenter());
            float angleBToPoint = angleBetweenWithDirection(end - radialCenter(), -endDirection(), point - radialCenter());
            float tolerance = thickness / radius();

            if (angleAToPoint <= angleSpan() + tolerance &&
                angleBToPoint <= angleSpan() + tolerance) {
                return std::min(angleSpan(), std::max(0.0f, angleAToPoint)) * radius();
            } else {
                if (angleAToPoint <= angleBToPoint) return angleAToPoint * radius();
                else return -(angleBToPoint - angleSpan()) * radius();
            }
        }
    }

    float distanceTo(vec2 point) {
            float offsetAlong = offsetAt(point);
            if (offsetAlong < 0)
                return (point - start).norm();
            else if (offsetAlong <= length())
                if (isStraight())
                    return std::abs(direction.unitOrthogonal().dot(point - start));
                else return std::abs((point - radialCenter()).norm() - radius());
            else
                return (point - end).norm();
    }

    bool contains (vec2 pointAnywhere) {
        float distance = distanceTo(pointAnywhere);
        return distance < thickness/2;
    }

    Segment reverse() {
        if (isStraight()) return Segment(end, start);
        else return Segment(end, endDirection(), start);
    }

    AtMost<2, Segment> subdivide (vec2 divider) {
        if (isStraight()) {
            return {Segment(start, divider), Segment(divider, end)};
        } else {
            vec2 dividerDirection = std::copysign(1, signedRadius()) * (divider - radialCenter()).unitOrthogonal();
            return {Segment(start, direction, divider), Segment(divider, dividerDirection, end)};
        }
    };
};

#endif //COMPASS_PRIMITIVES_H
