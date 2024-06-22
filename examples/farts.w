_start :: proc {
	exit_code := initialize;
	exit_code < 0 ? terminate();

	-- Initialize as  Wayland client
	{
		g_wl_display = wl_display_connect 0;
		!g_wl_display ? {
			err "Failed to connect to Wayland display.";
			exit -1;
		};
		note "Connected to Wayland display.";

		registry: @wl_registry = wl_display_get_registry g_wl_display;
		#static registry_listener: wl_registry_listener = {
			global        = @handle_global_wayland_object_notification;
			global_remove = @handle_global_wayland_object_removal_notification;
		};
		wl_registry_add_listener registry, @registry_listener, 0;
		g_wl_display
			|> wl_display_dispatch
			|> wl_display_roundtrip;

		note "Global Wayland objects:";
		objects_count := g_global_wayland_objects.size / #size_of Global_Wayland_Object;
		objects := (@Global_Wayland_Object)@g_global_wayland_objects.data};

		{
			i := 0;
		[a]
			i >= objects_count ? jump_to b;
			object := @objects[i];
			object.name != i + 1 ? set_memory object, #size_of Global_Wayland_Object;
			jump_to a;
		[b]
		}
	}

	{
	[a]
		!check_terminability ? jump_to b;
		jump_to a;
	[b]
	}

	wl_display_disconnect g_wl_display;
	terminate;
	return exit_code;
}

initialize :: proc -> S32 {
	return 0;
}
