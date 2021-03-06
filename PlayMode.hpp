#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right;

	//player:
	glm::vec2 player_at = glm::vec2(124.0f, 100.0f);
	bool dropping = true;
	bool lost = false;

	float peak_height = 100.0f;
	float new_peak_height = 100.0f;

	//coin:
	glm::vec2 coin_at = glm::vec2(0.0f);

	//cloud:
	std::vector< glm::vec2 > cloud_at;

	//camera:
	float camera_height = 0.0f;

	bool restart = true;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
