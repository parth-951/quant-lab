#include "position.hpp"

namespace ql {

double Position::marketValue() const {
    return quantity * price;
}

} // namespace ql