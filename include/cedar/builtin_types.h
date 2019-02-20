// This defines the builin types for use in the VM. It does so by invoking
// an BUILTIN_TYPE() macro which is expected to be defined at the point that this is
// included. (See: http://en.wikipedia.org/wiki/X_Macro for more.)


// the first argument is the internal "name" of the type, the second is the
// in-language string name of the type
//
//

#ifdef BUILTIN_TYPE

BUILTIN_TYPE(type, "Type")
BUILTIN_TYPE(lambda, "Lambda")
BUILTIN_TYPE(object, "Object")
BUILTIN_TYPE(list, "List")
BUILTIN_TYPE(nil, "Nil")
BUILTIN_TYPE(number, "Number")
BUILTIN_TYPE(string, "String")
BUILTIN_TYPE(vector, "Vector")
BUILTIN_TYPE(dict, "Dict")
BUILTIN_TYPE(symbol, "Symbol")
BUILTIN_TYPE(keyword, "Keyword")
BUILTIN_TYPE(fiber, "Fiber")

#endif
