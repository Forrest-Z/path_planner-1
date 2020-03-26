#include <path_planner_common/State.h>

State::State(double x, double y, double heading, double speed, double t) {
    setX(x);
    setY(y);
    setHeading(heading);
    setSpeed(speed);
    setTime(t);
}

State State::push(double timeInterval) const {
    State s;
    double displacement = timeInterval * speed();
    s.x() = x() + sin(heading()) * displacement;
    s.y() = y() + cos(heading()) * displacement;
    s.heading() = heading();
    s.speed() = speed();
    s.time() = time() + timeInterval;
    return s;
}

void State::move(double distance) {
    x() += cos(yaw()) * distance;
    y() += sin(yaw()) * distance;
}

std::string State::toString() const {
    return std::to_string(x()) + " " +
           std::to_string(y()) + " " +
           std::to_string(heading()*180/M_PI) + " " +
           std::to_string(speed()) + " " +
           std::to_string(time());
}

std::string State::toStringRad() const {
    return std::to_string(x()) + " " +
           std::to_string(y()) + " " +
           std::to_string(heading()) + " " +
           std::to_string(speed()) + " " +
           std::to_string(time());
}

double State::headingTo(const std::pair<double, double> p) const {
    return headingTo(p.first, p.second);
}

double State::headingTo(const State& other) const {
    return headingTo(other.x(), other.y());
}

double State::headingTo(double x1, double y1) const {
    double dx = x1 - this->x();
    double dy = y1 - this->y();
    double h = M_PI_2 - atan2(dy, dx); // TODO! -- is this correct?
    if (h < 0) h += 2 * M_PI;
    return h;
}

void State::setHeadingTowards(const State& other) {
    heading() = headingTo(other);
    if (heading() < 0) heading() += 2 * M_PI;
}

void State::setHeadingTowards(double x1, double y1) {
    heading() = headingTo(x1, y1);
    if (heading() < 0) heading() += 2 * M_PI;
}

double State::timeUntil(const State& other) const {
    return other.time() - time();
}

bool State::operator==(const State& rhs) const {
    return x() == rhs.x() &&
           y() == rhs.y() &&
           heading() == rhs.heading() &&
           speed() == rhs.speed() &&
           time() == rhs.time();
}

bool State::isCoLocated(const State& rhs) const {
    return x() == rhs.x() &&
           y() == rhs.y() &&
           heading() == rhs.heading();
}

double State::distanceTo(const State& other) const {
    return distanceTo(other.x(), other.y());
}

double State::distanceTo(double x1, double y1) const {
    return sqrt((this->x() - x1)*(this->x() - x1) + (this->y() - y1)*(this->y() - y1));
}

State State::interpolate(const State& other, double desiredTime) const {
//        std::cerr << "Interpolating between " << other.toString() << " and " << toString() << std::endl;
    auto dt = other.time() - time();
    auto dx = (other.x() - x()) / dt;
    auto dy = (other.y() - y()) / dt;
    // assume the headings have changed the closer way, not all the way around the other way

    auto dh = headingDifference(other) / dt;
    auto ds = (other.speed() - speed()) / dt;
    dt = desiredTime - time();
    State s = *this;
    s.x() += dx * dt;
    s.y() += dy * dt;
    s.heading() = this->heading() + (dh * dt);
    if (s.heading() >= twoPi) s.heading() -= twoPi;
    s.speed() += ds * dt;
    s.time() = desiredTime;
    return s;
}

double State::headingDifference(const State& other) const {
    return headingDifference(other.heading());
}

double State::headingDifference(double otherHeading) const {
    return (fmod(fmod((otherHeading - heading()), twoPi) + 3 * M_PI, twoPi) - M_PI);
}
