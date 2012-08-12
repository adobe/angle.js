//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/OutputJS.h"
#include "compiler/debug.h"

namespace
{
TString getTypeName(const TType& type)
{
    TInfoSinkBase out;
    if (type.isMatrix())
    {
        out << "mat";
        out << type.getNominalSize();
    }
    else if (type.isVector())
    {
        switch (type.getBasicType())
        {
            case EbtFloat: out << "vec"; break;
            case EbtInt: out << "ivec"; break;
            case EbtBool: out << "bvec"; break;
            default: UNREACHABLE(); break;
        }
        out << type.getNominalSize();
    }
    else
    {
        if (type.getBasicType() == EbtStruct)
            out << type.getTypeName();
        else
            out << type.getBasicString();
    }
    return TString(out.c_str());
}
    
TString getTypeFn(const TType& type)
{
    TInfoSinkBase out;
    out << "[\"type\", \"";
    out << getTypeName(type);
    out << "\"";
    if (type.isArray()) {
        out << ", { \"arraySize\": ";
        out << type.getArraySize();
        out << " }";
    }
    out << "]";
    return TString(out.c_str());
}

}  // namespace

TOutputJS::TOutputJS(TInfoSinkBase& objSink)
    : TIntermTraverser(true, true, true),
      mObjSink(objSink),
      mDeclaringVariables(false)
{
}

void TOutputJS::writeTriplet(Visit visit, const char* preStr, const char* inStr, const char* postStr)
{
    TInfoSinkBase& out = objSink();
    if (visit == PreVisit && preStr)
    {
        out << preStr;
    }
    else if (visit == InVisit && inStr)
    {
        out << inStr;
    }
    else if (visit == PostVisit && postStr)
    {
        out << postStr;
    }
}

void TOutputJS::writeOperation(Visit visit, const char* operation)
{
    assert(strlen(operation) > 0);
    TInfoSinkBase& out = objSink();
    if (visit == PreVisit)
    {
        out << "[\"" << operation << "\", ";
    }
    else if (visit == InVisit)
    {
        out << ", ";
    }
    else if (visit == PostVisit)
    {
        out << "]";
    }
}

void TOutputJS::writeVariableType(const TType& type)
{
    TInfoSinkBase& out = objSink();
    TQualifier qualifier = type.getQualifier();
    // TODO(alokp): Validate qualifier for variable declarations.
    bool writeQualifier = ((qualifier != EvqTemporary) && (qualifier != EvqGlobal));
    if (writeQualifier)
        out << "[\"varType\", \"" << type.getQualifierString() << "\", ";
    // Declare the struct if we have not done so already.
    if ((type.getBasicType() == EbtStruct) &&
        (mDeclaredStructs.find(type.getTypeName()) == mDeclaredStructs.end()))
    {
        out << "[\"struct\", \"" << type.getTypeName() << "\", [";
        const TTypeList* structure = type.getStruct();
        ASSERT(structure != NULL);
        for (size_t i = 0; i < structure->size(); ++i)
        {
            const TType* fieldType = (*structure)[i].type;
            ASSERT(fieldType != NULL);
            if (i > 0) out << ", ";
            out << "{ \"type\": " << getTypeFn(*fieldType) << ", \"name\": \"" << fieldType->getFieldName() << "\" }";
        }
        out << "]]";
        mDeclaredStructs.insert(type.getTypeName());
    }
    else
    {
        out << getTypeFn(type);
    }
    if (writeQualifier)
        out << "]";
}

void TOutputJS::writeFunctionParameters(const TIntermSequence& args)
{
    TInfoSinkBase& out = objSink();
    for (TIntermSequence::const_iterator iter = args.begin();
         iter != args.end(); ++iter)
    {
        const TIntermSymbol* arg = (*iter)->getAsSymbolNode();
        ASSERT(arg != NULL);

        const TType& type = arg->getType();
        out << "[\"parameter\", ";
        writeVariableType(type);
        const TString& name = arg->getSymbol();
        out << ", " << arg->getId();
        out << ", \"" << name << "\"";
        out << "]";
        // Put a comma if this is not the last argument.
        if (iter != args.end() - 1)
            out << ", ";
    }
}

const ConstantUnion* TOutputJS::writeConstantUnion(const TType& type,
                                                         const ConstantUnion* pConstUnion)
{
    TInfoSinkBase& out = objSink();

    if (type.getBasicType() == EbtStruct)
    {
        out << "[\"constant_struct\", \"" << type.getTypeName() << "\", {";
        const TTypeList* structure = type.getStruct();
        ASSERT(structure != NULL);
        for (size_t i = 0; i < structure->size(); ++i)
        {
            const TType* fieldType = (*structure)[i].type;
            ASSERT(fieldType != NULL);
            if (i > 0)
                out << ", ";
            out << "\"" << fieldType->getFieldName() << "\": ";
            pConstUnion = writeConstantUnion(*fieldType, pConstUnion);
        }
        out << "}]";
    }
    else
    {
        int size = type.getObjectSize();
        out << "[\"constant\", " << getTypeFn(type) << ", [";
        for (int i = 0; i < size; ++i, ++pConstUnion)
        {
            if (i > 0) out << ", ";
            switch (pConstUnion->getType())
            {
                case EbtFloat: out << pConstUnion->getFConst(); break;
                case EbtInt: out << pConstUnion->getIConst(); break;
                case EbtBool: out << pConstUnion->getBConst(); break;
                default: UNREACHABLE();
            }
        }
        out << "]]";
    }
    return pConstUnion;
}

void TOutputJS::visitSymbol(TIntermSymbol* node)
{
    TInfoSinkBase& out = objSink();
    out << "[\"symbol\", " << node->getId() << ", \"";
    if (mLoopUnroll.NeedsToReplaceSymbolWithValue(node))
        out << mLoopUnroll.GetLoopIndexValue(node);
    else
        out << node->getSymbol();
    out << "\"";
    int index;
    DecodeSourceLoc(node->getLine(), NULL, NULL, &index);
    out << ", " << index;
    out << "]";
}

void TOutputJS::visitConstantUnion(TIntermConstantUnion* node)
{
    writeConstantUnion(node->getType(), node->getUnionArrayPointer());
}

bool TOutputJS::visitBinary(Visit visit, TIntermBinary* node)
{
    bool visitChildren = true;
    TInfoSinkBase& out = objSink();
    switch (node->getOp())
    {
        case EOpInitialize:
            writeOperation(visit, "init");
            if (visit == InVisit)
            {
                // RHS of initialize is not being declared.
                mDeclaringVariables = false;
            }
            break;
        case EOpAssign: writeOperation(visit, "="); break;
        case EOpAddAssign: writeOperation(visit, "+="); break;
        case EOpSubAssign: writeOperation(visit, "-="); break;
        case EOpDivAssign: writeOperation(visit, "/="); break;
        // Notice the fall-through.
        case EOpMulAssign: 
        case EOpVectorTimesMatrixAssign:
        case EOpVectorTimesScalarAssign:
        case EOpMatrixTimesScalarAssign:
        case EOpMatrixTimesMatrixAssign:
            writeOperation(visit, "*=");
            break;

        case EOpIndexDirect:
        case EOpIndexIndirect:
            writeOperation(visit, "index");
            break;
        case EOpIndexDirectStruct:
            if (visit == PreVisit)
            {
                out << "[\"get_struct_field\", \"" << node->getType().getFieldName() << "\", ";
            }
            else if (visit == InVisit)
            {
                out << "]";
                visitChildren = false;
            }
            break;
        case EOpVectorSwizzle:
            if (visit == PreVisit)
            {
                out << "[\"swizzle\", \"";
                TInfoSinkBase sout;
                
                TIntermAggregate* rightChild = node->getRight()->getAsAggregate();
                TIntermSequence& sequence = rightChild->getSequence();
                for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); ++sit)
                {
                    TIntermConstantUnion* element = (*sit)->getAsConstantUnion();
                    ASSERT(element->getBasicType() == EbtInt);
                    ASSERT(element->getNominalSize() == 1);
                    const ConstantUnion& data = element->getUnionArrayPointer()[0];
                    ASSERT(data.getType() == EbtInt);
                    switch (data.getIConst())
                    {
                        case 0: out << "x"; break;
                        case 1: out << "y"; break;
                        case 2: out << "z"; break;
                        case 3: out << "w"; break;
                        default: UNREACHABLE(); break;
                    }
                }
                out << "\", ";
            } else if (visit == InVisit) {
                out << "]";
                visitChildren = false;
            }
            break;

        case EOpAdd: writeOperation(visit, "+"); break;
        case EOpSub: writeOperation(visit, "-"); break;
        case EOpMul: writeOperation(visit, "*"); break;
        case EOpDiv: writeOperation(visit, "/"); break;
        case EOpMod: UNIMPLEMENTED(); break;
        case EOpEqual: writeOperation(visit, "=="); break;
        case EOpNotEqual: writeOperation(visit, "!="); break;
        case EOpLessThan: writeOperation(visit, "<"); break;
        case EOpGreaterThan: writeOperation(visit, ">"); break;
        case EOpLessThanEqual: writeOperation(visit, "<="); break;
        case EOpGreaterThanEqual: writeOperation(visit, ">="); break;

        // Notice the fall-through.
        case EOpVectorTimesScalar:
        case EOpVectorTimesMatrix:
        case EOpMatrixTimesVector:
        case EOpMatrixTimesScalar:
        case EOpMatrixTimesMatrix:
            writeOperation(visit, "*");
            break;

        case EOpLogicalOr: writeOperation(visit, "||"); break;
        case EOpLogicalXor: writeOperation(visit, "^^"); break;
        case EOpLogicalAnd: writeOperation(visit, "&&"); break;
        default: UNREACHABLE(); break;
    }

    return visitChildren;
}

bool TOutputJS::visitUnary(Visit visit, TIntermUnary* node)
{
    TString preString;

    switch (node->getOp())
    {
        case EOpNegative: preString = "negative"; break;
        case EOpVectorLogicalNot: preString = "vectorLogicalNot"; break;
        case EOpLogicalNot: preString = "logicalNot"; break;

        case EOpPostIncrement: preString = "postIncrement"; break;
        case EOpPostDecrement: preString = "postDecrement"; break;
        case EOpPreIncrement: preString = "preIncrement"; break;
        case EOpPreDecrement: preString = "preDecrement"; break;

        case EOpConvIntToBool:
        case EOpConvFloatToBool:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: preString =  "convert_bool";  break;
                case 2: preString = "convert_bvec2"; break;
                case 3: preString = "convert_bvec3"; break;
                case 4: preString = "convert_bvec4"; break;
                default: UNREACHABLE();
            }
            break;
        case EOpConvBoolToFloat:
        case EOpConvIntToFloat:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: preString = "convert_float";  break;
                case 2: preString = "convert_vec2"; break;
                case 3: preString = "convert_vec3"; break;
                case 4: preString = "convert_vec4"; break;
                default: UNREACHABLE();
            }
            break;
        case EOpConvFloatToInt:
        case EOpConvBoolToInt:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: preString = "convert_int";  break;
                case 2: preString = "convert_ivec2"; break;
                case 3: preString = "convert_ivec3"; break;
                case 4: preString = "convert_ivec4"; break;
                default: UNREACHABLE();
            }
            break;

        case EOpRadians: preString = "radians"; break;
        case EOpDegrees: preString = "degrees"; break;
        case EOpSin: preString = "sin"; break;
        case EOpCos: preString = "cos"; break;
        case EOpTan: preString = "tan"; break;
        case EOpAsin: preString = "asin"; break;
        case EOpAcos: preString = "acos"; break;
        case EOpAtan: preString = "atan"; break;

        case EOpExp: preString = "exp"; break;
        case EOpLog: preString = "log"; break;
        case EOpExp2: preString = "exp2"; break;
        case EOpLog2: preString = "log2"; break;
        case EOpSqrt: preString = "sqrt"; break;
        case EOpInverseSqrt: preString = "inversesqrt"; break;

        case EOpAbs: preString = "abs"; break;
        case EOpSign: preString = "sign"; break;
        case EOpFloor: preString = "floor"; break;
        case EOpCeil: preString = "ceil"; break;
        case EOpFract: preString = "fract"; break;

        case EOpLength: preString = "length"; break;
        case EOpNormalize: preString = "normalize"; break;

        case EOpDFdx: preString = "dFdx"; break;
        case EOpDFdy: preString = "dFdy"; break;
        case EOpFwidth: preString = "fwidth"; break;

        case EOpAny: preString = "any"; break;
        case EOpAll: preString = "all"; break;

        default: UNREACHABLE(); break;
    }

    writeOperation(visit, preString.c_str());

    return true;
}

bool TOutputJS::visitSelection(Visit visit, TIntermSelection* node)
{
    TInfoSinkBase& out = objSink();

    if (node->usesTernaryOperator())
    {
        out << "[\"if_value\", ";
        node->getCondition()->traverse(this);
        out << ", ";
        node->getTrueBlock()->traverse(this);
        out << ", ";
        node->getFalseBlock()->traverse(this);
        out << "]";
    }
    else
    {
        out << "[\"if\", ";
        node->getCondition()->traverse(this);
        out << ", ";

        incrementDepth();
        visitCodeBlock(node->getTrueBlock());

        if (node->getFalseBlock())
        {
            out << ", ";
            visitCodeBlock(node->getFalseBlock());
        }
        decrementDepth();
        out << "]";
    }
    return false;
}

bool TOutputJS::visitAggregate(Visit visit, TIntermAggregate* node)
{
    bool visitChildren = true;
    TInfoSinkBase& out = objSink();
    TString preString;
    bool delayedWrite = false;
    switch (node->getOp())
    {
        case EOpSequence: {
            out << "[\"block\", ";

            incrementDepth();
            const TIntermSequence& sequence = node->getSequence();
            for (TIntermSequence::const_iterator iter = sequence.begin();
                 iter != sequence.end(); ++iter)
            {
                TIntermNode* node = *iter;
                ASSERT(node != NULL);
                out << "[\"line\", ";
                node->traverse(this);
                
                int index;
                DecodeSourceLoc(node->getLine(), NULL, NULL, &index);
                out << ", " << index << "]";
                if (iter != sequence.end() - 1)
                    out << ",\n";
            }
            decrementDepth();

            out << "]";
            visitChildren = false;
            break;
        }
        case EOpPrototype: {
            // Function declaration.
            ASSERT(visit == PreVisit);
            out << "[\"declareFunction\", ";
            writeVariableType(node->getType());
            out << ", \"" << node->getName() << "\", ";

            out << "[";
            writeFunctionParameters(node->getSequence());
            out << "]]";

            visitChildren = false;
            break;
        }
        case EOpFunction: {
            // Function definition.
            ASSERT(visit == PreVisit);
            out << "[\"defineFunction\", ";
            writeVariableType(node->getType());
            out << ", \"" << TFunction::unmangleName(node->getName()) << "\", ";

            incrementDepth();
            // Function definition node contains one or two children nodes
            // representing function parameters and function body. The latter
            // is not present in case of empty function bodies.
            const TIntermSequence& sequence = node->getSequence();
            ASSERT((sequence.size() == 1) || (sequence.size() == 2));
            TIntermSequence::const_iterator seqIter = sequence.begin();

            // Traverse function parameters.
            TIntermAggregate* params = (*seqIter)->getAsAggregate();
            ASSERT(params != NULL);
            ASSERT(params->getOp() == EOpParameters);
            params->traverse(this);

            // Traverse function body.
            TIntermAggregate* body = ++seqIter != sequence.end() ?
                (*seqIter)->getAsAggregate() : NULL;
            out << ", ";
            visitCodeBlock(body);
            out << "]";
            decrementDepth();
 
            // Fully processed; no need to visit children.
            visitChildren = false;
            break;
        }
        case EOpFunctionCall:
            // Function call.
            if (visit == PreVisit)
            {
                TString functionName = TFunction::unmangleName(node->getName());
                out << "[\"call\", \"" << functionName << "\", ";
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << "]";
            }
            break;
        case EOpParameters: {
            // Function parameters.
            ASSERT(visit == PreVisit);
            out << "[";
            writeFunctionParameters(node->getSequence());
            out << "]";
            visitChildren = false;
            break;
        }
        case EOpDeclaration: {
            // Variable declaration.
            if (visit == PreVisit)
            {
                const TIntermSequence& sequence = node->getSequence();
                const TIntermTyped* variable = sequence.front()->getAsTyped();
                out << "[\"variables\", ";
                writeVariableType(variable->getType());
                out << ", [";
                mDeclaringVariables = true;
            }
            else if (visit == InVisit)
            {
                out << ", ";
                mDeclaringVariables = true;
            }
            else
            {
                out << "]]";
                mDeclaringVariables = false;
            }
            break;
        }
        case EOpConstructFloat: writeOperation(visit, "float"); break;
        case EOpConstructVec2: writeOperation(visit, "vec2"); break;
        case EOpConstructVec3: writeOperation(visit, "vec3"); break;
        case EOpConstructVec4: writeOperation(visit, "vec4"); break;
        case EOpConstructBool: writeOperation(visit, "bool"); break;
        case EOpConstructBVec2: writeOperation(visit, "bvec2"); break;
        case EOpConstructBVec3: writeOperation(visit, "bvec3"); break;
        case EOpConstructBVec4: writeOperation(visit, "bvec4"); break;
        case EOpConstructInt: writeOperation(visit, "int"); break;
        case EOpConstructIVec2: writeOperation(visit, "ivec2"); break;
        case EOpConstructIVec3: writeOperation(visit, "ivec3"); break;
        case EOpConstructIVec4: writeOperation(visit, "ivec4"); break;
        case EOpConstructMat2: writeOperation(visit, "mat2"); break;
        case EOpConstructMat3: writeOperation(visit, "mat3"); break;
        case EOpConstructMat4: writeOperation(visit, "mat4"); break;
        case EOpConstructStruct:
            if (visit == PreVisit)
            {
                const TType& type = node->getType();
                ASSERT(type.getBasicType() == EbtStruct);
                out << "[\"construct\", \"" << type.getTypeName() << "\", [";
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << "]]";
            }
            break;

        case EOpLessThan: preString = "lessThan"; delayedWrite = true; break;
        case EOpGreaterThan: preString = "greaterThan"; delayedWrite = true; break;
        case EOpLessThanEqual: preString = "lessThanEqual"; delayedWrite = true; break;
        case EOpGreaterThanEqual: preString = "greaterThanEqual"; delayedWrite = true; break;
        case EOpVectorEqual: preString = "equal"; delayedWrite = true; break;
        case EOpVectorNotEqual: preString = "notEqual"; delayedWrite = true; break;
        case EOpComma: writeTriplet(visit, NULL, ", ", NULL); break;

        case EOpMod: preString = "mod"; delayedWrite = true; break;
        case EOpPow: preString = "pow"; delayedWrite = true; break;
        case EOpAtan: preString = "atan"; delayedWrite = true; break;
        case EOpMin: preString = "min"; delayedWrite = true; break;
        case EOpMax: preString = "max"; delayedWrite = true; break;
        case EOpClamp: preString = "clamp"; delayedWrite = true; break;
        case EOpMix: preString = "mix"; delayedWrite = true; break;
        case EOpStep: preString = "step"; delayedWrite = true; break;
        case EOpSmoothStep: preString = "smoothstep"; delayedWrite = true; break;

        case EOpDistance: preString = "distance"; delayedWrite = true; break;
        case EOpDot: preString = "dot"; delayedWrite = true; break;
        case EOpCross: preString = "cross"; delayedWrite = true; break;
        case EOpFaceForward: preString = "faceforward"; delayedWrite = true; break;
        case EOpReflect: preString = "reflect"; delayedWrite = true; break;
        case EOpRefract: preString = "refract"; delayedWrite = true; break;
        case EOpMul: preString = "matrixCompMult"; delayedWrite = true; break;

        default: UNREACHABLE(); break;
    }
    if (delayedWrite)
        writeOperation(visit, preString.c_str());
    return visitChildren;
}

bool TOutputJS::visitLoop(Visit visit, TIntermLoop* node)
{
    TInfoSinkBase& out = objSink();

    incrementDepth();
    // Loop header.
    TLoopType loopType = node->getType();
    if (loopType == ELoopFor)  // for loop
    {
        if (!node->getUnrollFlag()) {
            out << "[\"for\", ";
            if (node->getInit())
                node->getInit()->traverse(this);
            else
                out << "null";
            out << ", ";

            if (node->getCondition())
                node->getCondition()->traverse(this);
            else
                out << "null";
            out << ", ";

            if (node->getExpression())
                node->getExpression()->traverse(this);
            else
                out << "null";
            out << ", ";
        }
    }
    else if (loopType == ELoopWhile)  // while loop
    {
        out << "[\"while\", ";
        ASSERT(node->getCondition() != NULL);
        node->getCondition()->traverse(this);
        out << ", ";
    }
    else  // do-while loop
    {
        ASSERT(loopType == ELoopDoWhile);
        out << "[\"do_while\", ";
        ASSERT(node->getCondition() != NULL);
        node->getCondition()->traverse(this);
        out << ", ";
    }

    // Loop body.
    if (node->getUnrollFlag())
    {
        out << "[\"block\", [";
        TLoopIndexInfo indexInfo;
        mLoopUnroll.FillLoopIndexInfo(node, indexInfo);
        mLoopUnroll.Push(indexInfo);
        while (mLoopUnroll.SatisfiesLoopCondition())
        {
            visitCodeBlock(node->getBody());
            out << ",\n";
            mLoopUnroll.Step();
        }
        mLoopUnroll.Pop();
        out << "]";
    }
    else
    {
        visitCodeBlock(node->getBody());
    }
    
    out << "]";

    decrementDepth();

    // No need to visit children. They have been already processed in
    // this function.
    return false;
}

bool TOutputJS::visitBranch(Visit visit, TIntermBranch* node)
{
    switch (node->getFlowOp())
    {
        case EOpKill: writeOperation(visit, "discard"); break;
        case EOpBreak: writeOperation(visit, "break"); break;
        case EOpContinue: writeOperation(visit, "continue"); break;
        case EOpReturn: writeOperation(visit, "return"); break;
        default: UNREACHABLE(); break;
    }

    return true;
}

void TOutputJS::visitCodeBlock(TIntermNode* node) {
    TInfoSinkBase &out = objSink();
    if (node != NULL)
    {
        node->traverse(this);
    }
    else
    {
        out << "[]";  // Empty code block.
    }
}
