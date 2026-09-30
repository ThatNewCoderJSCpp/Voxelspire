#ifndef VOXELSPIRE_CORE_FUNCTION_REF_HPP
#define VOXELSPIRE_CORE_FUNCTION_REF_HPP

#include <memory>
#include <type_traits>
#include <utility>

namespace voxelspire {

template <typename Signature>
class FunctionRef;

template <typename R, typename... Args>
class FunctionRef<R(Args...)> {
public:
    constexpr FunctionRef() noexcept = default;

    template <typename Fn, typename = typename std::enable_if<!std::is_same<typename std::decay<Fn>::type, FunctionRef>::value>::type>
    FunctionRef(Fn&& fn) noexcept
        : m_object(const_cast<void*>(static_cast<const void*>(std::addressof(fn)))),
          m_call([](void* object, Args... args) -> R {
              return (*static_cast<typename std::remove_reference<Fn>::type*>(object))(std::forward<Args>(args)...);
          }) {}

    explicit operator bool() const noexcept { return m_call != nullptr; }

    R operator()(Args... args) const { return m_call(m_object, std::forward<Args>(args)...); }

private:
    void* m_object = nullptr;
    R (*m_call)(void*, Args...) = nullptr;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_FUNCTION_REF_HPP