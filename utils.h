#pragma once

//For some reason, we run out of ram (even though we have plenty) when
//printing the values
#if defined(PRINTSTATIC) || defined(PRINTAGGRESSION)
ARDUBOY_NO_USB
#endif