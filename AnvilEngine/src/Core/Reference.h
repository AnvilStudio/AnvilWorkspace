#pragma once
/**
 * @file Reference.h
 * @brief Smart reference counting utility for managing shared object lifetimes.
 *
 * This file provides two classes, `RefCounter` and `Ref<T>`, for implementing
 * reference counting in C++ to manage object lifetimes dynamically and efficiently.
 *
 * @namespace anv
 * The `anv` namespace encapsulates the reference counting utilities.
 *
 * @details
 * - `RefCounter`: A base class that provides atomic reference counting methods.
 * - `Ref<T>`: A templated smart pointer class that manages objects derived from `RefCounter`.
 *   It ensures automatic memory management with reference counting.
 * - Supports copy, move, and conversion semantics.
 * - Thread-safe via `std::atomic` for managing reference counts.
 *
 * Usage:
 * - Any class that needs to be reference-counted should derive from `RefCounter`.
 * - Use `Ref<T>` for managing instances of such classes.
 *
 * Key Features:
 * - Automatic memory management using reference counting.
 * - Safe handling of `nullptr`.
 * - Conversion between compatible types.
 * - Supports `operator->`, `operator*`, and comparison operators.
 * - Thread-safe reference count management.
 */

#include <memory>

namespace anv{

	class RefCounter
	{
	public:
		virtual ~RefCounter() = default;

		void IncRef() const
		{
			m_RefCount++;
		}

		void DecRef() const
		{
			m_RefCount--;
		}

		uint32_t GetRefCount() const
		{
			return m_RefCount.load();
		}

	private:
		mutable std::atomic<uint32_t> m_RefCount = 0;
	};

	template<typename T>
	class Ref
	{
	public:
		Ref()
			: m_Instance(nullptr)
		{
		}

		Ref(std::nullptr_t n)
			: m_Instance(nullptr)
		{
		}

		Ref(T* instance)
			: m_Instance(instance)
		{
			static_assert(std::is_base_of<RefCounter, T>::value, "Class is not RefCounted!");

			IncRef();
		}

		template<typename T2>
		Ref(const Ref<T2>& other)
		{
			m_Instance = (T*)other.m_Instance;
			IncRef();
		}

		template<typename T2>
		Ref(Ref<T2>&& other)
		{
			m_Instance = (T*)other.m_Instance;
			other.m_Instance = nullptr;
		}

		static Ref<T> CopyWithoutIncrement(const Ref<T>& other)
		{
			Ref<T> result = nullptr;
			result->m_Instance = other.m_Instance;
			return result;
		}

		~Ref()
		{
			DecRef();
		}

		Ref(const Ref<T>& other)
			: m_Instance(other.m_Instance)
		{
			IncRef();
		}

		Ref& operator=(std::nullptr_t)
		{
			DecRef();
			m_Instance = nullptr;
			return *this;
		}

		Ref& operator=(const Ref<T>& other)
		{
			if (this == &other)
				return *this;

			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		Ref& operator=(const Ref<T2>& other)
		{
			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		Ref& operator=(Ref<T2>&& other)
		{
			DecRef();

			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}

		operator bool() { return m_Instance != nullptr; }
		operator bool() const { return m_Instance != nullptr; }

		T* operator->() { return m_Instance; }
		const T* operator->() const { return m_Instance; }

		T& operator*() { return *m_Instance; }
		const T& operator*() const { return *m_Instance; }

		T* Raw() { return  m_Instance; }
		const T* Raw() const { return  m_Instance; }

		void Reset(T* instance = nullptr)
		{
			DecRef();
			m_Instance = instance;
		}

		template<typename T2>
		Ref<T2> As() const
		{
			return Ref<T2>(*this);
		}

		template<typename... Args>
		static Ref<T> Create(Args&&... args)
		{
			return Ref<T>(new T(std::forward<Args>(args)...));
		}

		bool operator==(const Ref<T>& other) const
		{
			return m_Instance == other.m_Instance;
		}

		bool operator!=(const Ref<T>& other) const
		{
			return !(*this == other);
		}

		bool EqualsObject(const Ref<T>& other)
		{
			if (!m_Instance || !other.m_Instance)
				return false;

			return *m_Instance == *other.m_Instance;
		}
	private:
		void IncRef() const
		{
			if (m_Instance)
			{
				m_Instance->IncRef();
			}
		}

		void DecRef() const
		{
			if (m_Instance)
			{
				m_Instance->DecRef();

				if (m_Instance->GetRefCount() == 0)
				{
					delete m_Instance;
					m_Instance = nullptr;
				}
			}
		}

		template<class T2>
		friend class Ref;
		mutable T* m_Instance;
	};
}
