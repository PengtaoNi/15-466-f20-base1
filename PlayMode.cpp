#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

#include "load_save_png.hpp"

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	std::vector< std::string > sprites = {
		"player_rising_upper", 
		"player_rising_lower",
		"player_dropping_upper",
		"player_dropping_lower",
		"coin",
		"cloud"
	};

	for (uint32_t i = 0; i < sprites.size(); i++) {
		std::vector< glm::u8vec4 > data;
		glm::uvec2 size;
		load_png("assets/" + sprites[i] + ".png", &size, &data, UpperLeftOrigin);

		uint32_t color_count = 0;
		uint32_t pixel_index, bit0, bit1;
		glm::u8vec4 transparent = glm::u8vec4(0x00, 0x00, 0x00, 0x00);

		for (uint32_t j = 0; j < 4; ++j) ppu.palette_table[i][j] = transparent;

		for (uint32_t x = 0; x < 8; ++x) {
			ppu.tile_table[i].bit0[x] = 0;
			ppu.tile_table[i].bit1[x] = 0;
			for (uint32_t y = 0; y < 8; ++y) {
				pixel_index = 8 * (7 - x) + (7 - y);
				bool found_color = false;
				uint32_t palette_index;

				if (data[pixel_index].a == 0) data[pixel_index] = transparent;
				for (palette_index = 0; palette_index < 4; ++palette_index) {
					if (data[pixel_index] == ppu.palette_table[i][palette_index]) {
						found_color = true;
						break;
					}
				}
				if (!found_color) {
					color_count++;
					ppu.palette_table[i][color_count] = data[pixel_index];
					palette_index = color_count;
				}
				bit0 = palette_index % 2;
				bit1 = palette_index / 2;
				ppu.tile_table[i].bit0[x] |= bit0 << y;
				ppu.tile_table[i].bit1[x] |= bit1 << y;
			}
		}
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	float vSpeed = std::min(2000.0f, (peak_height - player_at.y) * 2.0f);
	if (dropping) vSpeed = -vSpeed;

	constexpr float hSpeed = 30.0f;
	if (left.pressed) player_at.x -= hSpeed * elapsed;
	if (right.pressed) player_at.x += hSpeed * elapsed;

	if (dropping) {
		player_at.y += std::max(-player_at.y, std::min(-0.2f, vSpeed * elapsed));
		if (player_at.y == 0) dropping = false;
	}
	else {
		player_at.y += std::min(peak_height - player_at.y, std::max(0.2f, vSpeed * elapsed));
		if (player_at.y == peak_height) dropping = true;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	////background color will be some hsv-like fade:
	//ppu.background_color = glm::u8vec4(
	//	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
	//	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
	//	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
	//	0xff
	//);
	ppu.background_color = glm::u8vec4(0x87, 0xce, 0xeb, 0xff);

	////tilemap gets recomputed every frame as some weird plasma thing:
	////NOTE: don't do this in your game! actually make a map or something :-)
	//for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
	//	for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
	//		//TODO: make weird plasma thing
	//		ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
	//	}
	//}

	////background scroll:
	//ppu.background_position.x = int32_t(-0.5f * player_at.x);
	//ppu.background_position.y = int32_t(-0.5f * player_at.y);

	////player sprite:
	//ppu.sprites[0].x = int32_t(player_at.x);
	//ppu.sprites[0].y = int32_t(player_at.y);
	//ppu.sprites[0].index = 0;
	//ppu.sprites[0].attributes = 0;

	////some other misc sprites:
	//for (uint32_t i = 1; i < 63; ++i) {
	//	float amt = (i + 2.0f * background_fade) / 62.0f;
	//	ppu.sprites[i].x = int32_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * PPU466::ScreenWidth);
	//	ppu.sprites[i].y = int32_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * PPU466::ScreenWidth);
	//	ppu.sprites[i].index = 32;
	//	ppu.sprites[i].attributes = 6;
	//	if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	//}

	//player sprite
	if (dropping) {
		ppu.sprites[3].x = int32_t(player_at.x);
		ppu.sprites[3].y = int32_t(player_at.y);
		ppu.sprites[3].index = 3;
		ppu.sprites[3].attributes = 3;
		ppu.sprites[2].x = int32_t(player_at.x);
		ppu.sprites[2].y = int32_t(player_at.y + 8);
		ppu.sprites[2].index = 2;
		ppu.sprites[2].attributes = 2;
	}
	else {
		ppu.sprites[1].x = int32_t(player_at.x);
		ppu.sprites[1].y = int32_t(player_at.y);
		ppu.sprites[1].index = 1;
		ppu.sprites[1].attributes = 1;
		ppu.sprites[0].x = int32_t(player_at.x);
		ppu.sprites[0].y = int32_t(player_at.y + 8);
		ppu.sprites[0].index = 0;
		ppu.sprites[0].attributes = 0;
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
