#include "system/types.hpp"
#include "system/menu.hpp"

int main()
{
	Menu_init();

	// Main loop
	while (aptMainLoop())
	{
		if (Menu_query_must_exit_flag())
			break;

		Menu_main();
	}

	Menu_exit();
	return 0;
}
