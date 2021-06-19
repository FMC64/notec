#include "Stream.hpp"

class Tokenizer
{
public:
	Stream &m_stream;

	Tokenizer(Stream &stream) :
		m_stream(stream)
	{
	}

	bool next(void)
	{
	}
};