# coreSw - Native C++ Template Project Inspired by Qt

**coreSw** is a native C++ library inspired by the Qt framework, aiming to provide a standalone, lightweight toolkit with key features commonly found in Qt, but without any Qt dependencies. The project is fully compatible with C++11, making it accessible for a wide range of environments.

## Features

### JSON, Cryptography, and Custom Types

coreSw extends beyond what Qt offers in key areas like JSON handling, cryptography, and generic type management, providing advanced features for secure data manipulation and runtime flexibility:

#### **SwJsonDocument**: JSON Navigation and Encryption
SwJsonDocument introduces a powerful hierarchical JSON management system. It offers:
- **Flexible Navigation**: Using the `find("path/to/key", createIfNotExist)` method, developers can efficiently navigate deep into a JSON tree and retrieve references to specific `SwJsonValue` nodes. This eliminates the need for repetitive manual traversal and allows modifications directly on the referenced JSON node.
- **Selective Encryption**: Values within the JSON document can be encrypted natively, while the keys and overall tree structure remain visible. This is particularly useful for applications requiring partial data obfuscation or secure storage of sensitive information. The encryption leverages **SwCrypto** (explained below) for robust AES-256 encryption.
- **Encrypting the Entire JSON Tree**: To encrypt the entire JSON document, developers can use the `toJson()` method, which returns a `SwString`. By simply calling the `encryptAES("myKey")` method on the resulting `SwString`, the entire JSON content can be securely encrypted before being written to a file. This ensures that all data within the JSON tree is protected while maintaining compatibility with other SwCrypto features.


#### **SwCrypto**: Advanced Encryption Module
SwCrypto is a static cryptographic library providing secure AES-256 encryption and decryption. Unlike Qt, which lacks native high-level cryptographic support, SwCrypto is tightly integrated into coreSw’s ecosystem. For instance:
- `SwJsonDocument` uses SwCrypto to encrypt JSON values seamlessly.
- `SwString` utilizes SwCrypto for string-level encryption and decryption, allowing developers to secure sensitive textual data efficiently.
- All cryptographic methods are static, making them lightweight and easy to use without requiring additional setup.

#### **SwFile**: Metadata and Integrity Management
SwFile is a file management module that goes beyond standard file operations by adding:
- **Metadata Handling**: Developers can read, write, and update metadata associated with files directly.
- **Integrity Verification**: The module provides a built-in mechanism to generate and verify file checksums using SHA-256, ensuring the integrity and authenticity of file contents.
- **File Monitoring**: SwFile includes a signal that allows developers to monitor changes to a file. This feature enables applications to react dynamically to file modifications, making it particularly useful for real-time systems or scenarios where file content is updated externally.

#### **SwString**: Enhanced String Management
SwString offers features similar to Qt’s QString and adds significant improvements from conventional std::string:
- **String-Level Encryption**: SwString makes it straightforward to encrypt and decrypt strings with AES-256, automatically encoding encrypted data for storage or transmission.
- **Ease of Use**: As QString, SwString maintains simplicity and compatibility with standard C++ string operations while extending functionality for secure applications.

#### **SwAny**: Generic Runtime Container
SwAny addresses a critical gap in C++17 and more by allowing storage and manipulation of custom types at runtime:
- **Custom Type Registration at Compile-Time**: Developers can register custom types at compile-time using `SwAny::registerMetaType<T>()`. This allows seamless integration of user-defined types, making them compatible with the generic SwAny container. This is ideal for situations where type definitions are known during development and need to be consistently handled in a type-safe manner.
- **Dynamic Type Handling at Runtime**: For cases where types are unknown at compile-time, SwAny provides the `SwAny::fromVoidPtr(void*, "typeName")` method. This feature allows you to store and manage pointers to dynamically created types, along with their associated type name. By leveraging this mechanism, SwAny can act as a generic container that enables runtime flexibility. Developers can safely handle and retrieve these unknown types later using the stored metadata.
- **Generic Type Storage**: SwAny acts as a highly flexible container for storing pointers to types unknown at compile-time. This capability is invaluable for creating generic systems or plugins where the type information is only available during runtime.

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
