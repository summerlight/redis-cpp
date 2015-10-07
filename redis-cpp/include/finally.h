#ifndef FINALLY_H
#define FINALLY_H

template<typename functor>
struct finalizer
{
    finalizer(functor&& func) : func(func) {}
    ~finalizer() {
        func();
    }

private:
    functor func;
};

template<typename functor>
finalizer<functor> finally(functor&& func)
{
    return finalizer<functor>(std::forward<functor>(func));
}

#endif
