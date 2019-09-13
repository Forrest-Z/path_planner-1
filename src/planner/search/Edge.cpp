#include <utility>
#include <algorithm>
#include <memory>
#include "Edge.h"
#include "../utilities/Path.h"
#include <robust_dubins/RobustDubins.h>
#include <cfloat>

extern "C" {
#include <dubins.h>
}

Edge::Edge(std::shared_ptr<Vertex> start, bool useRibbons) : m_UseRibbons(useRibbons) {
    this->m_Start = std::move(start);
}

double Edge::computeTrueCost(const Map::SharedPtr& map, DynamicObstaclesManager *obstacles,
                             double maxSpeed, double maxTurningRadius) {
    if (start()->state().colocated(end()->state())) {
        std::cerr << "Computing cost of edge between two co-located states is likely an error" << std::endl;
    }
    if (m_ApproxCost == -1) computeApproxCost(maxSpeed, maxTurningRadius);
    if (m_ApproxCost <= 0) throw std::runtime_error("Could not compute approximate cost");
    double collisionPenalty = 0;
    std::vector<std::pair<double, double>> result;
    double q[3];
    double lengthSoFar = 0;
    double length = dubins_path_length(&dubinsPath);
    double staticDistance = 0, dynamicDistance = 0, toCoverDistance = 0;
    std::vector<std::pair<double, double>> newlyCovered;
    double lastYaw = start()->state().yaw();

    // collision check along the curve (and watch out for newly covered points, too)
    while (lengthSoFar <= length) {
        dubins_path_sample(&dubinsPath, lengthSoFar, q);
        if (staticDistance > Edge::dubinsIncrement()) {
            staticDistance -= Edge::dubinsIncrement();
        } else {
            staticDistance = map->getUnblockedDistance(q[0], q[1]);
            if (staticDistance <= Edge::dubinsIncrement()) {
                collisionPenalty += Edge::collisionPenalty();
                staticDistance = 0;
                m_Infeasible = true;
                break;
            }
        }
        if (dynamicDistance > Edge::dubinsIncrement()) {
            dynamicDistance -= Edge::dubinsIncrement();
        } else {
            dynamicDistance = obstacles->distanceToNearestPossibleCollision(q[0], q[1], start()->state().speed,
                                                                            start()->state().time +
                                                                            (lengthSoFar / maxSpeed));
            if (dynamicDistance <= Edge::dubinsIncrement()) {
                collisionPenalty += obstacles->collisionExists(q[0], q[1], start()->state().time + (lengthSoFar / maxSpeed)) *
                                    Edge::collisionPenalty();
                dynamicDistance = 0;
            }
        }
        if (toCoverDistance > Edge::dubinsIncrement()) {
            toCoverDistance -= Edge::dubinsIncrement();
        } else {
            if (m_UseRibbons) {
                // do this first because cover splits ribbons so you'd never get one that "contains" the point so it
                // could be a bit more work
                toCoverDistance = end()->ribbonManager().minDistanceFrom(q[0], q[1]);
                if (lastYaw == q[2]) {
                    end()->ribbonManager().cover(q[0], q[1]);
                }
            } else {
                toCoverDistance = DBL_MAX;
                for (auto p : start()->uncovered().get()) {
                    auto d = Path::distance(p, q[0], q[1]);
                    if (Path::covers(d)) {
                        newlyCovered.push_back(p);
                    } else {
                        toCoverDistance = fmin(toCoverDistance, d - Path::coverageThreshold());
                    }
                }
            }
        }
        lengthSoFar += Edge::dubinsIncrement();
        lastYaw = q[2];
    }

    // set end's state's time
    end()->state().time = start()->state().time + dubins_path_length(&dubinsPath) / maxSpeed;

    if (!m_UseRibbons) {
        // remove duplicates for efficiency for the next bit
        std::sort(newlyCovered.begin(), newlyCovered.end());
        newlyCovered.erase(std::unique(newlyCovered.begin(), newlyCovered.end()), newlyCovered.end());

        // set end's uncovered list
        end()->uncovered().clear();
        for (auto p : start()->uncovered().get()) {
            if (std::find(newlyCovered.begin(), newlyCovered.end(), p) == newlyCovered.end()) {
                end()->uncovered().add(p);
            }
        }
    }
    m_TrueCost = netTime() * Edge::timePenalty() + collisionPenalty;

    end()->setCurrentCost();

    return m_TrueCost;
}

double Edge::computeApproxCost(double maxSpeed, double maxTurningRadius) {
    if (start()->state().colocated(end()->state())) {
        m_ApproxCost = 0;
    } else {
        double q0[3] = {start()->state().x, start()->state().y, start()->state().yaw()};
        double q1[3] = {end()->state().x, end()->state().y, end()->state().yaw()};
        int err = dubins_shortest_path(&dubinsPath, q0, q1, maxTurningRadius);
        if (err != EDUBOK) {
            std::cerr << "Encountered an error in the Dubins library" << std::endl;
        } else {
            m_ApproxCost = dubins_path_length(&dubinsPath) / maxSpeed * Edge::timePenalty();
        }
    }
    return m_ApproxCost;
}

double Edge::netTime() {
    return end()->state().time - start()->state().time;
}

void Edge::smooth(Map::SharedPtr map, DynamicObstaclesManager* obstacles, double maxSpeed, double maxTurningRadius) {
    if (start()->isRoot()) return;
    double parentCost = start()->parentEdge()->m_TrueCost; // should be up to date in A*, check for BIT*
    auto smoothed = Vertex::connect(start()->parent(), end()->state());
    double smoothedCost = smoothed->parentEdge()->computeTrueCost(map, obstacles, maxSpeed, maxTurningRadius);
    if (smoothedCost < parentCost + m_TrueCost && smoothed->approxToGo() <= end()->approxToGo()) {
        // Should be memory-safe, as smoothed will delete the old vertex when it goes out of scope, and all pointers
        // to *end() will still be valid
        // TODO! -- apparently this is actually broken (something throws std::bad_weak_ptr when smoothing runs)
        std::swap(*(smoothed.get()), *(end().get()));
    }
}

Plan Edge::getPlan(double maxSpeed) {
    double q[3];
    double lengthSoFar = 0;
    double length = dubins_path_length(&dubinsPath);
    Plan plan1;
    while (lengthSoFar < length) {
        dubins_path_sample(&dubinsPath, lengthSoFar, q);
        plan1.append(State(q[0], q[1], M_PI_2 - q[2], maxSpeed, lengthSoFar / maxSpeed + start()->state().time));
        lengthSoFar += Edge::dubinsIncrement();
    }
    return plan1;
}

std::shared_ptr<Vertex> Edge::setEnd(const State &state) {
    auto ptr = std::make_shared<Vertex>(state, std::shared_ptr<Edge>(this), m_UseRibbons);
    m_End = ptr;
    return ptr;
}

std::shared_ptr<Vertex> Edge::start() {
    return m_Start;
}

std::shared_ptr<Vertex> Edge::end() {
    std::shared_ptr<Vertex> s(m_End);
    return s;
}

double Edge::trueCost() const {
    if (m_TrueCost == -1) throw std::logic_error("Fetching unset cached edge cost");
    return m_TrueCost;
}

double Edge::approxCost() const {
    if (m_ApproxCost == -1) throw std::logic_error("Fetching unset cached approximate edge cost");
    return m_ApproxCost;
}

bool Edge::infeasible() const {
    return m_Infeasible;
}

Edge::~Edge() = default;