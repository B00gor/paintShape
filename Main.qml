import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import VKCanvas

Window {
    id: mainWindow
    width: 1200
    height: 700
    visible: true
    title: "Редактор векторной графики"

    ColorDialog {
        id: colorDialog
        title: "Выберите цвет фигуры"
        visible: false

        onAccepted: {
            if (canvas.selectedShapeId !== -1) {
                canvas.setShapeColor(canvas.selectedShapeId, colorDialog.selectedColor)
                currentColor = colorDialog.selectedColor
            }
        }
    }

    // Стилизованный компонент кнопки
    Component {
        id: styledButton

        Button {
            id: button
            height: 35
            hoverEnabled: true

            property color normalColor: "#007acc"
            property color hoverColor: "#1e90ff"
            property color pressedColor: "#005a9e"
            property color disabledColor: "#555555"
            property color textColor: "white"
            property real borderRadius: 3

            background: Rectangle {
                id: bgRect
                color: {
                    if (!button.enabled) return disabledColor
                    if (button.pressed) return pressedColor
                    if (button.hovered) return hoverColor
                    return normalColor
                }
                radius: borderRadius
                border.color: button.enabled ? Qt.darker(bgRect.color, 1.2) : "#666666"
                border.width: 1

                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }

            contentItem: Text {
                text: button.text
                color: button.enabled ? textColor : "#999999"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }

    // Стилизованный компонент ComboBox
    Component {
        id: styledComboBox

        ComboBox {
            id: combo
            height: 35
            hoverEnabled: true

            property color normalColor: "#007acc"
            property color hoverColor: "#1e90ff"
            property color pressedColor: "#005a9e"
            property color textColor: "white"
            property real borderRadius: 3

            background: Rectangle {
                id: comboBg
                color: {
                    if (combo.pressed) return pressedColor
                    if (combo.hovered) return hoverColor
                    return normalColor
                }
                radius: borderRadius
                border.color: Qt.darker(comboBg.color, 1.2)
                border.width: 1

                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }

            contentItem: Text {
                text: combo.displayText
                color: textColor
                font.pixelSize: 12
                font.bold: true
                verticalAlignment: Text.AlignVCenter
                leftPadding: 10
                rightPadding: combo.indicator.width + 10
                elide: Text.ElideRight
            }

            indicator: Canvas {
                id: arrowCanvas
                x: combo.width - width - 10
                y: combo.topPadding + (combo.availableHeight - height) / 2
                width: 12
                height: 8
                contextType: "2d"

                Connections {
                    target: combo
                    function onPressedChanged() { arrowCanvas.requestPaint() }
                    function onHoveredChanged() { arrowCanvas.requestPaint() }
                }

                onPaint: {
                    var context = getContext("2d");
                    context.reset();
                    context.strokeStyle = combo.textColor;
                    context.fillStyle = combo.textColor;
                    context.lineWidth = 1;
                    context.beginPath();
                    context.moveTo(0, 0);
                    context.lineTo(width, 0);
                    context.lineTo(width / 2, height);
                    context.closePath();
                    context.fill();
                }
            }

            popup: Popup {
                y: combo.height
                width: combo.width
                implicitHeight: Math.min(contentItem.implicitHeight, 200)
                padding: 1

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: combo.popup.visible ? combo.delegateModel : null
                    currentIndex: combo.highlightedIndex

                    ScrollIndicator.vertical: ScrollIndicator { }
                }

                background: Rectangle {
                    color: "#252526"
                    border.color: "#007acc"
                    border.width: 1
                    radius: 3
                }
            }

            delegate: ItemDelegate {
                id: delegateItem
                width: combo.width
                height: 30
                hoverEnabled: true

                background: Rectangle {
                    color: delegateItem.hovered ? "#007acc" : "transparent"
                    radius: 2
                }

                contentItem: Text {
                    text: modelData
                    color: delegateItem.hovered ? "white" : "#cccccc"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                    elide: Text.ElideRight
                }
            }
        }
    }

    Component {
        id: compactSpinBox

        Item {
            id: root
            width: parent ? parent.width : 120
            height: 24

            property real currentValue: 0
            property real minValue: -100000
            property real maxValue: 100000
            property real stepSize: 0.1
            property color textColor: "white"
            property alias text: input.text
            property bool editing: false
            property bool externalDisabled: false
            enabled: !externalDisabled

            signal valueChanged(real v)
            function clamp(v) {
                return Math.min(maxValue, Math.max(minValue, v))
            }

            function format(v) {
                return v.toFixed(5).replace(".", ",")
            }

            function parse(text) {
                if (text === "" || text === "-" || text === "," || text === ".")
                    return currentValue
                return parseFloat(text.replace(",", "."))
            }

            function applyValue(v) {
                v = clamp(v)
                currentValue = v
                valueChanged(v)
                input.text = format(v)
            }
            onCurrentValueChanged: {
                if (!editing)
                    input.text = format(currentValue)
            }

            Rectangle {
                anchors.fill: parent
                radius: 2
                border.color: "#666"
                color: "transparent"
            }

            Row {
                anchors.fill: parent
                spacing: 1
                Rectangle {
                    width: parent.width * 0.15
                    height: parent.height
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "◀"
                        color: enabled ? textColor : "#666"
                        font.pixelSize: 10
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: root.enabled && !canvas.dragging
                        propagateComposedEvents: true

                        onClicked: {
                            if (!canvas.dragging) {
                                applyValue(currentValue - stepSize)
                            } else {
                                mouse.accepted = false
                            }
                        }
                    }
                }
                Rectangle {
                    width: parent.width * 0.45
                    height: parent.height
                    color: "transparent"

                    TextField {
                        id: input
                        anchors.fill: parent
                        color: textColor
                        font.pixelSize: 10
                        selectByMouse: true
                        verticalAlignment: Text.AlignVCenter
                        enabled: root.enabled && !canvas.dragging
                        opacity: enabled ? 1.0 : 0.6
                        background: Rectangle { color: "transparent" }

                        onPressed: editing = true

                        onTextChanged: {
                            if (!editing) return
                            var v = parse(text)
                            if (!isNaN(v))
                                currentValue = clamp(v)
                        }

                        onEditingFinished: {
                            editing = false
                            applyValue(parse(text))
                        }

                        Keys.onReturnPressed: focus = false
                        Keys.onEnterPressed: focus = false
                    }
                }
                Rectangle {
                    width: parent.width * 0.25
                    height: parent.height
                    color: "transparent"

                    TextField {
                        id: stepInput
                        anchors.fill: parent
                        color: textColor
                        font.pixelSize: 10
                        verticalAlignment: Text.AlignVCenter
                        text: stepSize.toFixed(5).replace(".", ",")
                        enabled: root.enabled && !canvas.dragging
                        opacity: enabled ? 1.0 : 0.6
                        background: Rectangle { color: "transparent" }

                        onEditingFinished: {
                            var v = parseFloat(text.replace(",", "."))
                            if (!isNaN(v) && v > 0)
                                stepSize = v
                            text = stepSize.toFixed(5).replace(".", ",")
                        }
                    }
                }
                Rectangle {
                    width: parent.width * 0.15
                    height: parent.height
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "▶"
                        color: enabled ? textColor : "#666"
                        font.pixelSize: 10
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: root.enabled && !canvas.dragging
                        propagateComposedEvents: true

                        onClicked: {
                            if (!canvas.dragging) {
                                applyValue(currentValue + stepSize)
                            } else {
                                mouse.accepted = false
                            }
                        }
                    }
                }
            }

            Component.onCompleted: input.text = format(currentValue)
        }
    }

    Component {
        id: vertexTableDelegate
        Row {
            id: vertexRow
            width: parent.width
            height: 40
            spacing: 10

            property int vertexIndex: model.index
            property bool isSelected: canvas.selectedVertexIndex === model.index
            property bool uiUpdating: false

            Rectangle {
                width: parent.width * 0.20
                height: parent.height
                color: vertexRow.isSelected ? "#007acc" : "transparent"

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        canvas.selectedVertexIndex = vertexRow.vertexIndex;
                        canvas.selectedEdgeIndex = -1;
                    }

                    Text {
                        text: "Вершина " + (vertexRow.vertexIndex + 1) + ":"
                        color: "white"
                        font.pixelSize: 12
                        anchors.centerIn: parent
                    }
                }
            }
            Loader {
                id: xLoader
                width: parent.width * 0.40 - 15
                height: parent.height
                active: true
                sourceComponent: compactSpinBox

                onLoaded: {
                    if (item) {
                        var xValue = model.x;
                        item.currentValue = xValue;
                        item.text = xValue.toFixed(5);
                        item.textColor = "white";
                        item.minValue = -10000;
                        item.maxValue = 10000;
                        item.stepSize = 0.1;

                        item.valueChanged.connect(function(newValue) {
                            if (canvas.selectedShapeId !== -1 && !vertexRow.uiUpdating) {
                                vertexRow.uiUpdating = true;
                                var x = newValue;
                                var y = yLoader.item ? yLoader.item.currentValue : model.y;
                                if (!isNaN(x) && !isNaN(y)) {
                                    var localPos = canvas.worldTolocal(canvas.selectedShapeId, Qt.point(x, y));
                                    canvas.setShapeVertex(canvas.selectedShapeId, vertexRow.vertexIndex, localPos.x, localPos.y);
                                }
                                vertexRow.uiUpdating = false;
                            }
                        });
                    }
                }
            }
            Loader {
                id: yLoader
                width: parent.width * 0.40 - 15
                height: parent.height
                active: true
                sourceComponent: compactSpinBox

                onLoaded: {
                    if (item) {
                        var yValue = model.y;
                        item.currentValue = yValue;
                        item.text = yValue.toFixed(5);
                        item.textColor = "white";
                        item.minValue = -10000;
                        item.maxValue = 10000;
                        item.stepSize = 0.1;

                        item.valueChanged.connect(function(newValue) {
                            if (canvas.selectedShapeId !== -1 && !vertexRow.uiUpdating) {
                                vertexRow.uiUpdating = true;
                                var x = xLoader.item ? xLoader.item.currentValue : model.x;
                                var y = newValue;
                                if (!isNaN(x) && !isNaN(y)) {
                                    var localPos = canvas.worldToLocal(canvas.selectedShapeId, Qt.point(x, y));
                                    canvas.setShapeVertex(canvas.selectedShapeId, vertexRow.vertexIndex, localPos.x, localPos.y);
                                }
                                vertexRow.uiUpdating = false;
                            }
                        });
                    }
                }
            }
            function updateFromModel() {
                if (!vertexRow.uiUpdating && xLoader.item && yLoader.item) {
                    vertexRow.uiUpdating = true;

                    var xValue = model.x;
                    var yValue = model.y;

                    if (Math.abs(xLoader.item.currentValue - xValue) > 0.00001) {
                        xLoader.item.currentValue = xValue;
                        xLoader.item.text = xValue.toFixed(5);
                    }

                    if (Math.abs(yLoader.item.currentValue - yValue) > 0.00001) {
                        yLoader.item.currentValue = yValue;
                        yLoader.item.text = yValue.toFixed(5);
                    }

                    vertexRow.uiUpdating = false;
                }
            }

            Component.onCompleted: {
                if (xLoader.item && yLoader.item) {
                    xLoader.item.currentValue = model.x;
                    xLoader.item.text = model.x.toFixed(5);
                    yLoader.item.currentValue = model.y;
                    yLoader.item.text = model.y.toFixed(5);
                }
            }
        }
    }

    Component {
        id: edgeTableDelegate
        Row {
            id: edgeRow
            width: parent.width
            height: 40
            spacing: 10

            property int edgeIndex: model.index
            property bool isSelected: canvas.selectedEdgeIndex === model.index
            property bool uiUpdating: false

            Rectangle {
                width: parent.width * 0.15
                height: parent.height
                color: edgeRow.isSelected ? "#007acc" : "transparent"

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        canvas.selectedEdgeIndex = edgeRow.edgeIndex;
                        canvas.selectedVertexIndex = -1;
                    }

                    Text {
                        text: "Ребро " + (edgeRow.edgeIndex + 1) + ":"
                        color: "white"
                        font.pixelSize: 12
                        anchors.centerIn: parent
                    }
                }
            }
            Loader {
                id: edgeLengthLoader
                width: parent.width * 0.85 - 20
                height: parent.height
                active: true
                sourceComponent: compactSpinBox

                onLoaded: {
                    if (item) {
                        var lengthValue = model.length;
                        item.currentValue = lengthValue;
                        item.text = lengthValue.toFixed(5);
                        item.textColor = "white";
                        item.minValue = 0.1;
                        item.maxValue = 10000;
                        item.stepSize = 0.1;

                        item.valueChanged.connect(function(newLength) {
                            if (canvas.selectedShapeId !== -1 && !edgeRow.uiUpdating) {
                                edgeRow.uiUpdating = true;
                                if (!isNaN(newLength)) {
                                    var shapeScale = canvas.getShapeScale(canvas.selectedShapeId);
                                    var globalScale = canvas.globalScale;
                                    newLength = newLength / (shapeScale + globalScale);
                                    canvas.setEdgeLength(canvas.selectedShapeId, edgeRow.edgeIndex, newLength);
                                }
                                edgeRow.uiUpdating = false;
                            }
                        });
                    }
                }
            }
            function updateFromModel() {
                if (!edgeRow.uiUpdating && edgeLengthLoader.item) {
                    edgeRow.uiUpdating = true;

                    var lengthValue = model.length;
                    if (Math.abs(edgeLengthLoader.item.currentValue - lengthValue) > 0.00001) {
                        edgeLengthLoader.item.currentValue = lengthValue;
                        edgeLengthLoader.item.text = lengthValue.toFixed(5);
                    }

                    edgeRow.uiUpdating = false;
                }
            }

            Component.onCompleted: {
                if (edgeLengthLoader.item) {
                    edgeLengthLoader.item.currentValue = model.length;
                    edgeLengthLoader.item.text = model.length.toFixed(5);
                }
            }
        }
    }

    property ListModel verticesModel: ListModel {}
    property ListModel edgesModel: ListModel {}
    property real currentRotation: 0
    property real currentScale: 1.0
    property real currentSize: 50.0
    property real currentSizeWidth: 90.0
    property real currentSizeHeigth: 50.0
    property color currentColor: "#0078d7"
    property bool shapeCollisions: false
    property alias centerXLoaderItem: centerXLoader.item
    property alias centerYLoaderItem: centerYLoader.item
    property bool uiUpdating: false

    onCurrentRotationChanged: {
        if (rotationSpinLoader.item && !uiUpdating) {
            rotationSpinLoader.item.currentValue = currentRotation;
            rotationSpinLoader.item.text = currentRotation.toFixed(5);
        }
    }

    onCurrentScaleChanged: {
        if (scaleSpinLoader.item && !uiUpdating) {
            scaleSpinLoader.item.currentValue = currentScale;
            scaleSpinLoader.item.text = currentScale.toFixed(5);
        }
    }

    onCurrentSizeWidthChanged: {
        if (widthSpinLoader.item && !uiUpdating) {
            widthSpinLoader.item.currentValue = currentSizeWidth;
            widthSpinLoader.item.text = currentSizeWidth.toFixed(5);
        }
    }

    onCurrentSizeHeigthChanged: {
        if (heightSpinLoader.item && !uiUpdating) {
            heightSpinLoader.item.currentValue = currentSizeHeigth;
            heightSpinLoader.item.text = currentSizeHeigth.toFixed(5);
        }
    }

    Row {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: shapesPanel
            width: 300
            height: parent.height
            color: "#252526"

            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Text {
                    text: "Фигуры"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#404040"
                }

                Column {
                    width: parent.width

                    Loader {
                        id: shapesComboBoxLoader
                        width: parent.width
                        sourceComponent: styledComboBox

                        onLoaded: {
                            item.model = ["Фигура", "Треугольник", "Квадрат", "Пятиугольник", "Шестиугольник"];
                            item.onActivated.connect(function(index) {
                                if(index == 0) {
                                    canvas.addShapeAndSelect(0, 0, 4, currentSizeWidth, currentSizeHeigth)
                                } else {
                                    canvas.addShapeAndSelect(0, 0, index+2, currentSize, currentSize)
                                }
                                // Устанавливаем текущий цвет новой фигуре
                                if (canvas.selectedShapeId !== -1) {
                                    canvas.setShapeColor(canvas.selectedShapeId, currentColor)
                                }
                            });
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 10

                    CheckBox {
                        id: gridCheckbox
                        text: "Сетка"
                        checked: true
                        onCheckedChanged: canvas.showGrid = checked
                        contentItem: Text {
                            text: gridCheckbox.text
                            color: "white"
                            font.pixelSize: 12
                            leftPadding: gridCheckbox.indicator.width + 10
                        }

                        indicator: Rectangle {
                            implicitWidth: 16
                            implicitHeight: 16
                            x: gridCheckbox.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 3
                            border.color: gridCheckbox.checked ? "#007acc" : "#666"
                            border.width: 1

                            Rectangle {
                                width: 10
                                height: 10
                                x: 3
                                y: 3
                                radius: 2
                                color: gridCheckbox.checked ? "#007acc" : "transparent"
                                visible: gridCheckbox.checked
                            }
                        }
                    }

                    CheckBox {
                        id: collisionsCheckbox
                        text: "Столкновения"
                        checked: true
                        onCheckedChanged: canvas.collisionsEnabled = checked
                        contentItem: Text {
                            text: collisionsCheckbox.text
                            color: "white"
                            font.pixelSize: 12
                            leftPadding: collisionsCheckbox.indicator.width + 10
                        }

                        indicator: Rectangle {
                            implicitWidth: 16
                            implicitHeight: 16
                            x: collisionsCheckbox.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 3
                            border.color: collisionsCheckbox.checked ? "#007acc" : "#666"
                            border.width: 1

                            Rectangle {
                                width: 10
                                height: 10
                                x: 3
                                y: 3
                                radius: 2
                                color: collisionsCheckbox.checked ? "#007acc" : "transparent"
                                visible: collisionsCheckbox.checked
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#404040"
                }

                Text {
                    text: "Список фигур:"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 14
                }

                ScrollView {
                    width: parent.width
                    height: parent.height - 320
                    clip: true

                    Column {
                        width: parent.width
                        spacing: 5

                        Repeater {
                            model: canvas.shapeCount

                            delegate: Rectangle {
                                id: shapeCard
                                property int shapeId: index < canvas.shapeCount ? canvas.getShapeIdByIndex(index) : -1

                                width: parent.width
                                height: 70
                                color: canvas.selectedShapeId === shapeId ? "#007acc" : "#1e1e1e"
                                radius: 5
                                border.width: 2
                                border.color: canvas.selectedShapeId === shapeId ? "#1e90ff" : "transparent"

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (shapeId !== -1) {
                                            canvas.selectedShapeId = shapeId;
                                            updateShapeInfo();
                                        }
                                    }
                                }

                                Row {
                                    anchors.fill: parent
                                    anchors.margins: 5
                                    spacing: 10

                                    Column {
                                        width: parent.width - 110
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 2

                                        Text {
                                            text: shapeId !== -1 ? canvas.getShapeName(shapeId) + " #" + shapeId : ""
                                            color: "white"
                                            font.pixelSize: 12
                                            font.bold: true
                                            elide: Text.ElideRight
                                            width: parent.width
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: canvasContainer
            width: parent.width - shapesPanel.width - verticesPanel.width
            height: parent.height
            color: "#1e1e1e"

            VKCanvas {
                id: canvas
                anchors.fill: parent
                clip: true

                onSelectedShapeIdChanged: {
                    updateShapeInfo();
                }

                onVertexInfoUpdated: {
                    if (canvas.activeTab === 1 && !uiUpdating) {
                        updateVerticesAndEdges();
                    }
                }

                onShapeUpdated: function(shapeId) {
                    if (shapeId === canvas.selectedShapeId) {
                        currentRotation = canvas.getShapeRotation(shapeId);
                        currentScale = canvas.getShapeScale(shapeId);
                        currentSizeWidth = canvas.getShapeSizeWidth(shapeId);
                        currentSizeHeigth = canvas.getShapeSizeHeight(shapeId);
                        shapeCollisions = canvas.getShapeCollisionsEnabled(shapeId);
                        currentColor = canvas.getShapeColor(shapeId);

                        var pos = canvas.getShapePosition(shapeId);
                        if (centerXLoader.item) {
                            centerXLoader.item.currentValue = pos.x;
                            centerXLoader.item.text = pos.x.toFixed(5);
                        }
                        if (centerYLoader.item) {
                            centerYLoader.item.currentValue = pos.y;
                            centerYLoader.item.text = pos.y.toFixed(5);
                        }

                        if (canvas.activeTab === 1 && !uiUpdating) {
                            updateVerticesAndEdges();
                        }
                    }
                }
            }
        }

        Rectangle {
            id: verticesPanel
            width: 400
            height: parent.height
            color: "#252526"

            Column {
                id: verticesColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 15

                Text {
                    text: canvas.selectedShapeId !== -1 ? canvas.getShapeName(canvas.selectedShapeId) + " #" + canvas.selectedShapeId : "Нет фигуры"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: canvas.selectedShapeId !== -1
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#404040"
                    visible: canvas.selectedShapeId !== -1
                }

                Row {
                    width: parent.width
                    height: 40
                    spacing: 0
                    visible: canvas.selectedShapeId !== -1

                    Rectangle {
                        width: parent.width / 2
                        height: parent.height
                        color: canvas.activeTab === 0 ? "#007acc" : "#1e1e1e"
                        border.color: "#404040"
                        border.width: 1

                        Text {
                            text: "Объект"
                            color: "white"
                            font.pixelSize: 14
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: canvas.activeTab = 0
                        }
                    }

                    Rectangle {
                        width: parent.width / 2
                        height: parent.height
                        color: canvas.activeTab === 1 ? "#007acc" : "#1e1e1e"
                        border.color: "#404040"
                        border.width: 1

                        Text {
                            text: "Редактировать"
                            color: "white"
                            font.pixelSize: 14
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                canvas.activeTab = 1;
                                updateVerticesAndEdges();
                            }
                        }
                    }
                }

                Column {
                    id: objectTabContent
                    width: parent.width
                    spacing: 15
                    visible: canvas.selectedShapeId !== -1 && canvas.activeTab === 0

                    Text {
                        text: "Координаты центра:"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                        width: parent.width
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Row {
                            width: parent.width * 0.45
                            spacing: 10

                            Text {
                                text: "X:"
                                color: "white"
                                font.pixelSize: 13
                                width: parent.width * 0.15
                                verticalAlignment: Text.AlignVCenter
                                height: 30
                                padding: 10
                            }

                            Loader {
                                id: centerXLoader
                                width: parent.width * 0.85
                                sourceComponent: compactSpinBox

                                onLoaded: {
                                    item.text = "0.00000";
                                    item.minValue = -10000;
                                    item.maxValue = 10000;
                                    item.stepSize = 0.1;

                                    item.valueChanged.connect(function(newValue) {
                                        if (canvas.selectedShapeId !== -1 && !uiUpdating) {
                                            uiUpdating = true;
                                            var x = newValue;
                                            var y = centerYLoader.item ? centerYLoader.item.currentValue : 0;
                                            if (!isNaN(x) && !isNaN(y)) {
                                                canvas.setShapePosition(canvas.selectedShapeId, x, y);
                                            }
                                            uiUpdating = false;
                                        }
                                    });
                                }

                                active: canvas.selectedShapeId !== -1
                            }
                        }

                        Row {
                            width: parent.width * 0.45
                            spacing: 10

                            Text {
                                text: "Y:"
                                color: "white"
                                font.pixelSize: 13
                                width: parent.width * 0.15
                                verticalAlignment: Text.AlignVCenter
                                height: 30
                                padding: 10
                            }

                            Loader {
                                id: centerYLoader
                                width: parent.width * 0.85
                                sourceComponent: compactSpinBox

                                onLoaded: {
                                    item.text = "0.00000";
                                    item.minValue = -10000;
                                    item.maxValue = 10000;
                                    item.stepSize = 0.1;

                                    item.valueChanged.connect(function(newValue) {
                                        if (canvas.selectedShapeId !== -1 && !uiUpdating) {
                                            uiUpdating = true;
                                            var x = centerXLoader.item ? centerXLoader.item.currentValue : 0;
                                            var y = newValue;
                                            if (!isNaN(x) && !isNaN(y)) {
                                                canvas.setShapePosition(canvas.selectedShapeId, x, y);
                                            }
                                            uiUpdating = false;
                                        }
                                    });
                                }

                                active: canvas.selectedShapeId !== -1
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Loader {
                            id: resetCenterButton
                            width: parent.width
                            sourceComponent: styledButton

                            onLoaded: {
                                item.text = "Сбросить центр";
                                item.onClicked.connect(function() {
                                    if (canvas.selectedShapeId !== -1) {
                                        canvas.setShapePosition(canvas.selectedShapeId, 0, 0);
                                        if (centerXLoader.item) centerXLoader.item.currentValue = 0;
                                        if (centerYLoader.item) centerYLoader.item.currentValue = 0;
                                    }
                                });
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#404040"
                    }

                    // Блок выбора цвета
                    Text {
                        text: "Цвет фигуры:"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                        width: parent.width
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Rectangle {
                            id: colorPreview
                            width: parent.width * 0.10
                            height: parent.width * 0.10
                            color: currentColor
                            border.color: "white"
                            border.width: 2
                            radius: 5

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (canvas.selectedShapeId !== -1) {
                                        colorDialog.selectedColor = currentColor;
                                        colorDialog.open();
                                    }
                                }
                            }
                        }

                        Column {
                            spacing: 5

                            Text {
                                text: "HEX: " + currentColor.toString().toUpperCase()
                                color: "white"
                                font.pixelSize: 12
                            }

                            Text {
                                text: "R: " + Math.round(currentColor.r * 255) +
                                      " G: " + Math.round(currentColor.g * 255) +
                                      " B: " + Math.round(currentColor.b * 255)
                                color: "white"
                                font.pixelSize: 10
                            }
                        }

                        Loader {
                            id: chooseColorButton
                            width: parent.width * 0.63
                            sourceComponent: styledButton

                            onLoaded: {
                                item.text = "Выбрать...";
                                item.onClicked.connect(function() {
                                    if (canvas.selectedShapeId !== -1) {
                                        colorDialog.selectedColor = currentColor;
                                        colorDialog.open();
                                    }
                                });
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#404040"
                    }

                    Grid {
                        width: parent.width
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 8
                        verticalItemAlignment: Grid.AlignVCenter

                        Text {
                            text: "Вращение:"
                            color: "white"
                            font.pixelSize: 12
                            font.bold: true
                            Layout.alignment: Qt.AlignRight
                            width: parent.width * 0.2
                        }

                        Loader {
                            id: rotationSpinLoader
                            height: 30
                            width: parent.width * 0.77
                            active: canvas.selectedShapeId !== -1
                            sourceComponent: compactSpinBox

                            onLoaded: {
                                item.currentValue = currentRotation
                                item.minValue = 0
                                item.maxValue = 360
                                width: parent.width * 0.20
                                item.stepSize = 1.0
                                item.textColor = "white"

                                item.valueChanged.connect(function(newValue) {
                                    if (canvas.selectedShapeId !== -1 && !uiUpdating) {
                                        uiUpdating = true
                                        canvas.setShapeRotation(canvas.selectedShapeId, newValue)
                                        currentRotation = newValue
                                        uiUpdating = false
                                    }
                                })
                            }
                        }

                        Text {
                            text: "Масштаб:"
                            color: "white"
                            font.pixelSize: 12
                            font.bold: true
                            width: parent.width * 0.2
                        }

                        Loader {
                            id: scaleSpinLoader
                            height: 30
                            width: parent.width * 0.77
                            active: canvas.selectedShapeId !== -1
                            sourceComponent: compactSpinBox

                            onLoaded: {
                                item.currentValue = currentScale
                                item.minValue = 0.1
                                item.maxValue = 3.0
                                item.stepSize = 0.05
                                item.textColor = "white"

                                item.valueChanged.connect(function(newValue) {
                                    if (canvas.selectedShapeId !== -1 && !uiUpdating) {
                                        uiUpdating = true
                                        canvas.setShapeScale(canvas.selectedShapeId, newValue)
                                        currentScale = newValue
                                        uiUpdating = false
                                    }
                                })
                            }
                        }

                        Text {
                            text: "Ширина:"
                            color: "white"
                            font.pixelSize: 12
                            font.bold: true
                            width: parent.width * 0.2
                        }

                        Loader {
                            id: widthSpinLoader
                            height: 30
                            width: parent.width * 0.77
                            active: canvas.selectedShapeId !== -1
                            sourceComponent: compactSpinBox

                            onLoaded: {
                                item.currentValue = currentSizeWidth
                                item.minValue = 10
                                item.maxValue = 10000
                                item.stepSize = 1.0
                                item.textColor = "white"

                                item.valueChanged.connect(function(newValue) {
                                    if (canvas.selectedShapeId !== -1 && !uiUpdating) {
                                        uiUpdating = true
                                        canvas.setShapeSizeWidgth(canvas.selectedShapeId, newValue)
                                        currentSizeWidth = newValue
                                        uiUpdating = false
                                    }
                                })
                            }
                        }

                        Text {
                            text: "Высота:"
                            color: "white"
                            font.pixelSize: 12
                            font.bold: true
                            width: parent.width * 0.2
                        }

                        Loader {
                            id: heightSpinLoader
                            height: 30
                            width: parent.width * 0.77
                            active: canvas.selectedShapeId !== -1
                            sourceComponent: compactSpinBox

                            onLoaded: {
                                item.currentValue = currentSizeHeigth
                                item.minValue = 10
                                item.maxValue = 10000
                                item.stepSize = 1.0
                                item.textColor = "white"

                                item.valueChanged.connect(function(newValue) {
                                    if (canvas.selectedShapeId !== -1 && !uiUpdating) {
                                        uiUpdating = true
                                        canvas.setShapeSizeHeight(canvas.selectedShapeId, newValue)
                                        currentSizeHeigth = newValue
                                        uiUpdating = false
                                    }
                                })
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Loader {
                            id: resetTransformButton
                            width: (parent.width - 10) / 2
                            sourceComponent: styledButton

                            onLoaded: {
                                item.text = "Сбросить трансформацию";
                                item.onClicked.connect(function() {
                                    if (canvas.selectedShapeId !== -1) {
                                        canvas.setShapeRotation(canvas.selectedShapeId, 0);
                                        canvas.setShapeScale(canvas.selectedShapeId, 1.0);
                                        updateShapeInfo();
                                    }
                                });
                            }
                        }

                        Loader {
                            id: resetAllButton
                            width: (parent.width - 10) / 2
                            sourceComponent: styledButton

                            onLoaded: {
                                item.text = "Сбросить всё";
                                item.onClicked.connect(function() {
                                    if (canvas.selectedShapeId !== -1) {
                                        canvas.setShapeRotation(canvas.selectedShapeId, 0);
                                        canvas.setShapeScale(canvas.selectedShapeId, 1.0);
                                        canvas.setShapePosition(canvas.selectedShapeId, 0, 0);
                                        canvas.setShapeSizeWidgth(canvas.selectedShapeId, 50.0);
                                        canvas.setShapeSizeHeight(canvas.selectedShapeId, 50.0);
                                        canvas.setShapeColor(canvas.selectedShapeId, "#0078d7");
                                        currentColor = "#0078d7";
                                        if (centerXLoader.item) centerXLoader.item.currentValue = 0;
                                        if (centerYLoader.item) centerYLoader.item.currentValue = 0;
                                        updateShapeInfo();
                                    }
                                });
                            }
                        }
                    }

                    Loader {
                        id: deleteShapeButton
                        width: parent.width
                        sourceComponent: styledButton

                        onLoaded: {
                            item.text = "Удалить фигуру";
                            item.enabled = canvas.selectedShapeId !== -1;

                            canvas.selectedShapeIdChanged.connect(function() {
                                item.enabled = canvas.selectedShapeId !== -1;
                            });

                            item.onClicked.connect(function() {
                                if (canvas.selectedShapeId !== -1) {
                                    canvas.removeShape(canvas.selectedShapeId);
                                }
                            });
                        }
                    }
                }

                Column {
                    id: editTabContent
                    width: parent.width
                    spacing: 15
                    visible: canvas.selectedShapeId !== -1 && canvas.activeTab === 1

                    Text {
                        text: "Редактирование вершин и рёбер"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                        width: parent.width
                    }

                    Row {
                        width: parent.width
                        spacing: 10

                        Loader {
                            id: addVertexButton
                            width: (parent.width - 10) / 2
                            sourceComponent: styledButton

                            onLoaded: {
                                item.text = "Добавить вершину";
                                item.enabled = canvas.selectedEdgeIndex !== -1;

                                canvas.selectedEdgeIndexChanged.connect(function() {
                                    item.enabled = canvas.selectedEdgeIndex !== -1;
                                });

                                item.onClicked.connect(function() {
                                    if (canvas.selectedEdgeIndex !== -1) {
                                        var shape = canvas.selectedShapeId;
                                        var edge = canvas.selectedEdgeIndex;
                                        var v1 = canvas.getShapeVertex(shape, edge);
                                        var v2 = canvas.getShapeVertex(shape, (edge + 1) % canvas.getShapeVertexCount(shape));
                                        var x = (v1.x + v2.x) / 2.0;
                                        var y = (v1.y + v2.y) / 2.0;
                                        canvas.addVertexToShape(shape, x, y);
                                    }
                                });
                            }
                        }

                        Loader {
                            id: removeVertexButton
                            width: (parent.width - 10) / 2
                            sourceComponent: styledButton

                            onLoaded: {
                                item.text = "Удалить вершину";
                                item.enabled = canvas.selectedVertexIndex !== -1;

                                canvas.selectedVertexIndexChanged.connect(function() {
                                    item.enabled = canvas.selectedVertexIndex !== -1;
                                });

                                item.onClicked.connect(function() {
                                    if (canvas.selectedVertexIndex !== -1) {
                                        canvas.removeVertexFromShape(canvas.selectedShapeId, canvas.selectedVertexIndex);
                                    }
                                });
                            }
                        }
                    }

                    Loader {
                        id: resetVerticesButton
                        width: parent.width
                        sourceComponent: styledButton

                        onLoaded: {
                            item.text = "Сбросить вершины";
                            item.onClicked.connect(function() {
                                canvas.resetShapeVertices(canvas.selectedShapeId);
                            });
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#404040"
                    }

                    Text {
                        text: "Координаты вершин:"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 12
                        width: parent.width
                    }

                    Rectangle {
                        id: verticesInfoContainer
                        width: parent.width
                        height: (50 * canvas.getShapeVertexCount(canvas.selectedShapeId)) + 20
                        color: "#1e1e1e"
                        border.color: "#404040"
                        border.width: 1

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 1
                            clip: true

                            Column {
                                id: verticesColumnDynamic
                                width: parent.width
                                spacing: 5

                                Rectangle {
                                    width: parent.width
                                    height: 30
                                    color: "#007acc"

                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 5
                                        spacing: 10

                                        Text {
                                            text: "Вершина"
                                            color: "white"
                                            font.pixelSize: 12
                                            font.bold: true
                                            width: parent.width * 20
                                        }

                                        Text {
                                            text: "X"
                                            color: "white"
                                            font.pixelSize: 12
                                            font.bold: true
                                            width: parent.width * 40
                                        }

                                        Text {
                                            text: "Y"
                                            color: "white"
                                            font.pixelSize: 12
                                            font.bold: true
                                            width: parent.width * 40
                                        }
                                    }
                                }

                                Repeater {
                                    id: verticesRepeater
                                    model: verticesModel
                                    delegate: vertexTableDelegate
                                }
                            }
                        }
                    }

                    Text {
                        text: "Длины рёбер:"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 12
                        width: parent.width
                    }

                    Rectangle {
                        id: edgesInfoContainer
                        width: parent.width
                        height: (50 * canvas.getShapeVertexCount(canvas.selectedShapeId)) + 20
                        color: "#1e1e1e"
                        border.color: "#404040"
                        border.width: 1

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 1
                            clip: true

                            Column {
                                id: edgesColumnDynamic
                                width: parent.width
                                spacing: 5

                                Rectangle {
                                    width: parent.width
                                    height: 30
                                    color: "#007acc"

                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 5
                                        spacing: 10

                                        Text {
                                            text: "Ребро"
                                            color: "white"
                                            font.pixelSize: 12
                                            font.bold: true
                                            width: parent.width * 0.15
                                        }

                                        Text {
                                            text: "Длина"
                                            color: "white"
                                            font.pixelSize: 12
                                            font.bold: true
                                            width: parent.width * 0.85
                                        }
                                    }
                                }

                                Repeater {
                                    id: edgesRepeater
                                    model: edgesModel
                                    delegate: edgeTableDelegate
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#404040"
                    }

                    CheckBox {
                        id: shapeCollisionsCheckbox
                        text: "Столкновения"
                        checked: shapeCollisions
                        onCheckedChanged: {
                            if (canvas.selectedShapeId !== -1) {
                                canvas.setShapeCollisionsEnabled(canvas.selectedShapeId, checked);
                                shapeCollisions = checked;
                            }
                        }
                        contentItem: Text {
                            text: shapeCollisionsCheckbox.text
                            color: "white"
                            font.pixelSize: 12
                            leftPadding: shapeCollisionsCheckbox.indicator.width + 10
                        }

                        indicator: Rectangle {
                            implicitWidth: 16
                            implicitHeight: 16
                            x: shapeCollisionsCheckbox.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 3
                            border.color: shapeCollisionsCheckbox.checked ? "#007acc" : "#666"
                            border.width: 1

                            Rectangle {
                                width: 10
                                height: 10
                                x: 3
                                y: 3
                                radius: 2
                                color: shapeCollisionsCheckbox.checked ? "#007acc" : "transparent"
                                visible: shapeCollisionsCheckbox.checked
                            }
                        }
                    }

                    Loader {
                        id: updateTableButton
                        width: parent.width
                        sourceComponent: styledButton

                        onLoaded: {
                            item.text = "Обновить данные таблицы";
                            item.onClicked.connect(updateVerticesAndEdges);
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 30
        anchors.bottom: parent.bottom
        color: "#252526"
        border.color: "#404040"
        border.width: 1

        Row {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 20

            Text {
                text: "Фигур: " + canvas.shapeCount
                color: "white"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Выбрана фигура: " + (canvas.selectedShapeId !== -1 ? canvas.selectedShapeId : "нет")
                color: canvas.selectedShapeId !== -1 ? "#4CAF50" : "#aaaaaa"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Вершин: " + (canvas.selectedShapeId !== -1 ? canvas.getShapeVertexCount(canvas.selectedShapeId) : "0")
                color: "#4CAF50"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Режим: " + (canvas.activeTab === 0 ? "Объект" : "Редактировать")
                color: canvas.activeTab === 0 ? "#4CAF50" : "#FFD700"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Масштаб: " + canvas.globalScale.toFixed(2) + "x"
                color: "#4CAF50"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: "Холст: " + canvas.offsetX.toFixed(5) + ", " + canvas.offsetY.toFixed(5);
                color: "#4CAF50"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: "Цвет: " + currentColor.toString().toUpperCase()
                color: currentColor
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    function updateShapeInfo() {
        if (canvas.selectedShapeId !== -1) {
            currentRotation = canvas.getShapeRotation(canvas.selectedShapeId)
            currentScale = canvas.getShapeScale(canvas.selectedShapeId)
            currentSizeWidth = canvas.getShapeSizeWidth(canvas.selectedShapeId)
            currentSizeHeigth = canvas.getShapeSizeHeight(canvas.selectedShapeId)
            shapeCollisions = canvas.getShapeCollisionsEnabled(canvas.selectedShapeId)
            currentColor = canvas.getShapeColor(canvas.selectedShapeId)

            var pos = canvas.getShapePosition(canvas.selectedShapeId)
            if (centerXLoader.item) {
                centerXLoader.item.currentValue = pos.x
                centerXLoader.item.text = pos.x.toFixed(5)
            }
            if (centerYLoader.item) {
                centerYLoader.item.currentValue = pos.y
                centerYLoader.item.text = pos.y.toFixed(5)
            }

            if (canvas.activeTab === 1) {
                updateVerticesAndEdges()
            }
        } else {
            currentRotation = 0
            currentScale = 1.0
            currentSizeWidth = 50.0
            currentSizeHeigth = 50.0
            shapeCollisions = false
            currentColor = "#0078d7"
            verticesModel.clear()
            edgesModel.clear()
            if (centerXLoader.item) {
                centerXLoader.item.currentValue = 0
                centerXLoader.item.text = "0.00000"
            }
            if (centerYLoader.item) {
                centerYLoader.item.currentValue = 0
                centerYLoader.item.text = "0.00000"
            }
        }
    }

    function updateVerticesAndEdges() {
        if (canvas.selectedShapeId === -1 || canvas.activeTab !== 1) {
            verticesModel.clear();
            edgesModel.clear();
            return;
        }

        var shapeId = canvas.selectedShapeId;
        var vertexCount = canvas.getShapeVertexCount(shapeId);

        for (var i = 0; i < vertexCount; i++) {
            var vertex = canvas.getShapeVertex(shapeId, i);
            vertex = canvas.vertexToWorld(shapeId, i);

            if (i < verticesModel.count) {
                verticesModel.set(i, {
                    index: i,
                    x: vertex.x,
                    y: vertex.y,
                    isSelected: (i === canvas.selectedVertexIndex)
                });
            } else {
                verticesModel.append({
                    index: i,
                    x: vertex.x,
                    y: vertex.y,
                    isSelected: (i === canvas.selectedVertexIndex)
                });
            }
        }

        while (verticesModel.count > vertexCount) {
            verticesModel.remove(vertexCount);
        }

        for (var j = 0; j < vertexCount; j++) {
            var nextJ = (j + 1) % vertexCount;
            var v1, v2;

            v1 = canvas.vertexToWorld(shapeId, j);
            v2 = canvas.vertexToWorld(shapeId, nextJ);

            var dx = v2.x - v1.x;
            var dy = v2.y - v1.y;
            var length = Math.sqrt(dx * dx + dy * dy);

            if (j < edgesModel.count) {
                edgesModel.set(j, {
                    index: j,
                    length: length,
                    isSelected: (j === canvas.selectedEdgeIndex)
                });
            } else {
                edgesModel.append({
                    index: j,
                    length: length,
                    isSelected: (j === canvas.selectedEdgeIndex)
                });
            }
        }

        while (edgesModel.count > vertexCount) {
            edgesModel.remove(vertexCount);
        }

        for (var k = 0; k < verticesRepeater.count; k++) {
            var item = verticesRepeater.itemAt(k);
            if (item && item.updateFromModel) {
                item.updateFromModel();
            }
        }

        for (var l = 0; l < edgesRepeater.count; l++) {
            var edgeItem = edgesRepeater.itemAt(l);
            if (edgeItem && edgeItem.updateFromModel) {
                edgeItem.updateFromModel();
            }
        }
    }

    Component.onCompleted: {
        updateShapeInfo();
        if (canvas.selectedShapeId !== -1 && canvas.activeTab === 1) {
            updateVerticesAndEdges();
        }
    }
}
