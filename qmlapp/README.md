# QMLAPP (Qt5/C++)

	It is a QML file interpreter, the JavaScript-based language developed by Qt, its function is to convert the code into a usable program.

## ¿How does it work?
	 [Steps to follow]
	    Step 1: Create a example.qml file.
	    Example:
		import QtQuick 2.5
		import QtQuick.Controls 2.5

		ApplicationWindow {
			visible: true
			width: 640
			height: 480
			title: "My QML App"
			Rectangle {
				width: 200
				height: 100
				color: "lightblue"
				Text {
					anchors.centerIn: parent
					text: "Hello, QML!"}}}

	    Step 2: Open terminal in linux and type qmlapp example.qml.

## Usage
	SetEnv: QML_IMPORT_PATH=PATH/TO/DIRECTORY variable used to import module directory.
		ENABLE_DEBUG=true Component debugging.
	
	Use variable in qml.txt file:
		appname: Instance name [example: test]
		version: Instance version [example: 0]
		window: Type of window to use in the instance [example: host]
		image: icon file
		module: path/to/directory

	qmlapp -h/--help, help menu, -i import path, --force quit instance.

## Installation
	git clone https://github.com/frerd7/Project.git
	cd ./Project/qmlapp
	make && sudo make install
 
