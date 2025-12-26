#ifndef SHAPE_H
#define SHAPE_H

#include <QPointF>
#include <QPolygonF>
#include <QColor>
#include <QString>
#include <QVector>

class Shape
{
public:
    Shape();
    Shape(int id, const QPointF& position, double sizeWidth, double sizeMax);
    void updateVertices(int sides, double size);
    bool checkCollision(const Shape& other) const;
    void resolveCollision(const Shape& other);
    QPolygonF getWorldPolygon() const;
    QPolygonF getWorldPolygon(double globalScale) const;
    QPointF getVertexWorldPosition(int index) const;
    QRectF getBoundingBox() const;
    int id() const { return s_id; }


    bool isVisible() const { return s_visible; }
    void setVisible(bool visible) { s_visible = visible; }

    double rotation() const { return s_rotation; }
    void setRotation(double rotation) { s_rotation = rotation; }

    double scale() const { return s_scale; }
    void setScale(double scale) { s_scale = qMax(0.1, qMin(scale, 3.0)); }

    double size() const { return s_size; }
    void setSize(double size) { s_size = qMax(10.0, qMin(size, 200.0)); }

    double sizeWidth() const { return s_sizeWidth; }

    double sizeHeigth() const { return s_sizeHeigth; }

    void setSizeWidth(double size)
    {
        s_sizeWidth = qMax(10.0, qMin(size, 10000.0));

        if (!s_useCustomVertices) {
            generateRegularPolygonVertices(s_sides, s_size);
        }
    }

    void setSizeHeigth(double size)
    {
        s_sizeHeigth = qMax(10.0, qMin(size, 10000.0));

        if (!s_useCustomVertices) {
            generateRegularPolygonVertices(s_sides, s_size);
        }
    }


    int sides() const { return s_sides; }
    void setSides(int sides) { s_sides = qMax(3, qMin(sides, 20)); }

    QString name() const { return s_name; }
    void setName(const QString &name) { s_name = name; }

    QColor color() const { return s_color; }
    void setColor(const QColor &color) { s_color = color; }

    bool collisionsEnabled() const { return s_collisionsEnabled; }
    void setCollisionsEnabled(bool enabled) { s_collisionsEnabled = enabled; }

    void adjustForEdgeLength(int edgeIndex, double newLength);

    QVector<QPointF> vertices() const { return s_vertices; }
    void setVertices(const QVector<QPointF> &vertices) {
        s_vertices = vertices;
        s_useCustomVertices = true;
    }

    QPointF position() const { return s_position; }
    void setPosition(const QPointF &position) { s_position = position; }
    float calculateOverlap(const Shape& other) const;

    bool useCustomVertices() const { return s_useCustomVertices; }
    void setUseCustomVertices(bool useCustom) { s_useCustomVertices = useCustom; }
    void addVertex(const QPointF& vertex);
    void removeVertex(int index);
    void setVertex(int index, const QPointF& vertex);
    void resetVertices();
    double boundingRadius(double globalScale = 1.0) const;
    QPointF localToWorld(const QPointF& localPoint, double globalScale = 1.0) const;
    QPointF worldToLocal(const QPointF& worldPoint, double globalScale = 1.0) const;

private:
    void generateRegularPolygonVertices(int sides, double radius);
    bool checkPolygonCollision(const QPolygonF& poly1, const QPolygonF& poly2) const;
    QPointF findMTV(const QPolygonF& poly1, const QPolygonF& poly2) const;
    QPointF polygonCenter(const QPolygonF& p) const;
    bool pointInPolygon(const QPolygonF& polygon, const QPointF& p) const;
    bool linesIntersect(const QPointF& p1, const QPointF& p2,
                        const QPointF& p3, const QPointF& p4) const;
    double crossProduct(const QPointF& a, const QPointF& b) const;
    bool onSegment(const QPointF& p, const QPointF& q, const QPointF& r) const;
    int s_id = -1;
    bool s_visible = true;
    double s_rotation = 0.0;
    double s_scale = 1.0;
    double s_size = 50.0;
    int s_sides = 3;
    double s_sizeWidth = 50;
    double s_sizeHeigth = 80;
    QString s_name = "Фигура";
    QColor s_color = QColor(0, 120, 255);
    bool s_collisionsEnabled = true;
    QVector<QPointF> s_vertices;
    QPointF s_position = QPointF(0, 0);
    bool s_useCustomVertices = false;
private:
    static const double COLLISION_EPSILON;
};

#endif // SHAPE_H
