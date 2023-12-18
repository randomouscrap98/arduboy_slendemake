#pragma once

//For some reason, we run out of ram (even though we have plenty) when
//printing the values
#ifdef PRINTSTATIC
ARDUBOY_NO_USB
#endif