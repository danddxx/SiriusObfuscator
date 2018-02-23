#include "swift/Obfuscation/DeclarationParsingUtils.h"
#include "swift/Obfuscation/Utils.h"

#include <sstream>
#include <string>
#include <vector>

namespace swift {
namespace obfuscation {
  
std::string combineIdentifier(std::vector<std::string> &Parts) {
  if (Parts.empty()) {
    return "";
  } else if (Parts.size() == 1) {
    return Parts[0];
  } else {
    //TODO: can we rewrite it to use llvm:raw_string_ostream?
    std::stringstream ResultStream;
    copyToStream(Parts, std::ostream_iterator<std::string>(ResultStream, "."));
    std::string Result = ResultStream.str();
    Result.pop_back();
    return Result;
  }
}

std::string declarationName(const ValueDecl* Declaration) {
  return Declaration->getName().str().str();
}
  
std::string moduleName(const Decl* Declaration) {
  return Declaration->getModuleContext()->getBaseName().getIdentifier().get();
}

std::string externalParameterName(const ParamDecl *Declaration) {
  return Declaration->getArgumentName().str().str();
}

std::string internalParameterName(const ParamDecl *Declaration) {
  return Declaration->getName().str().str();
}

std::string symbolName(const OperatorDecl *Declaration) {
  return Declaration->getName().str().str();
}

std::string functionName(const AbstractFunctionDecl* Declaration) {
    return Declaration->getName().str().str();
}

llvm::Expected<std::vector<std::string>>
nominalTypeIdentifierParts(const NominalTypeDecl *Declaration,
                           const std::string &SymbolName) {

  std::vector<std::string> Parts;

  if (auto *EnumDeclaration = dyn_cast<EnumDecl>(Declaration)) {
    Parts.push_back("enum." + SymbolName);
  } else if (auto *ClassDeclaration = dyn_cast<ClassDecl>(Declaration)) {
    Parts.push_back("class." + SymbolName);
  } else if (auto *ProtocolDeclaration = dyn_cast<ProtocolDecl>(Declaration)) {
    Parts.push_back("protocol." + SymbolName);
  } else if (auto *StructDeclaration = dyn_cast<StructDecl>(Declaration)) {
    Parts.push_back("struct." + SymbolName);
  } else {
    return stringError("found unsupported declaration type");
  }

  return Parts;
}

ModuleNameAndParts moduleNameAndIdentifierParts(const Decl *Declaration) {
  std::string ModuleName = moduleName(Declaration);
  std::vector<std::string> Parts;
  Parts.push_back("module");
  Parts.push_back(ModuleName);
  return std::make_pair(ModuleName, Parts);
}

std::string typeName(const NominalTypeDecl* Declaration) {
  return Declaration->getDeclaredInterfaceType()->getString();
}

llvm::Expected<std::string> enclosingTypeName(const Decl* Declaration) {
  auto *DeclarationContext = Declaration->getDeclContext();
  if (auto *TypeDeclaration =
      DeclarationContext->getAsNominalTypeOrNominalTypeExtensionContext()) {
    return typeName(TypeDeclaration);
  }
  return stringError("enclosing context of this declaration is not supported");
}

template<class T>
const T* findRecursivelyBaseOverridenDeclarationWithModules
(const T *Declaration, std::set<std::string> &Modules) {
  static_assert(std::is_base_of<Decl, T>::value, "T is not a subclass of Decl");
  if (auto* OverrideDeclaration = Declaration->getOverriddenDecl()) {
    Modules.insert(moduleName(OverrideDeclaration));
    return
    findRecursivelyBaseOverridenDeclarationWithModules(OverrideDeclaration,
                                                       Modules);
  } else {
    return Declaration;
  }
}
  
template<class T>
std::pair<const T*, std::set<std::string>>
getBaseOverridenDeclarationWithModules(const T *Declaration) {
  std::set<std::string> Modules;
  auto Base = findRecursivelyBaseOverridenDeclarationWithModules(Declaration,
                                                                 Modules);
  return std::make_pair(Base, Modules);
}

template
std::pair<const VarDecl*, std::set<std::string>>
getBaseOverridenDeclarationWithModules(const VarDecl *Declaration);
template
std::pair<const FuncDecl*, std::set<std::string>>
getBaseOverridenDeclarationWithModules(const FuncDecl *Declaration);
template
std::pair<const AbstractFunctionDecl*, std::set<std::string>>
getBaseOverridenDeclarationWithModules(const AbstractFunctionDecl *Declaration);

// Determines if the ConstructorDecl represents the Struct Memberwise
// Initializer. Checks if the declaration is implicit. Also checks if
// parameters list (other than self parameter) is not empty to exclude
// Default Initializers.
bool isMemberwiseConstructor(const ConstructorDecl* Declaration) {
  auto ConstructsStruct = Declaration->getResultInterfaceType()->
  getStructOrBoundGenericStruct() != nullptr;
  
  return ConstructsStruct
  && Declaration->isImplicit()
  && Declaration->getParameters()->size() != 0;
}
  
bool isMemberwiseConstructorParameter(const ParamDecl* Declaration) {
  auto *Context = Declaration->getDeclContext();
  if (const auto *ConstructorDeclaration = dyn_cast<ConstructorDecl>(Context)) {
    return isMemberwiseConstructor(ConstructorDeclaration);
  }
  return false;
}

} //namespace obfuscation
} //namespace swift
