//Includes.
#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"

#include "system/menu.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
//N/A.

//Code.
int main(void)
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
