#pragma once

#include "SwCoreApplication.h"

#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <future>
#include "SwAny.h"





#define VIRTUAL_PROPERTY(type, PROP_NAME) \
public: \
    /* Déclaration du setter virtuel pur */ \
    virtual void set##PROP_NAME(const type& value) = 0; \
    /* Déclaration du getter virtuel pur */ \
    virtual type get##PROP_NAME() const = 0;


#define CUSTOM_OVERRIDE_PROPERTY(type, PROP_NAME, defautValue) \
private: \
    type m_##PROP_NAME = defautValue; \
public: \
    /* Setter with change check, user-defined change method, and signal emission */ \
    void set##PROP_NAME(const type& value) override { \
        if (m_##PROP_NAME != value) { \
            m_##PROP_NAME = value; \
            on_##PROP_NAME##_changed(value); \
            emit PROP_NAME##Changed(value); \
        } \
    } \
    /* Getter for property */ \
    type get##PROP_NAME() const override { \
        return m_##PROP_NAME; \
    } \
    void register_##PROP_NAME##_setter() { \
        propertyArgumentTypeNameMap[#PROP_NAME] = typeid(type).name(); \
        propertySetterMap[#PROP_NAME] = [this](void* value) { \
        this->set##PROP_NAME(*static_cast<type*>(value)); \
        }; \
        propertyGetterMap[#PROP_NAME] = [this]() -> void* { \
            return static_cast<void*>(&this->m_##PROP_NAME); \
        }; \
    } \
signals: \
    /* Signal declaration */ \
    DECLARE_SIGNAL(PROP_NAME##Changed, const type&);\
protected: \
    virtual void on_##PROP_NAME##_changed(const type& value)

#define CUSTOM_PROPERTY(type, PROP_NAME, defautValue) \
private: \
    type m_##PROP_NAME = defautValue; \
public: \
    /* Setter with change check, user-defined change method, and signal emission */ \
    void set##PROP_NAME(const type& value) { \
        if (m_##PROP_NAME != value) { \
            m_##PROP_NAME = value; \
            on_##PROP_NAME##_changed(value); \
            emit PROP_NAME##Changed(value); \
        } \
    } \
    /* Getter for property */ \
    type get##PROP_NAME() const { \
        return m_##PROP_NAME; \
    } \
    void register_##PROP_NAME##_setter() { \
        propertyArgumentTypeNameMap[#PROP_NAME] = typeid(type).name(); \
        propertySetterMap[#PROP_NAME] = [this](void* value) { \
        this->set##PROP_NAME(*static_cast<type*>(value)); \
        }; \
        propertyGetterMap[#PROP_NAME] = [this]() -> void* { \
            return static_cast<void*>(&this->m_##PROP_NAME); \
        }; \
    } \
signals: \
    /* Signal declaration */ \
    DECLARE_SIGNAL(PROP_NAME##Changed, const type&);\
protected: \
    virtual void on_##PROP_NAME##_changed(const type& value)



#define PROPERTY(type, PROP_NAME, defautValue) \
    CUSTOM_PROPERTY(type, PROP_NAME, defautValue) { SW_UNUSED(value); }

#define SURCHARGE_ON_PROPERTY_CHANGED(type, PROP_NAME) \
protected: \
    virtual void on_##PROP_NAME##_changed(const type& value) override

#define REGISTER_PROPERTY(PROP_NAME) \
    addPropertyRegistrar([this]() { register_##PROP_NAME##_setter(); });




//#ifndef signals
#define signals public
//#endif

//#ifndef slots
#define slots
//#endif

//#ifndef SIGNAL
#define SIGNAL(signalName) #signalName
//#endif

//#ifndef SLOT
#define SLOT(slotName) #slotName
//#endif

#define DECLARE_SIGNAL(signalName, ...) \
    template <typename... Args> \
    void signalName(Args&&... args) { emitSignal(#signalName, std::forward<Args>(args)...); }

#define DECLARE_SLOT(slotName, ...) \
    void slotName(__VA_ARGS__)

#define emit 







// Macro SW_OBJECT pour générer className() et classHierarchy()
#define SW_OBJECT(DerivedClass, BaseClass)                              \
public:                                                                 \
    virtual SwString className() const override {                      \
        SwString fullName = typeid(DerivedClass).name();               \
        int spaceIndex = fullName.indexOf(' ');                        \
        if (spaceIndex != -1) {                                        \
            return fullName.mid(spaceIndex + 1);                       \
    }                                                              \
        return fullName;                                               \
}                                                                  \
    virtual std::vector<SwString> classHierarchy() const override {    \
        std::vector<SwString> hierarchy = BaseClass::classHierarchy(); \
        hierarchy.insert(hierarchy.begin(), DerivedClass::className());\
        return hierarchy;                                              \
}




#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

enum ConnectionType {
    DirectConnection,
    QueuedConnection,
    BlockingQueuedConnection
};

template<typename T, typename... Args>
class ISlot {
public:
    virtual ~ISlot() {}
    virtual void invoke(T* instance, Args... args) = 0;
    virtual T* receiveur() = 0;
};

// Slot pour une méthode membre
template <typename T, typename... Args>
class SlotMember : public ISlot<T, Args...> {
public:
    SlotMember(T* instance, void (T::* method)(Args...)) : instance(instance), method(method) {}

    void invoke(T* instance, Args... args) override {
        (instance->*method)(args...);
    }

    T* receiveur() override {
        return instance;
    }

private:
    T* instance;
    void (T::* method)(Args...);
};

// Slot fonctionnel (lambda, std::function)
template <typename... Args>
class SlotFunction : public ISlot<void, Args...> {
public:
    explicit SlotFunction(std::function<void(Args...)> func) : func(std::move(func)) {}

    void invoke(void*, Args... args) override {
        func(args...);
    }

    void* receiveur() override {
        return nullptr;
    }

private:
    std::function<void(Args...)> func;
};


// ============================================================================
//                          TRAITS DE FONCTION
// ============================================================================

// Extraction de la signature d'un callable (lambda, functor, fonction)
template <class F>
struct function_traits : function_traits<decltype(&F::operator())> {};

// Pour une fonction de la forme R(Args...)
template <class R, class... Args>
struct function_traits<R(Args...)> {
    using return_type = R;
    // On utilise std::decay_t pour éviter les références et const
    using args_tuple = std::tuple<std::decay_t<Args>...>;
    using std_function_type = std::function<R(Args...)>;
};


template <class R, class... Args>
struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const> : function_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)> : function_traits<R(Args...)> {};


// ============================================================================
//               FONCTION AUXILIAIRE POUR DEPLIER UN TUPLE EN PACK
// ============================================================================

template<class Tuple, class F, std::size_t... I>
auto apply_tuple_impl(Tuple&&, F&& f, std::index_sequence<I...>) {
    // On appelle f avec un pack de types déduit depuis Tuple
    // f est un foncteur template<...Args> ISlot<void, Args...>* operator()()
    return f.template operator()<typename std::tuple_element<I, Tuple>::type...>();
}

template<class Tuple, class F>
auto apply_tuple(Tuple&& t, F&& f) {
    return apply_tuple_impl(
        std::forward<Tuple>(t),
        std::forward<F>(f),
        std::make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{}
    );
}


// ============================================================================
//              FABRIQUE DE SLOT POUR LAMBDA/FONCTION GENERIQUE
// ============================================================================
template<class Func>
struct slot_factory {
    Func func;
    slot_factory(Func&& f) : func(std::forward<Func>(f)) {}

    // Operator() template qui va être appelé par apply_tuple avec Args...
    template<typename... A>
    ISlot<void, A...>* operator()() {
        using Fn = std::function<void(A...)>;
        Fn f(func); // Conversion implicite vers std::function
        return new SlotFunction<A...>(std::move(f));
    }
};



class Object {
    PROPERTY(SwString, ObjectName, "")

public:

    /**
     * @brief Constructor that initializes the Object with an optional parent.
     *
     * Registers the `ObjectName` property and sets the parent of the current object,
     * establishing its position in the object hierarchy.
     *
     * @param parent Pointer to the parent Object. Defaults to nullptr if no parent is specified.
     */
    Object(Object* parent = nullptr) :
          m_parent(nullptr)
        , ArePropertiesInitialised(false)
    {
        REGISTER_PROPERTY(ObjectName);

        // Enregistrement dans la map
        setParent(parent);
    }

    /**
     * @brief Virtual destructor for the Object class.
     *
     * Currently, this destructor does not perform any specific cleanup tasks.
     * It is designed to allow proper cleanup in derived classes, including disconnecting slots
     * and deleting child objects if necessary (commented out here for customization).
     */
    virtual ~Object() {
        //emit destroyed();
        //for (auto child : children) {
        //    if (child->m_parent == this) {
        //        delete child;
        //    }
        //}
        //disconnectAllSlots();
    }

    /**
     * @brief Checks if the given object is derived from a specified base class.
     *
     * @tparam Base The base class to check against.
     * @tparam Derived The derived class type of the object.
     * @param obj Pointer to the object to check.
     * @return true if the object is of type Base or derived from it, false otherwise.
     */
    template <typename Base, typename Derived>
    bool inherits(const Derived* obj) {
        return dynamic_cast<const Base*>(obj) != nullptr;
    }

    virtual SwString className() const {
        SwString fullName = typeid(Object).name();
        int spaceIndex = fullName.indexOf(' ');
        if (spaceIndex != -1) {
            return fullName.mid(spaceIndex + 1);
        }
        return fullName;
    }


    /**
     * @brief Retrieves the class name of the object.
     *
     * This method uses `typeid` to get the full type name and extracts the class name.
     *
     * @return SwString The name of the class.
     */
    virtual std::vector<SwString> classHierarchy() const {
        return { Object::className() };
    }

    /**
     * @brief Marks the object for deletion in the next event loop iteration.
     *
     * This method schedules the deletion of the object using `SwCoreApplication::postEvent`.
     * The actual deletion occurs asynchronously, ensuring that the object is safely
     * removed without disrupting the current execution flow.
     */
    void deleteLater() {
        Object* meAsAShitToClean = this;
        SwCoreApplication::instance()->postEvent([meAsAShitToClean]() {
            delete meAsAShitToClean;
        });
    }

    /**
     * @brief Safely deletes a pointer and sets it to nullptr.
     *
     * @tparam T The type of the pointer to delete.
     * @param _refPtr A reference to the pointer to be deleted. After deletion, it will be set to nullptr.
     */
    template <typename T>
    static void safeDelete(T*& _refPtr) {
        if (_refPtr) {
            delete _refPtr;
            _refPtr = nullptr;
        }
    }

    /**
     * @brief Sets a new parent for the object and updates the parent-child relationship.
     *
     * If the object already has a parent, it removes itself from the previous parent's children list.
     * It then adds itself to the new parent's children list and triggers the `newParentEvent` to handle
     * any custom behavior related to the parent change.
     *
     * @param parent The new parent object. Can be nullptr to detach the object from its current parent.
     */
    void setParent(Object *parent) 
    {
        if (m_parent) {
            m_parent->removeChild(this);
        }
        m_parent = parent;
        if (m_parent) {
            m_parent->addChild(this);
        }
        newParentEvent(parent);
    }

    /**
     * @brief Retrieves the parent of the current object.
     *
     * @return A pointer to the parent object, or nullptr if the object has no parent.
     */
    Object *parent() { return m_parent; }

    /**
     * @brief Adds a child object to the current object.
     *
     * This method appends the specified child to the list of children, emits a `childAdded` signal,
     * and triggers the `addChildEvent` to allow derived classes to handle additional logic.
     *
     * @param child The child object to add.
     */
    virtual void addChild(Object* child) {
        children.push_back(child);
        emit childAdded(child);
        addChildEvent(child);
    }

    /**
     * @brief Removes a child object from the current object's children list.
     *
     * This method removes the specified child from the list of children, emits a `childRemoved` signal,
     * and triggers the `removedChildEvent` to allow derived classes to handle additional logic.
     *
     * @param child The child object to remove.
     */
    void removeChild(Object* child) {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
        emit childRemoved(child);
        removedChildEvent(child);
    }

    /**
     * @brief Finds all child objects of a specific type, including nested children.
     *
     * This method searches for children of the current object that are of the specified type `T`.
     * The search is recursive, meaning it also checks the children of each child object.
     *
     * @tparam T The type of objects to find.
     * @return A vector of pointers to all child objects of type `T`.
     */
    template <typename T>
    std::vector<T*> findChildren() const {
        std::vector<T*> result;
        for (auto child : children) {
            // Utiliser dynamic_cast pour vérifier si l'enfant est du type T
            if (T* castedChild = dynamic_cast<T*>(child)) {
                result.push_back(castedChild);
            }

            // Si l'enfant est un Object, on effectue la recherche récursive
            if (auto objectChild = dynamic_cast<Object*>(child)) {
                // Recherche récursive dans les enfants de l'enfant
                std::vector<T*> nestedResults = objectChild->findChildren<T>();
                result.insert(result.end(), nestedResults.begin(), nestedResults.end());
            }
        }
        return result;
    }

    /**
     * @brief Retrieves all direct child objects of the current object.
     *
     * This method returns a constant reference to the vector containing the direct children
     * of the current object. The vector does not include nested children.
     *
     * @return A constant reference to the vector of child objects.
     */
    const std::vector<Object*>& getChildren() const {
        return children;
    }

    /**
     * @brief Handles the event triggered when a child object is added.
     *
     * This virtual method is called whenever a child object is added to the current object.
     * It can be overridden in derived classes to perform specific actions when a child is added.
     *
     * @param child Pointer to the object that was added as a child.
     */
    virtual void addChildEvent(Object* child) { SW_UNUSED(child) }

    /**
     * @brief Handles the event triggered when a child object is removed.
     *
     * This virtual method is called whenever a child object is removed from the current object.
     * It can be overridden in derived classes to perform specific actions when a child is removed.
     *
     * @param child Pointer to the object that was removed as a child.
     */
    virtual void removedChildEvent(Object* child) { SW_UNUSED(child)  }

    /**
     * @brief Connects a signal from a sender object to a slot in a receiver object.
     *
     * Establishes a connection between a signal emitted by the sender and a member function
     * (slot) of the receiver. The connection type determines how the signal is handled.
     *
     * @param sender Pointer to the sender object emitting the signal.
     * @param signalName Name of the signal to connect.
     * @param receiver Pointer to the receiver object receiving the signal.
     * @param slot Pointer to the receiver's member function (slot).
     * @param type Type of connection (e.g., DirectConnection, QueuedConnection, BlockingQueuedConnection). Default is DirectConnection.
     */
    template<typename Sender, typename Receiver, typename... Args>
    static void connect(Sender* sender, const SwString& signalName, Receiver* receiver, void (Receiver::* slot)(Args...), ConnectionType type = DirectConnection) {
        ISlot<Receiver, Args...>* newSlot = new SlotMember<Receiver, Args...>(receiver, slot);
        sender->addConnection(signalName, newSlot, type);
    }

    /**
     * @brief Connects a signal from a sender object to a standalone function or lambda.
     *
     * Establishes a connection between a signal emitted by the sender and a function or lambda.
     * The connection type determines how the signal is handled.
     *
     * @param sender Pointer to the sender object emitting the signal.
     * @param signalName Name of the signal to connect.
     * @param func The function or lambda to be executed when the signal is emitted.
     * @param type Type of connection (e.g., DirectConnection, QueuedConnection, BlockingQueuedConnection). Default is DirectConnection.
     */
    template<typename Sender, typename... Args>
    static void connect(Sender* sender, const SwString& signalName, std::function<void(Args...)> func, ConnectionType type = DirectConnection) {
        ISlot<void, Args...>* newSlot = new SlotFunction<Args...>(func);
        sender->addConnection(signalName, newSlot, type);
    }

    /**
     * @brief Connects a signal to a lambda function.
     *
     * Establishes a connection between a signal emitted by the sender and a lambda function.
     *
     * @tparam Sender The type of the sender object emitting the signal.
     * @tparam Func The type of the lambda function to connect.
     * @param sender Pointer to the sender object emitting the signal.
     * @param signalName The name of the signal to connect.
     * @param func The lambda to execute when the signal is emitted.
     * @param type Type of connection (e.g., DirectConnection, QueuedConnection, BlockingQueuedConnection).
     *             Default is DirectConnection.
     */
    template <typename SenderType, typename Func>
    static void connect(SenderType* sender, const SwString& signalName, Func&& func, ConnectionType type = DirectConnection) {
        using traits = function_traits<typename std::decay<Func>::type>;
        using R = typename traits::return_type;
        using args_tuple = typename traits::args_tuple;

        static_assert(std::is_void<R>::value, "Seules les fonctions retournant void sont supportées.");

        // On utilise apply_tuple pour déplier le tuple d'arguments (Args...) de la fonction
        auto slot = apply_tuple(args_tuple{}, slot_factory<Func>(std::forward<Func>(func)));

        sender->addConnection(signalName, slot, type);
    }


    /**
     * @brief Connects a signal to a slot using modern pointer-to-member function syntax.
     *
     * This method aims to establish a connection between a signal and a slot in a type-safe way.
     * It uses pointer-to-member functions to explicitly define the signal and slot connections.
     * Note: This implementation is under development and may not work as expected in all cases.
     *
     * @tparam Sender The type of the sender object emitting the signal.
     * @tparam Receiver The type of the receiver object handling the signal.
     * @tparam SignalArgs The argument types of the signal.
     * @tparam SlotArgs The argument types of the slot.
     * @param sender Pointer to the sender object emitting the signal.
     * @param signal The pointer-to-member function representing the signal.
     * @param receiver Pointer to the receiver object handling the signal.
     * @param slot The pointer-to-member function representing the slot.
     * @param type Type of connection (e.g., DirectConnection, QueuedConnection, BlockingQueuedConnection).
     *             Default is DirectConnection.
     *
     * @note This function is designed for modern signal-slot connections but requires further refinement to handle
     *       cases where Sender or Receiver are derived classes or when argument types do not match exactly.
     */
    template<typename Sender, typename Receiver, typename... SignalArgs, typename... SlotArgs>
    static void connect(
        Sender* sender,
        void (Sender::*signal)(SignalArgs...),
        Receiver* receiver,
        void (Receiver::*slot)(SlotArgs...),
        ConnectionType type = DirectConnection
        ) {

        // // Cast du signal pour gérer le cas où Sender est une classe de base
        // auto castedSignal = static_cast<void (Sender::*)(SignalArgs...)>(signal);

        // // Création d'un slot adapté
        // ISlot<Receiver, SlotArgs...>* newSlot = new SlotMember<Receiver, SlotArgs...>(receiver, slot);

        // SwString signalName = signalNameFromPointer(castedSignal);

        // // Ajout de la connexion
        // sender->addConnection(signalName, newSlot, type);
    }

    /**
     * @brief Disconnects a specific slot from a signal of a sender object.
     *
     * Removes the connection between a signal emitted by the sender and a specific slot of the receiver.
     * If no slots remain connected to the signal, the signal is removed from the sender's connections.
     *
     * @param sender Pointer to the sender object emitting the signal.
     * @param signalName Name of the signal to disconnect from.
     * @param receiver Pointer to the receiver object whose slot is being disconnected.
     * @param slot The specific slot of the receiver to disconnect.
     */
    template<typename Sender, typename Receiver>
    static void disconnect(Sender* sender, const SwString& signalName, Receiver* receiver, void (Receiver::*slot)()) {
        // Vérifie si le signal existe
        if (sender->connections.find(signalName) != sender->connections.end()) {
            auto& slotsConnetion = sender->connections[signalName];
            slotsConnetion.erase(
                std::remove_if(slotsConnetion.begin(), slotsConnetion.end(),
                    [receiver, slot](const std::pair<void*, ConnectionType>& connection) {
                        // Vérifie si le slot correspond
                        auto* memberSlot = static_cast<SlotMember<Receiver>*>(connection.first);
                        return memberSlot && memberSlot->receiveur() == receiver && memberSlot->method == slot;
                    }),
                slotsConnetion.end()
            );

            // Si plus de slots, supprime l'entrée pour le signal
            if (slotsConnetion.empty()) {
                sender->connections.erase(signalName);
            }
        }
    }

    /**
     * @brief Disconnects all slots of a receiver from all signals of a sender object.
     *
     * Iterates through all signals of the sender and removes any connections associated with the receiver.
     * If no slots remain connected to a signal, the signal is removed from the sender's connections.
     *
     * @param sender Pointer to the sender object emitting the signals.
     * @param receiver Pointer to the receiver object whose slots are being disconnected.
     */
    template<typename Sender, typename Receiver>
    static void disconnect(Sender* sender, Receiver* receiver) {
        // Parcourt tous les signaux et déconnecte ceux associés au receiver
        for (auto it = sender->connections.begin(); it != sender->connections.end(); ) {
            auto& slotsConnetion = it->second;
            slotsConnetion.erase(
                std::remove_if(slotsConnetion.begin(), slotsConnetion.end(),
                    [receiver](const std::pair<void*, ConnectionType>& connection) {
                        // Vérifie si le slot correspond
                        auto* memberSlot = dynamic_cast<SlotMember<Receiver>*>(static_cast<ISlot<Receiver>*>(connection.first));
                        return memberSlot && memberSlot->receiveur() == receiver;
                    }),
                slotsConnetion.end()
            );

            // Si plus de slots pour ce signal, supprime l'entrée
            if (slotsConnetion.empty()) {
                it = sender->connections.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Adds a new connection for a signal.
     *
     * Associates a given slot with a signal name and connection type. If the signal does not already exist,
     * it creates a new entry for it.
     *
     * @tparam Args Variadic template parameters representing the types of the arguments for the slot.
     * @param signalName The name of the signal to connect.
     * @param slot Pointer to the slot to be associated with the signal.
     * @param type The type of connection (e.g., DirectConnection, QueuedConnection).
     */
    template<typename... Args>
    void addConnection(const SwString& signalName, ISlot<Args...>* slot, ConnectionType type) {
        if (connections.find(signalName) == connections.end()) {
            connections[signalName] = std::vector<std::pair<void*, ConnectionType>>();
        }
        connections[signalName].push_back(std::make_pair(static_cast<void*>(slot), type));
    }

    /**
     * @brief Emits a signal to invoke all connected slots.
     *
     * Invokes all slots connected to the specified signal. Handles different connection types such as
     * DirectConnection, QueuedConnection, and BlockingQueuedConnection.
     *
     * @tparam Args Variadic template parameters representing the types of the arguments for the signal.
     * @param signalName The name of the signal to emit.
     * @param args Arguments to pass to the connected slots.
     *
     * - DirectConnection: The slot is invoked immediately in the current thread.
     * - QueuedConnection: The slot is added to the event queue for later execution.
     * - BlockingQueuedConnection: The current thread is blocked until the slot is executed.
     */
    template<typename... Args>
    void emitSignal(const SwString& signalName, Args... args) {
        if (connections.size() > 0 && connections.find(signalName) != connections.end()) {
            for (auto& connection : connections[signalName]) {
                auto slotPtr = connection.first;
                auto type = connection.second;
                ISlot<void, Args...>* slot = static_cast<ISlot<void, Args...>*>(slotPtr);
                
                if (type == DirectConnection) {
                    if (slot->receiveur()) {
                        static_cast<Object*>(slot->receiveur())->setSender(this);
                    }
                    slot->invoke(slot->receiveur(), args...);
                }
                else if (type == QueuedConnection) {
                    // Mettre en file d'attente pour un traitement ultérieur
                    SwCoreApplication::instance()->postEvent([this, slot, args...]() {
                        if (slot->receiveur()) {
                            static_cast<Object*>(slot->receiveur())->setSender(this);
                        }   
                        slot->invoke(slot->receiveur(), args...);
                    });
                }
                else if (type == BlockingQueuedConnection) {
                    // Utiliser future pour bloquer jusqu'à ce que le slot soit exécuté
                    auto future = std::async(std::launch::async, [this, slot, args...]() {
                        if (slot->receiveur()) {
                            static_cast<Object*>(slot->receiveur())->setSender(this);
                        }                        
                        slot->invoke(slot->receiveur(), args...);
                    });
                    future.wait();
                }
            }
        }
    }

    /**
     * @brief Retrieves the current sender of the signal.
     *
     * Returns the pointer to the `Object` that emitted the signal. Useful in slot functions
     * to determine which object triggered the signal.
     *
     * @return Object* Pointer to the sender object, or `nullptr` if no sender is set.
     */
    Object* sender() {
        return currentSender;
    }

    /**
     * @brief Sets the current sender of the signal.
     *
     * This function assigns the sender object for the currently emitted signal.
     * It is used internally when emitting signals to track the originating object.
     *
     * @param _sender Pointer to the `Object` that emits the signal.
     */
    void setSender(Object* _sender) {
        currentSender = _sender;
    }

    /**
     * @brief Disconnects all slots connected to this object.
     *
     * This function clears all signal-slot connections associated with this object.
     * It ensures that no slots remain connected to any of its signals.
     *
     * If any connections exist, they are removed, and a message is logged.
     */
    void disconnectAllSlots() {
        if (connections.size() > 0) {
            connections.clear();
            std::cout << "Tous les slots ont été déconnectés pour cet objet." << std::endl;
        }
    }

    /**
     * @brief Disconnects all slots associated with a specific receiver.
     *
     * This function iterates through all connections of this object and removes any
     * slots that are associated with the provided receiver object.
     *
     * @tparam Receiver The type of the receiver object.
     * @param receiver A pointer to the receiver object whose slots need to be disconnected.
     *
     * Logs a message indicating that all slots linked to the receiver have been disconnected.
     */
    template <typename Receiver>
    void disconnectReceiver(Receiver* receiver) {
        for (auto it = connections.begin(); it != connections.end(); ++it) {
            std::vector<std::pair<void*, ConnectionType>>& currentSlots = it->second;
            currentSlots.erase(std::remove_if(currentSlots.begin(), currentSlots.end(),
                [receiver](std::pair<void*, ConnectionType>& slotPair) {
                    SlotMember<Receiver>* slot = static_cast<SlotMember<Receiver>*>(slotPair.first);
                    return slot && slot->receiveur() == receiver;
                }),
                currentSlots.end());
        }
        std::cout << "Tous les slots liés au receiver ont été déconnectés." << std::endl;
    }

    /**
     * @brief Sets the value of a specified property.
     *
     * This function assigns a new value to a property identified by its name. If the property
     * is not initialized, it triggers the registration of all properties. It verifies if the
     * property exists and if the provided value matches the expected type before setting it.
     *
     * @param propertyName The name of the property to be updated.
     * @param value The new value to assign to the property, encapsulated in a SwAny object.
     *
     * @note Logs a message if the property is not found or if the value type does not match the expected type.
     */
    void setProperty(const SwString& propertyName, SwAny value) {
        if (!ArePropertiesInitialised) {
            ArePropertiesInitialised = registerProperties();
        }

        // Vérifier si la propriété existe dans la map
        if (propertySetterMap.find(propertyName) != propertySetterMap.end()) {
            // Appel du setter via SwAny
            if (propertyArgumentTypeNameMap[propertyName] == value.typeName()) {
                propertySetterMap[propertyName](value.data());
            } else {
                std::cout << "Oula jeune fou, la propriété que tu souhaite mettre ne matche pas avec le type: " << propertyArgumentTypeNameMap[propertyName] << std::endl;
            }
        }
        else {
            std::cout << "Property not found: " << propertyName << std::endl;
        }
    }

    /**
     * @brief Retrieves the value of a specified property.
     *
     * This function accesses a property by its name and returns its value encapsulated in a SwAny object.
     * If the properties have not been initialized, it triggers their registration.
     * If the property exists, its getter function is called to obtain the value,
     * which is then converted to a SwAny object.
     *
     * @param propertyName The name of the property to retrieve.
     * @return SwAny The value of the property if found, or an empty SwAny object if the property does not exist.
     *
     * @note Logs a message if the property is not found.
     */
    SwAny property(const SwString& propertyName) {
        SwAny retValue;

        // Initialiser les propriétés si nécessaire
        if (!ArePropertiesInitialised) {
            ArePropertiesInitialised = registerProperties();
        }

        // Vérifier si la propriété existe dans la map
        auto it = propertyGetterMap.find(propertyName);
        if (it != propertyGetterMap.end()) {
            // Appeler la fonction getter pour récupérer la valeur
            void* valuePtr = it->second();  // Appelle la lambda pour obtenir un pointeur générique
            const SwString& typeName = propertyArgumentTypeNameMap[propertyName];  // Obtenir le nom du type

            // Créer un SwAny à partir du pointeur et du type
            retValue = SwAny::fromVoidPtr(valuePtr, typeName);
        }
        else {
            std::cout << "Property not found: " << propertyName << std::endl;
        }

        return retValue;
    }

    /**
     * @brief Checks whether a specified property exists.
     *
     * This function verifies the existence of a property by its name.
     * If the properties have not been initialized, it triggers their registration.
     *
     * @param propertyName The name of the property to check.
     * @return bool `true` if the property exists, otherwise `false`.
     */
    bool propertyExist(const SwString& propertyName) {
        if (!ArePropertiesInitialised) {
            ArePropertiesInitialised = registerProperties();
        }

        auto it = propertyGetterMap.find(propertyName);
        return (it != propertyGetterMap.end());
    }

protected:

    /**
     * @brief Handles events triggered when a new parent is assigned.
     *
     * This function is called whenever the object is assigned a new parent.
     * It can be overridden to implement custom behavior for handling parent changes.
     *
     * @param parent The new parent object assigned to this object.
     */
    virtual void newParentEvent(Object* parent) { SW_UNUSED(parent) }

    /**
     * @brief Adds a property registrar function.
     *
     * This function registers a callback that initializes a property.
     * It is typically used during the registration process of dynamic properties.
     *
     * @param func A function that initializes a specific property.
     */
    void addPropertyRegistrar(const std::function<void()>& func) {
        propertyRegistrars.push_back(func);
    }

    /**
     * @brief Registers all properties using the registered property registrar functions.
     *
     * This function iterates over all the registered property registrar functions
     * and executes each one to initialize the properties.
     *
     * @return true Always returns true after successfully executing all property initializations.
     */
    bool registerProperties() {
        for (const auto& func : propertyRegistrars) {
            func();
        }
        return true;
    }

    /**
     * @brief Attempts to extract the name of a signal from a pointer-to-member function.
     *
     * This function uses `typeid` to deduce the type information of the signal. It aims to
     * return the signal's name as a string. However, this implementation does not provide
     * a reliable or accurate signal name in its current state.
     *
     * @tparam Signal The type of the signal pointer.
     * @param signal The pointer-to-member function representing the signal.
     * @return SwString The deduced signal name (currently unreliable).
     *
     * @note This implementation is incomplete. The returned name is derived from
     *       `typeid`, which includes mangled type information and does not
     *       correspond to the actual signal name. Further refinement is needed to
     *       handle demangling or mapping to human-readable names.
     */
    template <typename Signal>
    static SwString signalNameFromPointer(Signal signal) {
        // Extraire le nom de la méthode membre
        std::string fullName = typeid(signal).name();
        // Nettoyer ou adapter si nécessaire (selon vos conventions)
        return SwString(fullName.c_str());
    }



    std::map<SwString, std::function<void(void*)>> propertySetterMap;
    std::map<SwString, std::function<void*()>> propertyGetterMap;
    std::map<SwString, SwString> propertyArgumentTypeNameMap;
    std::vector<std::function<void()>> propertyRegistrars;

signals:
    DECLARE_SIGNAL(childRemoved)
    DECLARE_SIGNAL(childAdded)

private:
    Object* m_parent = nullptr;
    std::vector<Object*> children;
    bool ArePropertiesInitialised;
    SwString objectName;
    std::map<SwString, SwString> properties;
    std::map<SwString, std::vector<std::pair<void*, ConnectionType>>> connections;
    Object* currentSender = nullptr;
};




