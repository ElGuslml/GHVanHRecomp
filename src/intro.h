// ghvanhalen - the boot's videos skipped (src/intro.cpp).

#pragma once

#include <cstdint>

namespace ghvanhalen {

// The script function Wait's milliseconds for the script `script_name` (src/crowd.cpp's hook on Wait): 0 for the legal
// notice's wait at the boot while ghvanhalen_skip_intro is on, else `ms`.
uint32_t IntroWaitMs(uint32_t script_name, uint32_t ms);

}  // namespace ghvanhalen
