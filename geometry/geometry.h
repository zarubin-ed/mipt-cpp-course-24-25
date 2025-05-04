#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <tuple>
#include <sstream>
#include <vector>

using fType = double;

namespace MathFunc {
  struct Vector {
    fType x = 0;
    fType y = 0;
    Vector(fType x_, fType y_)
    : x(x_)
    , y(y_)
    {}
    Vector() = default;
  };

  template<typename T>
  int sign(T x) {
    if (x < 0) {
      return -1;
    }
    if (x > 0) {
      return 1;
    }
    return 0;
  }

  bool floatisEqual(fType lhs, fType rhs) {
    constexpr fType kEps = 1e-9;
    return fabs(lhs - rhs) < kEps;
  }
}

class Point {
public:
  fType x = 0;
  fType y = 0;

  Point(fType x_, fType y_)
  : x(x_)
  , y(y_)
  {}

  Point() = default;

  operator MathFunc::Vector() const {
    return MathFunc::Vector(x, y);
  }

  Point(const MathFunc::Vector& vector)
  : x(vector.x)
  , y(vector.y)
  {}

  Point rotate(fType angle) const;

  Point rotate(const Point& point, fType angle) const;

  Point reflect( const Point& point) const;

  Point scale(const Point& point, fType scale) const;

  Point perpendicular() const;

  fType dotProduct(const Point& another) const;

  fType crossProduct(const Point& another) const;

  fType length2() const;

  fType length() const;

  Point bisector(Point rhs) const;

  fType angleBetweenTwoVectors(const Point& another) const;
};

Point operator+(const Point& lhs, const Point& rhs) {
  return Point(lhs.x + rhs.x, lhs.y + rhs.y);
}

Point operator-(const Point& lhs, const Point& rhs) {
  return Point(lhs.x - rhs.x, lhs.y - rhs.y);
}

Point operator*(const Point& point, fType scalar) {
  return Point(point.x * scalar, point.y * scalar);
}

Point operator/(const Point& point, fType scalar) {
  return Point(point.x / scalar, point.y / scalar);
}

bool operator==(const Point& lhs, const Point& rhs) {
  return MathFunc::floatisEqual(lhs.x, rhs.x) && MathFunc::floatisEqual(lhs.y, rhs.y);
}

Point Point::rotate(fType angle) const {
  return Point(x * cos(angle) - y * sin(angle), x * sin(angle) + y * cos(angle));
}

Point Point::rotate(const Point& point, fType angle) const {
  return *this + (point - *this).rotate(angle);
}

Point Point::reflect(const Point& point) const {
  return *this + (*this - point);
}

Point Point::scale(const Point& point, fType scale) const {
  return *this + (point - *this) * scale;
}

Point Point::perpendicular() const {
  return Point(-y, x);
}

fType Point::dotProduct(const Point& another) const {
  return x * another.x + y * another.y;
}

fType Point::crossProduct(const Point& another) const {
  return x * another.y - y * another.x;
}

fType Point::length2() const {
  return dotProduct(*this);
}

fType Point::length() const {
  return std::sqrt(length2());
}

Point Point::bisector(Point rhs) const {
  Point lhs = *this;
  lhs = lhs / lhs.length();
  rhs = rhs / rhs.length();
  return Point(lhs.x + rhs.x, lhs.y + rhs.y);
}

fType Point::angleBetweenTwoVectors(const Point& another) const {
  return atan2(crossProduct(another), dotProduct(another));
}

class Line {
public:
  Line (const Point& point1, const Point& point2) {
    cA_ = point1.y - point2.y;
    cB_ = point2.x - point1.x;
    cC_ = -(cA_ * point1.x + cB_ * point1.y);
  }

  Line (fType k, fType b)
  : Line(Point(0, b), Point(1, k + b))
  {}

  Line (const Point& point, fType k)
  : Line(point, Point(point.x + 1, point.y + k))
  {}

  Line (const Point& p, const MathFunc::Vector& v)
    : cA_(-v.y)
    , cB_(v.x)
    , cC_(-(cA_ * p.x + cB_ * p.y))
  {}

  std::tuple<fType, fType, fType> getCoefficients() const {
    return std::make_tuple(cA_, cB_, cC_);
  }

  Point intersection (const Line& another) const {
    auto [a1, b1, c1] = another.getCoefficients();
    return Point(-(c1 * cB_ - cC_ * b1) / (a1 * cB_ - cA_ * b1), -(a1 * cC_ - cA_ * c1) / (a1 * cB_ - cA_ * b1));
  }

  Point reflect(const Point& point) const {
    MathFunc::Vector v(cA_, cB_);
    return point + (intersection(Line(point, v)) - point) * 2.0;
  }

protected:
  fType cA_;
  fType cB_;
  fType cC_;
};

bool operator==(const Line& line1, const Line& line2) {
  auto [cA1, cB1, cC1] = line1.getCoefficients();
  auto [cA2, cB2, cC2] = line2.getCoefficients();
  return MathFunc::floatisEqual(cA1 * cB2, cA2 * cB1)
      && MathFunc::floatisEqual(cC1 * cB2, cC2 * cB1)
      && MathFunc::floatisEqual(cC1 * cA2, cC2 * cA1);
}

std::ostream& operator << (std::ostream& os, const Line& line) {
  auto [a, b, c] = line.getCoefficients();
  os << a << ' ' << b << " " << c << std::endl;
  return os;
}

class Shape {
public:
  virtual fType perimeter() const  = 0;

  virtual fType area() const  = 0;

  virtual bool isCongruentTo(const Shape& another) const = 0;

  virtual bool isSimilarTo(const Shape& another) const = 0;

  virtual bool containsPoint(const Point& point) = 0;

  virtual void rotate(const Point& center, fType angle) = 0;

  virtual void reflect(const Point& center) = 0;

  virtual void reflect(const Line& axis) = 0;

  virtual void scale(const Point& center, fType coefficient) = 0;

  virtual ~Shape() = default;

  bool operator==(const Shape& rhs) const {
    return isEqual(rhs);
  }

protected:
  virtual bool isEqual(const Shape& another) const = 0;
};

class Polygon : public Shape {
public:
  Polygon() = default;

  Polygon(const std::vector<Point>& points_)
  : vertices_(points_)
  {}

  Polygon(const std::initializer_list<Point>& points_)
  : vertices_(points_)
  {}

  template<typename ...Pts>
  explicit Polygon(const Pts&... pts)
  : vertices_({pts...})
  {}

  std::vector<Point>getVertices() const {
    return vertices_;
  }

  size_t verticesCount() const {
    return vertices_.size();
  }

  bool isConvex() const {
    bool wasNeg = false;
    bool wasPos = false;
    for (size_t i = 0; i < vertices_.size(); ++i) {
      size_t j = (i + 1) % vertices_.size();
      size_t k = (i + 2) % vertices_.size();
      int result = MathFunc::sign((vertices_[j] - vertices_[i]).crossProduct(vertices_[k] - vertices_[j]));
      if (result == -1) {
        wasNeg = true;
      } else if (result == 1) {
        wasPos = true;
      }
    }
    return !(wasNeg && wasPos);
  }

  fType perimeter() const override {
   fType result = 0;
    for (size_t i = 0; i < vertices_.size(); ++i) {
      result += (vertices_[(i + 1) % vertices_.size()] - vertices_[i]).length();
    }
    return result;
  }

  fType area() const override {
    fType result = 0;
    for (size_t i = 1; i + 1 < vertices_.size(); ++i) {
      result += (vertices_[i] - vertices_[0]).crossProduct(vertices_[i + 1] - vertices_[0]);
    }
    result /= 2.0;
    return fabs(result);
  }

  bool isCongruentTo(const Shape& another) const override {
    auto result = getSimilarityСoefficient(another);
    if (result) {
      return MathFunc::floatisEqual(*result, 1);
    }
    return false;
  }

  bool isSimilarTo(const Shape& another) const override {
    return getSimilarityСoefficient(another) != std::nullopt;
  }

  bool isEqual(const Shape& another) const override {
    const Polygon* anotherPointer = dynamic_cast<const Polygon*>(&another);
    if (anotherPointer == nullptr) {
      return false;
    }

    std::vector<Point> anotherVertices = anotherPointer->getVertices();
    if (anotherVertices.size() != vertices_.size()) {
      return false;
    }

    auto isEqual = [&](const std::vector<Point>& anotherVertices) {
      const int numberOfVertices = vertices_.size();
      for (int shift = 0; shift <= numberOfVertices; ++shift) {
        bool ok = true;
        for (int i = 0; i < numberOfVertices && ok; ++i) {
          ok &= vertices_[i] == anotherVertices[(shift + i) % numberOfVertices];
        }
        if (ok) {
          return true;
        }
      }
      return false;
    };

    if (!isEqual(anotherVertices)) {
      reverse(anotherVertices.begin(), anotherVertices.end());
      return isEqual(anotherVertices);
    }
    return true;
  }

  bool containsPoint(const Point& point) override {
    bool wasNeg = false;
    bool wasPos = false;
    for (size_t i = 0; i < vertices_.size(); ++i) {
      size_t j = (i + 1) % vertices_.size();
      int result = MathFunc::sign((vertices_[j] - vertices_[i]).crossProduct(point - vertices_[i]));
      if (result == -1) {
        wasNeg = true;
      }
      if (result == 1) {
        wasPos = true;
      }
    }
    return !(wasNeg && wasPos);
  }

  void rotate(const Point& center, fType angle) override {
    angle = angle / 180.0 * M_PI;
    for (size_t i = 0; i < vertices_.size(); ++i) {
      vertices_[i] = center.rotate(vertices_[i], angle);
    }
  }

  void reflect(const Point& center) override {
    for (auto& p : vertices_) {
      p = center.reflect(p);
    }
  }

  void reflect(const Line& axis) override {
    for (Point& vertex : vertices_) {
      vertex = axis.reflect(vertex);
    }
  }

  void scale(const Point& center, fType coefficient) override {
    for (Point& vertex : vertices_) {
      vertex = center.scale(vertex, coefficient);
    }
  }

protected:
  std::vector<Point>vertices_;
private:
  std::optional<fType> getSimilarityСoefficient(const Shape& another) const {
    const Polygon* anotherPointer = dynamic_cast<const Polygon*>(&another);
    if (anotherPointer == nullptr) {
      return std::nullopt;
    }
    auto anotherVertices = anotherPointer->getVertices();
    if (anotherVertices.size() != vertices_.size()) {
      return std::nullopt;
    }
    auto isSimilar = [&](const std::vector<Point>& anotherVertices, bool isInverseAngleСomparison) -> std::optional<fType> {
      const int numberOfVertices = vertices_.size();
      for (int shift = 0; shift < numberOfVertices; ++shift) {
        bool ok = true;
        fType ratio = (anotherVertices[(shift + 1) % numberOfVertices] - anotherVertices[shift]).length() / (vertices_[1] - vertices_[0]).length();
        for (int i = 0; i < numberOfVertices && ok; ++i) {
          ok &= MathFunc::floatisEqual(ratio,
                                       (anotherVertices[(shift + i + 1) % numberOfVertices] - anotherVertices[(shift + i) % numberOfVertices]).length()
                                        / (vertices_[(i + 1) % numberOfVertices] - vertices_[i]).length());

          Point sideAnother1 = anotherVertices[(shift + i + 2) % numberOfVertices] - anotherVertices[(shift + i + 1) % numberOfVertices];
          Point sideAnother2 = anotherVertices[(shift + i) % numberOfVertices] - anotherVertices[(shift + i + 1) % numberOfVertices];

          Point sideThis1 = vertices_[(i + 2) % numberOfVertices] - vertices_[(i + 1) % numberOfVertices];
          Point sideThis2 = vertices_[i] - vertices_[(i + 1) % numberOfVertices];

          if (isInverseAngleСomparison) {
            std::swap(sideThis1, sideThis2);
          }
          ok &= MathFunc::floatisEqual(sideThis1.angleBetweenTwoVectors(sideThis2), sideAnother1.angleBetweenTwoVectors(sideAnother2));
        }
        if (ok) {
          return ratio;
        }
      }
      return std::nullopt;
    };

    auto tryOneWayOfAngleComparison = [&](std::vector<Point>& points_, bool isInverseAngleСomparison) {
      std::optional<fType> similarityСoefficient = isSimilar(points_, isInverseAngleСomparison);
      if (similarityСoefficient < 0) {
        std::reverse(points_.begin(), points_.end());
        return isSimilar(points_, isInverseAngleСomparison);
      }
      return similarityСoefficient;
    };

    auto similarityСoefficient = tryOneWayOfAngleComparison(anotherVertices, false);
    if (similarityСoefficient) {
      return similarityСoefficient;
    }
    return tryOneWayOfAngleComparison(anotherVertices, true);
  }
};

class Ellipse : public Shape {
public:
  Ellipse() = default;

  Ellipse(const Point& focus1, const Point& focus2, fType diameter)
    : focus1_(focus1), focus2_(focus2), diameter_(diameter)
  {}

  std::pair<Point,Point> focuses() const {
    return std::make_pair(focus1_, focus2_);
  }

  std::pair<Line, Line> directrices() const {
    Point middle =  (focus2_ + focus1_) / 2.0;
    fType distance = diameter_ / (eccentricity() * 2.0);

    MathFunc::Vector direction = (focus2_ - focus1_) / (focus2_ - focus1_).length() * distance;
    Point directrix1 = middle + Point(direction);
    Point directrix2 = middle - Point(direction);

    std::swap(direction.x, direction.y);
    direction.x = -direction.x;

    return std::make_pair(Line(directrix1, direction), Line(directrix2, direction));
  }

  fType eccentricity() const {
    return (focus1_ - focus2_).length() / diameter_;
  }

  fType perimeter() const override {
     fType a = getGreatHalfAxis();
     fType b = getLittleHalfAxis();
     fType wtf = 3.0 * ((a - b) * (a - b)) / ((a + b) * (a + b));
     return M_PI * (a + b) * (1.0 + wtf / (10.0 + sqrt(4.0 - wtf)));
  }

  fType area() const override {
    return M_PI * getGreatHalfAxis() * getLittleHalfAxis();
  }

  bool isCongruentTo(const Shape& another) const override {
    const Ellipse* anotherPointer = dynamic_cast<const Ellipse*>(&another);
    if (anotherPointer == nullptr) {
      return false;
    }
    return MathFunc::floatisEqual(getGreatHalfAxis(), anotherPointer->getGreatHalfAxis()) && MathFunc::floatisEqual(getLittleHalfAxis(), anotherPointer->getLittleHalfAxis());
  }

  bool isSimilarTo(const Shape& another) const override {
    const Ellipse* anotherPointer = dynamic_cast<const Ellipse*>(&another);
    if (anotherPointer == nullptr) {
      return false;
    }
    return MathFunc::floatisEqual(eccentricity(), anotherPointer->eccentricity());
  }

  bool isEqual(const Shape& another) const override {
    const Ellipse* anotherPointer = dynamic_cast<const Ellipse*>(&another);
    if (anotherPointer == nullptr) {
      return false;
    }
    return diameter_ == anotherPointer->diameter_ &&
          ((focus1_ == anotherPointer->focus1_ && focus2_ == anotherPointer->focus2_)
            || (focus2_ == anotherPointer->focus1_ && focus1_ == anotherPointer->focus2_));}

  bool containsPoint(const Point& point) override {
    return (point - focus1_).length() + (point - focus2_).length() <= diameter_;
  }

  void rotate(const Point& center, fType angle) override {
    angle = angle / 180.0 * M_PI;
    focus1_ = center.rotate(focus1_ , angle);
    focus2_ = center.rotate(focus2_, angle);
  }

  void reflect(const Point& center) override {
    focus1_ = center.reflect(focus1_);
    focus2_ = center.reflect(focus2_);
  }

  void reflect(const Line& axis) override {
    focus1_ = axis.reflect(focus1_);
    focus2_ = axis.reflect(focus2_);
  }

  void scale(const Point& center, fType coefficient) override {
    focus1_ = center.scale(focus1_, coefficient);
    focus2_ = center.scale(focus2_, coefficient);
    diameter_ *= fabs(coefficient);
  }

protected:
  Point focus1_, focus2_;
  fType diameter_;

  fType getGreatHalfAxis() const {
    return diameter_ / 2.0;
  }

  fType getLittleHalfAxis() const {
    return sqrt(diameter_ * diameter_ - (focus2_ - focus1_).length2()) / 2.0;
  }
};

class Circle final : public Ellipse {
public:
  Circle(const Point& center, fType radius)
    : Ellipse(center, center, radius * 2.0)
  {}

  fType radius() const {
    return diameter_ / 2.0;
  }

  fType perimeter() const final {
    return M_PI * diameter_;
  }

  fType area() const final {
    return M_PI * diameter_ * diameter_ / 4.0;
  }

  Point center() const {
    return focus1_;
  }
};

class Rectangle : public Polygon {
public:
  using Polygon::Polygon;

  Rectangle() = default;

  Rectangle (const Point& vertex1, const Point& vertex2, fType ratio) {
    fType diag2 = (vertex2 - vertex1).length2();

    fType lessEdge = sqrt(diag2 / (ratio * ratio + 1));
    fType bigEdge = ratio * lessEdge;
    if (lessEdge > bigEdge) {
      std::swap(lessEdge, bigEdge);
    }

    fType diagonal = sqrt(diag2);

    fType height = lessEdge * bigEdge / diagonal;
    fType smallHeightProjection = lessEdge * lessEdge / diagonal;

    Point direction = (vertex2 - vertex1) / (vertex2 - vertex1).length();
    Point perpendicularDirection = direction.perpendicular();

    Point pointC = vertex1 + direction * smallHeightProjection + perpendicularDirection * height;
    Point pointD = vertex2 - direction * smallHeightProjection - perpendicularDirection * height;

    vertices_ = {vertex1, pointC, vertex2, pointD};
  }

  Rectangle (const Point& vertex1, const Point& vertex2, int ratio)
  : Rectangle(vertex1, vertex2, static_cast<fType>(ratio))
  {}

  Point center() const {
    return vertices_[0] + (vertices_[2] - vertices_[0]) / 2.0;
  }

  std::pair<Line, Line> diagonals() const {
    return std::make_pair(Line(vertices_[0], vertices_[2]), Line(vertices_[1], vertices_[3]));
  }

  fType perimeter() const override {
    return 2 * ((vertices_[1] - vertices_[0]).length() + (vertices_[1] - vertices_[2]).length());
  }

  fType area() const override {
      return (vertices_[1] - vertices_[0]).length() * (vertices_[1] - vertices_[2]).length();
  }
};

class Square final: public Rectangle {
public:
  using Rectangle::Rectangle;

  Square() = default;

  Square(const Point& vertex1, const Point& vertex2)
    : Rectangle(vertex1, vertex2, 1.0)
  {}

  Circle circumscribedCircle() const {
    return Circle(center(), std::sqrt(2.0 * (vertices_[1] - vertices_[0]).length2()));
  }

  Circle inscribedCircle() const {
    return Circle(center(), (vertices_[1] - vertices_[0]).length() / 2.0);
  }
};

class Triangle final : public Polygon {
public:
  using Polygon::Polygon;

  Triangle() = default;

  Circle circumscribedCircle() const {
    Point middle01 = (vertices_[1] + vertices_[0]) / 2.0;
    Point middle12 = (vertices_[2] + vertices_[1]) / 2.0;

    MathFunc::Vector perpendicular01 = (vertices_[0] - vertices_[1]).perpendicular();
    MathFunc::Vector perpendicular12 = (vertices_[1] - vertices_[2]).perpendicular();

    Point mid =Line(middle01, perpendicular01).intersection(Line(middle12, perpendicular12));

    return Circle(mid, (vertices_[0] - mid).length());
  }

  Circle inscribedCircle() const {
    return Circle(orthocenter(), area() * 2.0 / perimeter());
  }

  Point centroid() const {
    Point middle01 = (vertices_[1] + vertices_[0]) / 2.0;
    Point middle12 = (vertices_[2] + vertices_[1]) / 2.0;
    return Line(vertices_[2], middle01).intersection(Line(vertices_[0], middle12));
  }

  Point orthocenter() const {
    MathFunc::Vector bisector102 = (vertices_[1] - vertices_[0]).bisector(vertices_[2] - vertices_[0]);
    MathFunc::Vector bisector012 = (vertices_[0] - vertices_[1]).bisector(vertices_[2] - vertices_[1]);
    return Line(vertices_[0], bisector102).intersection(Line(vertices_[1], bisector012));
  }

  Circle ninePointsCircle() const {
    auto circle = circumscribedCircle();
    Point mid = (orthocenter() + circle.center()) / 2.0;
    return Circle(mid, circle.radius() / 2.0);
  }

  Line EulerLine() const {
    return Line(ninePointsCircle().center(), centroid());
  }
};



