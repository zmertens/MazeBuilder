#ifndef RESOURCE_IDENTIFIERS_HPP
#define RESOURCE_IDENTIFIERS_HPP

namespace Textures {

	enum class ID {

		AVATAR = 0,
		SPLASH_SCREEN = 1
	};
}

namespace Fonts {
	
    enum class ID {
	
        MAIN = 0,
        DM_SANS_TTF = 1
	};
}

class Texture;
// class Font;

// Forward declaration and a few type definitions
template <typename Resource, typename Identifier>
class ResourceManager;

typedef ResourceManager<Texture, Textures::ID> TextureManager;
// typedef ResourceHolder<Font, Fonts::ID> FontHolder;

#endif // RESOURCE_IDENTIFIERS_HPP
