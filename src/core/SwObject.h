#pragma once
/***************************************************************************************************
 * This file is part of a project developed by Ariya Consulting and Eymeric O'Neill.
 *
 * Copyright (C) [year] Ariya Consulting
 * Author/Creator: Eymeric O'Neill
 * Contact: +33 6 52 83 83 31
 * Email: eymeric.oneill@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ***************************************************************************************************/

#include "SwCoreApplication.h"

#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <future>
#include "SwAny.h"
#include <tuple>
#include <type_traits>
#include <utility>




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

#define CUSTOM_PROPERTY(__prop_type__, __prop_name__, __prop_default_value__) \
private: \
    __prop_type__ m_##__prop_name__ = __prop_default_value__; \
    public: \
    void set##__prop_name__(const __prop_type__& value) { \
        if (m_##__prop_name__ != value) { \
            m_##__prop_name__ = value; \
            on_##__prop_name__##_changed(value); \
            emit __prop_name__##Changed(value); \
    } \
} \
    __prop_type__ get##__prop_name__() const { \
        return m_##__prop_name__; \
} \
    template<typename T> \
    static bool register_##__prop_name__##_setter(T* instance) { \
        instance->propertySetterMap[#__prop_name__] = [instance](void* value) { \
                  instance->set##__prop_name__(*static_cast<__prop_type__*>(value)); \
          }; \
        instance->propertyGetterMap[#__prop_name__] = [instance]() -> void* { \
            return static_cast<void*>(&instance->m_##__prop_name__); \
        }; \
        instance->propertyArgumentTypeNameMap[#__prop_name__] = typeid(__prop_type__).name(); \
        return true; \
} \
    DECLARE_SIGNAL(__prop_name__##Changed, const __prop_type__&); \
    protected: \
    bool __##__prop_name__##__prop = register_##__prop_name__##_setter<typename std::remove_reference<decltype(*this)>::type>(this); \
    virtual void on_##__prop_name__##_changed(const __prop_type__& value)



#define PROPERTY(type, PROP_NAME, defautValue) \
    CUSTOM_PROPERTY(type, PROP_NAME, defautValue) { SW_UNUSED(value); }

#define SURCHARGE_ON_PROPERTY_CHANGED(type, PROP_NAME) \
protected: \
    virtual void on_##PROP_NAME##_changed(const type& value) override





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

//#ifndef emit
#define emit
//#endif


#define DECLARE_SIGNAL(signalName, ...) \
template <typename... Args> \
    void signalName(Args&&... args) { \
        emitSignal(#signalName, std::forward<Args>(args)...); \
} \
    /* On définit d'abord la fonction template qui enregistre le pointeur de fonction */ \
    template<typename T> \
    static auto __##signalName##__addr(T* instance) { \
        using __type_name_##signalName = typename std::remove_reference<decltype(*instance)>::type; \
        instance->__nameToFunction__[#signalName] = toVoidPtr((void(__type_name_##signalName::*)())&__type_name_##signalName::signalName); \
        return true; \
} \
    /* Maintenant qu'elle est définie, on peut l'appeler pour initialiser ___signalName___ */ \
    bool ___##signalName##___ = __##signalName##__addr(this); \
    /* Déclaration de la fonction invoke_##signalName */ \
    template <typename... Args> \
    void invoke_##signalName(Args&&... args) { \
        auto it = __nameToFunction__.find(#signalName); \
        if (it != __nameToFunction__.end()) { \
            using __type_name_##signalName = typename std::remove_reference<decltype(*this)>::type; \
            decltype(&__type_name_##signalName::signalName) f = fromVoidPtr<decltype(&__type_name_##signalName::signalName)>(it->second); \
            (this->*f)(); \
    }\
}










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

// Slot fonctionnel avec receiver (this) et lambda
template <typename T, typename... Args>
class SlotFunctionReceiver : public ISlot<T, Args...> {
public:
    SlotFunctionReceiver(T* receiver, std::function<void(Args...)> func)
        : receiver_(receiver), func_(std::move(func)) {}

    void invoke(T*, Args... args) override {
        func_(args...);
    }

    T* receiveur() override {
        return receiver_;
    }

private:
    T* receiver_;
    std::function<void(Args...)> func_;
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

template<class ReceiverType, class Func>
struct slot_factory_with_receiver {
    ReceiverType* receiver;
    Func func;

    slot_factory_with_receiver(ReceiverType* r, Func&& f) : receiver(r), func(std::forward<Func>(f)) {}

    template<typename... A>
    ISlot<ReceiverType, A...>* operator()() {
        using Fn = std::function<void(A...)>;
        Fn f(func); // Conversion en std::function
        return new SlotFunctionReceiver<ReceiverType, A...>(receiver, std::move(f));
    }
};



class SwObject {
protected:
    std::map<std::string, void*> __nameToFunction__;
    std::map<SwString, std::function<void(void*)>> propertySetterMap;
    std::map<SwString, std::function<void*()>> propertyGetterMap;
    std::map<SwString, SwString> propertyArgumentTypeNameMap;

    PROPERTY(SwString, ObjectName, "")

public:

    /**
     * @brief Constructor that initializes the SwObject with an optional parent.
     *
     * Registers the `ObjectName` property and sets the parent of the current SwObject,
     * establishing its position in the SwObject hierarchy.
     *
     * @param parent Pointer to the parent Object. Defaults to nullptr if no parent is specified.
     */
    SwObject(SwObject* parent = nullptr) :
          m_parent(nullptr)
    {
        setParent(parent);
    }

    /**
     * @brief Virtual destructor for the SwObject class.
     *
     * Currently, this destructor does not perform any specific cleanup tasks.
     * It is designed to allow proper cleanup in derived classes, including disconnecting slots
     * and deleting child objects if necessary (commented out here for customization).
     */
    virtual ~SwObject() {
        //emit destroyed();
        //for (auto child : children) {
        //    if (child->m_parent == this) {
        //        delete child;
        //    }
        //}
        //disconnectAllSlots();
    }

    /**
     * @brief Checks if the given SwObject is derived from a specified base class.
     *
     * @tparam Base The base class to check against.
     * @tparam Derived The derived class type of the SwObject.
     * @param obj Pointer to the SwObject to check.
     * @return true if the SwObject is of type Base or derived from it, false otherwise.
     */
    template <typename Base, typename Derived>
    bool inherits(const Derived* obj) {
        return dynamic_cast<const Base*>(obj) != nullptr;
    }

    virtual SwString className() const {
        SwString fullName = typeid(SwObject).name();
        int spaceIndex = fullName.indexOf(' ');
        if (spaceIndex != -1) {
            return fullName.mid(spaceIndex + 1);
        }
        return fullName;
    }


    /**
     * @brief Retrieves the class name of the SwObject.
     *
     * This method uses `typeid` to get the full type name and extracts the class name.
     *
     * @return SwString The name of the class.
     */
    virtual std::vector<SwString> classHierarchy() const {
        return { SwObject::className() };
    }

    /**
     * @brief Marks the SwObject for deletion in the next event loop iteration.
     *
     * This method schedules the deletion of the SwObject using `SwCoreApplication::postEvent`.
     * The actual deletion occurs asynchronously, ensuring that the SwObject is safely
     * removed without disrupting the current execution flow.
     */
    void deleteLater() {
        SwObject* meAsDurtyToClean = this;
        SwCoreApplication::instance()->postEvent([meAsDurtyToClean]() {
            delete meAsDurtyToClean;
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
     * @brief Sets a new parent for the SwObject and updates the parent-child relationship.
     *
     * If the SwObject already has a parent, it removes itself from the previous parent's children list.
     * It then adds itself to the new parent's children list and triggers the `newParentEvent` to handle
     * any custom behavior related to the parent change.
     *
     * @param parent The new parent SwObject. Can be nullptr to detach the SwObject from its current parent.
     */
    void setParent(SwObject *parent)
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
     * @brief Retrieves the parent of the current SwObject.
     *
     * @return A pointer to the parent SwObject, or nullptr if the SwObject has no parent.
     */
    SwObject *parent() { return m_parent; }

    /**
     * @brief Adds a child SwObject to the current SwObject.
     *
     * This method appends the specified child to the list of children, emits a `childAdded` signal,
     * and triggers the `addChildEvent` to allow derived classes to handle additional logic.
     *
     * @param child The child SwObject to add.
     */
    virtual void addChild(SwObject* child) {
        children.push_back(child);
        emit childAdded(child);
        addChildEvent(child);
    }

    /**
     * @brief Removes a child SwObject from the current SwObject's children list.
     *
     * This method removes the specified child from the list of children, emits a `childRemoved` signal,
     * and triggers the `removedChildEvent` to allow derived classes to handle additional logic.
     *
     * @param child The child SwObject to remove.
     */
    virtual void removeChild(SwObject* child) {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
        emit childRemoved(child);
        removedChildEvent(child);
    }

    /**
     * @brief Finds all child objects of a specific type, including nested children.
     *
     * This method searches for children of the current SwObject that are of the specified type `T`.
     * The search is recursive, meaning it also checks the children of each child SwObject.
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
            if (auto objectChild = dynamic_cast<SwObject*>(child)) {
                // Recherche récursive dans les enfants de l'enfant
                std::vector<T*> nestedResults = objectChild->findChildren<T>();
                result.insert(result.end(), nestedResults.begin(), nestedResults.end());
            }
        }
        return result;
    }

    /**
     * @brief Retrieves all direct child objects of the current SwObject.
     *
     * This method returns a constant reference to the vector containing the direct children
     * of the current SwObject. The vector does not include nested children.
     *
     * @return A constant reference to the vector of child objects.
     */
    const std::vector<SwObject*>& getChildren() const {
        return children;
    }

    /**
     * @brief Handles the event triggered when a child SwObject is added.
     *
     * This virtual method is called whenever a child SwObject is added to the current SwObject.
     * It can be overridden in derived classes to perform specific actions when a child is added.
     *
     * @param child Pointer to the SwObject that was added as a child.
     */
    virtual void addChildEvent(SwObject* child) { SW_UNUSED(child) }

    /**
     * @brief Handles the event triggered when a child SwObject is removed.
     *
     * This virtual method is called whenever a child SwObject is removed from the current SwObject.
     * It can be overridden in derived classes to perform specific actions when a child is removed.
     *
     * @param child Pointer to the SwObject that was removed as a child.
     */
    virtual void removedChildEvent(SwObject* child) { SW_UNUSED(child)  }

    /**
     * @brief Connects a signal from a sender SwObject to a slot in a receiver SwObject.
     *
     * Establishes a connection between a signal emitted by the sender and a member function
     * (slot) of the receiver. The connection type determines how the signal is handled.
     *
     * @param sender Pointer to the sender SwObject emitting the signal.
     * @param signalName Name of the signal to connect.
     * @param receiver Pointer to the receiver SwObject receiving the signal.
     * @param slot Pointer to the receiver's member function (slot).
     * @param type Type of connection (e.g., DirectConnection, QueuedConnection, BlockingQueuedConnection). Default is DirectConnection.
     */
    template<typename Sender, typename Receiver, typename... Args>
    static void connect(Sender* sender, const SwString& signalName, Receiver* receiver, void (Receiver::* slot)(Args...), ConnectionType type = DirectConnection) {
        ISlot<Receiver, Args...>* newSlot = new SlotMember<Receiver, Args...>(receiver, slot);
        sender->addConnection(signalName, newSlot, type);
    }

    /**
     * @brief Connects a signal from a sender SwObject to a standalone function or lambda.
     *
     * Establishes a connection between a signal emitted by the sender and a function or lambda.
     * The connection type determines how the signal is handled.
     *
     * @param sender Pointer to the sender SwObject emitting the signal.
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
     * @tparam Sender The type of the sender SwObject emitting the signal.
     * @tparam Func The type of the lambda function to connect.
     * @param sender Pointer to the sender SwObject emitting the signal.
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

    template <typename SenderType, typename ReceiverType, typename Func>
    static void connect(SenderType* sender, const SwString& signalName, ReceiverType* receiver, Func&& func, ConnectionType type = DirectConnection) {
        using traits = function_traits<typename std::decay<Func>::type>;
        using R = typename traits::return_type;
        using args_tuple = typename traits::args_tuple;

        static_assert(std::is_void<R>::value, "Seules les fonctions retournant void sont supportées.");

        // On utilise apply_tuple pour déplier les arguments de la lambda
        auto slot = apply_tuple(args_tuple{}, slot_factory_with_receiver<ReceiverType, Func>(receiver, std::forward<Func>(func)));

        sender->addConnection(signalName, slot, type);
    }

    /**
     * @brief Connects a signal to a slot using modern pointer-to-member function syntax.
     *
     * This method aims to establish a connection between a signal and a slot in a type-safe way.
     * It uses pointer-to-member functions to explicitly define the signal and slot connections.
     * Note: This implementation is under development and may not work as expected in all cases.
     *
     * @tparam Sender The type of the sender SwObject emitting the signal.
     * @tparam Receiver The type of the receiver SwObject handling the signal.
     * @tparam SignalArgs The argument types of the signal.
     * @tparam SlotArgs The argument types of the slot.
     * @param sender Pointer to the sender SwObject emitting the signal.
     * @param signal The pointer-to-member function representing the signal.
     * @param receiver Pointer to the receiver SwObject handling the signal.
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
     * @brief Disconnects a specific slot from a signal of a sender SwObject.
     *
     * Removes the connection between a signal emitted by the sender and a specific slot of the receiver.
     * If no slots remain connected to the signal, the signal is removed from the sender's connections.
     *
     * @param sender Pointer to the sender SwObject emitting the signal.
     * @param signalName Name of the signal to disconnect from.
     * @param receiver Pointer to the receiver SwObject whose slot is being disconnected.
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
     * @brief Disconnects all slots of a receiver from all signals of a sender SwObject.
     *
     * Iterates through all signals of the sender and removes any connections associated with the receiver.
     * If no slots remain connected to a signal, the signal is removed from the sender's connections.
     *
     * @param sender Pointer to the sender SwObject emitting the signals.
     * @param receiver Pointer to the receiver SwObject whose slots are being disconnected.
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
     * @brief Retrieves the current sender of the signal.
     *
     * Returns the pointer to the `SwObject` that emitted the signal. Useful in slot functions
     * to determine which SwObject triggered the signal.
     *
     * @return Object* Pointer to the sender SwObject, or `nullptr` if no sender is set.
     */
    SwObject* sender() {
        return currentSender;
    }

    /**
     * @brief Sets the current sender of the signal.
     *
     * This function assigns the sender SwObject for the currently emitted signal.
     * It is used internally when emitting signals to track the originating SwObject.
     *
     * @param _sender Pointer to the `SwObject` that emits the signal.
     */
    void setSender(SwObject* _sender) {
        currentSender = _sender;
    }

    /**
     * @brief Disconnects all slots connected to this SwObject.
     *
     * This function clears all signal-slot connections associated with this SwObject.
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
     * This function iterates through all connections of this SwObject and removes any
     * slots that are associated with the provided receiver SwObject.
     *
     * @tparam Receiver The type of the receiver SwObject.
     * @param receiver A pointer to the receiver SwObject whose slots need to be disconnected.
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
     * @param value The new value to assign to the property, encapsulated in a SwAny SwObject.
     *
     * @note Logs a message if the property is not found or if the value type does not match the expected type.
     */
    void setProperty(const SwString& propertyName, SwAny value) {
        // Vérifier si la propriété existe dans la map
        if (propertySetterMap.find(propertyName) != propertySetterMap.end()) {
            // Appel du setter via SwAny
            if (propertyArgumentTypeNameMap[propertyName] == value.typeName()) {
                propertySetterMap[propertyName](value.data());
            } else if(value.canConvert(propertyArgumentTypeNameMap[propertyName])){
                std::cout << "from converted value\n";
                propertySetterMap[propertyName](value.convert(propertyArgumentTypeNameMap[propertyName]).data());
            } else {
                std::cout << "Whoa, hold on! The property you're trying to set doesn't match the expected type: "
                          << propertyArgumentTypeNameMap[propertyName]
                          << ". Received: " << value.typeName()
                          << ". You need to explicitly cast your value to the correct type." << std::endl;
            }
        }
        else {
            std::cout << "Property not found: " << propertyName << std::endl;
        }
    }

    /**
     * @brief Retrieves the value of a specified property.
     *
     * This function accesses a property by its name and returns its value encapsulated in a SwAny SwObject.
     * If the properties have not been initialized, it triggers their registration.
     * If the property exists, its getter function is called to obtain the value,
     * which is then converted to a SwAny SwObject.
     *
     * @param propertyName The name of the property to retrieve.
     * @return SwAny The value of the property if found, or an empty SwAny SwObject if the property does not exist.
     *
     * @note Logs a message if the property is not found.
     */
    SwAny property(const SwString& propertyName) {
        SwAny retValue;
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
        auto it = propertyGetterMap.find(propertyName);
        return (it != propertyGetterMap.end());
    }

protected:

    /**
     * @brief Handles events triggered when a new parent is assigned.
     *
     * This function is called whenever the SwObject is assigned a new parent.
     * It can be overridden to implement custom behavior for handling parent changes.
     *
     * @param parent The new parent SwObject assigned to this SwObject.
     */
    virtual void newParentEvent(SwObject* parent) { SW_UNUSED(parent) }


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

protected:
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
                        static_cast<SwObject*>(slot->receiveur())->setSender(this);
                    }
                    slot->invoke(slot->receiveur(), args...);
                }
                else if (type == QueuedConnection) {
                    // Mettre en file d'attente pour un traitement ultérieur
                    SwCoreApplication::instance()->postEvent([this, slot, args...]() {
                        if (slot->receiveur()) {
                            static_cast<SwObject*>(slot->receiveur())->setSender(this);
                        }
                        slot->invoke(slot->receiveur(), args...);
                    });
                }
                else if (type == BlockingQueuedConnection) {
                    // Utiliser future pour bloquer jusqu'à ce que le slot soit exécuté
                    auto future = std::async(std::launch::async, [this, slot, args...]() {
                        if (slot->receiveur()) {
                            static_cast<SwObject*>(slot->receiveur())->setSender(this);
                        }
                        slot->invoke(slot->receiveur(), args...);
                    });
                    future.wait();
                }
            }
        }
    }

    // Conversion générique pointeur de fonction membre -> void*
    template<typename T>
    static void* toVoidPtr(T func) {
        // static_assert(sizeof(T) == sizeof(void*), "Pointeur de fonction membre et void* tailles différentes.");
        void* ptr = nullptr;
        std::memcpy(&ptr, &func, sizeof(ptr));
        return ptr;
    }

    // Conversion inverse void* -> pointeur de fonction membre
    template<typename T>
    static T fromVoidPtr(void* ptr) {
        static_assert(sizeof(T) == sizeof(void*), "Pointeur de fonction membre et void* tailles différentes.");
        T func;
        std::memcpy(&func, &ptr, sizeof(func));
        return func;
    }

signals:
    DECLARE_SIGNAL(childRemoved)
    DECLARE_SIGNAL(childAdded)

private:
    SwObject* m_parent = nullptr;
    std::vector<SwObject*> children;
    SwString objectName;
    std::map<SwString, SwString> properties;
    std::map<SwString, std::vector<std::pair<void*, ConnectionType>>> connections;
    SwObject* currentSender = nullptr;
};




