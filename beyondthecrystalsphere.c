/*
// engine changes
inline float32 v2_length(Vector2 a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}
*/
// added please remove
inline float v2_dist(Vector2 a, Vector2 b)
{
	return v2_length(v2_sub(a, b));
}

#define m4_identity() m4_make_scale(v3(1, 1, 1))

Vector2 range2f_get_center(Range2f r)
{
	return (Vector2){(r.min.x - r.max.x) * 0.5 + r.min.x, (r.min.y - r.max.y) * 0.5 + r.min.y};
}

// the scuff zone

Draw_Quad ndc_quad_to_screen_quad(Draw_Quad ndc_quad)
{

	// NOTE: we're assuming these are the screen space matricies.
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;

	Matrix4 ndc_to_screen_space = m4_identity();
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, m4_inverse(proj));
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, view);

	ndc_quad.bottom_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_left), 0, 1)).xy;
	ndc_quad.bottom_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_right), 0, 1)).xy;
	ndc_quad.top_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_left), 0, 1)).xy;
	ndc_quad.top_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_right), 0, 1)).xy;

	return ndc_quad;
}

// 0 -> 1

// Item drops move up and down
float sin_breathe(float time, float rate)
{
	return (sin(time * rate) + 1.0 / 2.0);
}

// Checks if two floating-point numbers are almost equal within a given epsilon.
bool almost_equals(float a, float b, float epsilon)
{
	return fabs(a - b) <= epsilon;
}

// Animates a floating-point value towards a target value over time.
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

// Animates a 2D vector towards a target vector over time.
void animate_v2_to_target(Vector2 *value, Vector2 target, float delta_t, float rate)
{
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

Range2f quad_to_range(Draw_Quad quad)
{
	return (Range2f){quad.bottom_left, quad.top_right};
}

// utils
// :tweaks
Vector4 bg_box_col = {0, 0, 0, 0.5};

const int tile_width = 8;
const float entity_selection_radius = 16.0f;
const float player_pickup_radius = 16.0f;

const int rock000_health = 3;
const int tree000_health = 3;

// Converts a world position to a tile position by dividing by the tile width and rounding.
int world_pos_to_tile_pos(float world_pos)
{
	return roundf(world_pos / (float)tile_width);
}

// Converts a tile position to a world position by multiplying with the tile width.
float tile_pos_to_world_pos(int tile_pos)
{
	return ((float)tile_pos * (float)tile_width);
}

// Rounds a world position to the nearest tile position.
Vector2 round_v2_to_tile(Vector2 world_pos)
{
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}

// Structure representing a sprite with an image and size.
typedef struct Sprite
{
	Gfx_Image *image;
} Sprite;

// Enumeration for sprite IDs.
typedef enum SpriteID
{
	SPRITE_nil,
	SPRITE_player,
	SPRITE_tree000,
	SPRITE_tree001,
	SPRITE_rock000,
	SPRITE_item_oxygenplant000,
	SPRITE_item_wood_tree000,
	SPRITE_item_ore_rock000,
	SPRITE_MAX,
} SpriteID;

// Array of sprites.
Sprite sprites[SPRITE_MAX];

// Retrieves a sprite by its ID.
Sprite *get_sprite(SpriteID id)
{
	if (id >= 0 && id < SPRITE_MAX)
	{
		return &sprites[id];
	}
	return &sprites[0];
}

Vector2 get_sprite_size(Sprite *sprite)
{
	return (Vector2){sprite->image->width, sprite->image->height};
}

// Enumeration for entity archetypes.
typedef enum EntityArchetype
{
	arch_nil = 0,
	arch_rock000 = 1,
	arch_tree000 = 2,
	arch_player = 3,

	arch_item_oxygenplant000 = 4,
	arch_item_wood_tree000 = 5,
	arch_item_ore_rock000 = 6,
	ARCH_MAX,
} EntityArchetype;

SpriteID get_sprite_id_from_archetype(EntityArchetype arch)
{
	switch (arch)
	{
	case arch_rock000:
		return SPRITE_rock000;
	case arch_tree000:
		return SPRITE_tree000;
	case arch_player:
		return SPRITE_player;
	case arch_item_oxygenplant000:
		return SPRITE_item_oxygenplant000;
	case arch_item_wood_tree000:
		return SPRITE_item_wood_tree000;
	case arch_item_ore_rock000:
		return SPRITE_item_ore_rock000;
	default:
		return 0;
	}
}

// Structure representing an entity.
typedef struct Entity
{
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	bool render_sprite;
	SpriteID sprite_id;
	int health;
	bool destroyable_world_item;
	bool is_item;
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef struct ItemData
{
	int amount;

} ItemData;

// Structure representing the world with an array of entities.
typedef struct World
{

	Entity entities[MAX_ENTITY_COUNT];

	ItemData inventory_items[ARCH_MAX];

} World;
World *world = 0;

// Structure representing the world frame with a selected entity.
typedef struct WorldFrame
{
	Entity *selected_entity;
} WorldFrame;
WorldFrame world_frame;

// Creates a new entity and returns a pointer to it.
Entity *entity_create()
{
	Entity *entity_found = 0;
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

// Destroys an entity by resetting its memory.
void entity_destroy(Entity *entity)
{
	memset(entity, 0, sizeof(Entity));
}

// Sets up a player entity.
void setup_player(Entity *en)
{
	en->arch = arch_player;
	en->sprite_id = SPRITE_player;
}

// Sets up a rock entity.
void setup_rock(Entity *en)
{
	en->arch = arch_rock000;
	en->sprite_id = SPRITE_rock000;
	en->health = rock000_health;
	en->destroyable_world_item = true;
}

// Sets up a tree entity.
void setup_tree(Entity *en)
{
	en->arch = arch_tree000;
	en->sprite_id = SPRITE_tree000;
	en->health = tree000_health;
	en->destroyable_world_item = true;
}

void setup_item_oxygenplant000(Entity *en)
{
	en->arch = arch_item_oxygenplant000;
	en->sprite_id = SPRITE_item_oxygenplant000;
}

void setup_item_wood_tree000(Entity *en)
{
	en->arch = arch_item_wood_tree000;
	en->sprite_id = SPRITE_item_wood_tree000;
	en->is_item = true;
}

void setup_item_ore_rock000(Entity *en)
{
	en->arch = arch_item_ore_rock000;
	en->sprite_id = SPRITE_item_ore_rock000;
	en->is_item = true;
}

Vector2 get_mouse_pos_in_ndc()
{
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;
	float window_w = window.width;
	float window_h = window.height;

	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	return (Vector2){ndc_x, ndc_y};
}
// Converts screen coordinates to world coordinates.
Vector2 screen_to_world()
{
	// Get mouse position
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;

	// Get projection and view matrices
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;

	// Get window dimensions
	float window_w = window.width;
	float window_h = window.height;

	// Convert mouse position to normalized device coordinates (NDC)
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	// Create a Vector4 for the world position
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);

	// Transform the NDC coordinates to world coordinates
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);

	// Return the world coordinates as a Vector2
	return (Vector2){world_pos.x, world_pos.y};
}

// Entry point of the program.
int entry(int argc, char **argv)
{
	window.title = STR("Beyond The Crystal Sphere");
	window.width = 1280;
	window.height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0X081629FF);

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[0] = (Sprite){
		.image = load_image_from_disk(STR("asset/missing_tex.png"), get_heap_allocator())};
	sprites[SPRITE_player] = (Sprite){
		.image = load_image_from_disk(STR("asset/player.png"), get_heap_allocator())};
	sprites[SPRITE_tree000] = (Sprite){
		.image = load_image_from_disk(STR("asset/tree000.png"), get_heap_allocator())};
	sprites[SPRITE_tree001] = (Sprite){
		.image = load_image_from_disk(STR("asset/tree001.png"), get_heap_allocator())};
	sprites[SPRITE_rock000] = (Sprite){
		.image = load_image_from_disk(STR("asset/rock000.png"), get_heap_allocator())};
	sprites[SPRITE_item_oxygenplant000] = (Sprite){
		.image = load_image_from_disk(STR("asset/item_oxygenplant000.png"), get_heap_allocator())};
	sprites[SPRITE_item_wood_tree000] = (Sprite){
		.image = load_image_from_disk(STR("asset/item_wood_tree000.png"), get_heap_allocator())};
	sprites[SPRITE_item_ore_rock000] = (Sprite){
		.image = load_image_from_disk(STR("asset/item_ore_rock000.png"), get_heap_allocator())};

	Gfx_Font *font = load_font_from_disk(STR("asset/Roboto-Regular.ttf"), get_heap_allocator());
	assert(font, "Failed loading Roboto-Regular.ttf, %d", GetLastError());

	const u32 font_height = 48;

	// test item adding
	//{
	world->inventory_items[arch_item_wood_tree000].amount = 5;
	// world->inventory_items[arch_item_ore_rock000].amount = 5;
	//}
	Entity *player_en = entity_create();
	setup_player(player_en);

	for (int i = 0; i < 10; i++)
	{
		Entity *en = entity_create();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		en->pos = round_v2_to_tile(en->pos);
	}
	for (int i = 0; i < 10; i++)
	{
		Entity *en = entity_create();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		en->pos = round_v2_to_tile(en->pos);
	}

	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float zoom = 5.3;
	Vector2 camera_pos = v2(0, 0);

	float64 last_time = os_get_elapsed_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();
		world_frame = (WorldFrame){0};
		float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;

		last_time = now;
		os_update();

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		// Camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 30.0f);

			draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));
		}

		// Mouse position in world space
		Vector2 mouse_pos_world = screen_to_world();
		int mouse_tile_x = world_pos_to_tile_pos(mouse_pos_world.x);
		int mouse_tile_y = world_pos_to_tile_pos(mouse_pos_world.y);

		// Entity selection based on mouse position

		Entity *selected_entity;
		{
			float smallest_dist = INFINITY;

			for (int i = 0; i < MAX_ENTITY_COUNT; i++)
			{
				Entity *en = &world->entities[i];
				if (en->is_valid && en->destroyable_world_item)
				{
					Sprite *sprite = get_sprite(en->sprite_id);

					int entity_tile_x = world_pos_to_tile_pos(en->pos.x);
					int entity_tile_y = world_pos_to_tile_pos(en->pos.y);

					float dist = fabsf(v2_dist(en->pos, mouse_pos_world));
					if (dist < entity_selection_radius)
					{
						if (!world_frame.selected_entity || (dist < smallest_dist))
						{
							world_frame.selected_entity = en;
							smallest_dist = dist;
						}
					}
				}
			}
		}

		// Draw a bunch of squares
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
		}

		// pick up near by items use phyisc to pull in item
		{
			for (int i = 0; i < MAX_ENTITY_COUNT; i++)
			{
				Entity *en = &world->entities[i];
				if (en->is_valid && en->is_item)
				{
					if (v2_dist(en->pos, player_en->pos) < 1.0f)
					{
						world->inventory_items[en->arch].amount += 1;
						entity_destroy(en);
					}
					else if (v2_dist(en->pos, player_en->pos) < player_pickup_radius)
					{
						Vector2 direction = v2_sub(player_en->pos, en->pos);
						float distance = v2_length(direction);
						Vector2 force = v2_mulf(v2_normalize(direction), 9000.0f / (distance * distance));
						en->pos = v2_add(en->pos, v2_mulf(force, delta_t));
					}
				}
			}
		}

		// clicky mc clickface
		{
			Entity *selected_en = world_frame.selected_entity;

			if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);
				if (selected_en)
				{
					selected_en->health -= 1;
					if (selected_en->health <= 0)
					{
						switch (selected_en->arch)
						{
						case arch_tree000:
						{
							Entity *en = entity_create();
							setup_item_wood_tree000(en);
							en->pos = selected_en->pos;
						}
						break;

						case arch_rock000:
						{
							Entity *en = entity_create();
							setup_item_ore_rock000(en);
							en->pos = selected_en->pos;
						}
						break;

						default:
						{
						}
						break;
						}

						entity_destroy(selected_en);
					}
				}
			}
		}

		// Render entities
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
					if (sprite == NULL)
					{
						// Handle error or continue to the next entity
						continue;
					}

					// Get the size of the sprite
					Vector2 sprite_size = get_sprite_size(sprite);

					// Initialize the transformation matrix with a scalar value of 1.0 (identity matrix)
					Matrix4 xform = m4_scalar(1.0);

					// Apply translations to the transformation matrix
					if (en->is_item)
					{
						xform = m4_translate(xform, v3(0, 2.0 * sin_breathe(os_get_elapsed_seconds(), 5.0), 0));
					}

					xform = m4_translate(xform, v3(0, tile_width * -0.5, 0));
					xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
					xform = m4_translate(xform, v3(sprite_size.x * -0.5, 0.0, 0));

					// Set the color to white by default
					Vector4 col = COLOR_WHITE;

					// Change color to red if the entity is the selected entity
					if (world_frame.selected_entity == en)
					{
						col = COLOR_RED;
					}

					// Draw the sprite with the transformed matrix and the selected color
					draw_image_xform(sprite->image, xform, sprite_size, col);
					break;
				}
				}
			}
		}
		// do UI rendering
		{
			float width = 240.0;
			float height = 135.0;
			draw_frame.camera_xform = m4_scalar(1.0);
			draw_frame.projection = m4_make_orthographic_projection(0.0, width, 0.0, height, -1, 10);

			float y_pos = 70.0;

			int item_count = 0;
			for (int i = 0; i < ARCH_MAX; i++)
			{
				ItemData *item = &world->inventory_items[i];
				if (item->amount > 0)
				{
					item_count += 1;
				}
			}

			const float icon_thing = 8.0;
			float icon_width = icon_thing;

			const int icon_row_count = 8;

			float entire_thing_width_idk = icon_row_count * icon_width;
			float x_start_pos = (width / 2.0) - (entire_thing_width_idk / 2.0);

			// bg box rendering thing
			{
				Matrix4 xform = m4_identity();
				xform = m4_translate(xform, v3(x_start_pos, y_pos, 0.0));
				draw_rect_xform(xform, v2(entire_thing_width_idk, icon_width), bg_box_col);
			}

			int slot_index = 0;
			for (int i = 0; i < ARCH_MAX; i++)
			{
				ItemData *item = &world->inventory_items[i];
				if (item->amount > 0)
				{

					float slot_index_offset = slot_index * icon_width;

					Matrix4 xform = m4_scalar(1.0);
					xform = m4_translate(xform, v3(x_start_pos + slot_index_offset, y_pos, 0.0));

					Sprite *sprite = get_sprite(get_sprite_id_from_archetype(i));

					float is_selected_alpha = 0.0;

					Draw_Quad *quad = draw_rect_xform(xform, v2(8, 8), v4(1, 1, 1, 0.2));
					Range2f icon_box = quad_to_range(*quad);
					if (range2f_contains(icon_box, get_mouse_pos_in_ndc()))
					{
						is_selected_alpha = 1.0;
					}

					Matrix4 box_bottom_right_xform = xform;

					xform = m4_translate(xform, v3(icon_width * 0.5, icon_width * 0.5, 0.0));

					if (is_selected_alpha == 1.0)
					{
						float scale_adjust = 0.3; // 0.1 * sin_breathe(os_get_current_time_in_seconds(), 20.0);
						xform = m4_scale(xform, v3(1 + scale_adjust, 1 + scale_adjust, 1));
					}
					{
						// could also rotate ...
						// float adjust = PI32 * 2.0 * sin_breathe(os_get_current_time_in_seconds(), 1.0);
						// xform = m4_rotate_z(xform, adjust);
					}
					xform = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, get_sprite_size(sprite).y * -0.5, 0));
					draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);

					// draw_text_xform(font, STR("5"), font_height, box_bottom_right_xform, v2(0.1, 0.1), COLOR_WHITE);

					// tooltip
					if (is_selected_alpha == 1.0)
					{
						Draw_Quad screen_quad = ndc_quad_to_screen_quad(*quad);
						Range2f screen_range = quad_to_range(screen_quad);
						Vector2 icon_center = range2f_get_center(screen_range);

						// icon_center
						Matrix4 xform = m4_scalar(1.0);

						Vector2 box_size = v2(40, 60);

						// xform = m4_pivot_box(xform, box_size, PIVOT_top_center);
						// fix this
						xform = m4_translate(xform, v3(box_size.x * -0.3, -box_size.y + icon_width * 0.5, 0));
						// xform = m4_translate(xform, v3(box_size.x * -0.5, -box_size.y - icon_width * 0.5, 0));
						xform = m4_translate(xform, v3(icon_center.x, icon_center.y, 0));

						draw_rect_xform(xform, box_size, bg_box_col);

						string title = STR("Wood");
						Gfx_Text_Metrics metrics = measure_text(font, title, font_height, v2(0.1, 0.1));
						Vector2 draw_pos = icon_center;
						draw_pos = v2_sub(draw_pos, metrics.visual_pos_min);
						draw_pos = v2_add(draw_pos, v2_mul(metrics.visual_size, v2(0.0, 0.0)));

						draw_pos = v2_add(draw_pos, v2(0, icon_width * -0.5));
						draw_pos = v2_add(draw_pos, v2(0, -2.0));

						draw_text(font, title, font_height, draw_pos, v2(0.1, 0.1), COLOR_WHITE);
					}
					slot_index += 1;
				}
			}
		}

		// Handle input
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
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}