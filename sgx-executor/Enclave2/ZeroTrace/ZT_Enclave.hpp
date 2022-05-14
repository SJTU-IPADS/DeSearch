#include <string.h>
#include <vector>

#include "ZT_Globals.hpp"

#include "Globals_Enclave.hpp"
#include "Enclave_utils.hpp"
#include "ORAMTree.hpp"
#include "CircuitORAM_Enclave.hpp"

std::vector < CircuitORAM * > coram_instances;

uint32_t coram_instance_id = 0;
