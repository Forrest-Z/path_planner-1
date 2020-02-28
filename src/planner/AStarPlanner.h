#ifndef SRC_ASTARPLANNER_H
#define SRC_ASTARPLANNER_H

#include "SamplingBasedPlanner.h"

class AStarPlanner : public SamplingBasedPlanner {
public:

//    AStarPlanner(double maxSpeed, double maxTurningRadius, std::shared_ptr<Map> staticMap);
//    AStarPlanner(double maxSpeed, double maxTurningRadius, double coverageSpeed, double coverageTurningRadius, std::shared_ptr<Map> staticMap);

    AStarPlanner() = default;

    ~AStarPlanner() override = default;

    Plan plan(const RibbonManager& ribbonManager, const State& start,
                            PlannerConfig config, double timeRemaining) override;

//    std::vector<State> plan(const std::vector<std::pair<double, double>>& newlyCovered, const State& start,
//                            DynamicObstaclesManager dynamicObstacles, double timeRemaining) override;
//
//    std::vector<State> plan(const RibbonManager& ribbonManager, const State& start,
//                            DynamicObstaclesManager dynamicObstacles, double timeRemaining) override;

protected:
    int m_IterationCount = 0;

    std::function<bool(std::shared_ptr<Vertex> v1, std::shared_ptr<Vertex> v2)> getVertexComparator() override;

//    int k() const override { return 9; }

    std::shared_ptr<Vertex> aStar(const DynamicObstaclesManager& obstacles, double endTime);

    void expandToCoverSpecificSamples(Vertex::SharedPtr root, const std::vector<State>& samples,
                                      const DynamicObstaclesManager& obstacles, bool coverageAllowed);

    static constexpr double c_InitialSamples = 100;

private:
//    std::vector<State> plan(const State& start, DynamicObstaclesManager dynamicObstacles, double timeRemaining);
};


#endif //SRC_ASTARPLANNER_H
