//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATORJS_H_
#define COMPILER_TRANSLATORJS_H_

#include "compiler/ShHandle.h"

class TranslatorJS : public TCompiler {
public:
    TranslatorJS(ShShaderType type, ShShaderSpec spec);

protected:
    virtual void translate(TIntermNode* root);

private:
    void writeExtensionBehavior();
};

#endif  // COMPILER_TRANSLATORJS_H_
