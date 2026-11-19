# not my code :(
file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" hexlen)
math(EXPR size "${hexlen} / 2")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
file(WRITE "${OUTPUT}"
        "#include <cstddef>\n"
        "extern const unsigned char ${SYMBOL}[] = {${bytes}};\n"
        "extern const std::size_t ${SYMBOL}_size = ${size};\n")