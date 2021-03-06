//
//  CommonTypeFinder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Types/CommonTypeFinder.hpp"
#include "Compiler.hpp"
#include "Types/TypeContext.hpp"
#include "Types/TypeDefinition.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

void CommonTypeFinder::addType(const Type &type, const TypeContext &typeContext) {
    if (!firstTypeFound_) {
        setCommonType(type);
        firstTypeFound_ = true;
        if (type.canHaveProtocol()) {
            std::transform(type.typeDefinition()->protocols().begin(), type.typeDefinition()->protocols().end(),
                           std::back_inserter(commonProtocols_), [](auto &type) { return type->type(); });
        }
        return;
    }

    updateCommonType(type, typeContext);
    updateCommonProtocols(type, typeContext);
}

void CommonTypeFinder::setCommonType(const Type &type) {
    commonType_ = type.inexacted();
    commonType_.setReference(false);
}

void CommonTypeFinder::updateCommonType(const Type &type, const TypeContext &typeContext) {
    if (!type.compatibleTo(commonType_, typeContext)) {
        if (commonType_.compatibleTo(type, typeContext)) {
            setCommonType(type);
        }
        else if (type.type() == TypeType::Class && commonType_.type() == TypeType::Class) {
            commonType_ = Type::someobject();
        }
        else {
            commonType_ = Type::something();
        }
    }
}

void CommonTypeFinder::updateCommonProtocols(const Type &type, const TypeContext &typeContext) {
    if (!commonProtocols_.empty()) {
        if (!type.canHaveProtocol()) {
            commonProtocols_.clear();
            return;
        }

        auto &protocols = type.typeDefinition()->protocols();
        std::vector<Type> newCommonProtocols;
        for (auto &protocol : protocols) {
            auto b = std::any_of(commonProtocols_.begin(), commonProtocols_.end(), [&protocol, &typeContext](auto &p) {
                return protocol->type().identicalTo(p, typeContext, nullptr);
            });
            if (b) {
                newCommonProtocols.emplace_back(protocol->type());
            }
        }
        commonProtocols_ = newCommonProtocols;
    }
}

Type CommonTypeFinder::getCommonType(const SourcePosition &p, Compiler *app) const {
    if (!firstTypeFound_) {
        app->warn(p, "Type is ambiguous without more context.");
    }
    else if (commonType_.unboxedType() == TypeType::Something || commonType_.unboxedType() == TypeType::Someobject) {
        if (commonProtocols_.size() > 1) {
            return Type(commonProtocols_);
        }
        if (commonProtocols_.size() == 1) {
            return commonProtocols_.front();
        }
        app->warn(p, "Common type was inferred to be ", commonType_.toString(TypeContext()), ".");
    }
    return commonType_;
}

}  // namespace EmojicodeCompiler
