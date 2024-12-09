#pragma once

#include <iostream>
#include <thread>
#include <map>
#include <vector>
#include <queue>
#include <chrono>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <future>
#include "CoreApplication.h"
#include "SwAny.h"


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
    CUSTOM_PROPERTY(type, PROP_NAME, defautValue) {}

#define SURCHARGE_ON_PROPERTY_CHANGED(type, PROP_NAME) \
protected: \
    virtual void on_##PROP_NAME##_changed(const type& value) override

#define REGISTER_PROPERTY(PROP_NAME) \
    addPropertyRegistrar([this]() { register_##PROP_NAME##_setter(); });




#define signals private
#define slots 

#define SIGNAL(signalName) #signalName
#define SLOT(slotName) #slotName

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







// Enum pour les types de connexion
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

// Template pour un slot de méthode membre
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

// Template pour un slot fonctionnel (lambda ou std::function)
template <typename... Args>
class SlotFunction : public ISlot<void, Args...> {
public:
    SlotFunction(std::function<void(Args...)> func) : func(func) {}

    void invoke(void*, Args... args) override {
        func(args...);
    }

    void* receiveur() override {
        return nullptr;
    }

private:
    std::function<void(Args...)> func;
};

class Object {
    PROPERTY(SwString, ObjectName, "")

public:
    Object(Object* parent = nullptr) :
          m_parent(nullptr)
        , ArePropertiesInitialised(false)
    {
        REGISTER_PROPERTY(ObjectName);

        // Enregistrement dans la map
        setParent(parent);
    }

    virtual ~Object() {
        //emit destroyed();
        //for (auto child : children) {
        //    if (child->m_parent == this) {
        //        delete child;
        //    }
        //}
        //disconnectAllSlots();
    }

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


    // Retourne la hiérarchie d'héritage de l'objet sous forme de vecteur
    virtual std::vector<SwString> classHierarchy() const {
        return { Object::className() };
    }

    void deleteLater() {
        Object* meAsAShitToClean = this;
        CoreApplication::instance()->postEvent([meAsAShitToClean]() {
            delete meAsAShitToClean;
        });
    }

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

    Object *parent()
    {
        return m_parent;
    }

    virtual void addChild(Object* child) {
        children.push_back(child);
        emit childAdded(child);
        addChildEvent(child);
    }

    void removeChild(Object* child) {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
        emit childRemoved(child);
        removedChildEvent(child);
    }

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


    const std::vector<Object*>& getChildren() const {
        return children;
    }

    virtual void addChildEvent(Object* child) {
        // std::cout << "Enfant ajouté: " << child->getObjectName() << std::endl;
    }

    virtual void removedChildEvent(Object* child) {
        // std::cout << "Enfant supprimé: " << child->getObjectName() << std::endl;
    }

    template<typename Sender, typename Receiver, typename... Args>
    static void connect(Sender* sender, const SwString& signalName, Receiver* receiver, void (Receiver::* slot)(Args...), ConnectionType type = DirectConnection) {
        ISlot<Receiver, Args...>* newSlot = new SlotMember<Receiver, Args...>(receiver, slot);
        sender->addConnection(signalName, newSlot, type);
    }

    template<typename Sender, typename... Args>
    static void connect(Sender* sender, const SwString& signalName, std::function<void(Args...)> func, ConnectionType type = DirectConnection) {
        ISlot<void, Args...>* newSlot = new SlotFunction<Args...>(func);
        sender->addConnection(signalName, newSlot, type);
    }

    // Ajouter une connexion avec type
    template<typename... Args>
    void addConnection(const SwString& signalName, ISlot<Args...>* slot, ConnectionType type) {
        if (connections.find(signalName) == connections.end()) {
            connections[signalName] = std::vector<std::pair<void*, ConnectionType>>();
        }
        connections[signalName].push_back(std::make_pair(static_cast<void*>(slot), type));
    }

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
                    CoreApplication::instance()->postEvent([this, slot, args...]() {
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

    Object* sender() {
        return currentSender;
    }

    void setSender(Object* _sender) {
        currentSender = _sender;
    }

    void disconnectAllSlots() {
        if (connections.size() > 0) {
            connections.clear();
            std::cout << "Tous les slots ont été déconnectés pour cet objet." << std::endl;
        }
    }

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


    bool propertyExist(const SwString& propertyName) {
        if (!ArePropertiesInitialised) {
            ArePropertiesInitialised = registerProperties();
        }

        auto it = propertyGetterMap.find(propertyName);
        return (it != propertyGetterMap.end());
    }

protected:

    virtual void newParentEvent(Object* parent) {

    }


    void addPropertyRegistrar(const std::function<void()>& func) {
        propertyRegistrars.push_back(func);
    }

    bool registerProperties() {
        for (const auto& func : propertyRegistrars) {
            func();
        }
        return true;
    }


    std::map<SwString, std::function<void(void*)>> propertySetterMap;
    std::map<SwString, std::function<void*()>> propertyGetterMap;
    std::map<SwString, SwString> propertyArgumentTypeNameMap;
    std::vector<std::function<void()>> propertyRegistrars;

signals:
    DECLARE_SIGNAL(childRemoved);
    DECLARE_SIGNAL(childAdded);


    
        

private:
    Object* m_parent = nullptr;
    std::vector<Object*> children;
    bool ArePropertiesInitialised;
    SwString objectName;
    std::map<SwString, SwString> properties;
    std::map<SwString, std::vector<std::pair<void*, ConnectionType>>> connections;
    Object* currentSender = nullptr;
};




