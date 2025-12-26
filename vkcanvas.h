#ifndef VKCANVAS_H
#define VKCANVAS_H

#include <QQuickItem>
#include <QVector>
#include <qsgflatcolormaterial.h>
#include <qsgnode.h>
#include "shape.h"

class VKCanvas : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool dragging READ isDragging NOTIFY draggingChanged)
    Q_PROPERTY(float offsetX READ offsetX NOTIFY offsetChanged)
    Q_PROPERTY(float offsetY READ offsetY NOTIFY offsetChanged)
    Q_PROPERTY(float globalScale READ globalScale NOTIFY globalScaleChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    Q_PROPERTY(bool collisionsEnabled READ collisionsEnabled WRITE setCollisionsEnabled NOTIFY collisionsEnabledChanged)
    Q_PROPERTY(int activeTab READ activeTab WRITE setActiveTab NOTIFY activeTabChanged)
    Q_PROPERTY(int selectedShapeId READ selectedShapeId WRITE setSelectedShapeId NOTIFY selectedShapeIdChanged)
    Q_PROPERTY(int selectedVertexIndex READ selectedVertexIndex WRITE setSelectedVertexIndex NOTIFY selectedVertexIndexChanged)
    Q_PROPERTY(int selectedEdgeIndex READ selectedEdgeIndex WRITE setSelectedEdgeIndex NOTIFY selectedEdgeIndexChanged)
    Q_PROPERTY(int shapeCount READ shapeCount NOTIFY shapeCountChanged)

public:
    explicit VKCanvas(QQuickItem *parent = nullptr);
    ~VKCanvas();
    bool isDragging() const { return m_dragging; }

    float offsetX() const { return c_offsetX; }
    float offsetY() const { return c_offsetY; }
    float globalScale() const { return c_globalScale; }
    bool showGrid() const { return c_showGrid; }
    bool collisionsEnabled() const { return c_collisionsEnabled; }
    int activeTab() const { return c_activeTab; }
    int selectedShapeId() const { return c_selectedShapeId; }
    int selectedVertexIndex() const { return c_selectedVertexIndex; }
    int selectedEdgeIndex() const { return c_selectedEdgeIndex; }
    int shapeCount() const { return c_shapes.size(); }

    Q_INVOKABLE void centerOnZero();
    Q_INVOKABLE void resetView();
    void setShowGrid(bool show);
    void setCollisionsEnabled(bool enabled);
    void setActiveTab(int tab);
    void setSelectedShapeId(int id);
    void setSelectedVertexIndex(int index);
    void setSelectedEdgeIndex(int index);
    Q_INVOKABLE int addShapeWithSides(float x, float y, int sides, float sizeWidth, float sizeHeight);
    Q_INVOKABLE int addTriangle(float x, float y, float sizeWidth, float sizeHeight);
    Q_INVOKABLE int addSquare(float x, float y, float sizeWidth, float sizeHeight);
    Q_INVOKABLE int addPentagon(float x, float y, float sizeWidth, float sizeHeight);
    Q_INVOKABLE int addHexagon(float x, float y, float sizeWidth, float sizeHeight);
    Q_INVOKABLE int addHeptagon(float x, float y, float sizeWidth, float sizeHeight);
    Q_INVOKABLE int addOctagon(float x, float y, float sizeWidth, float sizeHeight);
    Q_INVOKABLE void removeShape(int id);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void setShapeRotation(int id, float rotation);
    Q_INVOKABLE void setShapeScale(int id, float scale);
    Q_INVOKABLE void setShapeColor(int id, const QColor &color);
    Q_INVOKABLE void setShapeSides(int id, int sides);
    Q_INVOKABLE void setShapeSizeHeight(int id, float sizeHeight);
    Q_INVOKABLE void setShapeSizeWidgth(int id, float sizeWidght);
    Q_INVOKABLE void setShapePosition(int id, float x, float y);
    Q_INVOKABLE void setShapeVertex(int id, int vertexIndex, float x, float y);
    Q_INVOKABLE void setEdgeLength(int shapeId, int edgeIndex, float newLength);
    Q_INVOKABLE void setShapeName(int id, const QString &name);
    Q_INVOKABLE void setShapeCollisionsEnabled(int id, bool enabled);
    Q_INVOKABLE void addVertexToShape(int id, float x, float y);
    Q_INVOKABLE void removeVertexFromShape(int id, int vertexIndex);
    Q_INVOKABLE void resetShapeVertices(int id);
    Q_INVOKABLE float getShapeRotation(int id);
    Q_INVOKABLE float getShapeScale(int id);
    Q_INVOKABLE QColor getShapeColor(int id);
    Q_INVOKABLE int getShapeSides(int id);
    Q_INVOKABLE float getShapeSize(int id);
    Q_INVOKABLE float getShapeSizeWidth(int id);
    Q_INVOKABLE float getShapeSizeHeight(int id);
    Q_INVOKABLE QPointF getShapePosition(int id);
    Q_INVOKABLE QPointF getShapeVertex(int id, int vertexIndex);
    Q_INVOKABLE QString getShapeName(int id);
    Q_INVOKABLE bool getShapeCollisionsEnabled(int id);
    Q_INVOKABLE int getShapeIdByIndex(int index);
    Q_INVOKABLE QPointF vertexToScreen(int shapeId, int vertexIndex) const;
    Q_INVOKABLE QPointF vertexToWorld(int shapeId, int vertexIndex) const;
    Q_INVOKABLE float edgeLength(int shapeId, int edgeIndex) const;
    Q_INVOKABLE int getShapeVertexCount(int id) const;
    Q_INVOKABLE QPointF screenToWorld(const QPointF &screenPos);
    Q_INVOKABLE QPointF worldToScreen(const QPointF &worldPos);
    Q_INVOKABLE QPointF screenToVertexLocal(int shapeId, const QPointF &screenPos) const;
    Q_INVOKABLE void requestVertexInfoUpdate();
    Q_INVOKABLE void addShapeAndSelect(float x, float y, int sides, float sizeWidth, float sizeHeight);
    Q_INVOKABLE QPointF getShapeVertexWorld(int shapeId, int vertexIndex) const;
    Q_INVOKABLE QPointF worldToLocal(int shapeId, const QPointF &worldPos) const;

signals:
    void draggingChanged();
    void offsetChanged();
    void globalScaleChanged();
    void showGridChanged();
    void collisionsEnabledChanged();
    void activeTabChanged();
    void selectedShapeIdChanged();
    void selectedVertexIndexChanged();
    void selectedEdgeIndexChanged();
    void shapeCountChanged();
    void shapeAdded(int shapeId);
    void shapeRemoved(int shapeId);
    void shapeUpdated(int shapeId);
    void vertexAdded(int shapeId, int vertexIndex);
    void vertexRemoved(int shapeId, int vertexIndex);
    void vertexMoved(int shapeId, int vertexIndex);
    void vertexInfoUpdated();
    void debugMessage(QString message);

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override;

private:
    QPointF screenToWorldNoRotation(const QPointF &screenPos) const;
    QPointF worldToScreenNoRotation(const QPointF &worldPos) const;
    QPolygonF createPolygon(const Shape &shape) const;
    float getShapeRadius(const Shape &shape) const;
    QPointF applyRotation(const QPointF &point, const QPointF &center, float rotation) const;
    QPointF applyInverseRotation(const QPointF &point, const QPointF &center, float rotation) const;
    QPointF findClosestVertex(const Shape &shape, const QPointF &screenPos, int &vertexIndex, float searchRadius) const;
    QPointF findClosestEdge(const Shape &shape, const QPointF &screenPos, int &edgeIndex, float searchRadius) const;
    int findShapeAtPoint(const QPointF &screenPos);
    bool pointInPolygon(const QPointF& point, const QPolygonF& polygon) const;
    Shape* getShapeById(int id);
    const Shape* getShapeById(int id) const;
    QSGGeometryNode* createShapeNode();
    QSGGeometryNode* createVertexNode(const QPointF &position, const QColor &color, float size = 8.0);
    QSGGeometryNode* createEdgeNode(const QPointF &start, const QPointF &end, const QColor &color, float width = 2.0);
    QSGGeometryNode* createTransformHandle(const QPointF &position, const QColor &color, float size = 10.0);
    void updateShapeGeometry(QSGGeometryNode *node, const Shape &shape);
    void updateGridGeometry(QSGGeometry *geometry, QSGFlatColorMaterial *material);
    void updateAxisXGeometry(QSGGeometry *geometry, QSGFlatColorMaterial *material);
    void updateAxisYGeometry(QSGGeometry *geometry, QSGFlatColorMaterial *material);
    void setBlockTableUpdates(bool block);
    bool m_dragging = false;

    float c_offsetX = 0;
    float c_offsetY = 0;
    float c_globalScale = 1.0;
    bool c_showGrid = true;
    bool c_collisionsEnabled = true;
    int c_activeTab = 0;
    int c_selectedShapeId = -1;
    int c_selectedVertexIndex = -1;
    int c_selectedEdgeIndex = -1;
    bool c_initialized = false;
    QVector<Shape> c_shapes;
    int c_nextShapeId = 0;
    bool m_blockTableUpdates;

    enum DragMode {
        NoDrag,
        PanCanvas,
        DragShape,
        DragVertex,
        DragEdge
    };
    DragMode c_dragMode = NoDrag;

    enum TransformMode {
        NoTransform,
        Move,
        Rotate,
        Scale,
        sWidth,
        sHeight,
        MoveX,
        MoveY
    };
    TransformMode c_transformMode = NoTransform;

    int c_draggingShapeId = -1;
    int c_draggingVertexIndex = -1;
    int c_draggingEdgeIndex = -1;
    QPointF c_lastMousePos;
    QPointF c_dragStartPos;
    QPointF c_dragShapeStartPos;
    QPointF c_dragVertexStartPos;
    QVector<QPointF> c_dragEdgeVertices;
};

#endif // VKCANVAS_H
