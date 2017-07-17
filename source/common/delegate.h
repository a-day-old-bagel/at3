/**
 * A Delegate in this case is a container for a stateful call to a member method or static function.
 * The goal is to be able to quickly and simply store a call to the method of any object or a call to a function
 * with state without worrying about how state is captured and whatnot.
 */
#pragma once

namespace at3 {
   /*
    * A note on type formattings: it's pretty much the same as for std::function (template arguments read like a
    * funciton signature: <return_type(param_type_0, param_type_1, ... , param_type_n)>
    * For example, for a function that returns float and takes an int and a char, a delegate type would be:
    * At3Delegate<float(int, char)>
    *
    * If you wonder why I don't just use std::function, one reason is because I've been using this since before
    * c++11 and I'm used to it. There doesn't seem to be a good motivation to change atm.
    */

   /*
    * The following macro is all you need to make delegates.  Usage:
    * AT3_DELEGATE(method, obj) makes a delegate to a member method, taking the method and class instance (object),
    * like so:
    * Given the code:
    *   int MyClass { public: int myMethod(const char a) { //do something } };
    *   MyClass myClassInstance;
    * A delegate for a call to myClassInstance.myMethod would be declared as:
    *   Delegate<int(const char)> myDelegate = AT3_DELEGATE(&MyClass::myMethod, &myClassInstance)
    * and later to invoke the delegate, just call myDelegate()
    *
    * A call to At3_DELEGATE_NOCLASS does the same but without the object reference:
    *   void function(const char* str) { ... }
    *   At3Delegate<void(const char*)> delegate = AT3_DELEGATE_NOCLASS(function);
    *   delegate("a string");
    */
  #ifndef AT3_DELEGATE
    #define AT3_DELEGATE(func, instRef) (NewAt3Delegate(func).Create<func>(instRef)) // deletage to member method
  #endif
  #ifndef AT3_DELEGATE_NOCLASS
    #define AT3_DELEGATE_NOCLASS(func) (NewAt3Delegate_NoClass(func).CreateForFunction<func>()) // delegate to function
  #endif

  // And here are come all the guts...
  template<typename returnType, typename... params>
  class At3Delegate; // this advance declaration allows for the templating ahead.
  // main Delegate class definition
  template<typename returnType, typename... params>
  class At3Delegate<returnType(params...)> {
      typedef returnType (*PtrToFunc)(void* callee, params...);
    public:
      At3Delegate() {}
      At3Delegate(void* callee, PtrToFunc function) : calleePtr(callee) , callbackFuncPtr(function) {}
      returnType operator()(params... args) const {
        return (*callbackFuncPtr)(calleePtr, args...);
      }
      bool operator==(const At3Delegate& rhs) const {
        return (calleePtr == rhs.calleePtr) && (callbackFuncPtr == rhs.callbackFuncPtr);
      }
    private:
      void* calleePtr;
      PtrToFunc callbackFuncPtr;
  };

  // Delegate spawner makes delegates, handles all the casting required for delegate operation.
  template<typename className, typename returnType, typename... params>
  struct At3DelegateSpawner {
    template<returnType (className::*func)(params...)>
    static returnType MethodCaller(void* o, params... xs){
      return (static_cast<className*>(o)->*func)(xs...);
    }
    template <returnType (*classFuncPtr)(params...)>
    static returnType FunctionCaller(void*, params... xs){
      return (classFuncPtr)(xs...);
    }
    template<returnType (className::*func)(params...)>
    inline static At3Delegate<returnType(params...)> Create(className* o){
      return At3Delegate<returnType(params...)>(o, &At3DelegateSpawner::MethodCaller<func>);
    }
    template<returnType (*classFuncPtr)(params...)>
    inline static At3Delegate<returnType(params...)> CreateForFunction(){
      return At3Delegate<returnType(params...)>(0L, &At3DelegateSpawner::FunctionCaller<classFuncPtr>);
    }
  };

  // helper function that returns delegate spawner of member method delegates
  template<typename className, typename returnType, typename... params>
  At3DelegateSpawner<className, returnType, params... > NewAt3Delegate(returnType (className::*)(params...)){
    return At3DelegateSpawner<className, returnType, params...>();
  }
  class noType{}; // noType class used in the function below
  // helper function that returns delegate spawner of function delegates
  template<typename returnType, typename... params>
  At3DelegateSpawner<noType, returnType, params... > NewAt3Delegate_NoClass(returnType (*TFnctPtr)(params...)){
    return At3DelegateSpawner<noType, returnType, params...>();
  }
}
