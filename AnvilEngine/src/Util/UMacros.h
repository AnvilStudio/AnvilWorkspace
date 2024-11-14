#pragma once

#include <memory>
#include <vector>

namespace anv
{
// simplification for std::vector
#define _vec(_ty) std::vector<_ty>  

// simplification for std::shared_ptr
#define _shared(_ty) std::shared_ptr<_ty>

// simplification for std::unique_ptr
#define _unique(_ty) std::unique_ptr<_ty>
}