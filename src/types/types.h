#ifndef _GEN_TYPES_H_
#define _GEN_TYPES_H_

namespace loadgen {
namespace types {

enum class Type {
    READ,
    WRITE,
    SCAN,
    DEL,
    UPDATE,
};

} // namespace types
} // namespace loadgen

#endif
