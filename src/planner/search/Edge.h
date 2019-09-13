#ifndef SRC_EDGE_H
#define SRC_EDGE_H

#include "Vertex.h"
#include <robust_dubins/RobustDubins.h>
#include "../../common/map/Map.h"
#include "../../common/dynamic_obstacles/DynamicObstaclesManager.h"
#include "../utilities/Path.h"
#include "../utilities/Plan.h"

extern "C" {
#include "dubins.h"
}
class Vertex;

class Edge {
public:
    typedef std::shared_ptr<Edge> SharedPtr;

    DubinsPath dubinsPath;
//    RobustDubins::Path dubinsPath;

    Edge(std::shared_ptr<Vertex> start, bool useRibbons);

    ~Edge();

    double approxCost() const;

    std::shared_ptr<Vertex> setEnd(const State& state);

    double computeTrueCost(const Map::SharedPtr& map, DynamicObstaclesManager *obstacles, double maxSpeed, double maxTurningRadius);

    double trueCost() const;

    double computeApproxCost(double maxSpeed, double maxTurningRadius);

    void smooth(Map::SharedPtr map, DynamicObstaclesManager* obstacles, double maxSpeed, double maxTurningRadius);

    Plan getPlan(double maxSpeed);

    std::shared_ptr<Vertex> start();

    std::shared_ptr<Vertex> end();

    bool infeasible() const;

    static double collisionPenalty() { return c_CollisionPenalty; }
    static double dubinsIncrement() { return c_DubinsIncrement; }
    static double timePenalty() { return c_TimePenalty; }

private:
    std::shared_ptr<Vertex> m_Start;
    std::weak_ptr<Vertex> m_End;

    bool m_Infeasible = false;

    static constexpr double c_CollisionPenalty = 600;
    static constexpr double c_DubinsIncrement = 0.1;
    static constexpr double c_TimePenalty = 1;

    double m_ApproxCost = -1, m_TrueCost = -1;

    bool m_UseRibbons;

    double netTime();
};


#endif //SRC_EDGE_H