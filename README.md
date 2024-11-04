# coreSw - Native C++ Template Project Inspired by Qt

**coreSw** is a native C++ library inspired by the Qt framework, aiming to provide a standalone, lightweight toolkit with key features commonly found in Qt, but without any Qt dependencies. The project is fully compatible with C++11, making it accessible for a wide range of environments.

## Features

### Event Loop
coreSw includes a fully functional event loop, enabling efficient management of asynchronous events and callbacks. This event loop serves as the foundation for responsive applications and can be used in both console and GUI contexts.

### CoreApplication & GuiApplication
- **CoreApplication**: Designed for console applications, `CoreApplication` provides a core entry point with basic event management, allowing for asynchronous operations and command-line utility support.
- **GuiApplication**: Extending the functionality of `CoreApplication`, `GuiApplication` is tailored for graphical applications. It provides the framework for window management and event handling for interactive GUI components, similar to `QApplication` in Qt.

### Object System with Signal-Slot Mechanism
At the heart of coreSw is an `Object` class that supports a dynamic property system and a signal-slot mechanism. This allows objects to communicate with each other asynchronously, mimicking Qt’s signal-slot architecture without dependencies. The signal-slot system enables modular and decoupled design, making it easier to handle complex interactions within the application.

### UI Components
coreSw includes a set of essential UI components:
- **Widget**: The base class for all visual components in coreSw. It provides the fundamental structure for creating custom GUI elements.
- **PushButton, Label, LineEdit**: Basic widgets inspired by Qt's UI elements, allowing developers to build interactive interfaces.
- **MainWindow**: A main application window class to host and manage multiple widgets, similar to `QMainWindow` in Qt.

These UI components are fully integrated with the event loop, enabling seamless interaction and event handling for user interfaces.

### Styling System
The library includes a styling system that allows developers to customize the appearance of UI components. Inspired by Qt’s style sheets, coreSw's `SwStyle` system provides flexibility to define colors, fonts, and other visual properties, making it easier to create consistent and appealing UI designs.

### Process Management
The `SwProcess` class in coreSw allows for process management, enabling the launch and control of external executables. This feature is particularly useful for applications that need to interact with other software or scripts, providing robust integration with the operating system.

### Planned Features

1. **Networking Support**: Future plans include adding a TCP socket and TCP server class to enable network communication, facilitating client-server architectures and network-based applications.
2. **Extended UI Components**: Additional UI widgets and layout options will be introduced, expanding the capabilities of the library for more complex user interfaces.
3. **Enhanced Styling Options**: Planned improvements to the styling system will offer more customization options, allowing developers greater control over the look and feel of their applications.

## Goals

- **Lightweight and Standalone**: coreSw is designed to bring Qt-like functionality to C++ applications without the overhead of Qt dependencies.
- **Modular Design**: The library is built with modularity in mind, making it easy to extend or customize components.
- **Cross-Platform Compatibility**: Although optimized for Windows, coreSw is intended to be portable and adaptable for other platforms in future releases.

coreSw provides a Qt-inspired development experience in pure C++, empowering developers to create console and GUI applications with familiar design patterns, while maintaining flexibility and control over their application’s architecture.
