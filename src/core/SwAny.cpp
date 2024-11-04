#include "SwAny.h"

// Définition des membres statiques en dehors de la classe dans le fichier .cpp
std::map<std::string, std::function<void(SwAny&, SwAny&&)>> SwAny::_actionMoveFrom;
std::map<std::string, std::function<void(SwAny&)>> SwAny::_actionClear;
std::map<std::string, std::function<void(SwAny&, const SwAny&)>> SwAny::_actionCopyFrom;
std::map<std::string, std::function<void* (const SwAny&)>> SwAny::_actionData;
std::map<std::string, std::function<SwAny(void*)>> SwAny::_actionFromVoidPtr;
