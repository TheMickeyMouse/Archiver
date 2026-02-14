set(PROJECT_NAME Quasi)

set(HEADER_FILES
    src/Utils/Debug/internal_debug_break.h
    src/Utils/Debug/Logger.h
    src/Utils/Debug/Timer.h

    src/Utils/Math/Constants.h
    src/Utils/Math/Matrix.h
    src/Utils/Math/Rect.h
    src/Utils/Math/Vector.h
    src/Utils/Math/Geometry.h
    src/Utils/Math/Angles.h
    src/Utils/Math/Random.h
    src/Utils/Math/Color.h
    src/Utils/Math/Complex.h
    src/Utils/Math/Quaternion.h
    src/Utils/Math/Transform2D.h
    src/Utils/Math/Transform3D.h

    src/Utils/Enum.h
    src/Utils/Text.h
    src/Utils/Option.h
    src/Utils/Macros.h
    src/Utils/RichString.h
    src/Utils/Type.h
    src/Utils/Match.h
    src/Utils/Memory.h
    src/Utils/Iterator.h
    src/Utils/Vec.h
    src/Utils/Span.h
    src/Utils/Algorithm.h
    src/Utils/Array.h
    src/Utils/Comparison.h
    src/Utils/Box.h
    src/Utils/ArrayBox.h
    src/Utils/Str.h
    src/Utils/String.h
    src/Utils/CStr.h
    src/Utils/Numeric.h
    src/Utils/Tuple.h
    src/Utils/Bitwise.h
    src/Utils/Hash.h
    src/Utils/HashMap.h
    src/Utils/Range.h
    src/Utils/MacroIteration.h
    src/Utils/Text/Parsing.h
    src/Utils/Text/Num.h
    src/Utils/Text/StringWriter.h
    src/Utils/Text/Formatting.h
    src/Utils/Iter/MapIter.h
    src/Utils/Iter/EnumerateIter.h
    src/Utils/Iter/LinesIter.h
    src/Utils/Iter/SplitIter.h
)
source_group("Header Files" FILES ${HEADER_FILES})

set(SOURCE_FILES
    src/Utils/Debug/Logger.cpp
    src/Utils/Debug/Timer.cpp

    src/Utils/Math/Vector.cpp
    src/Utils/Math/Matrix.cpp
    src/Utils/Math/Rect.cpp
    src/Utils/Math/Color.cpp
    src/Utils/Math/Complex.cpp
    src/Utils/Math/Quaternion.cpp
    src/Utils/Math/Transform2D.cpp
    src/Utils/Math/Transform3D.cpp

    src/Utils/RichString.cpp
    src/Utils/Text.cpp
    src/Utils/Str.cpp
    src/Utils/String.cpp
    src/Utils/CStr.cpp
    src/Utils/Memory.cpp
    src/Utils/Bitwise.cpp
    src/Utils/Hash.cpp
    src/Utils/Range.cpp
    src/Utils/Text/Parsing.cpp
    src/Utils/Text/Num.cpp
    src/Utils/Text/StringWriter.cpp
    src/Utils/Text/Formatting.cpp
    src/Utils/Iter/LinesIter.cpp
)
source_group("Source Files" FILES ${SOURCE_FILES})

set(ALL_FILES
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    src
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    Q_EXT_MATCH_SYNTAX # for cool syntax features for pattern matching
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -pedantic -Wall -Wextra
    -Wcast-align
    -Wctor-dtor-privacy
    -Wdisabled-optimization
    -Wformat=2
    -Winit-self
    -Wmissing-include-dirs
    -Woverloaded-virtual
    -Wstrict-null-sentinel
    -Wstrict-overflow=5
    -Wswitch-default
    -Wundef
    -Werror

    -Wno-strict-overflow # way too buggy and aggresive
    -Wno-unused-but-set-variable
    -Wno-unused-variable # allow empty code paths (ex: not calculating logging info) when optimizing
    -Wno-sign-compare # allow comparing ints to uints
    -Wno-misleading-indentation # too lazy to fix
    -Wno-sizeof-pointer-div # allow sizeof(U) / sizeof(T)
    -Wno-missing-field-initializers # allows not initializing certain fields
    -Wno-dangling-reference # too many false-positives, especially for my Ref<T> class.
    -Wno-unused-parameter # it's simply too annoying.
)
