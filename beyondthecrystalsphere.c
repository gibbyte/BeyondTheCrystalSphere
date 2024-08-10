const int tile_width = 8;
const float entity_selection_radius = 16.0f;

int world_pos_to_tile_pos(float world_pos)
{
	return roundf(world_pos / (float)tile_width);
}

float tile_pos_to_world_pos(int tile_pos)
{
	return ((float)tile_pos * (float)tile_width);
}

Vector2 round_v2_to_tile(Vector2 world_pos)
{
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}

bool almost_equals(float a, float b, float epsilon)
{
	return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float *value, float target, float delta_t, float rate)
{
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true; // reached
	}
	return false;
}

void animate_v2_to_target(Vector2 *value, Vector2 target, float delta_t, float rate)
{
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

typedef struct Sprite
{
	Gfx_Image *image;
	Vector2 size;
} Sprite;
typedef enum SpriteID
{
	SPRITE_nil,
	SPRITE_player,
	SPRITE_tree000,
	SPRITE_tree001,
	SPRITE_rock000,
	SPRITE_MAX,
} SpriteID;
Sprite sprites[SPRITE_MAX];
Sprite *get_sprite(SpriteID id)
{
	if (id >= 0 && id < SPRITE_MAX)
	{
		return &sprites[id];
	}
	return &sprites[0];
}
typedef enum EntityArchetype
{
	arch_nil = 0,
	arch_rock = 1,
	arch_tree = 2,
	arch_player = 3,
} EntityArchetype;

typedef struct Entity
{
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	bool render_sprite;
	SpriteID sprite_id;
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
} World;
World *world = 0;

Entity *entity_create()
{
	Entity *entity_found = 0; // SCALE_400_PERCENT;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++)
	{
		Entity *existing_entity = &world->entities[i];
		if (!existing_entity->is_valid)
		{
			entity_found = existing_entity;
			break;
		}
	}
	assert(entity_found, "No more entities left to create");
	entity_found->is_valid = true;
	return entity_found;
}

void entity_destroy(Entity *entity)
{
	memset(entity, 0, sizeof(Entity));
}
void setup_player(Entity *en)
{
	en->arch = arch_player;
	en->sprite_id = SPRITE_player;
}
void setup_rock(Entity *en)
{
	en->arch = arch_rock;
	en->sprite_id = SPRITE_rock000;
}

void setup_tree(Entity *en)
{
	en->arch = arch_tree;
	en->sprite_id = SPRITE_tree000;
	//	en->sprite_id = SPRITE_tree001;
}

Vector2 screen_to_world()
{

	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;
	// Compute the inverse matrices
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	// Transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	// log("%f, %f", world_pos.x, world_pos.y);

	// Return as 2D vector
	return (Vector2){world_pos.x, world_pos.y};
}

int entry(int argc, char **argv)
{
	window.title = STR("Beyond The Crystal Sphere");
	window.width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0X081629FF); // Missing closing parenthesis added here

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(STR("asset/player.png"), get_heap_allocator()), .size = v2(6.0, 10.0)};
	sprites[SPRITE_tree000] = (Sprite){.image = load_image_from_disk(STR("asset/tree000.png"), get_heap_allocator()), .size = v2(12.0, 21.0)};
	sprites[SPRITE_tree001] = (Sprite){.image = load_image_from_disk(STR("asset/tree001.png"), get_heap_allocator()), .size = v2(12.0, 21.0)};
	sprites[SPRITE_rock000] = (Sprite){.image = load_image_from_disk(STR("asset/rockore000.png"), get_heap_allocator()), .size = v2(6.0, 4.0)};

	Gfx_Font *font = load_font_from_disk(STR("asset/Roboto-Regular.ttf"), get_heap_allocator());
	assert(font, "Failed loading Roboto-Regular.ttf, %d", GetLastError());

	const u32 font_height = 48;

	Entity *player_en = entity_create();
	setup_player(player_en);

	for (int i = 0; i < 10; i++)
	{
		Entity *en = entity_create();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		en->pos = round_v2_to_tile(en->pos);
		// en->pos.y -= tile_width * 0.5;
	}
	for (int i = 0; i < 10; i++)
	{
		Entity *en = entity_create();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		en->pos = round_v2_to_tile(en->pos);
		// en->pos.y -= tile_width * 0.5;
	}

	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float zoom = 5.3;
	Vector2 camera_pos = v2(0, 0);

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;

		// if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000);
		last_time = now;
		os_update();

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		// Camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 30.0f);

			//	float zoom = 5.3;
			draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));
		}

		// mouse pos in world space test

		Vector2 mouse_pos_world = screen_to_world();
		// log("%f, %f", input_frame.mouse_x, input_frame.mouse_y);
		// draw_text(font, sprint(temp, STR("%f %f"), mouse_pos_world.x, mouse_pos_world.y), font_height, mouse_pos_world, v2(0.1, 0.1), COLOR_RED);
		int mouse_tile_x = world_pos_to_tile_pos(mouse_pos_world.x);
		int mouse_tile_y = world_pos_to_tile_pos(mouse_pos_world.y);
		// mouse pos in world space test
		{
			// log("%f, %f", mouse_pos_world.x, mouse_pos_world.y);
			// draw_text(font, sprint(temp, STR("%f %f"), mouse_pos_world.x, mouse_pos_world.y), font_height, mouse_pos_world, v2(0.1, 0.1), COLOR_RED);
			for (int i = 0; i < MAX_ENTITY_COUNT; i++)
			{
				Entity *en = &world->entities[i];
				if (en->is_valid)
				{
					Sprite *sprite = get_sprite(en->sprite_id);

					int entity_tile_x = world_pos_to_tile_pos(en->pos.x);
					int entity_tile_y = world_pos_to_tile_pos(en->pos.y);

					if (fabsf(v2_dist(en->pos, mouse_pos_world)) < entity_selection_radius)
						;
					draw_rect(v2(tile_pos_to_world_pos(entity_tile_x) + tile_width * -0.5, tile_pos_to_world_pos(entity_tile_y) + tile_width * -0.5), v2(tile_width, tile_width), v4(0.5, 0.5, 0.5, 0.5));

					/*
					Range2f bounds = range2f_make_bottom_center(sprite->size);
					bounds = range2f_shift(bounds, en->pos);
					bounds.min = v2_sub(bounds.min, v2(10.0, 10.0));
					bounds.max = v2_add(bounds.max, v2(10.0, 10.0));SSS

					Vector4 col = COLOR_WHITE;
					col.a = 0.4;
					if (range2f_contains(bounds, mouse_pos_world))
					{
						col.a = 1.0;
					}

					draw_rect(bounds.min, range2f_size(bounds), col);
					*/
				}
			}
		}

		// draw a bunch of Squares
		{
			int player_tile_x = world_pos_to_tile_pos(player_en->pos.x);
			int player_tile_y = world_pos_to_tile_pos(player_en->pos.y);
			int tile_radius_x = 40;
			int tile_radius_y = 30;
			for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++)
			{
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++)
				{

					if ((x + (y % 2 == 0)) % 2 == 0)
					{
						Vector4 col = v4(0.5, 0.5, 0.5, 0.1); // Lighter color
						float x_pos = x * tile_width;
						float y_pos = y * tile_width;
						draw_rect(v2(x_pos + tile_width * -0.5, y_pos + tile_width * -0.5), v2(tile_width, tile_width), col);
					}
				}
			}

			// draw_rect(v2(tile_pos_to_world_pos(mouse_tile_x) + tile_width * -0.5, tile_pos_to_world_pos(mouse_tile_y) + tile_width * -0.5), v2(tile_width, tile_width), v4(0.5, 0.5, 0.5, 0.5));
		}

		// render

		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *en = &world->entities[i];
			if (en->is_valid)
			{

				switch (en->arch)
				{

				default:
				{
					Sprite *sprite = get_sprite(en->sprite_id);
					Matrix4 xform = m4_scalar(1.0);
					xform = m4_translate(xform, v3(0, tile_width * -0.5, 0));
					xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
					xform = m4_translate(xform, v3(sprite->size.x * -0.5, 0.0, 0));
					draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);

					// debug pos
					// draw_text(font, sprint(temp, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
					break;
				}
				}
			}
		}

		if (is_key_just_pressed(KEY_ESCAPE))
		{
			window.should_close = true;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A'))
		{
			input_axis.x -= 1.0;
		}
		if (is_key_down('D'))
		{
			input_axis.x += 1.0;
		}
		if (is_key_down('S'))
		{
			input_axis.y -= 1.0;
		}
		if (is_key_down('W'))
		{
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, 100.0 * delta_t));

		gfx_update();
		seconds_counter += delta_t;
		frame_count += 1;
		if (seconds_counter > 1.0)
		{
			// log("fps: %i", frame_count);
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}