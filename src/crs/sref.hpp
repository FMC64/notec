#pragma once

template <typename T>
class sref
{
	T *m_ref = nullptr;

	inline void set(T &ref)
	{
		m_ref = &ref;
	}

	inline operator bool(void) const
	{
		return m_ref != nullptr;
	}

	inline T& get(void)
	{
		return *m_ref;
	}

	inline void reset(void)
	{
		m_ref = nullptr;
	}

public:
	inline void destroy(void)
	{
		if (*this)
			get().~T();
	}

	class life;
	friend class life;

	class life
	{
		sref &m_ref;

	public:
		inline life(sref &ref, T &v) :
			m_ref(ref)
		{
			m_ref.set(v);
		}
		inline ~life(void)
		{
			m_ref.reset();
		}
	};

	inline life capture(T &v)
	{
		return life(*this, v);
	}
};