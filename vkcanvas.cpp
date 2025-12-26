#include "vkcanvas.h"
#include <cmath>
#include <QTransform>
#include <QPainter>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QTimer>
#include <qcursor.h>

const double PI = 3.141592653589793;

VKCanvas::VKCanvas(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(false);
    m_dragging = false;
    m_blockTableUpdates = false;

    if (c_shapes.isEmpty()) {
        addSquare(0, 0, 90,50);
        emit shapeCountChanged();
        update();
    }
}

QPointF VKCanvas::getShapeVertexWorld(int shapeId, int vertexIndex) const
{
    const Shape* shape = getShapeById(shapeId);
    if (shape && vertexIndex >= 0 && vertexIndex < shape->vertices().size()) {
        QPointF worldPos = shape->getVertexWorldPosition(vertexIndex);
        return worldPos;
    }
    return QPointF();
}

QPointF VKCanvas::worldToLocal(int shapeId, const QPointF &worldPos) const
{
    const Shape* shape = getShapeById(shapeId);
    if (shape) {
        QPointF localPos = shape->worldToLocal(worldPos, 1.0);
        return localPos;
    }
    return worldPos;
}

void VKCanvas::setSelectedShapeId(int id)
{
    if (c_selectedShapeId == id) return;

    int oldId = c_selectedShapeId;
    c_selectedShapeId = id;
    setSelectedVertexIndex(-1);
    setSelectedEdgeIndex(-1);

    emit selectedShapeIdChanged();
    emit vertexInfoUpdated();

    update();
}

int VKCanvas::addShapeWithSides(float x, float y, int sides, float sizeWidth, float sizeHeight) {
    Shape shape(c_nextShapeId++, QPointF(x,y), sizeWidth, sizeHeight);
    shape.setSides(sides);
    shape.updateVertices(sides, shape.size());

    static const QString shapeNames[] = { "Фигура", "Квадрат", "Треугольник", "Пятиугольник", "Шестиугольник", "dddd" };
    qDebug() << sides;
    if (sides == 4) {
        if(sizeWidth == sizeHeight) {
            shape.setName(shapeNames[1]);
        } else
            shape.setName(shapeNames[0]);
    } else if (sides == 3) {
        shape.setName(shapeNames[2]);
    } else if (sides >= 5 && sides <= 6) {
        shape.setName(shapeNames[sides-2]);
    } else {
        shape.setName(QString("%1-угольник").arg(sides));
    }

    qDebug() << shape.name();
    shape.setColor(QColor(0, 120, 255));
    shape.setCollisionsEnabled(true);

    c_shapes.append(shape);

    qDebug() << "Координаты:" << QString("(%1, %2)").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2);
    QVector<QPointF> vertices = shape.vertices();
    for (int i = 0; i < vertices.size(); i++) {
        int nextI = (i + 1) % vertices.size();
        QPointF p1 = vertices[i];
        QPointF p2 = vertices[nextI];
        QPointF edgeCenter = (p1 + p2) / 2.0;
        float length = std::sqrt(std::pow(p2.x() - p1.x(), 10) + std::pow(p2.y() - p1.y(), 10));

        qDebug() << "Сторона #" << i << ":";
        qDebug() << " Вершины (" << QString::number(p1.x(), 'f', 10)
                 << "," << QString::number(p1.y(), 'f', 10) << ") ->\n ("
                 << QString::number(p2.x(), 'f', 10) << "," << QString::number(p2.y(), 'f', 10) << ")";
        qDebug() << " Ребра (" << QString::number(edgeCenter.x(), 'f', 10)
                 << "," << QString::number(edgeCenter.y(), 'f', 10) << ")";
        qDebug() << " Ребра " << QString::number(length, 'f', 10) << '\n';
    }

    emit shapeAdded(shape.id());
    emit shapeCountChanged();
    setSelectedShapeId(shape.id());

    if (!m_blockTableUpdates) {
        emit vertexInfoUpdated();
    }
    update();
    return shape.id();
}

void VKCanvas::removeShape(int id) {
    for (int i = 0; i < c_shapes.size(); ++i) {
        if (c_shapes[i].id() == id) {
            bool wasSelected = (c_selectedShapeId == id);

            c_shapes.removeAt(i);
            emit shapeRemoved(id);
            emit shapeCountChanged();

            if (wasSelected) {
                if (c_shapes.size() > 0) {
                    int newIndex = qMin(i, c_shapes.size() - 1);
                    setSelectedShapeId(c_shapes[newIndex].id());
                } else {
                    setSelectedShapeId(-1);
                    c_selectedVertexIndex = -1;
                    c_selectedEdgeIndex = -1;
                    emit selectedVertexIndexChanged();
                    emit selectedEdgeIndexChanged();
                }
            }

            if (!m_blockTableUpdates) {
                emit vertexInfoUpdated();
            }
            update();
            return;
        }
    }
}


void VKCanvas::addShapeAndSelect(float x, float y, int sides, float sizeWidth, float sizeHeight) {
    int id = addShapeWithSides(x, y, sides, sizeWidth, sizeHeight);
    setSelectedShapeId(id);
}

int VKCanvas::getShapeVertexCount(int id) const
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->vertices().size() : 0;
}

float VKCanvas::edgeLength(int shapeId, int edgeIndex) const
{
    const Shape* shape = getShapeById(shapeId);
    if (!shape || shape->vertices().size() < 2) {
        return 0.0;
    }

    int vertexCount = shape->vertices().size();
    int v1 = edgeIndex % vertexCount;
    int v2 = (v1 + 1) % vertexCount;
    QPointF p1 = shape->vertices()[v1];
    QPointF p2 = shape->vertices()[v2];

    float length = std::sqrt(std::pow(p2.x() - p1.x(), 2) + std::pow(p2.y() - p1.y(), 2));

    return length;
}

QPointF VKCanvas::getShapeVertex(int id, int vertexIndex)
{
    const Shape* shape = getShapeById(id);
    QPointF point = shape ? shape->vertices().value(vertexIndex) : QPointF();
    return point;
}

const Shape* VKCanvas::getShapeById(int id) const
{
    for (const Shape &shape : c_shapes) {
        if (shape.id() == id) {
            return &shape;
        }
    }
    return nullptr;
}

Shape* VKCanvas::getShapeById(int id)
{
    for (Shape &shape : c_shapes) {
        if (shape.id() == id) {
            return &shape;
        }
    }
    return nullptr;
}

VKCanvas::~VKCanvas() {}

void VKCanvas::centerOnZero()
{
    c_offsetX = width() / 2;
    c_offsetY = height() / 2;
    emit offsetChanged();
    update();
}

void VKCanvas::setShowGrid(bool show)
{
    if (c_showGrid == show) return;
    c_showGrid = show;
    emit showGridChanged();
    update();
}

void VKCanvas::setCollisionsEnabled(bool enabled)
{
    if (c_collisionsEnabled == enabled) return;
    c_collisionsEnabled = enabled;
    emit collisionsEnabledChanged();
    update();
}

void VKCanvas::setActiveTab(int tab)
{
    if (c_activeTab == tab) return;
    c_activeTab = tab;
    c_selectedVertexIndex = -1;
    c_selectedEdgeIndex = -1;
    emit activeTabChanged();
    emit selectedVertexIndexChanged();
    emit selectedEdgeIndexChanged();
    update();
}

int VKCanvas::addTriangle(float x, float y, float sizeWidth, float sizeHeight)
{
    return addShapeWithSides(x, y, 3, sizeWidth, sizeHeight);
}

int VKCanvas::addSquare(float x, float y, float sizeWidth, float sizeHeight)
{
    return addShapeWithSides(x, y, 4, sizeWidth, sizeHeight);
}

int VKCanvas::addPentagon(float x, float y, float sizeWidth, float sizeHeight)
{
    return addShapeWithSides(x, y, 5, sizeWidth, sizeHeight);
}

int VKCanvas::addHexagon(float x, float y, float sizeWidth, float sizeHeight)
{
    return addShapeWithSides(x, y, 6, sizeWidth, sizeHeight);
}

int VKCanvas::addHeptagon(float x, float y, float sizeWidth, float sizeHeight)
{
    return addShapeWithSides(x, y, 7, sizeWidth, sizeHeight);
}

int VKCanvas::addOctagon(float x, float y, float sizeWidth, float sizeHeight)
{
    return addShapeWithSides(x, y, 8, sizeWidth, sizeHeight);
}

void VKCanvas::setShapeRotation(int id, float rotation)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setRotation(rotation);
        emit shapeUpdated(id);
        update();
    }
}

void VKCanvas::setShapeScale(int id, float scale)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setScale(scale);
        emit shapeUpdated(id);
        update();
    }
}

void VKCanvas::setShapeColor(int id, const QColor &color)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setColor(color);
        emit shapeUpdated(id);
        update();
    }
}

void VKCanvas::setShapeSides(int id, int sides)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setSides(sides);
        shape->updateVertices(sides, shape->size());
        emit shapeUpdated(id);
        if (!m_blockTableUpdates) {
            emit vertexInfoUpdated();
        }
        update();
    }
}

void VKCanvas::setShapeSizeWidgth(int id, float sizeWidgth)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setSizeWidth(sizeWidgth);
        shape->updateVertices(shape->sides(), sizeWidgth);
        emit shapeUpdated(id);
        if (!m_blockTableUpdates) {
            emit vertexInfoUpdated();
        }
        update();
    }
}

void VKCanvas::setShapeSizeHeight(int id, float sizeHeight)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setSizeHeigth(sizeHeight);
        shape->updateVertices(shape->sides(), sizeHeight);
        emit shapeUpdated(id);
        if (!m_blockTableUpdates) {
            emit vertexInfoUpdated();
        }
        update();
    }
}

void VKCanvas::setShapeName(int id, const QString &name)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setName(name);
        emit shapeUpdated(id);
        update();
    }
}

void VKCanvas::setShapeCollisionsEnabled(int id, bool enabled)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setCollisionsEnabled(enabled);
        emit shapeUpdated(id);
        update();
    }
}

float VKCanvas::getShapeRotation(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->rotation() : 0.0;
}

float VKCanvas::getShapeScale(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->scale() : 1.0;
}

QColor VKCanvas::getShapeColor(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->color() : QColor();
}

int VKCanvas::getShapeSides(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->sides() : 3;
}

float VKCanvas::getShapeSize(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->size() : 50.0;
}

float VKCanvas::getShapeSizeWidth(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->sizeWidth() : 50.0;
}

float VKCanvas::getShapeSizeHeight(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->sizeHeigth() : 50.0;
}

QString VKCanvas::getShapeName(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->name() : "";
}

bool VKCanvas::getShapeCollisionsEnabled(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->collisionsEnabled() : true;
}

int VKCanvas::getShapeIdByIndex(int index)
{
    if (index >= 0 && index < c_shapes.size()) {
        return c_shapes[index].id();
    }
    return -1;
}

QPointF VKCanvas::vertexToWorld(int shapeId, int vertexIndex) const
{
    const Shape* shape = getShapeById(shapeId);
    if (shape && vertexIndex >= 0 && vertexIndex < shape->vertices().size()) {
        return shape->getVertexWorldPosition(vertexIndex);
    }
    return QPointF();
}

void VKCanvas::clear()
{
    c_shapes.clear();
    c_nextShapeId = 0;
    c_selectedShapeId = -1;
    c_selectedVertexIndex = -1;
    c_selectedEdgeIndex = -1;
    emit shapeCountChanged();
    emit selectedShapeIdChanged();
    emit selectedVertexIndexChanged();
    emit selectedEdgeIndexChanged();
    if (!m_blockTableUpdates) {
        emit vertexInfoUpdated();
    }
    update();
}

void VKCanvas::resetView()
{
    c_globalScale = 1.0;
    centerOnZero();
    emit globalScaleChanged();
    update();
}

QPointF VKCanvas::screenToWorld(const QPointF &screenPos)
{
    return screenToWorldNoRotation(screenPos);
}

QPointF VKCanvas::worldToScreen(const QPointF &worldPos)
{
    return worldToScreenNoRotation(worldPos);
}

QPointF VKCanvas::vertexToScreen(int shapeId, int vertexIndex) const
{
    const Shape* shape = getShapeById(shapeId);
    if (shape && vertexIndex >= 0 && vertexIndex < shape->vertices().size()) {
        QPointF vertexWorldPos = shape->getVertexWorldPosition(vertexIndex);
        return worldToScreenNoRotation(vertexWorldPos);
    }
    return QPointF();
}

QPointF VKCanvas::screenToVertexLocal(int shapeId, const QPointF &screenPos) const
{
    const Shape* shape = getShapeById(shapeId);
    if (shape) {
        QPointF worldPos = screenToWorldNoRotation(screenPos);
        return shape->worldToLocal(worldPos, c_globalScale);
    }
    return QPointF();
}

QPointF VKCanvas::screenToWorldNoRotation(const QPointF &screenPos) const
{
    QPointF worldPos = screenPos;
    worldPos.rx() -= c_offsetX;
    worldPos.ry() -= c_offsetY;
    worldPos.rx() /= c_globalScale;
    worldPos.ry() /= c_globalScale;
    return worldPos;
}

QPointF VKCanvas::worldToScreenNoRotation(const QPointF &worldPos) const
{
    QPointF screenPos = worldPos;
    screenPos.rx() *= c_globalScale;
    screenPos.ry() *= c_globalScale;
    screenPos.rx() += c_offsetX;
    screenPos.ry() += c_offsetY;
    return screenPos;
}

QPolygonF VKCanvas::createPolygon(const Shape &shape) const
{
    QPolygonF polygon = shape.getWorldPolygon(c_globalScale);
    for (int i = 0; i < polygon.size(); ++i) {
        polygon[i] = worldToScreenNoRotation(polygon[i]);
    }

    return polygon;
}

float VKCanvas::getShapeRadius(const Shape &shape) const
{
    return shape.boundingRadius(c_globalScale);
}

QPointF VKCanvas::applyRotation(const QPointF &point, const QPointF &center, float rotation) const
{
    if (qFuzzyIsNull(rotation)) return point;

    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(rotation);
    transform.translate(-center.x(), -center.y());
    return transform.map(point);
}

QPointF VKCanvas::applyInverseRotation(const QPointF &point, const QPointF &center, float rotation) const
{
    if (qFuzzyIsNull(rotation)) return point;

    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(-rotation);
    transform.translate(-center.x(), -center.y());
    return transform.map(point);
}

QPointF VKCanvas::findClosestVertex(const Shape &shape, const QPointF &screenPos, int &vertexIndex, float searchRadius) const
{
    vertexIndex = -1;
    float minDistance = searchRadius * searchRadius;
    QPointF closestPoint;

    for (int i = 0; i < shape.vertices().size(); ++i) {
        QPointF vertexScreenPos = vertexToScreen(shape.id(), i);

        float dx = screenPos.x() - vertexScreenPos.x();
        float dy = screenPos.y() - vertexScreenPos.y();
        float distanceSquared = dx * dx + dy * dy;

        if (distanceSquared < minDistance) {
            minDistance = distanceSquared;
            vertexIndex = i;
            closestPoint = vertexScreenPos;
        }
    }

    return closestPoint;
}

QPointF VKCanvas::findClosestEdge(const Shape &shape, const QPointF &screenPos, int &edgeIndex, float searchRadius) const
{
    edgeIndex = -1;
    float minDistance = searchRadius * searchRadius;
    QPointF closestPoint;
    QPointF edgeCenter;

    int vertexCount = shape.vertices().size();
    for (int i = 0; i < vertexCount; ++i) {
        int nextI = (i + 1) % vertexCount;
        QPointF p1 = vertexToScreen(shape.id(), i);
        QPointF p2 = vertexToScreen(shape.id(), nextI);
        QPointF p1p2 = p2 - p1;
        QPointF p1p = screenPos - p1;
        float lengthSquared = p1p2.x() * p1p2.x() + p1p2.y() * p1p2.y();

        if (lengthSquared == 0) continue;

        float t = qMax(0.0, qMin(1.0, (p1p.x() * p1p2.x() + p1p.y() * p1p2.y()) / lengthSquared));
        QPointF projection = p1 + t * p1p2;

        float dx = screenPos.x() - projection.x();
        float dy = screenPos.y() - projection.y();
        float distanceSquared = dx * dx + dy * dy;

        if (distanceSquared < minDistance) {
            minDistance = distanceSquared;
            edgeIndex = i;
            closestPoint = projection;
            edgeCenter = (shape.vertices()[i] + shape.vertices()[nextI]) / 2.0;
        }
    }

    return edgeCenter;
}

int VKCanvas::findShapeAtPoint(const QPointF &screenPos)
{
    QPointF worldPos = screenToWorldNoRotation(screenPos);
    for (int i = c_shapes.size() - 1; i >= 0; --i) {
        const Shape &shape = c_shapes[i];
        if (!shape.isVisible()) continue;
        QPolygonF worldPolygon = shape.getWorldPolygon(c_globalScale);
        if (pointInPolygon(worldPos, worldPolygon)) {
            return shape.id();
        }
    }

    return -1;
}

bool VKCanvas::pointInPolygon(const QPointF& point, const QPolygonF& polygon) const
{
    if (polygon.size() < 3)
        return false;

    bool inside = false;
    int n = polygon.size();

    for (int i = 0, j = n - 1; i < n; j = i++) {
        QPointF pi = polygon[i];
        QPointF pj = polygon[j];

        if (((pi.y() > point.y()) != (pj.y() > point.y())) &&
            (point.x() < (pj.x() - pi.x()) * (point.y() - pi.y()) / (pj.y() - pi.y()) + pi.x())) {
            inside = !inside;
        }
    }

    return inside;
}

QSGGeometryNode* VKCanvas::createShapeNode()
{
    QSGGeometryNode *node = new QSGGeometryNode();
    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
    QSGFlatColorMaterial *material = new QSGFlatColorMaterial();

    geometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);

    return node;
}

QSGGeometryNode* VKCanvas::createVertexNode(const QPointF &position, const QColor &color, float size)
{
    QSGGeometryNode *node = new QSGGeometryNode();
    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    QSGFlatColorMaterial *material = new QSGFlatColorMaterial();

    geometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();

    vertices[0].set(position.x() - size/2, position.y() - size/2);
    vertices[1].set(position.x() + size/2, position.y() - size/2);
    vertices[2].set(position.x() + size/2, position.y() + size/2);
    vertices[3].set(position.x() - size/2, position.y() + size/2);

    material->setColor(color);

    return node;
}

QSGGeometryNode* VKCanvas::createEdgeNode(const QPointF &start, const QPointF &end, const QColor &color, float width)
{
    QSGGeometryNode *node = new QSGGeometryNode();
    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2);
    QSGFlatColorMaterial *material = new QSGFlatColorMaterial();

    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(width);
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
    vertices[0].set(start.x(), start.y());
    vertices[1].set(end.x(), end.y());

    material->setColor(color);

    return node;
}

QSGGeometryNode* VKCanvas::createTransformHandle(const QPointF &position, const QColor &color, float size)
{
    QSGGeometryNode *node = new QSGGeometryNode();
    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    QSGFlatColorMaterial *material = new QSGFlatColorMaterial();

    geometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();

    vertices[0].set(position.x() - size/2, position.y() - size/2);
    vertices[1].set(position.x() + size/2, position.y() - size/2);
    vertices[2].set(position.x() + size/2, position.y() + size/2);
    vertices[3].set(position.x() - size/2, position.y() + size/2);

    material->setColor(color);

    return node;
}

void VKCanvas::updateShapeGeometry(QSGGeometryNode *node, const Shape &shape)
{
    if (!shape.isVisible()) {
        node->geometry()->allocate(0);
        return;
    }

    QPolygonF polygon = createPolygon(shape);
    QSGGeometry *geometry = node->geometry();
    geometry->allocate(polygon.size());

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();

    for (int i = 0; i < polygon.size(); ++i) {
        vertices[i].set(polygon[i].x(), polygon[i].y());
    }

    QSGFlatColorMaterial *material = static_cast<QSGFlatColorMaterial *>(node->material());
    if (shape.id() == c_selectedShapeId) {
        QColor selectedColor = shape.color().lighter(150);
        selectedColor.setAlpha(200);
        material->setColor(selectedColor);
    } else {
        QColor normalColor = shape.color();
        normalColor.setAlpha(180);
        material->setColor(normalColor);
    }
}

void VKCanvas::addVertexToShape(int id, float x, float y)
{
    Shape* shape = getShapeById(id);
    if (shape && shape->vertices().size() < 20) {
        if (c_selectedEdgeIndex != -1) {
            QVector<QPointF> vertices = shape->vertices();
            int vertexCount = vertices.size();

            if (vertexCount >= 2) {
                int nextIndex = (c_selectedEdgeIndex + 1) % vertexCount;
                QPointF currentVertex = vertices[c_selectedEdgeIndex];
                QPointF nextVertex = vertices[nextIndex];
                QPointF newVertex = QPointF(
                    (currentVertex.x() + nextVertex.x()) / 2.0,
                    (currentVertex.y() + nextVertex.y()) / 2.0
                    );
                vertices.insert(nextIndex, newVertex);
                shape->setVertices(vertices);

                emit vertexAdded(id, nextIndex);
                emit shapeUpdated(id);
                if (!m_blockTableUpdates) {
                    emit vertexInfoUpdated();
                }
                update();
            }
        } else {
            shape->addVertex(QPointF(x, y));
            emit vertexAdded(id, shape->vertices().size() - 1);
            emit shapeUpdated(id);
            if (!m_blockTableUpdates) {
                emit vertexInfoUpdated();
            }
            update();
        }
    }
}

void VKCanvas::removeVertexFromShape(int id, int vertexIndex)
{
    Shape* shape = getShapeById(id);
    if (shape && shape->vertices().size() > 3) {
        shape->removeVertex(vertexIndex);
        emit vertexRemoved(id, vertexIndex);
        emit shapeUpdated(id);
        if (!m_blockTableUpdates) {
            emit vertexInfoUpdated();
        }
        update();
    }
}

void VKCanvas::resetShapeVertices(int id)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->resetVertices();
        emit shapeUpdated(id);
        if (!m_blockTableUpdates) {
            emit vertexInfoUpdated();
        }
        update();
    }
}

void VKCanvas::setShapeVertex(int id, int vertexIndex, float x, float y)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setVertex(vertexIndex, QPointF(x, y));
        emit vertexMoved(id, vertexIndex);
        emit shapeUpdated(id);
        if (!m_blockTableUpdates) {
            emit vertexInfoUpdated();
        }
        update();
    }
}

void VKCanvas::setEdgeLength(int shapeId, int edgeIndex, float newLength)
{
    Shape* shape = getShapeById(shapeId);
    if (!shape || shape->vertices().size() < 2) {
        return;
    }

    int vertexCount = shape->vertices().size();
    int v1 = edgeIndex % vertexCount;
    int v2 = (edgeIndex + 1) % vertexCount;

    QPointF p1 = shape->vertices()[v1];
    QPointF p2 = shape->vertices()[v2];

    QPointF midPoint = (p1 + p2) / 2.0;

    QPointF edge = p2 - p1;
    float currentLength = std::sqrt(edge.x() * edge.x() + edge.y() * edge.y());

    if (currentLength < 1e-6) {
        return;
    }

    QPointF dir = edge / currentLength;
    float halfNewLength = newLength / 2.0;

    QPointF newP1 = midPoint - dir * halfNewLength;
    QPointF newP2 = midPoint + dir * halfNewLength;

    shape->setVertex(v1, newP1);
    shape->setVertex(v2, newP2);

    shape->setUseCustomVertices(true);

    if (c_collisionsEnabled && shape->collisionsEnabled()) {
        const int maxIterations = 5;
        for (int iter = 0; iter < maxIterations; ++iter) {
            bool anyCollision = false;

            for (Shape &otherShape : c_shapes) {
                if (otherShape.id() == shape->id()) continue;
                if (!otherShape.isVisible() || !otherShape.collisionsEnabled()) continue;

                if (shape->checkCollision(otherShape)) {
                    shape->resolveCollision(otherShape);
                    anyCollision = true;
                }
            }

            if (!anyCollision) break;
        }
    }

    emit vertexMoved(shapeId, v1);
    emit vertexMoved(shapeId, v2);
    emit shapeUpdated(shapeId);
    if (!m_blockTableUpdates) {
        emit vertexInfoUpdated();
    }
    update();
}

void VKCanvas::updateAxisXGeometry(QSGGeometry *geometry, QSGFlatColorMaterial *material)
{
    if (!c_showGrid) {
        geometry->allocate(0);
        return;
    }

    const float gridSize = 50.0;
    const float axisWidth = 1.0;

    QPointF topLeft = screenToWorldNoRotation(QPointF(0, 0));
    QPointF bottomRight = screenToWorldNoRotation(QPointF(width(), height()));

    int startX = floor(topLeft.x() / gridSize) * gridSize;
    int endX = ceil(bottomRight.x() / gridSize) * gridSize;
    if (topLeft.y() <= 0 && bottomRight.y() >= 0) {
        geometry->allocate(2);
        geometry->setDrawingMode(QSGGeometry::DrawLines);
        geometry->setLineWidth(axisWidth);

        QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
        QPointF p1 = worldToScreenNoRotation(QPointF(startX - gridSize, 0));
        QPointF p2 = worldToScreenNoRotation(QPointF(endX + gridSize, 0));

        vertices[0].set(p1.x(), p1.y());
        vertices[1].set(p2.x(), p2.y());

        material->setColor(QColor(255, 0, 0, 200));
    } else {
        geometry->allocate(0);
    }
}

void VKCanvas::updateAxisYGeometry(QSGGeometry *geometry, QSGFlatColorMaterial *material)
{
    if (!c_showGrid) {
        geometry->allocate(0);
        return;
    }

    const float gridSize = 50.0;
    const float axisWidth = 1.0;

    QPointF topLeft = screenToWorldNoRotation(QPointF(0, 0));
    QPointF bottomRight = screenToWorldNoRotation(QPointF(width(), height()));

    int startY = floor(topLeft.y() / gridSize) * gridSize;
    int endY = ceil(bottomRight.y() / gridSize) * gridSize;
    if (topLeft.x() <= 0 && bottomRight.x() >= 0) {
        geometry->allocate(2);
        geometry->setDrawingMode(QSGGeometry::DrawLines);
        geometry->setLineWidth(axisWidth);

        QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
        QPointF p1 = worldToScreenNoRotation(QPointF(0, startY - gridSize));
        QPointF p2 = worldToScreenNoRotation(QPointF(0, endY + gridSize));

        vertices[0].set(p1.x(), p1.y());
        vertices[1].set(p2.x(), p2.y());

        material->setColor(QColor(0, 0, 255, 200));
    } else {
        geometry->allocate(0);
    }
}

void VKCanvas::updateGridGeometry(QSGGeometry *geometry, QSGFlatColorMaterial *material)
{
    if (!c_showGrid) {
        geometry->allocate(0);
        return;
    }

    const float gridSize = 50.0;
    const float gridWidth = 1.0;

    QPointF topLeft = screenToWorldNoRotation(QPointF(0, 0));
    QPointF bottomRight = screenToWorldNoRotation(QPointF(width(), height()));

    int startX = floor(topLeft.x() / gridSize) * gridSize;
    int endX = ceil(bottomRight.x() / gridSize) * gridSize;
    int startY = floor(topLeft.y() / gridSize) * gridSize;
    int endY = ceil(bottomRight.y() / gridSize) * gridSize;

    int numLinesX = (endX - startX) / gridSize + 1;
    int numLinesY = (endY - startY) / gridSize + 1;
    int vertexCount = 0;
    for (float x = startX; x <= endX; x += gridSize) {
        if (!qFuzzyIsNull(x)) vertexCount += 2;
    }
    for (float y = startY; y <= endY; y += gridSize) {
        if (!qFuzzyIsNull(y)) vertexCount += 2;
    }

    geometry->allocate(vertexCount);
    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(gridWidth);

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
    int vertexIndex = 0;
    for (float x = startX; x <= endX; x += gridSize) {
        if (qFuzzyIsNull(x)) continue;

        QPointF p1 = worldToScreenNoRotation(QPointF(x, startY - gridSize));
        QPointF p2 = worldToScreenNoRotation(QPointF(x, endY + gridSize));

        vertices[vertexIndex++].set(p1.x(), p1.y());
        vertices[vertexIndex++].set(p2.x(), p2.y());
    }
    for (float y = startY; y <= endY; y += gridSize) {
        if (qFuzzyIsNull(y)) continue;

        QPointF p1 = worldToScreenNoRotation(QPointF(startX - gridSize, y));
        QPointF p2 = worldToScreenNoRotation(QPointF(endX + gridSize, y));

        vertices[vertexIndex++].set(p1.x(), p1.y());
        vertices[vertexIndex++].set(p2.x(), p2.y());
    }

    material->setColor(QColor(100, 100, 100, 80));
}

void VKCanvas::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        if (width() > 0 && height() > 0) {
            c_offsetX = width() / 2.0;
            c_offsetY = height() / 2.0;
            emit offsetChanged();
            update();
        }
    }
}

void VKCanvas::mouseMoveEvent(QMouseEvent *event)
{
    QPointF delta = event->position() - c_lastMousePos;
    c_lastMousePos = event->position();

    if (c_dragMode == PanCanvas) {
        c_offsetX += delta.x();
        c_offsetY += delta.y();
        emit offsetChanged();
        update();
        event->accept();
    } else if (c_dragMode == DragShape && c_draggingShapeId != -1) {
        Shape *shape = getShapeById(c_draggingShapeId);
        if (shape) {
            bool shapeChanged = false;

            if (c_transformMode == Rotate) {
                QPointF center = worldToScreenNoRotation(shape->position());
                QPointF startVector = c_dragStartPos - center;
                QPointF currentVector = event->position() - center;

                float startAngle = atan2(startVector.y(), startVector.x()) * 180.0 / PI;
                float currentAngle = atan2(currentVector.y(), currentVector.x()) * 180.0 / PI;
                float rotationDelta = currentAngle - startAngle;

                if (fabs(rotationDelta) > 0.5) {
                    shape->setRotation(shape->rotation() + rotationDelta);
                    c_dragStartPos = event->position();
                    shapeChanged = true;
                }
            } else if (c_transformMode == Scale) {
                QPointF center = worldToScreenNoRotation(shape->position());
                float startDistance = QLineF(center, c_dragStartPos).length();
                float currentDistance = QLineF(center, event->position()).length();

                if (startDistance > 0) {
                    float scaleFactor = currentDistance / startDistance;
                    if (fabs(scaleFactor - 1.0) > 0.01) {
                        shape->setScale(shape->scale() * scaleFactor);
                        shape->setScale(qMax(0.1, qMin(shape->scale(), 3.0)));
                        c_dragStartPos = event->position();
                        shapeChanged = true;
                    }
                }
            } else if (c_transformMode == Move) {
                QPointF worldDelta = screenToWorldNoRotation(event->position()) -
                                     screenToWorldNoRotation(c_dragStartPos);

                QPointF newPos = c_dragShapeStartPos + worldDelta;
                shape->setPosition(newPos);

                if (c_collisionsEnabled && shape->collisionsEnabled()) {
                    const int maxIterations = 5;
                    for (int iter = 0; iter < maxIterations; ++iter) {
                        bool anyCollision = false;

                        for (Shape &otherShape : c_shapes) {
                            if (otherShape.id() == shape->id()) continue;
                            if (!otherShape.isVisible() || !otherShape.collisionsEnabled()) continue;

                            if (shape->checkCollision(otherShape)) {
                                shape->resolveCollision(otherShape);
                                anyCollision = true;
                            }
                        }

                        if (!anyCollision) break;
                    }
                }
                shapeChanged = true;
            } else if (c_transformMode == sWidth) {
                QPointF center = worldToScreenNoRotation(shape->position());
                QPointF startWorldPos = screenToWorldNoRotation(c_dragStartPos);
                QPointF currentWorldPos = screenToWorldNoRotation(event->position());
                QPointF startLocal = shape->worldToLocal(startWorldPos, c_globalScale);
                QPointF currentLocal = shape->worldToLocal(currentWorldPos, c_globalScale);

                float deltaX = currentLocal.x() - startLocal.x();
                if (fabs(deltaX) > 0.01) {
                    float newWidth = shape->sizeWidth() + deltaX * 2.0;
                    newWidth = qMax(10.0, newWidth);
                    shape->setSizeWidth(newWidth);
                    shape->updateVertices(shape->sides(), shape->size());
                    c_dragStartPos = event->position();
                    shapeChanged = true;
                }
            } else if (c_transformMode == sHeight) {
                QPointF center = worldToScreenNoRotation(shape->position());
                QPointF startWorldPos = screenToWorldNoRotation(c_dragStartPos);
                QPointF currentWorldPos = screenToWorldNoRotation(event->position());
                QPointF startLocal = shape->worldToLocal(startWorldPos, c_globalScale);
                QPointF currentLocal = shape->worldToLocal(currentWorldPos, c_globalScale);

                float deltaY = currentLocal.y() - startLocal.y();
                if (fabs(deltaY) > 0.01) {
                    float newHeight = shape->sizeHeigth() + deltaY * 2.0;
                    newHeight = qMax(10.0, newHeight);
                    shape->setSizeHeigth(newHeight);
                    shape->updateVertices(shape->sides(), shape->size());
                    c_dragStartPos = event->position();
                    shapeChanged = true;
                }
            } else if (c_transformMode == MoveX) {
                QPointF worldDelta = screenToWorldNoRotation(event->position()) -
                                     screenToWorldNoRotation(c_dragStartPos);
                float rotationRad = shape->rotation() * PI / 180.0;
                QPointF localAxisX(cos(rotationRad), sin(rotationRad));
                float projection = (worldDelta.x() * localAxisX.x() + worldDelta.y() * localAxisX.y());

                QPointF newPos = c_dragShapeStartPos + localAxisX * projection;

                shape->setPosition(newPos);

                if (c_collisionsEnabled && shape->collisionsEnabled()) {
                    const int maxIterations = 5;
                    for (int iter = 0; iter < maxIterations; ++iter) {
                        bool anyCollision = false;

                        for (Shape &otherShape : c_shapes) {
                            if (otherShape.id() == shape->id()) continue;
                            if (!otherShape.isVisible() || !otherShape.collisionsEnabled()) continue;

                            if (shape->checkCollision(otherShape)) {
                                shape->resolveCollision(otherShape);
                                anyCollision = true;
                            }
                        }

                        if (!anyCollision) break;
                    }
                }
                shapeChanged = true;
            }
            else if (c_transformMode == MoveY) {
                QPointF worldDelta = screenToWorldNoRotation(event->position()) -
                                     screenToWorldNoRotation(c_dragStartPos);
                float rotationRad = shape->rotation() * PI / 180.0;
                QPointF localAxisY(-sin(rotationRad), cos(rotationRad));
                float projection = (worldDelta.x() * localAxisY.x() + worldDelta.y() * localAxisY.y());

                QPointF newPos = c_dragShapeStartPos + localAxisY * projection;

                shape->setPosition(newPos);

                if (c_collisionsEnabled && shape->collisionsEnabled()) {
                    const int maxIterations = 5;
                    for (int iter = 0; iter < maxIterations; ++iter) {
                        bool anyCollision = false;

                        for (Shape &otherShape : c_shapes) {
                            if (otherShape.id() == shape->id()) continue;
                            if (!otherShape.isVisible() || !otherShape.collisionsEnabled()) continue;

                            if (shape->checkCollision(otherShape)) {
                                shape->resolveCollision(otherShape);
                                anyCollision = true;
                            }
                        }

                        if (!anyCollision) break;
                    }
                }
                shapeChanged = true;
            }

            if (shapeChanged) {
                emit shapeUpdated(c_draggingShapeId);
                update();
            }
        }
        event->accept();
    } else if (c_dragMode == DragVertex && c_draggingShapeId != -1 && c_draggingVertexIndex != -1) {
        Shape *shape = getShapeById(c_draggingShapeId);
        if (shape && c_draggingVertexIndex >= 0 && c_draggingVertexIndex < shape->vertices().size()) {

            QPointF worldPos = screenToWorldNoRotation(event->position());
            QPointF localPos = shape->worldToLocal(worldPos, c_globalScale);
            shape->setVertex(c_draggingVertexIndex, localPos);

            if (c_collisionsEnabled && shape->collisionsEnabled()) {
                const int maxIterations = 5;
                for (int iter = 0; iter < maxIterations; ++iter) {
                    bool anyCollision = false;

                    for (Shape &otherShape : c_shapes) {
                        if (otherShape.id() == shape->id()) continue;
                        if (!otherShape.isVisible() || !otherShape.collisionsEnabled()) continue;

                        if (shape->checkCollision(otherShape)) {
                            shape->resolveCollision(otherShape);
                            anyCollision = true;
                        }
                    }

                    if (!anyCollision) break;
                }
            }

            if (shape->vertices()[c_draggingVertexIndex] != c_dragVertexStartPos) {
                emit vertexMoved(shape->id(), c_draggingVertexIndex);
                emit shapeUpdated(shape->id());
                update();
            }
        }
        event->accept();
    } else if (c_dragMode == DragEdge && c_draggingShapeId != -1 && c_draggingEdgeIndex != -1) {
        Shape *shape = getShapeById(c_draggingShapeId);
        if (shape && c_draggingEdgeIndex >= 0 && !c_dragEdgeVertices.isEmpty()) {
            QPointF worldDelta = screenToWorldNoRotation(event->position()) -
                                 screenToWorldNoRotation(c_dragStartPos);

            QPointF localDelta = shape->worldToLocal(worldDelta, c_globalScale);
            int vertexCount = shape->vertices().size();
            int nextIndex = (c_draggingEdgeIndex + 1) % vertexCount;

            QVector<QPointF> vertices = shape->vertices();
            vertices[c_draggingEdgeIndex] = c_dragEdgeVertices[0] + localDelta;
            vertices[nextIndex] = c_dragEdgeVertices[1] + localDelta;

            shape->setVertices(vertices);

            if (c_collisionsEnabled && shape->collisionsEnabled()) {
                const int maxIterations = 5;
                for (int iter = 0; iter < maxIterations; ++iter) {
                    bool anyCollision = false;

                    for (Shape &otherShape : c_shapes) {
                        if (otherShape.id() == shape->id()) continue;
                        if (!otherShape.isVisible() || !otherShape.collisionsEnabled()) continue;

                        if (shape->checkCollision(otherShape)) {
                            shape->resolveCollision(otherShape);
                            anyCollision = true;
                        }
                    }

                    if (!anyCollision) break;
                }
            }

            emit vertexMoved(shape->id(), c_draggingEdgeIndex);
            emit vertexMoved(shape->id(), nextIndex);
            emit shapeUpdated(shape->id());
            update();
        }
        event->accept();
    } else {
        event->ignore();
    }
}

void VKCanvas::mousePressEvent(QMouseEvent *event)
{
    c_lastMousePos = event->position();
    c_dragStartPos = c_lastMousePos;

    if (event->button() == Qt::RightButton) {
        c_dragMode = PanCanvas;
        setCursor(QCursor(Qt::ClosedHandCursor));
        event->accept();
    } else if (event->button() == Qt::LeftButton) {
        if (c_activeTab == 1) {
            bool vertexClicked = false;
            bool edgeClicked = false;

            if (c_selectedShapeId != -1 && c_activeTab == 1) {
                Shape *shape = getShapeById(c_selectedShapeId);
                if (shape) {
                    float searchRadius = 20.0; // Увеличиваем радиус поиска
                    int vertexIndex = -1;
                    int edgeIndex = -1;

                    // Ищем ближайшую вершину
                    findClosestVertex(*shape, event->position(), vertexIndex, searchRadius);
                    // Ищем ближайшее ребро
                    findClosestEdge(*shape, event->position(), edgeIndex, searchRadius);

                    // Сначала проверяем вершину (приоритет выше)
                    if (vertexIndex != -1) {
                        c_dragMode = DragVertex;
                        c_draggingShapeId = c_selectedShapeId;
                        c_draggingVertexIndex = vertexIndex;
                        c_dragVertexStartPos = shape->vertices()[vertexIndex];

                        // Устанавливаем выбранную вершину
                        setSelectedVertexIndex(vertexIndex);

                        setCursor(QCursor(Qt::SizeAllCursor));
                        event->accept();
                        return;
                    }
                    // Затем проверяем ребро
                    else if (edgeIndex != -1) {
                        c_dragMode = DragEdge;
                        c_draggingShapeId = c_selectedShapeId;
                        c_draggingEdgeIndex = edgeIndex;
                        c_dragEdgeVertices.clear();

                        int vertexCount = shape->vertices().size();
                        int nextIndex = (edgeIndex + 1) % vertexCount;
                        c_dragEdgeVertices.append(shape->vertices()[edgeIndex]);
                        c_dragEdgeVertices.append(shape->vertices()[nextIndex]);

                        // Устанавливаем выбранное ребро
                        setSelectedEdgeIndex(edgeIndex);

                        setCursor(QCursor(Qt::SizeAllCursor));
                        event->accept();
                        return;
                    }
                }
            }

            if (!vertexClicked && !edgeClicked) {
                int shapeId = findShapeAtPoint(event->position());
                if (shapeId != -1) {
                    c_dragMode = DragShape;
                    c_draggingShapeId = shapeId;
                    Shape *shape = getShapeById(shapeId);
                    if (shape) {
                        c_dragShapeStartPos = shape->position();
                        c_transformMode = Move;
                        if (shapeId != c_selectedShapeId) {
                            setSelectedShapeId(shapeId);
                        }
                    }
                    setCursor(QCursor(Qt::SizeAllCursor));
                } else {
                    setSelectedShapeId(-1);
                    c_dragMode = NoDrag;
                    setCursor(QCursor(Qt::ArrowCursor));
                }
            }

            if (c_dragMode == DragVertex || c_dragMode == DragEdge ||
                (c_dragMode == DragShape && c_transformMode == Move)) {
                if (!m_dragging) {
                    m_dragging = true;
                    emit draggingChanged();
                }
            }

            event->accept();
        } else {
            bool clickedOnTransformHandle = false;

            if (c_selectedShapeId != -1) {
                Shape *shape = getShapeById(c_selectedShapeId);
                if (shape) {
                    QPointF center = worldToScreenNoRotation(shape->position());
                    float sizeHeightShape = shape->sizeHeigth() * shape->scale() * c_globalScale;
                    float sizeWidthShape = shape->sizeWidth() * shape->scale() * c_globalScale;

                    float rotation = shape->rotation();
                    QTransform transform;
                    transform.rotate(rotation);
                    float maxSize = qMax(sizeWidthShape, sizeHeightShape);
                    float ringRadius = maxSize + 40.0;
                    float ringThickness = 3.0;

                    float distance = QLineF(event->position(), center).length();
                    if (distance >= ringRadius - ringThickness * 2 && distance <= ringRadius + ringThickness * 2) {
                        c_dragMode = DragShape;
                        c_draggingShapeId = c_selectedShapeId;
                        c_dragShapeStartPos = shape->position();
                        c_transformMode = Rotate;
                        setCursor(QCursor(Qt::SizeAllCursor));
                        clickedOnTransformHandle = true;

                        if (!m_dragging) {
                            m_dragging = true;
                            emit draggingChanged();
                        }

                        event->accept();
                        return;
                    }
                    QPointF scaleHandleLocal = QPointF(sizeWidthShape + 20, 0);
                    QPointF scaleHandle = center + transform.map(scaleHandleLocal);
                    if (QLineF(event->position(), scaleHandle).length() < 10) {
                        c_dragMode = DragShape;
                        c_draggingShapeId = c_selectedShapeId;
                        c_dragShapeStartPos = shape->position();
                        c_transformMode = Scale;
                        setCursor(Qt::SizeHorCursor);
                        clickedOnTransformHandle = true;

                        if (!m_dragging) {
                            m_dragging = true;
                            emit draggingChanged();
                        }

                        event->accept();
                        return;
                    }
                    QPointF sizeWidthLocal = QPointF(0, sizeHeightShape + 20);
                    QPointF sizeWidthPoint = center + transform.map(sizeWidthLocal);
                    if (QLineF(event->position(), sizeWidthPoint).length() < 10) {
                        c_dragMode = DragShape;
                        c_draggingShapeId = c_selectedShapeId;
                        c_dragShapeStartPos = shape->position();
                        c_transformMode = sWidth;
                        setCursor(Qt::SizeHorCursor);
                        clickedOnTransformHandle = true;

                        if (!m_dragging) {
                            m_dragging = true;
                            emit draggingChanged();
                        }

                        event->accept();
                        return;
                    }
                    QPointF sizeHeightLocal = QPointF(-sizeWidthShape - 20, 0);
                    QPointF sizeHeightPoint = center + transform.map(sizeHeightLocal);
                    if (QLineF(event->position(), sizeHeightPoint).length() < 10) {
                        c_dragMode = DragShape;
                        c_draggingShapeId = c_selectedShapeId;
                        c_dragShapeStartPos = shape->position();
                        c_transformMode = sHeight;
                        setCursor(Qt::SizeHorCursor);
                        clickedOnTransformHandle = true;

                        if (!m_dragging) {
                            m_dragging = true;
                            emit draggingChanged();
                        }

                        event->accept();
                        return;
                    }
                    QPointF moveXLocal = QPointF(ringRadius + 20, 0);
                    QPointF moveXPoint = center + transform.map(moveXLocal);
                    if (QLineF(event->position(), moveXPoint).length() < 10) {
                        c_dragMode = DragShape;
                        c_draggingShapeId = c_selectedShapeId;
                        c_dragShapeStartPos = shape->position();
                        c_transformMode = MoveX;
                        setCursor(Qt::SizeHorCursor);
                        clickedOnTransformHandle = true;

                        if (!m_dragging) {
                            m_dragging = true;
                            emit draggingChanged();
                        }

                        event->accept();
                        return;
                    }
                    QPointF moveYLocal = QPointF(0, -ringRadius - 20);
                    QPointF moveYPoint = center + transform.map(moveYLocal);
                    if (QLineF(event->position(), moveYPoint).length() < 10) {
                        c_dragMode = DragShape;
                        c_draggingShapeId = c_selectedShapeId;
                        c_dragShapeStartPos = shape->position();
                        c_transformMode = MoveY;
                        setCursor(Qt::SizeVerCursor);
                        clickedOnTransformHandle = true;

                        if (!m_dragging) {
                            m_dragging = true;
                            emit draggingChanged();
                        }

                        event->accept();
                        return;
                    }
                }
            }

            int shapeId = findShapeAtPoint(event->position());
            if (shapeId != -1) {
                c_dragMode = DragShape;
                c_draggingShapeId = shapeId;
                Shape *shape = getShapeById(shapeId);
                if (shape) {
                    c_dragShapeStartPos = shape->position();
                    c_transformMode = Move;
                    if (shapeId != c_selectedShapeId) {
                        setSelectedShapeId(shapeId);
                    }
                }
                setCursor(QCursor(Qt::SizeAllCursor));

                if (!m_dragging) {
                    m_dragging = true;
                    emit draggingChanged();
                }

                event->accept();
            } else {
                setSelectedShapeId(-1);
                c_dragMode = NoDrag;
                setCursor(QCursor(Qt::ArrowCursor));
                event->accept();
            }
        }
    } else if (event->button() == Qt::MiddleButton) {
        resetView();
        event->accept();
    }
}

void VKCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::RightButton && c_dragMode == PanCanvas) ||
        (event->button() == Qt::LeftButton && (c_dragMode == DragShape || c_dragMode == DragVertex || c_dragMode == DragEdge))) {
        if (c_dragMode == DragVertex || c_dragMode == DragEdge || c_dragMode == DragShape) {
            emit vertexInfoUpdated();
        }

        c_dragMode = NoDrag;
        c_draggingShapeId = -1;
        c_draggingVertexIndex = -1;
        c_draggingEdgeIndex = -1;
        c_transformMode = NoTransform;
        c_dragEdgeVertices.clear();
        if (m_dragging) {
            m_dragging = false;
            emit draggingChanged();
        }

        QTimer::singleShot(10, this, [this]() {
            if (c_dragMode == NoDrag) {
                QHoverEvent hoverEvent(QEvent::HoverMove, mapFromGlobal(QCursor::pos()),
                                       mapFromGlobal(QCursor::pos()));
                hoverMoveEvent(&hoverEvent);
            }
        });

        event->accept();
    } else if (event->button() == Qt::LeftButton && c_dragMode == NoDrag) {
        event->accept();
    } else {
        event->ignore();
    }
}

QSGNode *VKCanvas::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    QSGNode *rootNode = node;

    if (!rootNode) {
        rootNode = new QSGNode();
        QSGGeometryNode *gridNode = new QSGGeometryNode();
        QSGGeometry *gridGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        QSGFlatColorMaterial *gridMaterial = new QSGFlatColorMaterial();
        gridNode->setGeometry(gridGeometry);
        gridNode->setMaterial(gridMaterial);
        gridNode->setFlag(QSGNode::OwnsGeometry);
        gridNode->setFlag(QSGNode::OwnsMaterial);
        rootNode->appendChildNode(gridNode);
        QSGGeometryNode *axisXNode = new QSGGeometryNode();
        QSGGeometry *axisXGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        QSGFlatColorMaterial *axisXMaterial = new QSGFlatColorMaterial();
        axisXNode->setGeometry(axisXGeometry);
        axisXNode->setMaterial(axisXMaterial);
        axisXNode->setFlag(QSGNode::OwnsGeometry);
        axisXNode->setFlag(QSGNode::OwnsMaterial);
        rootNode->appendChildNode(axisXNode);
        QSGGeometryNode *axisYNode = new QSGGeometryNode();
        QSGGeometry *axisYGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        QSGFlatColorMaterial *axisYMaterial = new QSGFlatColorMaterial();
        axisYNode->setGeometry(axisYGeometry);
        axisYNode->setMaterial(axisYMaterial);
        axisYNode->setFlag(QSGNode::OwnsGeometry);
        axisYNode->setFlag(QSGNode::OwnsMaterial);
        rootNode->appendChildNode(axisYNode);
    } else {
        while (rootNode->childCount() > 3) {
            delete rootNode->lastChild();
        }
    }

    if (!c_initialized && width() > 0 && height() > 0) {
        centerOnZero();
        c_initialized = true;
    }
    QSGGeometryNode *gridNode = static_cast<QSGGeometryNode *>(rootNode->childAtIndex(0));
    QSGGeometryNode *axisXNode = static_cast<QSGGeometryNode *>(rootNode->childAtIndex(1));
    QSGGeometryNode *axisYNode = static_cast<QSGGeometryNode *>(rootNode->childAtIndex(2));

    updateGridGeometry(gridNode->geometry(),
                       static_cast<QSGFlatColorMaterial *>(gridNode->material()));
    updateAxisXGeometry(axisXNode->geometry(),
                        static_cast<QSGFlatColorMaterial *>(axisXNode->material()));
    updateAxisYGeometry(axisYNode->geometry(),
                        static_cast<QSGFlatColorMaterial *>(axisYNode->material()));

    for (const Shape &shape : c_shapes) {
        QSGGeometryNode *shapeNode = createShapeNode();
        updateShapeGeometry(shapeNode, shape);
        rootNode->appendChildNode(shapeNode);
        if (shape.id() == c_selectedShapeId) {
            if (c_activeTab == 1) {
                int vertexCount = shape.vertices().size();
                for (int i = 0; i < vertexCount; ++i) {
                    int nextI = (i + 1) % vertexCount;
                    QPointF p1 = vertexToScreen(shape.id(), i);
                    QPointF p2 = vertexToScreen(shape.id(), nextI);

                    QColor edgeColor;
                    if (i == c_selectedEdgeIndex) {
                        edgeColor = QColor(255, 255, 0, 180);
                    } else {
                        edgeColor = QColor(255, 255, 255, 120);
                    }

                    QSGGeometryNode *edgeNode = createEdgeNode(p1, p2, edgeColor);
                    rootNode->appendChildNode(edgeNode);
                }
                for (int i = 0; i < vertexCount; ++i) {
                    QPointF vertexScreenPos = vertexToScreen(shape.id(), i);
                    QColor vertexColor;

                    if (i == c_selectedVertexIndex) {
                        vertexColor = QColor(255, 0, 0);
                    } else {
                        vertexColor = QColor(255, 255, 255);
                    }

                    QSGGeometryNode *vertexNode = createVertexNode(vertexScreenPos, vertexColor);
                    rootNode->appendChildNode(vertexNode);
                }
            } else {
                QPointF center = worldToScreenNoRotation(shape.position());
                float sizeHeightShape = shape.sizeHeigth() * shape.scale() * c_globalScale;
                float sizeWidthShape = shape.sizeWidth() * shape.scale() * c_globalScale;
                float rotation = shape.rotation();
                QTransform transform;
                transform.rotate(rotation);
                float maxSize = qMax(sizeWidthShape, sizeHeightShape);
                float ringRadius = maxSize + 40.0;
                const int ringSegments = 64;
                QSGGeometryNode *ringNode = new QSGGeometryNode();
                QSGGeometry *ringGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), ringSegments * 2);
                QSGFlatColorMaterial *ringMaterial = new QSGFlatColorMaterial();
                ringGeometry->setDrawingMode(QSGGeometry::DrawLines);
                ringGeometry->setLineWidth(3.0);
                ringNode->setGeometry(ringGeometry);
                ringNode->setMaterial(ringMaterial);
                ringNode->setFlag(QSGNode::OwnsGeometry);
                ringNode->setFlag(QSGNode::OwnsMaterial);

                QSGGeometry::Point2D *ringVertices = ringGeometry->vertexDataAsPoint2D();
                for (int i = 0; i < ringSegments; ++i) {
                    float angle1 = 2.0 * PI * i / ringSegments;
                    float angle2 = 2.0 * PI * (i + 1) / ringSegments;

                    QPointF p1(cos(angle1) * ringRadius, sin(angle1) * ringRadius);
                    QPointF p2(cos(angle2) * ringRadius, sin(angle2) * ringRadius);

                    p1 += center;
                    p2 += center;

                    ringVertices[i * 2].set(p1.x(), p1.y());
                    ringVertices[i * 2 + 1].set(p2.x(), p2.y());
                }
                ringMaterial->setColor(QColor(0, 200, 0, 150));
                rootNode->appendChildNode(ringNode);
                QPointF scaleHandleLocal = QPointF(sizeWidthShape + 20, 0);
                QPointF scaleHandle = center + transform.map(scaleHandleLocal);
                QSGGeometryNode *scaleNode = createTransformHandle(scaleHandle, QColor(200, 200, 200));
                rootNode->appendChildNode(scaleNode);
                QPointF sizeWidthLocal = QPointF(0, sizeHeightShape + 20);
                QPointF sizeWidthPoint = center + transform.map(sizeWidthLocal);
                QSGGeometryNode *sizeWidthNode = createTransformHandle(sizeWidthPoint, QColor(0, 0, 200));
                rootNode->appendChildNode(sizeWidthNode);
                QPointF sizeHeightLocal = QPointF(-sizeWidthShape - 20, 0);
                QPointF sizeHeightPoint = center + transform.map(sizeHeightLocal);
                QSGGeometryNode *sizeHeightNode = createTransformHandle(sizeHeightPoint, QColor(200, 0, 0));
                rootNode->appendChildNode(sizeHeightNode);
                QPointF moveXLocal = QPointF(ringRadius + 20, 0);
                QPointF moveXPoint = center + transform.map(moveXLocal);
                QSGGeometryNode *moveXNode = new QSGGeometryNode();
                QSGGeometry *moveXGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 3);
                QSGFlatColorMaterial *moveXMaterial = new QSGFlatColorMaterial();
                moveXGeometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
                moveXNode->setGeometry(moveXGeometry);
                moveXNode->setMaterial(moveXMaterial);
                moveXNode->setFlag(QSGNode::OwnsGeometry);
                moveXNode->setFlag(QSGNode::OwnsMaterial);

                QSGGeometry::Point2D *moveXVertices = moveXGeometry->vertexDataAsPoint2D();
                const float triangleSize = 10.0;
                QPointF dirX = transform.map(QPointF(1, 0));
                QPointF perpX = transform.map(QPointF(0, 1));
                moveXVertices[0].set(moveXPoint.x(), moveXPoint.y());
                moveXVertices[1].set(moveXPoint.x() - dirX.x() * triangleSize + perpX.x() * triangleSize/2,
                                     moveXPoint.y() - dirX.y() * triangleSize + perpX.y() * triangleSize/2);
                moveXVertices[2].set(moveXPoint.x() - dirX.x() * triangleSize - perpX.x() * triangleSize/2,
                                     moveXPoint.y() - dirX.y() * triangleSize - perpX.y() * triangleSize/2);
                moveXMaterial->setColor(QColor(255, 0, 0));
                rootNode->appendChildNode(moveXNode);
                QPointF moveYLocal = QPointF(0, -ringRadius - 20);
                QPointF moveYPoint = center + transform.map(moveYLocal);
                QSGGeometryNode *moveYNode = new QSGGeometryNode();
                QSGGeometry *moveYGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 3);
                QSGFlatColorMaterial *moveYMaterial = new QSGFlatColorMaterial();
                moveYGeometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
                moveYNode->setGeometry(moveYGeometry);
                moveYNode->setMaterial(moveYMaterial);
                moveYNode->setFlag(QSGNode::OwnsGeometry);
                moveYNode->setFlag(QSGNode::OwnsMaterial);

                QSGGeometry::Point2D *moveYVertices = moveYGeometry->vertexDataAsPoint2D();
                QPointF dirY = transform.map(QPointF(0, 1));
                QPointF perpY = transform.map(QPointF(1, 0));
                moveYVertices[0].set(moveYPoint.x(), moveYPoint.y());
                moveYVertices[1].set(moveYPoint.x() + dirY.x() * triangleSize + perpY.x() * triangleSize/2,
                                     moveYPoint.y() + dirY.y() * triangleSize + perpY.y() * triangleSize/2);
                moveYVertices[2].set(moveYPoint.x() + dirY.x() * triangleSize - perpY.x() * triangleSize/2,
                                     moveYPoint.y() + dirY.y() * triangleSize - perpY.y() * triangleSize/2);
                moveYMaterial->setColor(QColor(0, 0, 255));
                rootNode->appendChildNode(moveYNode);
            }
        }
    }

    return rootNode;
}

void VKCanvas::hoverMoveEvent(QHoverEvent *event)
{
    if (c_dragMode == NoDrag) {
        if (c_activeTab == 1) {
            // Режим редактирования
            if (c_selectedShapeId != -1) {
                Shape *shape = getShapeById(c_selectedShapeId);
                if (shape) {
                    float searchRadius = 20.0;
                    int vertexIndex = -1;
                    int edgeIndex = -1;

                    findClosestVertex(*shape, event->position(), vertexIndex, searchRadius);
                    findClosestEdge(*shape, event->position(), edgeIndex, searchRadius);

                    if (vertexIndex != -1) {
                        setCursor(QCursor(Qt::CrossCursor));
                        return;
                    } else if (edgeIndex != -1) {
                        setCursor(QCursor(Qt::PointingHandCursor));
                        return;
                    }
                }
            }

            int shapeId = findShapeAtPoint(event->position());
            if (shapeId != -1) {
                setCursor(QCursor(Qt::PointingHandCursor));
            } else {
                setCursor(QCursor(Qt::ArrowCursor));
            }
        } else {
            if (c_selectedShapeId != -1) {
                Shape *shape = getShapeById(c_selectedShapeId);
                if (shape) {
                    QPointF center = worldToScreenNoRotation(shape->position());
                    float sizeHeightShape = shape->sizeHeigth() * shape->scale() * c_globalScale;
                    float sizeWidthShape = shape->sizeWidth() * shape->scale() * c_globalScale;

                    float rotation = shape->rotation();
                    QTransform transform;
                    transform.rotate(rotation);
                    float maxSize = qMax(sizeWidthShape, sizeHeightShape);
                    float ringRadius = maxSize + 40.0;
                    float ringThickness = 3.0;

                    float distance = QLineF(event->position(), center).length();
                    if (distance >= ringRadius - ringThickness * 2 && distance <= ringRadius + ringThickness * 2) {
                        setCursor(QCursor(Qt::SizeAllCursor));
                        return;
                    }
                    QPointF scaleHandleLocal = QPointF(sizeWidthShape + 20, 0);
                    QPointF scaleHandle = center + transform.map(scaleHandleLocal);
                    if (QLineF(event->position(), scaleHandle).length() < 10) {
                        setCursor(Qt::CrossCursor);
                        return;
                    }
                    QPointF sizeWidthLocal = QPointF(0, sizeHeightShape + 20);
                    QPointF sizeWidthPoint = center + transform.map(sizeWidthLocal);
                    if (QLineF(event->position(), sizeWidthPoint).length() < 10) {
                        setCursor(Qt::SizeVerCursor);
                        return;
                    }
                    QPointF sizeHeightLocal = QPointF(-sizeWidthShape - 20, 0);
                    QPointF sizeHeightPoint = center + transform.map(sizeHeightLocal);
                    if (QLineF(event->position(), sizeHeightPoint).length() < 10) {
                        setCursor(Qt::SizeHorCursor);
                        return;
                    }
                    QPointF moveXLocal = QPointF(ringRadius + 20, 0);
                    QPointF moveXPoint = center + transform.map(moveXLocal);
                    if (QLineF(event->position(), moveXPoint).length() < 10) {
                        setCursor(Qt::SizeAllCursor);
                        return;
                    }
                    QPointF moveYLocal = QPointF(0, -ringRadius - 20);
                    QPointF moveYPoint = center + transform.map(moveYLocal);
                    if (QLineF(event->position(), moveYPoint).length() < 10) {
                        setCursor(Qt::SizeAllCursor);
                        return;
                    }
                }
            }

            int shapeId = findShapeAtPoint(event->position());
            if (shapeId != -1) {
                setCursor(QCursor(Qt::PointingHandCursor));
            } else {
                setCursor(QCursor(Qt::ArrowCursor));
            }
        }
    }
}

void VKCanvas::wheelEvent(QWheelEvent *event)
{
    QPointF angleDelta = event->angleDelta();

    if (!angleDelta.isNull()) {
        float zoomFactor = 1.1;
        float scaleChange = 1.0;

        if (angleDelta.y() > 0) {
            scaleChange = zoomFactor;
        } else if (angleDelta.y() < 0) {
            scaleChange = 1.0 / zoomFactor;
        }

        QPointF mousePos = event->position();
        QPointF worldBefore = screenToWorldNoRotation(mousePos);

        c_globalScale *= scaleChange;
        c_globalScale = qBound(0.1, c_globalScale, 100.0);

        QPointF worldAfter = screenToWorldNoRotation(mousePos);
        c_offsetX += (worldAfter.x() - worldBefore.x()) * c_globalScale;
        c_offsetY += (worldAfter.y() - worldBefore.y()) * c_globalScale;

        emit offsetChanged();
        emit globalScaleChanged();
        update();
        event->accept();
    } else {
        event->ignore();
    }
}

void VKCanvas::requestVertexInfoUpdate()
{
    emit vertexInfoUpdated();
}

void VKCanvas::setSelectedVertexIndex(int index)
{
    if (c_selectedVertexIndex == index)
        return;
    if (c_selectedShapeId != -1) {
        Shape* shape = getShapeById(c_selectedShapeId);
        if (shape) {
            if (index >= -1 && index < shape->vertices().size()) {
                c_selectedVertexIndex = index;
                c_selectedEdgeIndex = -1;
                emit selectedVertexIndexChanged();
                emit selectedEdgeIndexChanged();
                emit vertexInfoUpdated();
                update();
                return;
            }
        }
    }
    if (c_selectedVertexIndex != -1) {
        c_selectedVertexIndex = -1;
        emit selectedVertexIndexChanged();
        emit vertexInfoUpdated();
        update();
    }
}

QPointF VKCanvas::getShapePosition(int id)
{
    const Shape* shape = getShapeById(id);
    return shape ? shape->position() : QPointF();
}

void VKCanvas::setShapePosition(int id, float x, float y)
{
    Shape* shape = getShapeById(id);
    if (shape) {
        shape->setPosition(QPointF(x, y));

        if (c_collisionsEnabled && shape->collisionsEnabled()) {
            const int maxIterations = 5;
            for (int iter = 0; iter < maxIterations; ++iter) {
                bool anyCollision = false;

                for (Shape &otherShape : c_shapes) {
                    if (otherShape.id() == shape->id()) continue;
                    if (!otherShape.isVisible() || !otherShape.collisionsEnabled()) continue;

                    if (shape->checkCollision(otherShape)) {
                        shape->resolveCollision(otherShape);
                        anyCollision = true;
                    }
                }

                if (!anyCollision) break;
            }
        }

        emit shapeUpdated(id);
        emit vertexInfoUpdated();
        update();
    }
}

void VKCanvas::setSelectedEdgeIndex(int index)
{
    if (c_selectedEdgeIndex == index)
        return;

    if (c_selectedShapeId != -1) {
        Shape* shape = getShapeById(c_selectedShapeId);
        if (shape) {
            int vertexCount = shape->vertices().size();
            if (index >= -1 && index < vertexCount) {
                c_selectedEdgeIndex = index;
                c_selectedVertexIndex = -1;
                emit selectedEdgeIndexChanged();
                emit selectedVertexIndexChanged();
                emit vertexInfoUpdated();
                update();
                return;
            }
        }
    }

    if (c_selectedEdgeIndex != -1) {
        c_selectedEdgeIndex = -1;
        emit selectedEdgeIndexChanged();
        emit vertexInfoUpdated();
        update();
    }
}
void VKCanvas::setBlockTableUpdates(bool block)
{
    if (m_blockTableUpdates != block) {
        m_blockTableUpdates = block;
        if (!block) {
            emit vertexInfoUpdated();
        }
    }
}
