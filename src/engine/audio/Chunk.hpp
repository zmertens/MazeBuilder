#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <string>
#include <memory>

#include "../SdlWindow.hpp"

class Chunk
{
public:
	typedef std::unique_ptr<Chunk> Ptr;
public:
	explicit Chunk(const std::string& path);
	virtual ~Chunk();

	void cleanUp();

	Mix_Chunk* getChunk() const;

private:
	Mix_Chunk* mChunk;
private:
	Chunk(const Chunk& other);
	Chunk& operator=(const Chunk& other);
};

#endif // CHUNK_HPP
