#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <time.h>

#include "load_save_png.hpp"

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	std::vector< std::string > raw_tiles = {
		"player_rising_upper", 
		"player_rising_lower",
		"player_dropping_upper",
		"player_dropping_lower",
		"coin",
		"cloud",
		"bg",
		"tramp"
	};

	for (uint32_t i = 0; i < raw_tiles.size(); i++) {
		std::vector< glm::u8vec4 > data;
		glm::uvec2 size;
		load_png("assets/" + raw_tiles[i] + ".png", &size, &data, UpperLeftOrigin);

		uint32_t color_count = 0;
		uint32_t pixel_index, bit0, bit1;
		glm::u8vec4 transparent = glm::u8vec4(0x00, 0x00, 0x00, 0x00);

		for (uint32_t x = 0; x < 8; ++x) {
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
		} else if (evt.key.keysym.sym == SDLK_r) {
			restart = true;
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

	if (restart) {
		player_at = glm::vec2(124.0f, 100.0f);
		dropping = true;
		peak_height = 100.0f;
		new_peak_height = 100.0f;
		coin_at = glm::vec2(0.0f);
		cloud_at = {};
		for (uint32_t i = 0; i < 16; i++) {
			cloud_at.emplace_back(glm::vec2((rand() / (float)RAND_MAX) * 256.0f,
				                            (rand() / (float)RAND_MAX) * 240.0f + 120.0f));
		}
		camera_height = 0.0f;
		lost = false;
		restart = false;
	}

	float vSpeed = std::min(200.0f, (peak_height - player_at.y) * 3.0f);
	if (dropping) vSpeed = -vSpeed;

	constexpr float hSpeed = 80.0f;
	if (left.pressed) player_at.x -= hSpeed * elapsed;
	if (right.pressed) player_at.x += hSpeed * elapsed;
	player_at.x = std::max(0.0f, std::min(248.0f, player_at.x));

	if (!lost) {
		if (dropping) {
			player_at.y += std::max(13.0f - player_at.y, std::min(-0.2f, vSpeed * elapsed));
			if (player_at.y == 13.0f) {
				dropping = false;
				if (player_at.x < 116 || player_at.x > 132) lost = true;
				else peak_height = new_peak_height;
			}
		} else {
			player_at.y += std::min(peak_height - player_at.y, std::max(0.2f, vSpeed * elapsed));
			if (player_at.y == peak_height) dropping = true;
		}
	}

	if (coin_at == glm::vec2(0.0f)) {
		coin_at = glm::vec2((rand() / (float)RAND_MAX) * 160.0f + 44.0f, new_peak_height + 12.0f);
	}
	if (-16.0f <= player_at.y - coin_at.y && player_at.y - coin_at.y <= 8.0f &&
		-8.0f <= player_at.x - coin_at.x && player_at.x - coin_at.x <= 8.0f) {
		new_peak_height = peak_height + 20.0f;
		coin_at = glm::vec2((rand() / (float)RAND_MAX) * 160.0f + 44.0f, new_peak_height + 12.0f);
	}

	if (player_at.y > 120.0f) {
		camera_height = player_at.y - 120.0f;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	ppu.background_color = glm::u8vec4(0x87, 0xce, 0xeb, 0xff);

	for (uint32_t i = 0; i < 24; ++i) {
		ppu.sprites[i].x = 255;
		ppu.sprites[i].y = 255;
		ppu.sprites[3].index = 0;
		ppu.sprites[3].attributes = 128;
	}

	//player sprite
	if (dropping) {
		ppu.sprites[3].x = int32_t(player_at.x);
		ppu.sprites[3].y = int32_t(player_at.y - camera_height);
		ppu.sprites[3].index = 3;
		ppu.sprites[3].attributes = 3;
		ppu.sprites[2].x = int32_t(player_at.x);
		ppu.sprites[2].y = int32_t(player_at.y + 8 - camera_height);
		ppu.sprites[2].index = 2;
		ppu.sprites[2].attributes = 2;
	}
	else {
		ppu.sprites[1].x = int32_t(player_at.x);
		ppu.sprites[1].y = int32_t(player_at.y - camera_height);
		ppu.sprites[1].index = 1;
		ppu.sprites[1].attributes = 1;
		ppu.sprites[0].x = int32_t(player_at.x);
		ppu.sprites[0].y = int32_t(player_at.y + 8 - camera_height);
		ppu.sprites[0].index = 0;
		ppu.sprites[0].attributes = 0;
	}

	//coin sprite
	if (coin_at.y - camera_height <= 240) {
		ppu.sprites[4].x = int32_t(coin_at.x);
		ppu.sprites[4].y = int32_t(coin_at.y - camera_height);
		ppu.sprites[4].index = 4;
		ppu.sprites[4].attributes = 4;
	}

	//trampoline sprite
	if (camera_height <= 4.0f) {
		ppu.sprites[7].x = int32_t(124);
		ppu.sprites[7].y = int32_t(10 - camera_height);
		ppu.sprites[7].index = 7;
		ppu.sprites[7].attributes = 7;
	}

	//cloud sprites
	for (uint32_t i = 0; i < 16; i++) {
		if (!(camera_height < 120 && cloud_at[i].y - camera_height > 240)) {
			ppu.sprites[8 + i].x = int32_t(cloud_at[i].x);
			ppu.sprites[8 + i].y = int32_t(cloud_at[i].y - camera_height);
			ppu.sprites[8 + i].index = 5;
			ppu.sprites[8 + i].attributes = 5;
		}
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
