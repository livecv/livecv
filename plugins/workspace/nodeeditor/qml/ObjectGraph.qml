import QtQuick                   2.8
import QtQuick.Controls          2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Layouts           1.3
import live                      1.0

import workspace.quickqanava 2.0 as Qan

Rectangle{
    id: root
    
    width: 500
    height: 700
    color: '#000511'
    clip: true
    radius: 5
    border.width: 1
    border.color: "#333"
    
    property Component resizeConnectionsFactory: Component{
        Connections {
            target: null
            property var node: null
            ignoreUnknownSignals: true
            onWidthChanged: {
                var maxWidth = 340
                for (var i=0; i < node.propertyContainer.children.length; ++i)
                {
                    var child = node.propertyContainer.children[i]
                    if (child.width > maxWidth) maxWidth = child.width
                }

                if (maxWidth !== node.width)
                    node.width = maxWidth + 20
            }
        }
    }
    property Component propertyDestructorFactory: Component {
        Connections {
            target: null
            property var node: null
            ignoreUnknownSignals: true
            onPropertyToBeDestroyed: {
                node.item.removePropertyName(name)
            }
        }
    }

    property Component addBoxFactory: null

    property var connections: []
    property Component propertyDelegate : ObjectNodeProperty{}
    property alias nodeDelegate : graph.nodeDelegate
    property var palette: null
    property var documentHandler: null
    property var editor: null
    property var editingFragment: null

    property alias zoom: graphView.zoom
    property alias zoomOrigin: graphView.zoomOrigin

    property int inPort: 1
    property int outPort: 2
    property int noPort : 0
    property int inOutPort : 3
    
    property var selectedEdge: null

    signal userEdgeInserted(QtObject edge)
    signal nodeClicked(QtObject node)
    signal doubleClicked(var pos)
    signal rightClicked(var pos)
    signal edgeClicked(QtObject edge)


    onUserEdgeInserted: {
        var item = edge.item

        var srcPort = item.sourceItem
        var dstPort = item.destinationItem

        var name = srcPort.objectProperty.propertyName

        if (name.substr(0,2) === "on" && name.substr(name.length-7,7) === "Changed")
        {
            // source is event, different direction
            var nodeId = dstPort.objectProperty.node.item.id
            if (!nodeId) return
            if (dstPort.objectProperty.editingFragment) return

            var funcName = dstPort.objectProperty.propertyName
            var value = nodeId + "." + funcName + "()"

            var result = root.palette.extension.bindExpressionForFragment(
                srcPort.objectProperty.editingFragment,
                value
            )

            if ( result ){
                root.palette.extension.writeForFragment(
                    srcPort.objectProperty.editingFragment,
                    {'__ref': value}
                )
            }

        } else {
            var value =
                    srcPort.objectProperty.node.item.id + "." + srcPort.objectProperty.propertyName

            var result = root.palette.extension.bindExpressionForFragment(
                dstPort.objectProperty.editingFragment,
                value
            )
            if ( result ){
                root.palette.extension.writeForFragment(
                    dstPort.objectProperty.editingFragment,
                    {'__ref': value}
                )
            }
        }

        srcPort.outEdges.push(edge)
        dstPort.inEdge = edge
    }

    onEdgeClicked: {
        if (selectedEdge)
        {
            selectedEdge.item.color = '#6a6a6a'
        }
        selectedEdge = edge
        edge.item.color = '#ff0000'
    }

    onNodeClicked: {
        if (selectedEdge)
            selectedEdge.item.color = '#6a6a6a'
        selectedEdge = null
    }

    Keys.onDeletePressed: {
        if (selectedEdge){
            graph.removeEdge(selectedEdge)
            selectedEdge = null
        }
    }

    onDoubleClicked: {
        var addBoxItem = addBoxFactory.createObject()
        var position = editingFragment.valuePosition() + editingFragment.valueLength() - 1
        var addOptions = documentHandler.codeHandler.getAddOptions(position)

        addBoxItem.addContainer = addOptions

        addBoxItem.objectsOnly = true
        addBoxItem.assignFocus()

        var rect = Qt.rect(pos.x, pos.y, 1, 1)
        var cursorCoords = Qt.point(pos.x, pos.y + 30)
        var addBox = lk.layers.editor.environment.createEditorBox(
            addBoxItem, rect, cursorCoords, lk.layers.editor.environment.placement.bottom
        )

        addBoxItem.accept = function(type, data){
            var opos = documentHandler.codeHandler.addItem(
                addBoxItem.addContainer.itemModel.addPosition, addBoxItem.addContainer.objectType, data
            )
            documentHandler.codeHandler.addItemToRuntime(editingFragment, data, project.appRoot())
            var ef = documentHandler.codeHandler.openNestedConnection(
                editingFragment, opos
            )
            if (ef)
                editingFragment.signalObjectAdded(ef, cursorCoords)

            addBox.destroy()
        }

        addBoxItem.cancel = function(){
            addBox.destroy()
        }

    }

    function bindPorts(src, dst){
        var srcNode = src.objectProperty.node
        var dstNode = dst.objectProperty.node
        
        var edge = graph.insertEdge(srcNode, dstNode, graph.edgeDelegate)
        graph.bindEdge(edge, src, dst)
        
        return edge
    }

    function findEdge(src, dst){
        var srcEdges = src.node.outEdges
        for ( var i = 0; i < srcEdges.length; ++i ){
            var edge = srcEdges.at(i)
            if ( edge.item.sourceItem === src && edge.item.destinationItem === dst)
                return edge
        }
        return null
    }

    function unbindPorts(src, dst){
        var edge = findEdge(src, dst)
        removeEdge(edge)
    }

    function removeEdge(edge){
        graph.removeEdge(edge)
    }


    function removeObjectNode(node){
        --palette.numOfObjects
        graph.removeNode(node)
    }
    
    function addObjectNode(x, y, label){
        var node = graph.insertNode()
        node.item.nodeParent = node
        node.item.removeNode = removeObjectNode
        node.item.addSubobject = addObjectNodeProperty
        node.item.connectable = Qan.NodeItem.UnConnectable
        node.item.x = x
        node.item.y = y
        node.item.label = label
        node.label = label

        node.item.documentHandler = documentHandler
        node.item.editor = editor

        var idx = label.indexOf('#')
        if (idx !== -1)
        {
            node.item.id = label.substr(idx+1)
        }
        
        return node
    }
    
    function addObjectNodeProperty(node, propertyName, ports, editingFragment){
        var item = node.item
        var propertyItem = root.propertyDelegate.createObject(item.propertyContainer)

        propertyItem.propertyName = propertyName
        propertyItem.node = node

        propertyItem.editingFragment = editingFragment
        propertyItem.documentHandler = root.documentHandler

        if (editingFragment) editingFragment.incrementRefCount()

        propertyItem.editor = root.editor

        var conn = resizeConnectionsFactory.createObject()
        conn.target = propertyItem
        conn.node = item

        connections.push(conn)

        var pdestructor = propertyDestructorFactory.createObject()
        pdestructor.target = propertyItem
        pdestructor.node = node

        connections.push(pdestructor)

        if ( ports === root.inPort || ports === root.inOutPort ){
            var port = graph.insertPort(node, Qan.NodeItem.Left, Qan.Port.In);
            port.label = propertyName + " In"
            port.y = Qt.binding(function(){ return propertyItem.y + 42 + (propertyItem.propertyTitle.height / 2) })
            propertyItem.inPort = port
            port.objectProperty = propertyItem
            port.multiplicity = Qan.PortItem.Single
        }
        if (ports === root.outPort || (node.item.id !== "" && ports === root.inOutPort) ){
            var port = graph.insertPort(node, Qan.NodeItem.Right, Qan.Port.Out);
            port.label = propertyName + " Out"
            port.y = Qt.binding(function(){ return propertyItem.y + 42 + (propertyItem.propertyTitle.height / 2) })
            propertyItem.outPort = port
            port.objectProperty = propertyItem
        }
        
        node.item.properties.push(propertyItem)

        return propertyItem
    }
    
    Qan.GraphView {
        id: graphView
        anchors.fill: parent
        navigable   : true
        gridThickColor: '#222'
        onDoubleClicked : root.doubleClicked(pos)
        onRightClicked : root.rightClicked(pos)
    
        graph: Qan.Graph {
            id: graph
            connectorEnabled: true
            connectorEdgeColor: "#666"
            connectorColor: "#666"
            edgeDelegate: Edge{}
            verticalDockDelegate : VerticalDock{}
            portDelegate: Port{}
            connectorItem : PortConnector{}
            onEdgeClicked: root.edgeClicked(edge)
            onNodeClicked : root.nodeClicked(node)
            onConnectorEdgeInserted : root.userEdgeInserted(edge)
            nodeDelegate: ObjectNode{}
            Component.onCompleted : {
                styleManager.styles.at(1).lineColor = '#666'
            }
        }
    }  // Qan.GraphView
    
    ResizeArea{
        minimumHeight: 200
        minimumWidth: 400
    }
}


