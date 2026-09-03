#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace anv
{
    /**
     * @brief Base class for objects managed by Ref.
     *
     * RefCounter implements intrusive reference counting: the reference count is
     * stored inside the managed object instead of in a separate control block.
     * Classes owned by Ref<T> must derive from RefCounter.
     *
     * The counter is atomic so references may be copied between threads. This
     * does not make the managed object itself thread-safe.
     */
    class RefCounter
    {
    public:
        virtual ~RefCounter() = default;

        /** @return The reference count after incrementing it. */
        std::uint32_t IncRef() const
        {
            return m_RefCount.fetch_add(1, std::memory_order_acq_rel) + 1;
        }

        /** @return The reference count after decrementing it. */
        std::uint32_t DecRef() const
        {
            return m_RefCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        }

        /** @return The object's current reference count. */
        std::uint32_t GetRefCount() const
        {
            return m_RefCount.load(std::memory_order_acquire);
        }

    private:
        mutable std::atomic<std::uint32_t> m_RefCount{0};
    };

    /**
     * @brief Intrusive smart pointer for RefCounter-derived objects.
     *
     * Ref owns one intrusive reference to its object. Copying a Ref increments
     * the object's counter, moving transfers ownership, and destruction releases
     * the reference. The object is deleted when the final Ref is released.
     *
     * @tparam T RefCounter-derived object type.
     */
    template<typename T>
    class Ref
    {
    public:
        Ref() = default;
        Ref(std::nullptr_t) {}

        /**
         * @brief Takes shared ownership of an intrusive reference-counted object.
         * @param _instance Object to retain. May be null.
         */
        Ref(T* _instance)
            : m_Instance(_instance)
        {
            static_assert(std::is_base_of_v<RefCounter, T>,
                          "Ref<T> requires T to derive from RefCounter");
            inc_ref();
        }

        Ref(const Ref& _other)
            : m_Instance(_other.m_Instance)
        {
            inc_ref();
        }

        Ref(Ref&& _other) noexcept
            : m_Instance(_other.m_Instance)
        {
            _other.m_Instance = nullptr;
        }

        template<typename T2>
        Ref(const Ref<T2>& _other)
            : m_Instance(static_cast<T*>(_other.m_Instance))
        {
            static_assert(std::is_convertible_v<T2*, T*>,
                          "Ref conversion requires compatible pointer types");
            inc_ref();
        }

        template<typename T2>
        Ref(Ref<T2>&& _other) noexcept
            : m_Instance(static_cast<T*>(_other.m_Instance))
        {
            static_assert(std::is_convertible_v<T2*, T*>,
                          "Ref conversion requires compatible pointer types");
            _other.m_Instance = nullptr;
        }

        /**
         * @brief Creates a non-retaining alias to an existing Ref.
         *
         * @warning This helper intentionally does not increment the object's
         * reference count. The returned Ref must never outlive an owning Ref.
         */
        static Ref CopyWithoutIncrement(const Ref& _other)
        {
            Ref result;
            result.m_Instance = _other.m_Instance;
            return result;
        }

        ~Ref()
        {
            dec_ref();
        }

        Ref& operator=(std::nullptr_t)
        {
            dec_ref();
            m_Instance = nullptr;
            return *this;
        }

        Ref& operator=(const Ref& _other)
        {
            if (this == &_other)
                return *this;

            _other.inc_ref();
            dec_ref();
            m_Instance = _other.m_Instance;
            return *this;
        }

        Ref& operator=(Ref&& _other) noexcept
        {
            if (this == &_other)
                return *this;

            dec_ref();
            m_Instance = _other.m_Instance;
            _other.m_Instance = nullptr;
            return *this;
        }

        template<typename T2>
        Ref& operator=(const Ref<T2>& _other)
        {
            static_assert(std::is_convertible_v<T2*, T*>,
                          "Ref conversion requires compatible pointer types");

            _other.inc_ref();
            dec_ref();
            m_Instance = static_cast<T*>(_other.m_Instance);
            return *this;
        }

        template<typename T2>
        Ref& operator=(Ref<T2>&& _other) noexcept
        {
            static_assert(std::is_convertible_v<T2*, T*>,
                          "Ref conversion requires compatible pointer types");

            dec_ref();
            m_Instance = static_cast<T*>(_other.m_Instance);
            _other.m_Instance = nullptr;
            return *this;
        }

        operator bool() const { return m_Instance != nullptr; }

        T* operator->() { return m_Instance; }
        const T* operator->() const { return m_Instance; }

        T& operator*() { return *m_Instance; }
        const T& operator*() const { return *m_Instance; }

        T* Raw() { return m_Instance; }
        const T* Raw() const { return m_Instance; }

        /**
         * @brief Replaces the managed object.
         * @param _instance New object to retain. May be null.
         */
        void Reset(T* _instance = nullptr)
        {
            if (m_Instance == _instance)
                return;

            dec_ref();
            m_Instance = _instance;
            inc_ref();
        }

        /**
         * @brief Converts this reference to a related reference type.
         *
         * Upcasts are resolved at compile time. Downcasts between polymorphic
         * related types are checked at runtime and return a null Ref on failure.
         */
        template<typename T2>
        Ref<T2> As() const
        {
            static_assert(std::is_base_of_v<RefCounter, T2>,
                          "Ref::As requires a RefCounter-derived target type");

            if (!m_Instance)
                return nullptr;

            if constexpr (std::is_convertible_v<T*, T2*>)
            {
                return Ref<T2>(static_cast<T2*>(m_Instance));
            }
            else
            {
                static_assert(std::is_polymorphic_v<T>,
                              "Ref::As downcasts require a polymorphic source type");
                T2* castInstance = dynamic_cast<T2*>(m_Instance);
                return castInstance ? Ref<T2>(castInstance) : Ref<T2>(nullptr);
            }
        }

        /** @brief Performs a runtime-checked polymorphic cast. */
        template<typename T2>
        Ref<T2> Cast() const
        {
            static_assert(std::is_base_of_v<RefCounter, T2>);

            if (!m_Instance)
                return nullptr;

            T2* castInstance = dynamic_cast<T2*>(m_Instance);
            return castInstance ? Ref<T2>(castInstance) : Ref<T2>(nullptr);
        }

        /** @brief Constructs a reference-counted object and returns its first owner. */
        template<typename... Args>
        static Ref Create(Args&&... _args)
        {
            return Ref(new T(std::forward<Args>(_args)...));
        }

        bool operator==(const Ref& _other) const
        {
            return m_Instance == _other.m_Instance;
        }

        bool operator!=(const Ref& _other) const
        {
            return !(*this == _other);
        }

        /**
         * @brief Compares the values of two managed objects.
         * @return False when either reference is null; otherwise the result of T::operator==.
         */
        bool EqualsObject(const Ref& _other) const
        {
            if (!m_Instance || !_other.m_Instance)
                return false;

            return *m_Instance == *_other.m_Instance;
        }

    private:
        void inc_ref() const
        {
            if (m_Instance)
                m_Instance->IncRef();
        }

        void dec_ref() const
        {
            if (!m_Instance)
                return;

            if (m_Instance->DecRef() == 0)
            {
                T* instance = m_Instance;
                m_Instance = nullptr;
                delete instance;
            }
        }

        template<class T2>
        friend class Ref;

        mutable T* m_Instance = nullptr;
    };
}
