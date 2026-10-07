#include "environment.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace {
void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void checkRoute(const Environment &environment)
{
    const std::vector<Point3D> &route = environment.drone().route();
    check(route.size() >= 2, "route has endpoints");
    check(distance(route.front(), environment.start()) < 1e-8, "exact start");
    check(distance(route.back(), environment.goal()) < 1e-8, "exact goal");
    double total = 0;
    for (std::size_t i = 1; i < route.size(); ++i) {
        check(Environment::validPosition(route.at(i)), "route stays in the flight volume");
        check(environment.zone().distanceToSegment(route.at(i - 1), route.at(i))
              >= environment.zone().radius() + Drone::radius + Environment::clearance / 2 - 1e-7,
              "entire segment keeps drone outside sphere, including between arc samples");
        total += distance(route.at(i - 1), route.at(i));
    }
    check(std::abs(total - environment.routeLength()) < 1e-8, "reported polyline length");
    check(total + 1e-8 >= distance(environment.start(), environment.goal()), "length lower bound");
}
}

int main()
{
    using Result = Environment::Result;
    Environment environment;
    check(environment.drone().route().empty(), "reset clears route");
    check(!environment.startFlight(), "cannot fly without a route");
    check(environment.planRoute(environment.start(), environment.goal(), 120) == Result::Detour,
          "default scenario requires detour");
    checkRoute(environment);

    // The infinite line hits the sphere, but this finite segment does not.
    check(environment.planRoute({20, 200, 200}, {100, 200, 200}, 120) == Result::Direct,
          "segment intersection, not infinite line intersection");
    checkRoute(environment);
    check(std::abs(environment.routeLength() - 80) < 1e-8, "direct route length");

    check(environment.planRoute({50, 200, 500}, {550, 200, 500}, 120) == Result::Direct,
          "same XY projection can fly above the zone");
    checkRoute(environment);

    for (double radius : {1.0, 120.0, 180.0}) {
        check(environment.planRoute({10, 200, 200}, {590, 200, 200}, radius) == Result::Detour,
              "collinear horizontal endpoints");
        checkRoute(environment);
        check(environment.planRoute({300, 200, 8}, {300, 200, 592}, radius) == Result::Detour,
              "collinear vertical endpoints");
        checkRoute(environment);
    }
    check(environment.planRoute({50, 330, 200}, {550, 330, 200}, 120) == Result::Detour,
          "tangent to safety boundary remains safe");
    checkRoute(environment);

    check(environment.planRoute({50, 50, 50}, {50, 50, 50}, 120) == Result::Direct,
          "identical endpoints outside sphere");
    checkRoute(environment);
    check(environment.routeLength() == 0, "zero length route");
    check(environment.startFlight(), "start zero length route");
    environment.step(0.02);
    check(environment.drone().state() == Drone::State::Arrived, "zero length arrival");

    check(environment.planRoute({300, 200, 200}, {550, 50, 280}, 120) == Result::StartInZone,
          "reject start inside sphere");
    check(environment.drone().route().empty(), "failed plan clears stale route");
    check(environment.planRoute({50, 350, 100}, {430, 200, 200}, 120) == Result::GoalInZone,
          "reject goal on safety boundary");
    check(environment.planRoute({50, 350, 100}, {550, 50, 280}, -1) == Result::InvalidInput,
          "reject negative radius");
    check(environment.planRoute({50, 350, 100}, {550, 50, 280}, 181) == Result::InvalidInput,
          "reject oversized sphere");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    check(environment.planRoute({nan, 50, 50}, {550, 50, 280}, 120) == Result::InvalidInput,
          "reject nonfinite coordinates");
    check(environment.planRoute({50, 350, 100}, {550, 50, 280}, nan) == Result::InvalidInput,
          "reject nonfinite radius");
    check(environment.planRoute({0, 50, 50}, {550, 50, 280}, 120) == Result::InvalidInput,
          "keep drone body within bounds");

    // Deterministic coverage of arbitrary 3D planes and near-boundary endpoints.
    std::mt19937 random(3);
    std::uniform_real_distribution<double> xz(8, 592), y(8, 392), radius(1, 180);
    int detours = 0;
    for (int i = 0; i < 3000; ++i) {
        const Point3D start{xz(random), y(random), xz(random)};
        const Point3D goal{xz(random), y(random), xz(random)};
        const Result result = environment.planRoute(start, goal, radius(random));
        if (result == Result::Direct || result == Result::Detour)
            checkRoute(environment);
        if (result == Result::Detour)
            ++detours;
    }
    check(detours > 100, "random cases exercise detours");

    environment.reset();
    environment.planRoute(environment.start(), environment.goal(), 120);
    check(environment.startFlight(), "start planned flight");
    environment.step(0.02);
    const Point3D pausedPosition = environment.drone().position();
    environment.pauseFlight();
    environment.step(1);
    check(distance(pausedPosition, environment.drone().position()) == 0, "pause stops movement");
    check(environment.startFlight(), "resume flight");
    environment.step(-1);
    environment.step(nan);
    check(distance(pausedPosition, environment.drone().position()) == 0, "invalid dt ignored");
    for (int i = 0; i < 10000 && environment.drone().state() == Drone::State::Running; ++i) {
        environment.step(0.02);
        check(!environment.zone().contains(environment.drone().position(), Drone::radius),
              "animated drone stays outside zone");
    }
    check(environment.drone().state() == Drone::State::Arrived, "flight finishes");
    check(distance(environment.drone().position(), environment.goal()) < 1e-8, "exact arrival");
    check(!environment.startFlight(), "finished drone requires a new route for replay");

    // A long frame must consume successive segments, without shortcutting the arc.
    Drone drone;
    drone.setRoute({{0, 0, 0}, {10, 0, 0}, {10, 10, 0}, {10, 10, 10}});
    drone.setSpeed(20);
    drone.start();
    drone.step(1.25);
    check(distance(drone.position(), {10, 10, 5}) < 1e-8, "travel budget follows each segment");
    drone.step(10);
    check(distance(drone.position(), {10, 10, 10}) < 1e-8, "large step clamps to final target");

    environment.reset();
    check(environment.drone().route().empty() && environment.routeLength() == 0, "full reset");
    check(environment.drone().speed() == 80, "reset restores speed");
    std::cout << "Route and drone tests passed (3000 deterministic 3D cases).\n";
}
