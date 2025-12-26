#include "shape.h"
#include <cmath>
#include <algorithm>
#include <QTransform>
#include <qmath.h>

const double PI = 3.141592653589793;
constexpr double EPS = 5e-5;

Shape::Shape() {}

Shape::Shape(int id, const QPointF& position, double sizeWidth, double sizeHeigth)
    : s_id(id), s_position(position), s_sizeWidth(sizeWidth), s_sizeHeigth(sizeHeigth)
{
    updateVertices(s_sides, s_size);
}

void Shape::updateVertices(int sides, double size)
{
    if (!s_useCustomVertices) {
        s_sides = sides;
        s_size = size;
        generateRegularPolygonVertices(sides, size);
    }
}

void Shape::adjustForEdgeLength(int edgeIndex, double newLength)
{
    if (s_vertices.size() < 3) return;

    int v1 = edgeIndex % s_vertices.size();
    int v2 = (edgeIndex + 1) % s_vertices.size();

    QPointF currentEdge = s_vertices[v2] - s_vertices[v1];
    double currentLength = std::hypot(currentEdge.x(), currentEdge.y());

    if (currentLength < 1e-12) return;
    double scaleFactor = newLength / currentLength;
    s_vertices[v2] = s_vertices[v1] + currentEdge * scaleFactor;
}

void Shape::generateRegularPolygonVertices(int sides, double radius)
{
    s_vertices.clear();
    double baseAngle = (sides == 4) ? -PI/4.0 : -PI/2.0;

    for (int i = 0; i < sides; ++i)
    {
        double angle = 2.0 * PI * i / sides + baseAngle;
        double x = std::cos(angle) * s_sizeWidth;
        double y = std::sin(angle) * s_sizeHeigth;
        s_vertices.append(QPointF(x, y));
    }
}


bool Shape::checkCollision(const Shape& other) const
{
    if (!s_collisionsEnabled || !other.s_collisionsEnabled)
        return false;

    QRectF box1 = getBoundingBox();
    QRectF box2 = other.getBoundingBox();

    if (!box1.intersects(box2)) {
        return false;
    }

    QPolygonF poly1 = getWorldPolygon();
    QPolygonF poly2 = other.getWorldPolygon();

    return checkPolygonCollision(poly1, poly2);
}

bool Shape::checkPolygonCollision(const QPolygonF& poly1, const QPolygonF& poly2) const
{
    int n1 = poly1.size();
    int n2 = poly2.size();
    for (int i = 0; i < n1; i++) {
        QPointF p1 = poly1[i];
        QPointF p2 = poly1[(i + 1) % n1];

        for (int j = 0; j < n2; j++) {
            QPointF p3 = poly2[j];
            QPointF p4 = poly2[(j + 1) % n2];

            if (linesIntersect(p1, p2, p3, p4))
                return true;
        }
    }

    if (pointInPolygon(poly1, polygonCenter(poly2)))
        return true;

    if (pointInPolygon(poly2, polygonCenter(poly1)))
        return true;

    return false;
}


QPointF Shape::findMTV(const QPolygonF& poly1, const QPolygonF& poly2) const
{
    double smallestOverlap = std::numeric_limits<double>::infinity();
    QPointF smallestAxis(0, 0);

    auto project = [](const QPolygonF& p, const QPointF& axis, double& min, double& max)
    {
        min = max = QPointF::dotProduct(p[0], axis);
        for (int i = 1; i < p.size(); ++i)
        {
            double proj = QPointF::dotProduct(p[i], axis);
            if (proj < min) min = proj;
            if (proj > max) max = proj;
        }
    };

    auto normalize = [](const QPointF& v)
    {
        double len = std::sqrt(v.x()*v.x() + v.y()*v.y());
        return (len > 1e-6) ? v / len : QPointF(0, 0);
    };
    auto testAxes = [&](const QPolygonF& poly)
    {
        for (int i = 0; i < poly.size(); i++)
        {
            QPointF p1 = poly[i];
            QPointF p2 = poly[(i + 1) % poly.size()];
            QPointF edge = p2 - p1;

            QPointF axis(-edge.y(), edge.x());
            axis = normalize(axis);
            if (axis.isNull()) continue;

            double minA, maxA, minB, maxB;
            project(poly1, axis, minA, maxA);
            project(poly2, axis, minB, maxB);

            double overlap = std::min(maxA, maxB) - std::max(minA, minB);

            if (overlap < 0)
                return false;

            if (overlap < smallestOverlap)
            {
                smallestOverlap = overlap;
                smallestAxis = axis;
            }
        }
        return true;
    };

    if (!testAxes(poly1)) return QPointF(0, 0);
    if (!testAxes(poly2)) return QPointF(0, 0);
    return smallestAxis * smallestOverlap;
}

void Shape::resolveCollision(const Shape& other)
{
    QPolygonF poly1 = getWorldPolygon();
    QPolygonF poly2 = other.getWorldPolygon();

    QPointF mtv = findMTV(poly1, poly2);
    if (mtv.isNull()) return;
    QPointF dir = polygonCenter(poly1) - polygonCenter(poly2);
    if (QPointF::dotProduct(mtv, dir) < 0)
        mtv = -mtv;
    s_position += mtv;
}

QPointF Shape::polygonCenter(const QPolygonF& p) const
{
    QPointF c(0,0);
    for (auto& v : p) c += v;
    return c / p.size();
}

bool Shape::pointInPolygon(const QPolygonF& polygon, const QPointF& p) const
{
    bool inside = false;
    int count = polygon.size();

    for (int i = 0, j = count - 1; i < count; j = i++)
    {
        const QPointF& pi = polygon[i];
        const QPointF& pj = polygon[j];

        bool intersect = ((pi.y() > p.y()) != (pj.y() > p.y())) &&
                         (p.x() < (pj.x() - pi.x()) * (p.y() - pi.y()) / (pj.y() - pi.y()) + pi.x());

        if (intersect)
            inside = !inside;
    }

    return inside;
}

bool Shape::linesIntersect(const QPointF& p1, const QPointF& p2,
                           const QPointF& p3, const QPointF& p4) const
{
    auto cross = [](const QPointF& a, const QPointF& b){
        return a.x()*b.y() - a.y()*b.x();
    };

    QPointF r = p2 - p1;
    QPointF s = p4 - p3;

    double rxs = cross(r, s);
    double qpxr = cross(p3 - p1, r);

    if (std::abs(rxs) < EPS && std::abs(qpxr) < EPS) {
        auto between = [](double a, double b, double c){
            return c >= std::min(a, b) - EPS && c <= std::max(a, b) + EPS;
        };
        if (between(p1.x(), p2.x(), p3.x()) || between(p1.x(), p2.x(), p4.x()) ||
            between(p3.x(), p4.x(), p1.x()) || between(p3.x(), p4.x(), p2.x()))
            return true;

        if (between(p1.y(), p2.y(), p3.y()) || between(p1.y(), p2.y(), p4.y()) ||
            between(p3.y(), p4.y(), p1.y()) || between(p3.y(), p4.y(), p2.y()))
            return true;

        return false;
    }

    if (std::abs(rxs) < EPS && std::abs(qpxr) < EPS)
        return false;

    double t = cross(p3 - p1, s) / rxs;
    double u = cross(p3 - p1, r) / rxs;

    return (t >= -EPS && t <= 1.0 + EPS && u >= -EPS && u <= 1.0 + EPS);
}

double Shape::crossProduct(const QPointF& a, const QPointF& b) const
{
    return a.x() * b.y() - a.y() * b.x();
}

bool Shape::onSegment(const QPointF& p, const QPointF& q, const QPointF& r) const
{
    if (q.x() <= qMax(p.x(), r.x()) && q.x() >= qMin(p.x(), r.x()) &&
        q.y() <= qMax(p.y(), r.y()) && q.y() >= qMin(p.y(), r.y())) {
        return true;
    }
    return false;
}

QPolygonF Shape::getWorldPolygon() const
{
    QPolygonF polygon;

    for (const QPointF& vertex : s_vertices) {
        QPointF scaledVertex = vertex * s_scale;

        if (!std::abs(s_rotation) < EPS) {
            QTransform transform;
            transform.rotate(s_rotation);
            scaledVertex = transform.map(scaledVertex);
        }

        scaledVertex += s_position;

        polygon << scaledVertex;
    }

    return polygon;
}

QPolygonF Shape::getWorldPolygon(double globalScale) const
{
    QPolygonF polygon;

    for (const QPointF& vertex : s_vertices) {
        QPointF scaledVertex = vertex * s_scale;
        if (!std::abs(s_rotation) < EPS) {
            QTransform transform;
            transform.rotate(s_rotation);
            scaledVertex = transform.map(scaledVertex);
        }
        scaledVertex += s_position;

        polygon << scaledVertex;
    }

    return polygon;
}

QPointF Shape::getVertexWorldPosition(int index) const
{
    if (index < 0 || index >= s_vertices.size())
        return QPointF();

    QPointF localPos = s_vertices[index] * s_scale;
    if (!std::abs(s_rotation) < EPS) {
        QTransform transform;
        transform.rotate(s_rotation);
        localPos = transform.map(localPos);
    }
    return s_position + localPos;
}

QRectF Shape::getBoundingBox() const
{
    QPolygonF polygon = getWorldPolygon();
    return polygon.boundingRect();
}

void Shape::addVertex(const QPointF& vertex)
{
    s_vertices.append(vertex);
    s_sides = s_vertices.size();
    s_useCustomVertices = true;
}

void Shape::removeVertex(int index)
{
    if (index >= 0 && index < s_vertices.size()) {
        s_vertices.remove(index);
        s_sides = s_vertices.size();
        s_useCustomVertices = true;
    }
}

void Shape::setVertex(int index, const QPointF& vertex)
{
    if (index >= 0 && index < s_vertices.size()) {
        s_vertices[index] = vertex;
        s_useCustomVertices = true;
    }
}

void Shape::resetVertices()
{
    s_useCustomVertices = false;
    generateRegularPolygonVertices(s_sides, s_size);
}

double Shape::boundingRadius(double globalScale) const
{
    if (s_vertices.isEmpty())
        return s_size * s_scale;

    double maxDistance = 0;
    for (const QPointF& vertex : s_vertices) {
        double distance = qSqrt(vertex.x() * vertex.x() + vertex.y() * vertex.y());
        if (distance > maxDistance) maxDistance = distance;
    }

    return maxDistance * s_scale;
}

QPointF Shape::localToWorld(const QPointF& localPoint, double globalScale) const
{
    QPointF transformed = localPoint * s_scale;
    if (!std::abs(s_rotation) < EPS) {
        QTransform transform;
        transform.rotate(s_rotation);
        transformed = transform.map(transformed);
    }

    return s_position + transformed;
}

QPointF Shape::worldToLocal(const QPointF& worldPoint, double globalScale) const
{
    QPointF local = worldPoint - s_position;

    if (!std::abs(s_rotation) < EPS) {
        QTransform transform;
        transform.rotate(-s_rotation);
        local = transform.map(local);
    }

    return local / s_scale;
}
