
int entry(int argc, char **argv) {
	
	window.title = STR("Beyond The Crystal Sphere");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0X081629FF);

	Gfx_Image* player = load_image_from_disk(fixed_string("asset/player.png"), get_heap_allocator());
	assert(player, "Player broke game");
	Gfx_Image* tree000 = load_image_from_disk(fixed_string("asset/tree000.png"), get_heap_allocator());
	assert(tree000, "tree000 broke game");
	Gfx_Image* tree001 = load_image_from_disk(fixed_string("asset/tree001.png"), get_heap_allocator());
	assert(tree001, "tree001 broke game");
	Gfx_Image* rock000 = load_image_from_disk(fixed_string("asset/rockore000.png"), get_heap_allocator());
	assert(rock000, "rock000 broke game");

	Vector2 player_pos = v2(0,0);
	float64 seconds_counter = 0.0;
	s32 player_frame = 0;

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;

		if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000);
		last_time = now;
		
		reset_temporary_storage();

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		float zoom = 5.3;

		draw_frame.view = m4_make_scale(v3(1.0/5.3,1.0/5.3,1.0));

//		float64 now = os_get_current_time_in_seconds
//		float64 delta_t = now - last_time;
//		last_time

		os_update();

		if (is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}
		

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A')) {
			input_axis.x += 1.0;
		}
		if (is_key_down('D')) {
			input_axis.x -= 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y += 1.0;
		}
		if (is_key_down('W')) {
			input_axis.y -= 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_pos = v2_add(player_pos, v2_mulf(input_axis, 100.0 * delta_t));

		{
			Vector2 size = v2(6.0, 10.0);
			Matrix4 xform = m4_scalar(1.0);
			xform         = m4_translate(xform, v3(-player_pos.x, -player_pos.y, 0));
			xform         = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
			draw_image_xform(player, xform, size, COLOR_WHITE);
		}

		{
			Vector2 size = v2(12.0, 21.0);
			Matrix4 xform = m4_scalar(1.0);
			xform         = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
			draw_image_xform(tree000, xform, size, COLOR_WHITE);
		}

		{
			Vector2 size = v2(6.0, 10.0);
			Matrix4 xform = m4_scalar(1.0);
			xform         = m4_translate(xform, v3(-player_pos.x, -player_pos.y, 0));
			xform         = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
			draw_image_xform(player, xform, size, COLOR_WHITE);
		}

		{
			Vector2 size = v2(6.0, 10.0);
			Matrix4 xform = m4_scalar(1.0);
			xform         = m4_translate(xform, v3(-player_pos.x, -player_pos.y, 0));
			xform         = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
			draw_image_xform(player, xform, size, COLOR_WHITE);
		}

	//	draw_rect(v2(sin(now), -.8), v2(.5, .25), COLOR_RED);
		
//		float aspect = (f32)window.width/(f32)window.height;
//		float mx = (input_frame.mouse_x/(f32)window.width  * 2.0 - 1.0)*aspect;
//		float my = input_frame.mouse_y/(f32)window.height * 2.0 - 1.0;
		
//		draw_line(v2(-.75, -.75), v2(mx, my), 0.005, COLOR_WHITE);
		
//		os_update(); 
		gfx_update();
		seconds_counter += delta_t;
//		frame_count += 1;
		if (seconds_counter > 1.0) {
			seconds_counter -= 1.0;
			log("FPS: %.2f", 1.0/delta_t);
			seconds_counter = 0.0;
//			frame_c = 0;

		}
	}

	return 0;
}