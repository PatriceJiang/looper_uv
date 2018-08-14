#pragma once

#include <cassert>
#include <functional>
#include <memory>

template<typename T>
class Option {
public:
    Option() : is_empty(true) {}
    Option(T v) :data(v),is_empty(false) {}
    Option(const Option &b) :data(b.data),is_empty(b.is_empty){}

    T& get() {
        assert(!is_empty);
        return data;
    }
    
    bool isDefined() const {  return !is_empty; }
    bool isEmpty() const { return is_empty; }
    bool isUndefined() const {return !is_empty;}

    template<typename S>
    Option<S> map(std::function<S(T)> mapF) const {
        if(isEmpty()) {
            return Option<S>::None;
        }
        return Option<S>(mapF(data));
    }

    static Option<T> None;
    static Option<T> Some(T value);

    operator bool() const {
        return isDefined();
    }

    bool operator == (const Option<T> &b) {
        return isDefined() && b.isDefined() && data == b.data;
    }

    T& operator *() {
        return get();
    }

private:
    T data;
    bool is_empty{ true };
};

template<typename T> 
Option<T> Option<T>::None = Option<T>();

template<typename T>
Option<T> Option<T>::Some(T value) {
    return Option<T>(value);    
}

