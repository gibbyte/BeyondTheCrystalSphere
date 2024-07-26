
bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true; // reached
	}
	return false;
}

void animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}


typedef enum EntityArchetype {
	arch_nil = 0,
	arch_rock = 1,
	arch_tree = 2,
	arch_player = 3,
} EntityArchetype;

typedef struct DrawFrame {
	Gfx_Image* image;
	Vector2 size;
} Sprite;
typedef enum SpriteID {
	SPRITE_nil,
	SPRITE_player,
	SPRITE_tree000,
	SPRITE_tree001,
	SPRITE_rock000,
	SPRITE_MAX,
} SpriteID;
Sprite sprites[SPRITE_MAX]; 
Sprite* get_sprite(SpriteID id) {
	if (id >= 0 && id < SPRITE_MAX) {
		return &sprites[id];
	}
	return &sprites[0];
}

typedef struct Entity {
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	bool render_sprite;
	SpriteID sprite_id;
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef struct  World {
	Entity entities[MAX_ENTITY_COUNT];
} World;
World* world = 0;

Entity* entity_create() {
	Entity* entity_found = 0; //SCALE_400_PERCENT;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* existing_entity = &world->entities[i];
		if (!existing_entity->is_valid) {
			entity_found = existing_entity;
			break;
		}
	}
	assert(entity_found, "No more entities left to create");
	entity_found->is_valid = true;
	return entity_found;
}

void entity_destroy(Entity* entity) {
	memset(entity, 0, sizeof(Entity));
}

void setup_rock(Entity* en) {
	en->arch = arch_rock;
	en->sprite_id = SPRITE_rock000;

}

void setup_tree(Entity* en) {
	en->arch = arch_tree;
	en->sprite_id = SPRITE_tree000;
//	en->sprite_id = SPRITE_tree001;
}

void setup_player(Entity* en) {
	en->arch = arch_player;
	en->sprite_id = SPRITE_player;
}

Vector2 screen_to_world(float mouseX, float mouseY, Matrix4 projection, Matrix4 view, float windowwidth, float windowheight) {
	// Normalize the mouse coordinates
	float ndcX = (mouseX / (windowwidth* 0.5f)) - 1.0f;
	float ndcY = 1.0f - (mouseY / (windowheight * 0.5f));

	//convert to homogeneous coordinates
	Vector4 ndcPos = { ndcX, ndcY, 0, 1 };	

	//Compute the inverse matrices
	Matrix4 invProJ = m4_inverse(projection);
	Matrix4 invView = m4_inverse(view);

	//Transform to world coordinates
	Vector4 clipPos = m4_transform(invProJ, ndcPos);
	Vector4 worldPos = m4_transform(invView, clipPos);

	// Return as 2d vector
	return (Vector2){ worldPos.x, worldPos.y };
}



int entry(int argc, char **argv) {
	window.title = STR("Beyond The Crystal Sphere");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0X081629FF); // Missing closing parenthesis added here

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	//sprites[SPRITE_player] = (Sprite){.image = NULL, .size = {0, 0}};

	sprites[SPRITE_player] = (Sprite){ .image = load_image_from_disk(STR("asset/player.png"), get_heap_allocator()), .size = v2(6.0, 10.0) };
	sprites[SPRITE_tree000] = (Sprite){ .image = load_image_from_disk(STR("asset/tree000.png"), get_heap_allocator()), .size = v2(12.0, 21.0) };
	sprites[SPRITE_tree001] = (Sprite){ .image = load_image_from_disk(STR("asset/tree001.png"), get_heap_allocator()), .size = v2(12.0, 21.0) };
	sprites[SPRITE_rock000] = (Sprite){ .image = load_image_from_disk(STR("asset/rockore000.png"), get_heap_allocator()), .size = v2(6.0, 4.0) };

	Gfx_Font *font = load_font_from_disk(STR("asset/Roboto-Regular.ttf"), get_heap_allocator());
	assert(font, "Failed loading Roboto-Regular.ttf, %d", GetLastError());

	const u32 font_height = 48;

	Gfx_Image* player = load_image_from_disk(fixed_string("asset/player.png"), get_heap_allocator());
	assert(player, "Player broke game");
	Gfx_Image* tree000 = load_image_from_disk(fixed_string("asset/tree000.png"), get_heap_allocator());
	assert(tree000, "tree000 broke game");
	Gfx_Image* tree001 = load_image_from_disk(fixed_string("asset/tree001.png"), get_heap_allocator());
	assert(tree001, "tree001 broke game");
	Gfx_Image* rock000 = load_image_from_disk(fixed_string("asset/rockore000.png"), get_heap_allocator());
	assert(rock000, "rock000 broke game");


	Entity* player_en = entity_create();
	setup_player(player_en);
	Entity* rock_en = entity_create();
	Entity* tree_en = entity_create();

	for (int i = 0; i < 10; i++) {
		Entity* en = entity_create();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
	}
	for (int i = 0; i < 10; i++) {
		Entity* en = entity_create();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
	}

	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float zoom = 5.3;
	Vector2 camera_pos = v2(0, 0);

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;

		//if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000);
		last_time = now;
		os_update();
		
		

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		// Camera
		{
		Vector2 target_pos = player_en->pos;
		animate_v2_to_target(&camera_pos, target_pos, delta_t, 15.0f);

	//	float zoom = 5.3;
		draw_frame.view = m4_make_scale(v3(1.0,1.0,1.0));
		draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
		draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0/5.3,1.0/5.3,1.0)));
		}
		{
			Vector2 pos = screen_to_world(input_frame.mouse_x, input_frame.mouse_y, draw_frame.projection, draw_frame.view, window.width, window.height);
			log("%f, %f", input_frame.mouse_x, input_frame.mouse_y);
			//Sprite* sprite = get_sprite(SPRITE_tree000);
			//Matrix4 xform = m4_scalar(1.0);
			//xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
			//xform         = m4_translate(xform, v3(sprite->size.x * -0.5, 0.0, 0));
			//draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);
			//break;
		}	

		// Draw entities		

		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity* en = &world->entities[i];
			if (en->is_valid) { 
		
				switch (en->arch) {

					default: {
						Sprite* sprite = get_sprite(en->sprite_id);
		
						Matrix4 xform = m4_scalar(1.0);
						xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
						xform         = m4_translate(xform, v3(sprite->size.x * -0.5, 0.0, 0));
						draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);

						draw_text(font, STR("Beer"), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
						break;
					}	
				}			
			}
		}
		
		if (is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}
		
		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A')) {
			input_axis.x -= 1.0;
		}
		if (is_key_down('D')) {
			input_axis.x += 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y -= 1.0;
		}
		if (is_key_down('W')) {
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, 100.0 * delta_t));

		gfx_update();
		seconds_counter += delta_t;
		if (seconds_counter > 1.0) {
			seconds_counter -= 1.0;
			log("FPS: %.2f", 1.0/delta_t);
			seconds_counter = 0.0;

		}
	}

	return 0;
}