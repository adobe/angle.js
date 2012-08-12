//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_SOURCE_LOCATION_H_
#define COMPILER_PREPROCESSOR_SOURCE_LOCATION_H_

namespace pp
{

struct SourceLocation
{
    SourceLocation() : file(0), line(0), index(0) { }
    SourceLocation(int f, int l, int i) : file(f), line(l), index(i) { }

    bool equals(const SourceLocation& other) const
    {
        return (file == other.file) && (line == other.line) && (index == other.index);
    }

    int file;
    int line;
    int index;
};

inline bool operator==(const SourceLocation& lhs, const SourceLocation& rhs)
{
    return lhs.equals(rhs);
}

inline bool operator!=(const SourceLocation& lhs, const SourceLocation& rhs)
{
    return !lhs.equals(rhs);
}

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_SOURCE_LOCATION_H_
